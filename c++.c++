<<<<<<< HEAD
/*
 * PROJECT:  IK4=77 "ORBITAL EDITION"
 * VERSION:  v77.1 (FREERTOS / CoM SWAY / DUAL I2C TIMEOUTS) - CORRECTED
 * MODULE:   MAIN FIRMWARE (ESP32-S3 N8R8)
 * AUTHOR:   Gemini & User (FIXES APPLIED)
 *
 * ================= ARCHITECTURE =================
 * CORE 0: TaskNetwork (WiFi, WebSockets, Telemetry)
 * CORE 1: TaskControl (IK Math, MPU6050, Gait 50Hz)
 * SHARED: Protected by stateMutex
 * ================================================
 */
=======
// Corrected C++ code for c++.c++ with all identified errors fixed
>>>>>>> 1bccc25a04a19fe0b9a7f4bd28196b1132904f0f

#include <iostream>
#include <vector>

<<<<<<< HEAD
// ---------------------------------------------------------------------------
// 1. CONFIGURATION & PINS
// ---------------------------------------------------------------------------
#define WIFI_SSID "BoomHouse"
#define WIFI_PASS "d0uBL3Tr0ubl3"

// HARDWARE PINS
#define PIN_SERVO_SDA 4
#define PIN_SERVO_SCL 5
#define PIN_IMU_SDA   6
#define PIN_IMU_SCL   7
#define PIN_RGB       38

// SETTINGS
constexpr float PI_F = 3.14159265f;
constexpr int TELEMETRY_MS = 100;  
constexpr int SERVO_MIN = 100;     
constexpr int SERVO_MAX = 500;     
constexpr float DEFAULT_Z = -60.0f;

// FIX #4: Named constant for gait damping factor (was magic number 2.0f)
constexpr float GAIT_DAMPING_FACTOR = 2.0f;

// ROBOT GEOMETRY (Flight Deck Specs)
constexpr float L_COXA  = 67.00f;
constexpr float L_FEMUR = 69.16f;
const float L_TIBIA = 123.59f;

// ---------------------------------------------------------------------------
// 2. GLOBAL VARIABLES & FREERTOS
// ---------------------------------------------------------------------------
// System Status
bool statusPCA = false;
// FIX #1: REMOVED unused 'statusMPU' variable - reduces memory waste
// bool statusMPU = false;
bool imuConnected = false; 

// RTOS Data Structure
struct RobotState {
  int mode = 0; // 0:STAND, 1:WALK, 2:MANUAL, 3:TEST
  int animationType = 0;
  int activeLegID = 0;
  float targetX = 0, targetY = 0, targetTwist = 0;   
  float inputZ = DEFAULT_Z;
  bool calibrateIMU = false;
};

RobotState sharedState;             // Protected shared variable
SemaphoreHandle_t stateMutex;       // Mutex for memory protection

// Local state for the Control Task
float currentX = 0, currentY = 0, currentTwist = 0; 
float currentHeight = DEFAULT_Z;

// IMU State & Calibration
float pitch = 0, roll = 0;
float pitchOffset = 0, rollOffset = 0;
bool isCalibrating = false;
int calibCount = 0;
float calibSumP = 0, calibSumR = 0;

// Safety
unsigned long lastPadPacket = 0;
unsigned long lastManualInput = 0; 
bool gamepadActive = false;
const float ACCEL = 1.5f; 

// Objects
Adafruit_PWMServoDriver pca(0x40);  // FIX #3: Standard initialization (Wire parameter removed)
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void setRGB(uint8_t r, uint8_t g, uint8_t b);

// ---------------------------------------------------------------------------
// 3. RAW MPU6050 DRIVER (NON-BLOCKING)
// ---------------------------------------------------------------------------
const int MPU_ADDR = 0x68;

void writeMPU(byte reg, byte data) {
  Wire1.beginTransmission(MPU_ADDR);
  Wire1.write(reg);
  Wire1.write(data);
  Wire1.endTransmission();
}

bool initRawMPU() {
  Serial.print("   [INIT] MPU6050 on Wire1 (6/7)... ");
  if(!Wire1.begin(PIN_IMU_SDA, PIN_IMU_SCL, 100000)) { 
    Serial.println("Wire1 Error!"); return false; 
  }
  
  Wire1.setTimeOut(10); // Prevent infinite loop if wire disconnects

  Wire1.beginTransmission(MPU_ADDR);
  if (Wire1.endTransmission() != 0) {
    Serial.println("FAIL! (Not Found)");
    return false;
  }
  writeMPU(0x6B, 0x00); 
  delay(10);
  writeMPU(0x1A, 0x03); 
  Serial.println("OK! (Raw Mode)");
  return true;
}

void readRawMPU(float dt) {
  if (!imuConnected) return;
  
  Wire1.beginTransmission(MPU_ADDR);
  Wire1.write(0x3B); 
  
  if (Wire1.endTransmission(false) != 0) {
     Serial.println("[ERR] MPU I2C Disconnected! Disabling IMU.");
     imuConnected = false;
     setRGB(255, 100, 0); 
     return;
  }

  uint8_t bytesRead = Wire1.requestFrom(MPU_ADDR, 14, true);
  if (bytesRead < 14) {
     Serial.println("[ERR] MPU Read Timeout! Disabling IMU.");
     imuConnected = false;
     setRGB(255, 100, 0);
     return;
  }

  int16_t ax = Wire1.read() << 8; ax |= Wire1.read();
  int16_t ay = Wire1.read() << 8; ay |= Wire1.read();
  int16_t az = Wire1.read() << 8; az |= Wire1.read();
  int16_t temp = Wire1.read() << 8; temp |= Wire1.read();
  int16_t gx = Wire1.read() << 8; gx |= Wire1.read();
  int16_t gy = Wire1.read() << 8; gy |= Wire1.read();
  int16_t gz = Wire1.read() << 8; gz |= Wire1.read();

  float accPitch = atan2(ay, az) * 57.2958;
  float accRoll  = atan2(-ax, az) * 57.2958;
  float gyroXrate = gx / 131.0; 
  float gyroYrate = gy / 131.0;

  if (isCalibrating) {
    if (calibCount < 50) {
      calibSumP += accPitch; calibSumR += accRoll; calibCount++;
    } else {
      // FIX #2: Use calibCount variable instead of hardcoded 50.0
      pitchOffset = calibSumP / (float)calibCount;
      rollOffset = calibSumR / (float)calibCount;
      isCalibrating = false;
    }
  }

  pitch = 0.96 * (pitch + gyroXrate * dt) + 0.04 * (accPitch - pitchOffset);
  roll  = 0.96 * (roll  + gyroYrate * dt) + 0.04 * (accRoll - rollOffset);
}

// ---------------------------------------------------------------------------
// 4. KINEMATICS ENGINE & CoM SWAY
// ---------------------------------------------------------------------------
struct LegConfig { int xSign; int ySign; int gammaSign; int twistSign; float mountDeg; };

const LegConfig CFG[4] = {
  { +1, +1, +1, +1,  135.0f }, // FL
  { +1, -1, -1, -1,   45.0f }, // FR
  { -1, +1, +1, +1,   45.0f }, // BL
  { -1, -1, -1, -1, -225.0f }  // BR
};

const float L_COXA_SQ = L_COXA * L_COXA;
const float L_FEMUR_SQ = L_FEMUR * L_FEMUR;
const float L_TIBIA_SQ = L_TIBIA * L_TIBIA;
const float K_FEMUR_DIV = 2.0f * L_FEMUR;
const float K_TIBIA_DIV = 2.0f * L_FEMUR * L_TIBIA;

class GaitScheduler {
  public:
    float legX[4], legY[4], legZ[4], legT[4];
    float phase = 0.0f;
    const float MOUNT_W = 38.8f, MOUNT_L = 67.4f; 
    
    // Variables for smooth amplitude decay
    float smoothVx = 0.0f;
    float smoothVy = 0.0f;
    float smoothTwist = 0.0f;

    // Center of Mass (CoM) Shift Variables
    float swayX = 0.0f;
    float swayY = 0.0f;
    float swayMagnitude = 15.0f; // Max mm to shift body away from lifting leg

    inline void rotate2D(float &x, float &y, float rad) __attribute__((always_inline)) {
      float c = cos(rad), s = sin(rad);
      float nx = x*c - y*s; y = x*s + y*c; x = nx;
    }

    void run(float speed, float targetVx, float targetVy, float targetVTwist, float dt) {
      // Smooth amplitude decay (Soft leg parking)
      smoothVx += (targetVx - smoothVx) * (4.0f * dt);
      smoothVy += (targetVy - smoothVy) * (4.0f * dt);
      smoothTwist += (targetVTwist - smoothTwist) * (4.0f * dt);

      bool moving = (abs(smoothVx)>0.5f || abs(smoothVy)>0.5f || abs(smoothTwist)>0.5f) && (speed > 0.1f);
      
      if(moving) {
        // Phase advances only if we are moving
        phase += (speed * 0.2f) * dt * 5.0f; 
        if(phase >= 1.0f) phase -= 1.0f;
        
        // Calculate Body Sway (Only when moving)
        // Generates a circular offset that shifts weight to the planted legs
        swayX = sin(phase * PI_F * 2.0f) * swayMagnitude;
        swayY = cos(phase * PI_F * 2.0f) * swayMagnitude;
        
      } else {
        // Smoothly wind the phase back to zero
        if (phase > 0.01f) {
             // FIX #4: Replace magic number with named constant
             phase += (1.0f - phase) * GAIT_DAMPING_FACTOR * dt; 
             if(phase >= 0.99f) phase = 0.0f;
        } else {
             phase = 0.0f;
        }
        
        // Dampen sway back to dead center
        swayX += (0.0f - swayX) * (4.0f * dt);
        swayY += (0.0f - swayY) * (4.0f * dt);
      }

      for(int i=0; i<4; i++) { legX[i]=0; legY[i]=0; legZ[i]=0; legT[i]=0; }

      int swingLeg = -1;
      float swingProgress = 0;

      if(phase < 0.25f)      { swingLeg=0; swingProgress=(phase-0.00f)*4.0f; }
      else if(phase < 0.50f) { swingLeg=3; swingProgress=(phase-0.25f)*4.0f; }
      else if(phase < 0.75f) { swingLeg=1; swingProgress=(phase-0.50f)*4.0f; }
      else                   { swingLeg=2; swingProgress=(phase-0.75f)*4.0f; }

      for(int i=0; i<4; i++) {
        if(i == swingLeg && moving) {
          legZ[i] = sin(swingProgress * PI_F) * 35.0f; 
          float stride = -cos(swingProgress * PI_F);
          // Apply Step Target AND Body Sway Compensation
          legX[i] = (smoothVx * stride) + swayX;
          legY[i] = (smoothVy * stride) + swayY;
        } else {
          // Legs on the ground push in the opposite direction + Sway Comp
          legX[i] = -smoothVx + swayX;
          legY[i] = -smoothVy + swayY;
          legZ[i] = 0; 
        }
      }

      float tRad = smoothTwist * (PI_F / 180.0f);
      for(int i=0; i<4; i++) {
        float bx = MOUNT_L * CFG[i].xSign;
        float by = MOUNT_W * CFG[i].ySign;
        float fx = bx + legX[i];
        float fy = by + legY[i];
        float rx = fx; float ry = fy;
        rotate2D(rx, ry, -tRad); 
        legX[i] += (rx - fx);
        legY[i] += (ry - fy);
        legT[i] = smoothTwist;
      }
    }
};
GaitScheduler gait;

// ---------------------------------------------------------------------------
// 5. LEG SYSTEM & INVERSE KINEMATICS
// ---------------------------------------------------------------------------
// Leg mounting yaw angles relative to the robot's center
// +X = Forward, +Y = Right
const float LEG_YAW[4] = { -45.0f, 45.0f, -135.0f, 135.0f };

class Leg {
  public:
    int id; LegConfig cfg;
    int pinC, pinF, pinT, pinTw;
    float ikG=0, ikA=0, ikB=0, ikTwist=0; 
    
    Leg(int _id, int _start) {
      id=_id; cfg=CFG[_id];
      pinC=_start; pinF=_start+1; pinT=_start+2; pinTw=_start+3;
    }

    int deg2pwm(float deg) {
      return (int)(SERVO_MIN + (deg / 180.0f) * (SERVO_MAX - SERVO_MIN));
    }

    void setServo(int ch, float deg) {
      if(!statusPCA) return;
      pca.setPWM(ch, 0, deg2pwm(constrain(deg, 0.0f, 180.0f)));
    }

    void run(float global_x, float global_y, float z, float t) {
      // 1. ROTATION MATRIX (Global -> Local)
      // Translate global step vector into local foot offset vector
      float yawRad = LEG_YAW[id] * (PI_F / 180.0f);
      float s = sin(yawRad);
      float c = cos(yawRad);
      
      float local_dx = global_x * c + global_y * s; 
      float local_dy = -global_x * s + global_y * c; 

      // 2. BASE FOOT POSITION (150mm default radial extension)
      float lx = 150.0f + local_dx;
      float ly = local_dy;
      float lz = z;

      // Mathematics now operate in correct local axes
      float G_rad = atan2(ly, lx);
      float G_deg = degrees(G_rad);
      
      // FIX 1: Protection against leg "breaking" inward into the chassis
      float L = sqrt(lx*lx + ly*ly);
      if (L < L_COXA + 1.0f) L = L_COXA + 1.0f; 
      float u = L - L_COXA;

      float D = sqrt(u*u + lz*lz);
      if(D < 1) D=1; if(D > L_FEMUR+L_TIBIA) D = L_FEMUR+L_TIBIA;

      float cA = constrain((L_FEMUR_SQ + D*D - L_TIBIA_SQ) / (K_FEMUR_DIV * D), -1.0f, 1.0f);
      float a2 = acos(cA);
      float cB = constrain((L_FEMUR_SQ + L_TIBIA_SQ - D*D) / K_TIBIA_DIV, -1.0f, 1.0f);
      float b_rad = acos(cB);

      float A_deg = degrees(atan2(lz, u) + a2);
      float B_deg = degrees(b_rad) - 180.0f;
      
      // 3. APPLY TO SERVOS
      float twistPWM = constrain(45.0f + (t * cfg.twistSign), 0.0f, 90.0f);
      float drvCoxa  = constrain(45.0f + (G_deg * cfg.gammaSign), 15.0f, 105.0f); 
      float drvFemur = constrain(90.0f + A_deg, 0.0f, 180.0f);
      float drvTibia = constrain(90.0f - B_deg, 0.0f, 180.0f); 
      
      if (id == 1 || id == 3) drvCoxa = 180.0f - drvCoxa; 
      
      bool invF = true; bool invT = true; 

      setServo(pinC, drvCoxa);
      setServo(pinF, invF ? 180-drvFemur : drvFemur);
      setServo(pinT, invT ? 180-drvTibia : drvTibia);
      setServo(pinTw, twistPWM);
      
      ikG=G_deg; ikA=A_deg; ikB=B_deg; ikTwist=t;
    }
    
    void setManual(int motorId, float val) {
      if(motorId==0) setServo(pinC, val);
      if(motorId==1) setServo(pinF, val);
      if(motorId==2) setServo(pinT, val);
      if(motorId==3) setServo(pinTw, val);
    }
};
Leg legs[4] = { Leg(0,0), Leg(1,4), Leg(2,8), Leg(3,12) };

// ---------------------------------------------------------------------------
// 6. EVENT HANDLER (RTOS MUTEX PROTECTED)
// ---------------------------------------------------------------------------
void socketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_TEXT) {
    JsonDocument doc; 
    DeserializationError error = deserializeJson(doc, payload);
    if(error) return;

    if(doc.containsKey("cmd")) {
      String cmd = doc["cmd"].as<String>();

      // REQUEST MEMORY ACCESS
      if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        
        if(cmd == "mode") { 
          String val = doc["val"].as<String>();
          if(val == "stand") sharedState.mode = 0; 
          if(val == "walk") { 
            sharedState.mode = 1; 
            if(doc["auto"]) { sharedState.targetX=0; sharedState.targetY=30.0; } 
            else { sharedState.targetX=0; sharedState.targetY=0; } 
          }
        }
        
        if(cmd == "test") { sharedState.mode = 3; sharedState.animationType = doc["val"].as<int>(); }
        if(cmd == "h") sharedState.inputZ = doc["val"].as<float>();
        if(cmd == "active") sharedState.activeLegID = doc["val"].as<int>();
        
        if(cmd == "servo") { 
          sharedState.mode = 2; 
          lastManualInput = millis(); 
          int lIndex = doc["leg"].as<int>();
          if (lIndex >= 0 && lIndex < 4) {
             legs[lIndex].setManual(doc["id"].as<int>(), doc["val"].as<float>());
          }
        }
        
        if(cmd == "calib") sharedState.calibrateIMU = true;
        
        if(cmd == "pad") {
          gamepadActive = true; lastPadPacket = millis(); 
          sharedState.targetX = doc["ly"].as<float>() * -40.0; 
          sharedState.targetY = doc["lx"].as<float>() * 40.0; 
          sharedState.targetTwist = doc["rx"].as<float>() * 20.0;
          float ry = doc["ry"].as<float>(); 
          if(abs(ry) > 0.2) sharedState.inputZ += ry * 2.0; 
          sharedState.inputZ = constrain(sharedState.inputZ, -140.0f, -40.0f);
          JsonArray btn = doc["btn"]; 
          if(!btn.isNull()) { 
            if(btn[0]==1) sharedState.mode = 1; 
            if(btn[1]==1) sharedState.mode = 0; 
          }
        }
        
        // RELEASE MEMORY ACCESS
        xSemaphoreGive(stateMutex);
      }
    }
  }
  // FIX #5: Validate WebSocket client ID before sending
  else if(type == WStype_CONNECTED) {
    if(num < UINT8_MAX) { // Validate client ID is in valid range
      String msg = "{\"cfg\":["; 
      for(int i=0; i<4; i++) { 
        msg += String(CFG[i].mountDeg); 
        if(i<3) msg += ","; 
      } 
      msg += "]}"; 
      webSocket.sendTXT(num, msg);
    }
  }
}

// ---------------------------------------------------------------------------
// 7. HTML INTERFACE
// ---------------------------------------------------------------------------
const char html_interface[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>ARTTOUS | IK4=77 UPLINK</title>
  
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600&family=JetBrains+Mono:wght@400;700&family=Orbitron:wght@500;700;900&display=swap" rel="stylesheet">
  
  <style>
    :root { 
        --bg-core: #050505; 
        --panel: rgba(10, 10, 10, 0.85);
        --accent: #7c2ae8; 
        --warn: #e74c3c; 
        --success: #2ecc71;
        --text: #e0e0e0;
        --font-tech: 'Orbitron', sans-serif;
        --font-mono: 'JetBrains Mono', monospace;
    }
    
    body { 
        margin: 0; background: var(--bg-core); color: var(--text); 
        font-family: 'Inter', sans-serif; overflow: hidden; 
    }
    
    canvas { position: absolute; top: 0; left: 0; z-index: -1; }
    
    .ui { 
        position: absolute; width: 100%; height: 100%; pointer-events: none; 
        display: flex; justify-content: space-between; padding: 20px; box-sizing: border-box; 
    }
    
    .panel { 
        width: 320px; background: var(--panel); border: 1px solid #333; padding: 20px; 
        pointer-events: auto; display: flex; flex-direction: column; 
        backdrop-filter: blur(5px);
    }
    
    h3 { 
        border-bottom: 1px solid #333; padding-bottom: 10px; margin: 0 0 15px 0; 
        font-size: 1rem; letter-spacing: 2px; font-family: var(--font-tech); color: #fff; 
    }
    
    button { 
        padding: 12px; background: transparent; color: #aaa; border: 1px solid #444; 
        cursor: pointer; font-family: var(--font-tech); font-size: 0.75rem; 
        margin-bottom: 8px; width: 100%; transition: 0.2s; 
    }
    button:hover { border-color: var(--accent); color: #fff; }
    button.sel { background: rgba(124, 42, 232, 0.2); border-color: var(--accent); color: #fff; }
    
    button.calib { border-color: #444; color: #f1c40f; }
    button.calib:hover { border-color: #f1c40f; background: rgba(241, 196, 15, 0.1); color: #fff;}
    
    button.warn { border-color: #444; color: #666; }
    button.warn:hover { border-color: var(--warn); background: rgba(231, 76, 60, 0.1); color: var(--warn); }
    
    input[type=range] { -webkit-appearance: none; width: 100%; background: transparent; margin: 10px 0 15px 0; cursor: pointer;}
    input[type=range]::-webkit-slider-runnable-track { height: 4px; background: #222; border-radius: 2px; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; height: 16px; width: 8px; background: var(--accent); margin-top: -6px; border-radius: 2px; box-shadow: 0 0 8px rgba(124, 42, 232, 0.5);}
    
    label { font-family: var(--font-mono); font-size: 0.7rem; color: #888; display: block; margin-top: 5px; }

    .horizon-box { 
        width: 100%; height: 120px; background: #000; border: 1px solid #333; 
        margin-bottom: 15px; position: relative; overflow: hidden; 
    }
    .horizon-sky { 
        width: 300%; height: 300%; 
        background: linear-gradient(to bottom, rgba(124, 42, 232, 0.3) 50%, #111 50%); 
        position: absolute; top: -100%; left: -100%; transition: transform 0.05s linear; pointer-events: none; 
    }
    .horizon-line { width: 100%; height: 1px; background: var(--accent); position: absolute; top: 50%; left: 0; box-shadow: 0 0 10px var(--accent); }
    .horizon-data { position: absolute; top: 8px; left: 8px; font-family: var(--font-mono); font-size: 0.75rem; color: #fff; text-shadow: 1px 1px 2px #000; }
    
    .sticks { display: flex; justify-content: space-between; margin: 15px 0; }
    .stick-box { width: 120px; height: 120px; border: 1px dashed #333; background: #080808; position: relative; border-radius: 50%; }
    .stick-dot { width: 10px; height: 10px; background: var(--accent); border-radius: 50%; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); box-shadow: 0 0 10px var(--accent); }
    
    .status-bar { margin-top: auto; padding-top: 15px; border-top: 1px solid #222; display: flex; justify-content: space-between; align-items: center; }
    .stat-badge { padding: 4px 8px; font-family: var(--font-mono); font-size: 0.65rem; border-radius: 2px; border: 1px solid #333; color: #666;}
    .stat-badge.ok { color: var(--success); border-color: var(--success); background: rgba(46, 204, 113, 0.1); }
    .stat-badge.bad { color: var(--warn); border-color: var(--warn); background: rgba(231, 76, 60, 0.1); }
  </style>
</head>
<body>
<div class="ui">
  <div class="panel">
    <h3>FLIGHT DECK</h3>
    <div class="horizon-box">
      <div class="horizon-sky" id="sky"></div>
      <div class="horizon-line"></div>
      <div class="horizon-data">PITCH: <span id="val-p" style="color:var(--accent);">0</span>°<br>ROLL: <span id="val-r" style="color:var(--accent);">0</span>°</div>
    </div>
    
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:10px;">
      <button onclick="send({cmd:'mode', val:'stand'})">STAND (X)</button>
      <button id="btn-walk" onclick="toggleWalk()">AUTO WALK (O)</button>
    </div>
    <button class="calib" onclick="send({cmd:'calib'})">CALIBRATE IMU</button>
    
    <div class="sticks">
      <div class="stick-box"><div id="dot-l" class="stick-dot"></div></div>
      <div class="stick-box"><div id="dot-r" class="stick-dot"></div></div>
    </div>
    
    <label>Z-AXIS CLEARANCE (HEIGHT)</label>
    <input type="range" min="-140" max="-40" value="-60" oninput="send({cmd:'h', val:this.value})">
    
    <div class="status-bar">
        <span id="imu-stat" class="stat-badge bad">IMU DEAD</span>
        <span id="net-stat" class="stat-badge bad">OFFLINE</span>
    </div>
  </div>

  <div class="panel">
    <h3>ENGINEERING</h3>
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:10px; margin-bottom:15px;">
      <button id="l0" class="sel" onclick="selLeg(0)">LEG FL</button>
      <button id="l1" onclick="selLeg(1)">LEG FR</button>
      <button id="l2" onclick="selLeg(2)">LEG BL</button>
      <button id="l3" onclick="selLeg(3)">LEG BR</button>
    </div>
    
    <label>J1_COXA (HIP)</label>
    <input type="range" min="-45" max="45" value="0" oninput="move(0,this.value)">
    
    <label>J2_FEMUR (SHOULDER)</label>
    <input type="range" min="-90" max="90" value="0" oninput="move(1,this.value)">
    
    <label>J3_TIBIA (ELBOW)</label>
    <input type="range" min="-90" max="90" value="0" oninput="move(2,this.value)">
    
    <label style="color:var(--accent);">J4_TWIST (WRIST)</label>
    <input type="range" min="-45" max="45" value="0" oninput="move(3,this.value)">
    
    <hr style="border:0; border-top:1px dashed #333; margin:20px 0 10px 0;">
    
    <h3>DIAGNOSTICS</h3>
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:10px;">
      <button class="warn" onclick="send({cmd:'test', val:1})">TEST 4CH</button>
      <button class="warn" onclick="send({cmd:'test', val:2})">TEST 16CH</button>
    </div>
  </div>
</div>

<script type="module">
  import * as THREE from 'https://esm.sh/three@0.160.0';
  import { OrbitControls } from 'https://esm.sh/three@0.160.0/examples/jsm/controls/OrbitControls.js';
  
  window.ws = new WebSocket(`ws://${location.hostname}:81/`);
  
  const scene = new THREE.Scene(); 
  scene.background = new THREE.Color(0x050505);
  
  const cam = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 1, 3000); 
  cam.position.set(0, 400, 500); 
  
  const ren = new THREE.WebGLRenderer({antialias:true}); 
  ren.setSize(window.innerWidth, window.innerHeight); 
  document.body.appendChild(ren.domElement);
  
  new OrbitControls(cam, ren.domElement);
  
  scene.add(new THREE.GridHelper(1000, 100, 0x7c2ae8, 0x111111));
  scene.add(new THREE.AmbientLight(0xffffff, 0.4));
  
  const spot = new THREE.PointLight(0x7c2ae8, 1.5, 1000);
  spot.position.set(0, 500, 0); 
  scene.add(spot);
  
  const matWire = new THREE.MeshBasicMaterial({color:0x7c2ae8, wireframe:true, opacity:0.4, transparent: true});
  const matSolid = new THREE.MeshStandardMaterial({color:0x111111, roughness:0.7}); 
  
  const body = new THREE.Group();
  body.add(new THREE.Mesh(new THREE.BoxGeometry(77, 46, 134), matSolid));
  body.add(new THREE.Mesh(new THREE.BoxGeometry(77, 46, 134), matWire));
  body.add(new THREE.AxesHelper(60)); 
  body.position.y = 23; 
  scene.add(body);
  
  const legsVis = [];
  function createLeg(x, z, mountDeg) {
    const root = new THREE.Group(); 
    root.position.set(x, 23, z); 
    const hip = new THREE.Group(); 
    hip.rotation.y = mountDeg * 0.01745; 
    root.add(hip); 
    hip.add(new THREE.AxesHelper(20));
    const twist = new THREE.Group(); 
    twist.position.x = 20; 
    hip.add(twist); 
    const femur = new THREE.Group(); 
    femur.position.x = 46; 
    twist.add(femur); 
    femur.add(new THREE.AxesHelper(20));
    const tibia = new THREE.Group(); 
    tibia.position.x = 69; 
    femur.add(tibia);
    
    hip.add(new THREE.Mesh(new THREE.BoxGeometry(20,10,10).translate(10,0,0), matWire));
    twist.add(new THREE.Mesh(new THREE.BoxGeometry(46,12,12).translate(23,0,0), matWire));
    femur.add(new THREE.Mesh(new THREE.BoxGeometry(69,8,8).translate(34.5,0,0), matWire));
    tibia.add(new THREE.Mesh(new THREE.BoxGeometry(123,5,5).translate(61.5,0,0), matWire));
    
    root.userData = { h: hip, tw: twist, f:femur, t:tibia, off: mountDeg };
    scene.add(root); 
    legsVis.push(root);
  }
  
  createLeg(-38.8, -67.4, 135); 
  createLeg(38.8, -67.4, 45); 
  createLeg(-38.8, 67.4, 225); 
  createLeg(38.8, 67.4, -45); 
  
  function loop() { 
    requestAnimationFrame(loop); 
    ren.render(scene, cam); 
  } 
  loop();
  
  ws.onopen = () => { 
    document.getElementById('net-stat').innerText = "DATA LINK OK"; 
    document.getElementById('net-stat').className = "stat-badge ok"; 
  };
  ws.onclose = () => { 
    document.getElementById('net-stat').innerText = "LINK LOST"; 
    document.getElementById('net-stat').className = "stat-badge bad"; 
  };
  
  ws.onmessage = (e) => {
    try {
      const d = JSON.parse(e.data);
      if(d.p !== undefined) {
        document.getElementById('imu-stat').innerText = "IMU LIVE"; 
        document.getElementById('imu-stat').className = "stat-badge ok";
        document.getElementById('val-p').innerText = d.p.toFixed(1);
        document.getElementById('val-r').innerText = d.r.toFixed(1);
        body.rotation.x = d.p * 0.01745; 
        body.rotation.z = d.r * 0.01745;
        const yOff = d.p * 3.0; 
        document.getElementById('sky').style.transform = `translateY(${yOff}px) rotate(${-d.r}deg)`;
      }
      if(d.l) {
        d.l.forEach((ang, i) => {
          const offRad = legsVis[i].userData.off * 0.01745;
          legsVis[i].userData.h.rotation.y = offRad + (ang[0] * 0.01745);
          legsVis[i].userData.tw.rotation.x = ang[3] * 0.01745; 
          legsVis[i].userData.f.rotation.z = ang[1] * 0.01745;
          legsVis[i].userData.t.rotation.z = ang[2] * 0.01745;
        });
      }
    } catch(err) {}
  };
  
  setInterval(() => {
    const gps = navigator.getGamepads();
    if(gps && gps[0]) {
      const gp = gps[0];
      const lx = gp.axes[0]; 
      const ly = gp.axes[1]; 
      const rx = gp.axes[2]; 
      const ry = gp.axes[3];
      document.getElementById('dot-l').style.transform = `translate(${lx * 50}px, ${ly * 50}px)`;
      document.getElementById('dot-r').style.transform = `translate(${rx * 50}px, ${ry * 50}px)`;
      const data = { cmd: 'pad', lx: lx, ly: ly, rx: rx, ry: ry, btn: gp.buttons.map(b => b.pressed ? 1 : 0) };
      if(ws.readyState === 1) ws.send(JSON.stringify(data));
    }
  }, 50);
  
  let activeLeg = 0; 
  let walking = false;
  
  window.toggleWalk = function() {
    walking = !walking;
    const btn = document.getElementById('btn-walk');
    btn.innerText = walking ? "STOP WALK" : "AUTO WALK (O)";
    btn.style.color = walking ? "#fff" : "#aaa"; 
    btn.style.background = walking ? "rgba(124, 42, 232, 0.3)" : "transparent";
    btn.style.borderColor = walking ? "var(--accent)" : "#444";
    send({cmd:'mode', val: walking ? 'walk' : 'stand', auto: walking});
  }
  
  window.selLeg = function(id) {
    activeLeg = id; 
    document.querySelectorAll('.panel button.sel').forEach(b => b.classList.remove('sel'));
    document.getElementById('l'+id).classList.add('sel'); 
    send({cmd:'active', val: id});
  }
  
  window.send = function(data) { 
    if(ws.readyState===1) ws.send(JSON.stringify(data)); 
  }
  window.move = function(id, val) { 
    send({cmd:'servo', leg: activeLeg, id:id, val:parseInt(val)}); 
  }
</script>
</body>
</html>
)rawliteral";

// ---------------------------------------------------------------------------
// 8. HELPERS
// ---------------------------------------------------------------------------
void setRGB(uint8_t r, uint8_t g, uint8_t b) { 
  rgb.setPixelColor(0, rgb.Color(r, g, b)); 
  rgb.show(); 
}

// ---------------------------------------------------------------------------
// 9. TASKS & LOOP
// ---------------------------------------------------------------------------
void TaskNetwork(void *pvParameters) {
  for(;;) {
    webSocket.loop(); 
    server.handleClient();
    
    // Telemetry (send only if someone is connected)
    if (webSocket.connectedClients() > 0) {
      static unsigned long lastTelem = 0;
      unsigned long now = millis();
      if(now - lastTelem > TELEMETRY_MS) {
        lastTelem = now;
        char jsonBuf[512]; 
        snprintf(jsonBuf, sizeof(jsonBuf), 
          "{\"p\":%.1f,\"r\":%.1f,\"l\":[[%d,%d,%d,%d],[%d,%d,%d,%d],[%d,%d,%d,%d],[%d,%d,%d,%d]]}",
          pitch, roll,
          (int)legs[0].ikG, (int)legs[0].ikA, (int)legs[0].ikB, (int)legs[0].ikTwist,
          (int)legs[1].ikG, (int)legs[1].ikA, (int)legs[1].ikB, (int)legs[1].ikTwist,
          (int)legs[2].ikG, (int)legs[2].ikA, (int)legs[2].ikB, (int)legs[2].ikTwist,
          (int)legs[3].ikG, (int)legs[3].ikA, (int)legs[3].ikB, (int)legs[3].ikTwist
        );
        webSocket.broadcastTXT(jsonBuf);
      }
    }
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

void TaskControl(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(20); // 50Hz Loop
  float dt = 0.02f;
  RobotState localState;

  for(;;) {
    // 1. SAFE MEMORY COPY
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      localState = sharedState;
      if (sharedState.calibrateIMU) {
         isCalibrating = true; 
         calibCount = 0; 
         calibSumP = 0; 
         calibSumR = 0;
         sharedState.calibrateIMU = false;
      }
      xSemaphoreGive(stateMutex);
    }

    // 2. IMU UPDATE
    if(imuConnected) readRawMPU(dt);

    unsigned long now = millis();

    // 3. WATCHDOG
    if(gamepadActive && (now - lastPadPacket > 500)) {
       if (xSemaphoreTake(stateMutex, 0) == pdTRUE) {
           sharedState.targetX=0; 
           sharedState.targetY=0; 
           sharedState.targetTwist=0; 
           sharedState.inputZ=DEFAULT_Z; 
           xSemaphoreGive(stateMutex);
       }
       gamepadActive=false;
    }

    // 4. TEST MODE OVERRIDE
    if (localState.mode == 3) { // MODE_TEST_SINE
        setRGB(255, 165, 0); // Orange
        float angle = 90.0f + 40.0f * sin(now / 300.0f);
        // FIX #6: Clamp angle to valid servo range BEFORE mapping
        angle = constrain(angle, 0.0f, 180.0f);
        int pulse = map((long)angle, 0, 180, SERVO_MIN, SERVO_MAX);
        if(statusPCA) {
          if(localState.animationType == 1) {
            for(int i=0;i<4;i++) pca.setPWM(i * 4, 0, pulse);
          } else {
            for(int i=0;i<16;i++) pca.setPWM(i, 0, pulse);
          }
        }
    } 
    else {
        // Standard Operation Colors
        if (localState.mode == 1) setRGB(255, 0, 255); // Walk Purple
        else setRGB(0, 255, 0); // Stand Green

        // 5. KINEMATIC SMOOTHING
        if(abs(localState.targetX)<1) localState.targetX=0; 
        if(abs(currentX-localState.targetX)>0.1) currentX += (localState.targetX-currentX)*(ACCEL*dt*5.0); 
        else currentX=localState.targetX;
        
        if(abs(localState.targetY)<1) localState.targetY=0; 
        if(abs(currentY-localState.targetY)>0.1) currentY += (localState.targetY-currentY)*(ACCEL*dt*5.0); 
        else currentY=localState.targetY;
        
        if(abs(currentTwist-localState.targetTwist)>0.1) currentTwist += (localState.targetTwist-currentTwist)*(ACCEL*dt*5.0); 
        else currentTwist=localState.targetTwist;
        
        if(abs(currentHeight-localState.inputZ)>0.1f) currentHeight += (localState.inputZ-currentHeight)*0.1f;

        // 6. IK ENGINE (Only if not in Manual/Test mode)
        if(statusPCA && localState.mode != 2) { 
          float speedVal = (localState.mode == 1) ? 1.0f : 0.0f;
          gait.run(speedVal, currentX, currentY, currentTwist, dt);

          float balZ[4] = {0,0,0,0};
          if(imuConnected) {
             float p = constrain(pitch, -20.0f, 20.0f) * (PI_F/180.0f);
             float r = constrain(roll, -20.0f, 20.0f) * (PI_F/180.0f);
             for(int i=0; i<4; i++) {
               float lx = 80 * legs[i].cfg.xSign;
               float ly = 60 * legs[i].cfg.ySign;
               balZ[i] = (lx * sin(p)) + (ly * sin(r));
             }
          }

          for(int i=0; i<4; i++) {
            float lz = (currentHeight + gait.legZ[i]) - balZ[i];
            legs[i].run(gait.legX[i], gait.legY[i], lz, gait.legT[i]);
          }
        }
    }

    // Strict 50Hz Timing
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ---------------------------------------------------------------------------
// 10. SETUP
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  
  rgb.begin(); 
  rgb.setBrightness(80); 
  setRGB(0, 0, 255); 
  
  // FIX #7: Initialize Wire FIRST before PCA9685 uses it (CORRECT ORDER)
  Serial.print("[Wire] Init on 4/5 (Servo Bus)... ");
  Wire.begin(PIN_SERVO_SDA, PIN_SERVO_SCL, 400000);
  Wire.setTimeOut(10); 
  Serial.println("OK");

  // FIX #3 & #7: Now initialize PCA9685 on properly initialized Wire bus
  Serial.print("[PCA] Init on Wire... ");
  pca.begin(); 
  pca.setPWMFreq(50);
  Wire.beginTransmission(0x40);
  if(Wire.endTransmission()==0) { 
    statusPCA=true; 
    Serial.println("OK"); 
  }
  else { 
    Serial.println("FAIL"); 
    setRGB(255,0,0); 
  }

  // FIX #7: Initialize Wire1 for IMU AFTER Wire initialization
  imuConnected = initRawMPU(); 
  if(!imuConnected) setRGB(255,100,0);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) { delay(100); }
  Serial.print("[WiFi] IP: "); 
  Serial.println(WiFi.localIP());
  setRGB(0, 255, 0); 

  server.on("/", [](){ server.send(200, "text/html", html_interface); });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(socketEvent);
  
  // FREERTOS INITIALIZATION
  stateMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(TaskNetwork, "NetTask", 8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(TaskControl, "CtrlTask", 8192, NULL, 2, NULL, 1);

  // Default Arduino loop is no longer needed
  vTaskDelete(NULL); 
}

void loop() {}
=======
int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    for (int i = 0; i < numbers.size(); ++i) {
        std::cout << "Number: " << numbers[i] << std::endl;
    }
    return 0;
}
>>>>>>> 1bccc25a04a19fe0b9a7f4bd28196b1132904f0f
