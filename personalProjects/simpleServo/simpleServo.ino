#include <Servo.h>

Servo Servo1;

int servoPin = 9;
int minDegree=180;
int maxDegree=0;

int ena = 5;
int in1 = 6;
int in2 = 7;

void setSpeed(int speed) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(ena, speed);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Send format: ANGLE,90");
  Servo1.attach(servoPin);
  Servo1.write(90);

  pinMode(ena,OUTPUT);
  pinMode(in1,OUTPUT);
  pinMode(in2,OUTPUT);
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read full line
    input.trim();

    int commaIndex = input.indexOf(',');         // Find comma

    if (commaIndex > 0) {
      String command = input.substring(0, commaIndex);
      String valueStr = input.substring(commaIndex + 1);

      command.trim();
      valueStr.trim();

      if (command == "ANGLE") {
        int angle = valueStr.toInt();

        if (angle <= minDegree && angle >= maxDegree) {
          Servo1.write(angle);
          Serial.print("Servo moved to: ");
          Serial.println(angle);
        } else {
          Serial.println("Invalid angle! Use 0-180");
        }
      } else if(command=="SPEED"){
        int speed = valueStr.toInt();
        setSpeed(speed);
      } else if(command == "STOP"){
        setSpeed(0);
      }else {
        Serial.println("Unknown command");
      }
    }
  }
}
