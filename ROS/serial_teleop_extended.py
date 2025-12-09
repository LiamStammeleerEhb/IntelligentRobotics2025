#!/usr/bin/env python3
import sys
import threading
import time
import termios
import tty
import serial
import select

import rclpy
from rclpy.node import Node

# Base speed
BASE_SPEED = 0

HELP = """\
Controls:
  z = increase speed         -> +5
  s = decrease speed         -> -5
  q = turn left              -> left wheel slower, right wheel faster
  d = turn right             -> left wheel faster, right wheel slower
  x = stop                   -> V,0,0
  h = help
  Ctrl-C = quit
"""

class SerialTeleop(Node):
    def __init__(self):
        super().__init__('serial_teleop')

        # Parameters (with defaults)
        self.declare_parameter('port', '/dev/ttyACM0')
        self.declare_parameter('baud', 57600)
        self.declare_parameter('timeout', 0.1)

        port = self.get_parameter('port').get_parameter_value().string_value
        baud = self.get_parameter('baud').get_parameter_value().integer_value
        timeout = self.get_parameter('timeout').get_parameter_value().double_value

        # Open serial
        try:
            self.ser = serial.Serial(port, baudrate=baud, timeout=timeout)
        except Exception as e:
            self.get_logger().error(f"Failed to open serial {port} @ {baud}: {e}")
            raise

        time.sleep(2.0)
        self.get_logger().info(f"Connected to {port} @ {baud}")
        self.get_logger().info("\n" + HELP)

        self.speed = BASE_SPEED
        self.left_wheel = self.speed
        self.right_wheel = self.speed

        # Background thread for serial reading
        self._stop_event = threading.Event()
        self.reader_thread = threading.Thread(target=self._serial_reader_loop, daemon=True)
        self.reader_thread.start()

        # Terminal raw mode
        self._orig_term = termios.tcgetattr(sys.stdin.fileno())
        tty.setcbreak(sys.stdin.fileno())

        # Timer to poll keyboard
        self.key_timer = self.create_timer(0.02, self._poll_keyboard)

    def _serial_reader_loop(self):
        while not self._stop_event.is_set():
            try:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode(errors='ignore').strip()
                    if line:
                        self.get_logger().info(f"< {line}")
                else:
                    time.sleep(0.01)
            except Exception as e:
                self.get_logger().warn(f"Serial read error: {e}")
                time.sleep(0.1)

    def _send_velocity(self, left, right, duration=0.2):
        cmd = f"V,{left},{right}"
        try:
            self.ser.write((cmd + "\n").encode())
            self.get_logger().info(f"> {cmd}")
        except Exception as e:
            self.get_logger().error(f"Serial write failed: {e}")

    def _poll_keyboard(self):
        try:
            rlist, _, _ = select.select([sys.stdin], [], [], 0)
            if rlist:
                ch = sys.stdin.read(1)
                if not ch:
                    return

                if ch == 'x':
                    self.left_wheel = 0
                    self.right_wheel = 0
                    self.speed = BASE_SPEED
                    self._send_velocity(0, 0)

                elif ch == 'z':
                    self.speed += 5
                    self.left_wheel = self.speed
                    self.right_wheel = self.speed
                    self._send_velocity(self.left_wheel, self.right_wheel)

                elif ch == 's':
                    self.speed -= 5
                    self.left_wheel = self.speed
                    self.right_wheel = self.speed
                    self._send_velocity(self.left_wheel, self.right_wheel)

                elif ch == 'q':
                    self._send_velocity(-self.speed, self.speed)

                elif ch == 'd':
                    self._send_velocity(self.speed, -self.speed)

                elif ch == 'h' or ch == '?':
                    self.get_logger().info("\n" + HELP)

                elif ch == '\x03' or ch == 'a':  # Quit
                    rclpy.shutdown()
        except Exception as e:
            self.get_logger().warn(f"Keyboard read error: {e}")

    def destroy_node(self):
        # Restore terminal
        try:
            termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, self._orig_term)
        except Exception:
            pass

        # Stop threads and close serial
        self._stop_event.set()
        try:
            if self.reader_thread.is_alive():
                self.reader_thread.join(timeout=0.5)
        except Exception:
            pass

        try:
            if self.ser and self.ser.is_open:
                self.ser.close()
        except Exception:
            pass

        super().destroy_node()


def main():
    rclpy.init()
    node = SerialTeleop()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()


if __name__ == '__main__':
    main()
