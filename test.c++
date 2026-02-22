/*
 * PROJECT:  IK4=75 "RTOS EDITION"
 * VERSION:  v75.0 (FREERTOS / DUAL CORE / MUTEX PROTECTED)
 * MODULE:   MAIN FIRMWARE (ESP32-S3 N8R8)
 * AUTHOR:   Gemini & User
 *
 * ================= ARCHITECTURE =================
 * CORE 0: TaskNetwork (WiFi, WebSockets, Telemetry)
 * CORE 1: TaskControl (IK Math, MPU6050, Gait 50Hz)
 * SHARED: Protected by stateMutex
 * ================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <math.h>

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

// ROBOT GEOMETRY (Flight Deck Specs)
constexpr float L_COXA  = 67.00f;
constexpr float L_FEMUR = 69.16f;
const float L_TIBIA = 123.59f;

// ---------------------------------------------------------------------------
// 2. GLOBAL VARIABLES & FREERTOS
// ---------------------------------------------------------------------------
// System Status
bool statusPCA = false;
bool statusMPU = false;
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
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40, Wire); // Bus 0
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
  
  Wire1.setTimeOut(10); 

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
      pitchOffset = calibSumP / 50.0; rollOffset = calibSumR / 50.0; 
      isCalibrating = false;
    }
  }

  pitch = 0.96 * (pitch + gyroXrate * dt) + 0.04 * (accPitch - pitchOffset);
  roll  = 0.96 * (roll  + gyroYrate * dt) + 0.04 * (accRoll - rollOffset);
}

// ---------------------------------------------------------------------------
// 4. KINEMATICS ENGINE
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

    inline void rotate2D(float &x, float &y, float rad) __attribute__((always_inline)) {
      float c = cos(rad), s = sin(rad);
      float nx = x*c - y*s; y = x*s + y*c; x = nx;
    }

    void run(float speed, float targetVx, float targetVy, float targetVTwist, float dt) {
      // FIX 3: Smooth amplitude decay (Soft leg parking)
      // Instead of an instant stop, we smoothly dampen the motion vectors.
      // The smoothing factor (here 4.0f) determines the "braking" speed.
      smoothVx += (targetVx - smoothVx) * (4.0f * dt);
      smoothVy += (targetVy - smoothVy) * (4.0f * dt);
      smoothTwist += (targetVTwist - smoothTwist) * (4.0f * dt);

      // Check if there is actual movement (even residual)
      bool moving = (abs(smoothVx)>0.5f || abs(smoothVy)>0.5f || abs(smoothTwist)>0.5f) && (speed > 0.1f);
      
      if(moving) {
        // Phase advances only if we are moving
        phase += (speed * 0.2f) * dt * 5.0f; 
        if(phase >= 1.0f) phase -= 1.0f;
      } else {
        // If stopped, smoothly "wind" the phase to the nearest convenient position (0.0, 0.25, 0.5, 0.75), 
        // so all legs stand firmly on the ground.
        // For simplicity, we currently just smoothly reset the phase to zero if the amplitude has dropped.
        if (phase > 0.01f) {
             phase += (1.0f - phase) * 2.0f * dt; // Smooth return to 0
             if(phase >= 0.99f) phase = 0.0f;
        } else {
             phase = 0.0f;
        }
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
          // Leg lift height. Can be tied to motion amplitude for a more natural step at low speeds.
          legZ[i] = sin(swingProgress * PI_F) * 35.0f; 
          float stride = -cos(swingProgress * PI_F);
          // Use smoothed values (smoothVx, smoothVy) instead of sharp ones (vx, vy)
          legX[i] = smoothVx * stride;
          legY[i] = smoothVy * stride;
        } else {
          // Legs on the ground push in the opposite direction
          legX[i] = -smoothVx;
          legY[i] = -smoothVy;
          legZ[i] = 0; 
        }
      }

      // Use smoothed twist
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

// ---------------------------------------------------------------------------
// 5. LEG SYSTEM
// ---------------------------------------------------------------------------
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

    void run(float x, float y, float z, float t) {
      float lx = (150 + x) * cfg.xSign;
      float ly = (0 + y) * cfg.ySign;
      float lz = z;

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
  else if(type == WStype_CONNECTED) {
    String msg = "{\"cfg\":["; for(int i=0; i<4; i++) { msg += String(CFG[i].mountDeg); if(i<3) msg += ","; } msg += "]}"; webSocket.sendTXT(num, msg);
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
  <title>IK4=75 RTOS</title>
  <style>
    :root { --bg: #0b0b0b; --panel: #141414; --accent: #00f3ff; --warn: #ff0055; --text: #eee; }
    body { margin: 0; background: var(--bg); color: var(--text); font-family: 'Segoe UI', monospace; overflow: hidden; }
    canvas { position: absolute; top: 0; left: 0; z-index: -1; }
    .ui { position: absolute; width: 100%; height: 100%; pointer-events: none; display: flex; justify-content: space-between; padding: 10px; box-sizing: border-box; }
    .panel { width: 300px; background: var(--panel); border: 1px solid #333; padding: 15px; pointer-events: auto; display: flex; flex-direction: column; border-radius: 6px; box-shadow: 0 0 20px rgba(0,0,0,0.8); }
    h3 { border-bottom: 2px solid var(--accent); padding-bottom: 5px; margin: 0 0 10px 0; font-size: 12px; letter-spacing: 2px; text-transform: uppercase; color: var(--accent); }
    button { padding: 12px; background: rgba(0,243,255,0.05); color: var(--accent); border: 1px solid var(--accent); cursor: pointer; font-weight: bold; margin-bottom: 5px; width: 100%; transition: 0.2s; font-size: 11px; }
    button:hover { background: var(--accent); color: #000; }
    button.sel { background: var(--accent); color: #000; }
    button.calib { border-color: #fc0; color: #fc0; }
    button.warn { border-color: var(--warn); color: var(--warn); }
    button.warn:hover { background: var(--warn); color: #fff; }
    input[type=range] { width: 100%; accent-color: var(--accent); cursor: pointer; height: 5px; background: #333; margin: 10px 0; }
    .horizon-box { width: 100%; height: 120px; background: #000; border: 1px solid var(--accent); margin-bottom: 10px; position: relative; overflow: hidden; border-radius: 4px; }
    .horizon-sky { width: 300%; height: 300%; background: linear-gradient(to bottom, #005566 50%, #333 50%); position: absolute; top: -100%; left: -100%; transition: transform 0.05s linear; pointer-events: none; opacity: 0.6; }
    .horizon-line { width: 100%; height: 2px; background: var(--accent); position: absolute; top: 50%; left: 0; box-shadow: 0 0 5px var(--accent); }
    .horizon-data { position: absolute; top: 5px; left: 5px; font-size: 10px; font-weight: bold; color: #fff; text-shadow: 1px 1px 0 #000; }
    .sticks { display: flex; justify-content: space-between; margin: 10px 0; }
    .stick-box { width: 120px; height: 120px; border: 1px solid #333; background: #000; position: relative; border-radius: 4px; }
    .stick-dot { width: 8px; height: 8px; background: #fff; border-radius: 50%; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); box-shadow: 0 0 8px var(--accent); }
    .status-bar { margin-top: auto; padding-top: 10px; border-top: 1px solid #333; font-size: 10px; display: flex; justify-content: space-between; }
    .ok { color: #0f0; } .bad { color: var(--warn); }
  </style>
</head>
<body>
<div class="ui">
  <div class="panel">
    <h3>Flight Deck</h3>
    <div class="horizon-box">
      <div class="horizon-sky" id="sky"></div>
      <div class="horizon-line"></div>
      <div class="horizon-data">P:<span id="val-p">0</span> R:<span id="val-r">0</span></div>
    </div>
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:5px;">
      <button onclick="send({cmd:'mode', val:'stand'})">STAND (X)</button>
      <button id="btn-walk" onclick="toggleWalk()">AUTO WALK (O)</button>
    </div>
    <button class="calib" onclick="send({cmd:'calib'})">CALIBRATE GYRO</button>
    <div class="sticks">
      <div class="stick-box"><div id="dot-l" class="stick-dot"></div></div>
      <div class="stick-box"><div id="dot-r" class="stick-dot"></div></div>
    </div>
    <input type="range" min="-140" max="-40" value="-60" oninput="send({cmd:'h', val:this.value})">
    <div class="status-bar"><span id="imu-stat" class="bad">IMU DEAD</span><span id="net-stat" class="bad">OFFLINE</span></div>
  </div>
  <div class="panel">
    <h3>Engineering</h3>
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:5px; margin-bottom:10px;">
      <button id="l0" class="sel" onclick="selLeg(0)">FL</button><button id="l1" onclick="selLeg(1)">FR</button>
      <button id="l2" onclick="selLeg(2)">BL</button><button id="l3" onclick="selLeg(3)">BR</button>
    </div>
    <label style="font-size:10px; color:#aaa;">HIP</label><input type="range" min="-45" max="45" value="0" oninput="move(0,this.value)">
    <label style="font-size:10px; color:#aaa;">FEMUR</label><input type="range" min="-90" max="90" value="0" oninput="move(1,this.value)">
    <label style="font-size:10px; color:#aaa;">TIBIA</label><input type="range" min="-90" max="90" value="0" oninput="move(2,this.value)">
    <label style="font-size:10px; color:var(--accent);">TWIST</label><input type="range" min="-45" max="45" value="0" oninput="move(3,this.value)">
    <hr style="border:0; border-top:1px solid #333; margin:10px 0;">
    <h3>Diagnostics</h3>
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:5px;">
      <button class="warn" onclick="send({cmd:'test', val:1})">TEST 4</button>
      <button class="warn" onclick="send({cmd:'test', val:2})">TEST 16</button>
    </div>
  </div>
</div>
<script type="module">
  import * as THREE from 'https://esm.sh/three@0.160.0';
  import { OrbitControls } from 'https://esm.sh/three@0.160.0/examples/jsm/controls/OrbitControls.js';
  window.ws = new WebSocket(`ws://${location.hostname}:81/`);
  const scene = new THREE.Scene(); scene.background = new THREE.Color(0x0b0b0b);
  const cam = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 1, 3000); 
  cam.position.set(0, 400, 500); 
  const ren = new THREE.WebGLRenderer({antialias:true}); ren.setSize(window.innerWidth, window.innerHeight); document.body.appendChild(ren.domElement);
  new OrbitControls(cam, ren.domElement);
  scene.add(new THREE.GridHelper(1000, 100, 0x00f3ff, 0x141414)); scene.add(new THREE.AmbientLight(0xffffff, 0.6));
  const spot = new THREE.PointLight(0x00f3ff, 1.0, 1000); spot.position.set(0, 500, 0); scene.add(spot);
  const matWire = new THREE.MeshBasicMaterial({color:0x00f3ff, wireframe:true, opacity:0.3});
  const matSolid = new THREE.MeshStandardMaterial({color:0x1a1a1a, roughness:0.4}); 
  const body = new THREE.Group();
  body.add(new THREE.Mesh(new THREE.BoxGeometry(77, 46, 134), matSolid));
  body.add(new THREE.Mesh(new THREE.BoxGeometry(77, 46, 134), matWire));
  body.add(new THREE.AxesHelper(60)); body.position.y = 23; scene.add(body);
  const legsVis = [];
  function createLeg(x, z, mountDeg) {
    const root = new THREE.Group(); root.position.set(x, 23, z); 
    const hip = new THREE.Group(); hip.rotation.y = mountDeg * 0.01745; root.add(hip); hip.add(new THREE.AxesHelper(20));
    const twist = new THREE.Group(); twist.position.x = 20; hip.add(twist); 
    const femur = new THREE.Group(); femur.position.x = 46; twist.add(femur); femur.add(new THREE.AxesHelper(20));
    const tibia = new THREE.Group(); tibia.position.x = 69; femur.add(tibia);
    hip.add(new THREE.Mesh(new THREE.BoxGeometry(20,10,10).translate(10,0,0), matWire));
    twist.add(new THREE.Mesh(new THREE.BoxGeometry(46,12,12).translate(23,0,0), matWire));
    femur.add(new THREE.Mesh(new THREE.BoxGeometry(69,8,8).translate(34.5,0,0), matWire));
    tibia.add(new THREE.Mesh(new THREE.BoxGeometry(123,5,5).translate(61.5,0,0), matWire));
    root.userData = { h: hip, tw: twist, f:femur, t:tibia, off: mountDeg };
    scene.add(root); legsVis.push(root);
  }
  createLeg(-38.8, -67.4, 135); createLeg(38.8, -67.4, 45); createLeg(-38.8, 67.4, 45); createLeg(38.8, 67.4, -225);
  function loop() { requestAnimationFrame(loop); ren.render(scene, cam); } loop();
  ws.onopen = () => { document.getElementById('net-stat').innerText = "ONLINE"; document.getElementById('net-stat').className = "ok"; };
  ws.onclose = () => { document.getElementById('net-stat').innerText = "OFFLINE"; document.getElementById('net-stat').className = "bad"; };
  ws.onmessage = (e) => {
    try {
      const d = JSON.parse(e.data);
      if(d.p !== undefined) {
        document.getElementById('imu-stat').innerText = "IMU LIVE"; document.getElementById('imu-stat').className = "ok";
        document.getElementById('val-p').innerText = d.p.toFixed(0);
        document.getElementById('val-r').innerText = d.r.toFixed(0);
        body.rotation.x = d.p * 0.01745; body.rotation.z = d.r * 0.01745;
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
      const lx = gp.axes[0]; const ly = gp.axes[1]; const rx = gp.axes[2]; const ry = gp.axes[3];
      document.getElementById('dot-l').style.transform = `translate(${lx * 50}px, ${ly * 50}px)`;
      document.getElementById('dot-r').style.transform = `translate(${rx * 50}px, ${ry * 50}px)`;
      const data = { cmd: 'pad', lx: lx, ly: ly, rx: rx, ry: ry, btn: gp.buttons.map(b => b.pressed ? 1 : 0) };
      if(ws.readyState === 1) ws.send(JSON.stringify(data));
    }
  }, 50);
  let activeLeg = 0; let walking = false;
  window.toggleWalk = function() {
    walking = !walking;
    const btn = document.getElementById('btn-walk');
    btn.innerText = walking ? "STOP WALK" : "AUTO WALK (O)";
    btn.style.color = walking ? "#000" : "#00f3ff"; btn.style.background = walking ? "#00f3ff" : "rgba(0,243,255,0.05)";
    send({cmd:'mode', val: walking ? 'walk' : 'stand', auto: walking});
  }
  window.selLeg = function(id) {
    activeLeg = id; document.querySelectorAll('.panel button.sel').forEach(b => b.classList.remove('sel'));
    document.getElementById('l'+id).classList.add('sel'); send({cmd:'active', val: id});
  }
  window.send = function(data) { if(ws.readyState===1) ws.send(JSON.stringify(data)); }
  window.move = function(id, val) { send({cmd:'servo', leg: activeLeg, id:id, val:parseInt(val)}); }
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
    vTaskDelay(5 / portTICK_PERIOD_MS); // Give the core a rest
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
         isCalibrating = true; calibCount = 0; calibSumP = 0; calibSumR = 0;
         sharedState.calibrateIMU = false; // Reset the flag
      }
      xSemaphoreGive(stateMutex);
    }

    // 2. IMU UPDATE
    if(imuConnected) readRawMPU(dt);

    unsigned long now = millis();

    // 3. WATCHDOG
    if(gamepadActive && (now - lastPadPacket > 500)) {
       if (xSemaphoreTake(stateMutex, 0) == pdTRUE) {
           sharedState.targetX=0; sharedState.targetY=0; sharedState.targetTwist=0; 
           sharedState.inputZ=DEFAULT_Z; 
           xSemaphoreGive(stateMutex);
       }
       gamepadActive=false;
    }

    // 4. TEST MODE OVERRIDE
    if (localState.mode == 3) { // MODE_TEST_SINE
        setRGB(255, 165, 0); // Orange
        float angle = 90.0f + 40.0f * sin(now / 300.0f);
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
        if(abs(currentX-localState.targetX)>0.1) currentX += (localState.targetX-currentX)*(ACCEL*dt*5.0); else currentX=localState.targetX;
        
        if(abs(localState.targetY)<1) localState.targetY=0; 
        if(abs(currentY-localState.targetY)>0.1) currentY += (localState.targetY-currentY)*(ACCEL*dt*5.0); else currentY=localState.targetY;
        
        if(abs(currentTwist-localState.targetTwist)>0.1) currentTwist += (localState.targetTwist-currentTwist)*(ACCEL*dt*5.0); else currentTwist=localState.targetTwist;
        
        if(abs(currentHeight-localState.inputZ)>0.1f) currentHeight += (localState.inputZ-currentHeight)*0.1f;

        // 6. IK ENGINE (Only if not in Manual/Test mode)
        if(statusPCA && localState.mode != 2) { 
          float speedVal = (localState.mode == 1) ? 1.0f : 0.0f;
          gait.run(speedVal, currentX, currentY, currentTwist, dt);

          float balZ[4] = {0,0,0,0};
          // FIX 2: Always balance when IMU is connected
          if(imuConnected) {
             float p = constrain(pitch, -20.0f, 20.0f) * (PI_F/180.0f);
             float r = constrain(roll, -20.0f, 20.0f) * (PI_F/180.0f);
             for(int i=0; i<4; i++) {
               float lx = 80 * legs[i].cfg.xSign;
               float ly = 60 * legs[i].cfg.ySign;
               // sin() provides more accurate height compensation during pitch/roll
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
  
  rgb.begin(); rgb.setBrightness(80); setRGB(0, 0, 255); 
  
  Serial.print("[PCA] Init on 4/5... ");
  Wire.begin(PIN_SERVO_SDA, PIN_SERVO_SCL, 400000);
  pca.begin(); pca.setPWMFreq(50);
  Wire.beginTransmission(0x40);
  if(Wire.endTransmission()==0) { statusPCA=true; Serial.println("OK"); }
  else { Serial.println("FAIL"); setRGB(255,0,0); }

  imuConnected = initRawMPU(); 
  if(!imuConnected) setRGB(255,100,0);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) { delay(100); }
  Serial.print("[WiFi] IP: "); Serial.println(WiFi.localIP());
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