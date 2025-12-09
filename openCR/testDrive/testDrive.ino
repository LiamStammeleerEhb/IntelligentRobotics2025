#include <DynamixelWorkbench.h>
#include "pitches.h"
#include "startupLiam.h"
#include "IMU_Read.h"

#if defined(__OPENCM904__)
  #define DEVICE_NAME "3" // Dynamixel on Serial3(USART3) <- OpenCM 485EXP
#elif defined(__OPENCR__)
  #define DEVICE_NAME ""
#endif

#define BAUDRATE     1000000
#define DXL_ID_1     1      // wiel links/rechts (wheel mode)
#define DXL_ID_2     2

#define PROFILE_VELOCITY     50
#define PROFILE_ACCELERATION 10

#define WHEEL_DIAMETER 6.6
#define WHEEL_DISTANCE 15


DynamixelWorkbench dxl_wb;

unsigned long lastBatteryCheck = 0;

// --- Move-for-time state ---
bool timedMoveActive = false;
unsigned long timedMoveStart = 0;
unsigned long timedMoveDuration = 0;
int timedV1 = 0, timedV2 = 0;


// ---------- Helpers wheel control ----------
bool setWheelMode(uint8_t id) {
  const char* log;
  bool ok = dxl_wb.wheelMode(id, 0, &log);
  if (!ok) {
    Serial.print("Failed to set wheel mode on ID ");
    Serial.print(id);
    Serial.print(": ");
    Serial.println(log);
  }
  return ok;
}

bool setVelocity(uint8_t id, int32_t vel) {
  const char* log;
  bool ok = dxl_wb.goalVelocity(id, vel, &log);
  if (!ok) {
    Serial.print("Failed to set velocity on ID ");
    Serial.print(id);
    Serial.print(": ");
    Serial.println(log);
  }
  return ok;
}


// ---------- Parsing ----------
static bool isNumeric(const String &s) {
  if (s.length() == 0) return false;
  int i = 0;
  if (s[0] == '+' || s[0] == '-') i = 1;
  for (; i < s.length(); i++) if (!isDigit(s[i])) return false;
  return true;
}

void splitCSV(const String &line, String parts[], int &count, int maxParts) {
  count = 0;
  int start = 0;
  while (count < maxParts-1) {
    int idx = line.indexOf(',', start);
    if (idx < 0) break;
    parts[count++] = line.substring(start, idx);
    start = idx + 1;
  }
  parts[count++] = line.substring(start);
  for (int i = 0; i < count; i++) parts[i].trim();
}

bool parseTwoSpeeds(const String &line, int &v1, int &v2) {
  String parts[2]; int n=0;
  splitCSV(line, parts, n, 2);
  if (n != 2) return false;
  if (!isNumeric(parts[0]) || !isNumeric(parts[1])) return false;
  v1 = parts[0].toInt();
  v2 = parts[1].toInt();
  v1 = v1 < -100 ? -100 : (v1 > 100 ? 100 : v1);
  v2 = v2 < -100 ? -100 : (v2 > 100 ? 100 : v2);
  return true;
}



void stopWheels() {
  setVelocity(DXL_ID_1, 0);
  setVelocity(DXL_ID_2, 0);
  timedMoveActive = false;
  Serial.println("Wheels halted.");
}


void moveFor(float seconds, int v1, int v2) {
  timedMoveStart = millis();
  timedMoveDuration = (unsigned long)(seconds * 1000);
  timedV1 = v1;
  timedV2 = v2;
  timedMoveActive = true;

  setVelocity(DXL_ID_1, v1);
  setVelocity(DXL_ID_2, v2);
}

void turn(int degree, int time) {
  const float wheelCircumference = PI * WHEEL_DIAMETER;
  const float arc = PI * WHEEL_DISTANCE * (abs(degree) / 360.0);

  float rotations = arc / wheelCircumference;

  float speed = rotations * (286.0 / time);

  if (speed > 150) speed = 150;

  int vLeft, vRight;

  if (degree > 0) { // Right turn
    vLeft = speed;
    vRight = -speed;
  } else { // Left turn
    vLeft = -speed;
    vRight = speed;
  }

  // Call your existing timed move
  moveFor(time, vLeft, vRight);

  Serial.print("Turning ");
  Serial.print(degree);
  Serial.print(" degrees in ");
  Serial.print(time);
  Serial.print("s -> Speeds: ");
  Serial.print(vLeft);
  Serial.print(", ");
  Serial.println(vRight);
}


// ---------- Setup & loop ----------
void setup() {
  Serial.begin(57600);
  while(!Serial);
  
  while(getBatteryVoltage()<11.3){
    Serial.println("Please connect to a powersource");
    delay(1000);
  }

  playstartsong();
  IMU.begin();


  const char *log;
  bool ok = dxl_wb.init(DEVICE_NAME, BAUDRATE, &log);
  if (!ok) {
    Serial.println(log);
    Serial.println("Failed to init");
  } else {
    Serial.print("Succeeded to init : ");
    Serial.println(BAUDRATE);
  }

  uint16_t model;
  uint8_t ids[] = {DXL_ID_1, DXL_ID_2};
  for (uint8_t i = 0; i < sizeof(ids)/sizeof(ids[0]); i++) {
    if (!dxl_wb.ping(ids[i], &model, &log)) {
      Serial.print("Ping failed for ID "); Serial.print(ids[i]); Serial.print(": "); Serial.println(log);
    } else {
      Serial.print("ID "); Serial.print(ids[i]); Serial.print(" model_number: "); Serial.println(model);
    }
  }

  setWheelMode(DXL_ID_1);
  setWheelMode(DXL_ID_2);

  setVelocity(DXL_ID_1, 0);
  setVelocity(DXL_ID_2, 0);

  Serial.println();
  Serial.println("Klaar.");
  Serial.println("Wielen: v1,v2          (bv. 40,-20)");
  Serial.println();
}

void loop() {

  float voltage = getBatteryVoltage();
  
  if (millis() - lastBatteryCheck >= 60000) {
    lastBatteryCheck = millis();  // Update the last check time
    float voltage = getBatteryVoltage(); // Read current voltage

    if (voltage < 11.3) {
      stopWheels();
      Serial.print("Critical battery: ");
      Serial.println(voltage);
    }
  }

  if (timedMoveActive && millis() - timedMoveStart >= timedMoveDuration) {
    stopWheels();
  }


  
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // STOP command
    if (input.equalsIgnoreCase("STOP")) {
      stopWheels();
      return;
    }

    String parts[4];
    int n = 0;
    splitCSV(input, parts, n, 4);

    if (n < 1) {
      Serial.println("Invalid input.");
      return;
    }

    String cmd = parts[0];

    // ---------------------- MOVE command (V,v1,v2,time) ----------------------
    if (cmd.equalsIgnoreCase("V")) {
      if (n != 3) {
        Serial.println("Use: V,v1,v2");
        return;
      }

      if (!isNumeric(parts[1]) || !isNumeric(parts[2])) {
        Serial.println("Invalid speeds");
        return;
      }

      int v1 = parts[1].toInt();
      int v2 = parts[2].toInt();
      float duration = parts[3].toFloat();

      setVelocity(1, v1);
      setVelocity(2, v2);

      Serial.print("Move: ");
      Serial.print(v1); Serial.print(", ");
      Serial.println(v2);
      return;
    }

    // ---------------------- TIMED MOVE command (D,v1,v2,time) ----------------------
    if (cmd.equalsIgnoreCase("D")) {
      if (n != 4) {
        Serial.println("Use: D,v1,v2,time");
        return;
      }

      if (!isNumeric(parts[1]) || !isNumeric(parts[2])) {
        Serial.println("Invalid speeds");
        return;
      }

      int v1 = parts[1].toInt();
      int v2 = parts[2].toInt();
      float duration = parts[3].toFloat();

      moveFor(duration, v1, v2);

      Serial.print("Move: ");
      Serial.print(v1); Serial.print(", ");
      Serial.print(v2); Serial.print(" for ");
      Serial.print(duration); Serial.println("s");
      return;
    }

    // ---------------------- TURN command (T,degree,time) ----------------------
    if (cmd.equalsIgnoreCase("T")) {
      if (n != 3) {
        Serial.println("Use: T,degree,time");
        return;
      }

      if (!isNumeric(parts[1]) || !isNumeric(parts[2])) {
        Serial.println("Invalid turn values");
        return;
      }

      float degree = parts[1].toFloat();
      float duration = parts[2].toFloat();

      turn(degree, duration);
      return;
    }

    // ---------------------- BATTERY command (B,degree,time) ----------------------
    if (cmd.equalsIgnoreCase("B")) {
      if (n != 1) {
        Serial.println("Use: B");
        return;
      }

      Serial.println(getBatteryVoltage());

      return;
    }

    Serial.println("Unknown command. Use:");
    Serial.println("  V,v1,v2,time");
    Serial.println("  D,v1,v2,time");
    Serial.println("  T,degree,time");
  }
}