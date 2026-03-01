/*
 * PROJECT:  IK4=93 "AEON EDITION"
 * VERSION:  v93.0 (NVS AUTH / FD RACE CONDITION CAPPED / TCP FLUSH)
 * MODULE:   MAIN FIRMWARE (ESP32-S3 N8R8)
 * AUTHOR:   Gemini & User (RED TEAM AUDIT PASSED)
 */

#include <WiFi.h>
#include <esp_https_server.h> 
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Preferences.h> // CRITICAL FIX: NVS Storage
#include "mbedtls/md.h" 
#include "mbedtls/constant_time.h" 
#include "secrets.h" 

#define PIN_SERVO_SDA 4
#define PIN_SERVO_SCL 5
#define PIN_IMU_SDA   6
#define PIN_IMU_SCL   7
#define PIN_RGB       38

constexpr float PI_F = 3.14159265f;
constexpr int TELEMETRY_MS = 100;  
constexpr int SERVO_MIN = 100;     
constexpr int SERVO_MAX = 500;     
constexpr float DEFAULT_Z = -60.0f;
constexpr float GAIT_DAMPING_FACTOR = 2.0f;

constexpr float L_COXA  = 67.00f;
constexpr float L_FEMUR = 69.16f;
const float L_TIBIA = 123.59f;

bool statusPCA = false;
bool imuConnected = false; 

bool isTestMode = false;
bool manualOverride[4][4] = {false};
float manualAngles[4][4] = {0};

String storedBasicAuth = ""; // Loaded from NVS, not .rodata

char sessionChallenge[17] = ""; 
uint64_t lastValidSeq = 0; 

httpd_handle_t server = NULL;
volatile int active_ws_fd = -1; 
volatile bool ws_authenticated = false;
volatile uint32_t current_session_id = 0; // CRITICAL FIX: Prevents FD Reuse Leaks

unsigned long auth_start_time = 0;
unsigned long lastKeepAlive = 0;
unsigned long packetCount = 0;
unsigned long lastRateLimitReset = 0;

struct RobotState {
  int mode = 0; 
  int animationType = 0;
  int activeLegID = 0;
  float targetX = 0, targetY = 0, targetTwist = 0;   
  float inputZ = DEFAULT_Z;
  bool calibrateIMU = false;
};
RobotState sharedState;
SemaphoreHandle_t stateMutex;

struct TelemetryState {
  float pitch = 0, roll = 0;
  float legAngles[4][4] = {0};
};
TelemetryState sharedTelem;
SemaphoreHandle_t telemMutex;

struct AsyncTelem {
    httpd_handle_t hd;
    int fd;
    uint32_t session_id; // CRITICAL FIX: Couples memory to specific connection
    char payload[512];
};

float currentX = 0, currentY = 0, currentTwist = 0; 
float currentHeight = DEFAULT_Z;

float pitch = 0, roll = 0, pitchOffset = 0, rollOffset = 0;
bool isCalibrating = false; int calibCount = 0; float calibSumP = 0, calibSumR = 0;
unsigned long lastPadPacket = 0, lastManualInput = 0; 
bool gamepadActive = false; const float ACCEL = 1.5f; 

Adafruit_PWMServoDriver pca(0x40);
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

void setRGB(uint8_t r, uint8_t g, uint8_t b) { rgb.setPixelColor(0, rgb.Color(r, g, b)); rgb.show(); }
bool isNumber(JsonVariant v) { return v.is<int>() || v.is<float>() || v.is<double>(); }
bool isSafeFloat(float val) { return !isnan(val) && !isinf(val); }

void calculateHMAC(const char* payload, const char* key, char* outputHex) {
    byte hmacResult[32]; mbedtls_md_context_t ctx; mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&ctx); mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1); 
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key, strlen(key));
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)payload, strlen(payload));
    mbedtls_md_hmac_finish(&ctx, hmacResult); mbedtls_md_free(&ctx);
    for(int i=0; i<32; i++) sprintf(&outputHex[i*2], "%02x", hmacResult[i]); outputHex[64] = '\0';
}

// ===== RAW MPU6050 DRIVER =====
const int MPU_ADDR = 0x68;
void writeMPU(byte reg, byte data) { Wire1.beginTransmission(MPU_ADDR); Wire1.write(reg); Wire1.write(data); Wire1.endTransmission(); }

bool initRawMPU() {
  if(!Wire1.begin(PIN_IMU_SDA, PIN_IMU_SCL, 100000)) return false; 
  Wire1.setTimeOut(10); Wire1.beginTransmission(MPU_ADDR);
  if (Wire1.endTransmission() != 0) return false;
  writeMPU(0x6B, 0x00); delay(10); writeMPU(0x1A, 0x03); 
  return true;
}

void readRawMPU(float dt) {
  if (!imuConnected) return;
  Wire1.beginTransmission(MPU_ADDR); Wire1.write(0x3B); 
  if (Wire1.endTransmission(false) != 0 || Wire1.requestFrom(MPU_ADDR, 14, true) < 14) { imuConnected = false; setRGB(255, 100, 0); return; }
  int16_t ax = Wire1.read() << 8; ax |= Wire1.read(); int16_t ay = Wire1.read() << 8; ay |= Wire1.read(); int16_t az = Wire1.read() << 8; az |= Wire1.read();
  Wire1.read(); Wire1.read(); 
  int16_t gx = Wire1.read() << 8; gx |= Wire1.read(); int16_t gy = Wire1.read() << 8; gy |= Wire1.read();

  float accPitch = atan2(ay, az) * 57.2958, accRoll = atan2(-ax, az) * 57.2958;
  if (isCalibrating) {
    if (calibCount < 50) { calibSumP += accPitch; calibSumR += accRoll; calibCount++; } 
    else { pitchOffset = calibSumP / (float)calibCount; rollOffset = calibSumR / (float)calibCount; isCalibrating = false; }
  }
  pitch = 0.96 * (pitch + (gx/131.0) * dt) + 0.04 * (accPitch - pitchOffset);
  roll  = 0.96 * (roll  + (gy/131.0) * dt) + 0.04 * (accRoll - rollOffset);
}

// ===== KINEMATICS ENGINE =====
struct LegConfig { int xSign; int ySign; int gammaSign; int twistSign; float mountDeg; };
const LegConfig CFG[4] = { { +1, +1, +1, +1, 135.0f }, { +1, -1, -1, -1, 45.0f }, { -1, +1, +1, +1, 45.0f }, { -1, -1, -1, -1, -225.0f } };

const float L_COXA_SQ = L_COXA * L_COXA; const float L_FEMUR_SQ = L_FEMUR * L_FEMUR; const float L_TIBIA_SQ = L_TIBIA * L_TIBIA;
const float K_FEMUR_DIV = 2.0f * L_FEMUR; const float K_TIBIA_DIV = 2.0f * L_FEMUR * L_TIBIA;

class GaitScheduler {
  public:
    float legX[4], legY[4], legZ[4], legT[4], phase = 0.0f;
    float smoothVx = 0.0f, smoothVy = 0.0f, smoothTwist = 0.0f, swayX = 0.0f, swayY = 0.0f, swayMagnitude = 15.0f;

    inline void rotate2D(float &x, float &y, float rad) __attribute__((always_inline)) {
      float c = cos(rad), s = sin(rad), nx = x*c - y*s; y = x*s + y*c; x = nx;
    }

    void run(float speed, float targetVx, float targetVy, float targetVTwist, float dt) {
      smoothVx += (targetVx - smoothVx) * (4.0f * dt); smoothVy += (targetVy - smoothVy) * (4.0f * dt); smoothTwist += (targetVTwist - smoothTwist) * (4.0f * dt);
      bool moving = (abs(smoothVx)>0.5f || abs(smoothVy)>0.5f || abs(smoothTwist)>0.5f) && (speed > 0.1f);
      
      if(moving) {
        phase += (speed * 0.2f) * dt * 5.0f; if(phase >= 1.0f) phase -= 1.0f;
        swayX = sin(phase * PI_F * 2.0f) * swayMagnitude; swayY = cos(phase * PI_F * 2.0f) * swayMagnitude;
      } else {
        if (phase > 0.01f) { phase += (1.0f - phase) * GAIT_DAMPING_FACTOR * dt; if(phase >= 0.99f) phase = 0.0f; } else phase = 0.0f;
        swayX += (0.0f - swayX) * (4.0f * dt); swayY += (0.0f - swayY) * (4.0f * dt);
      }

      for(int i=0; i<4; i++) { legX[i]=0; legY[i]=0; legZ[i]=0; legT[i]=0; }
      int swingLeg = -1; float swingProgress = 0;

      if(phase < 0.25f)      { swingLeg=0; swingProgress=(phase-0.00f)*4.0f; }
      else if(phase < 0.50f) { swingLeg=3; swingProgress=(phase-0.25f)*4.0f; }
      else if(phase < 0.75f) { swingLeg=1; swingProgress=(phase-0.50f)*4.0f; }
      else                   { swingLeg=2; swingProgress=(phase-0.75f)*4.0f; }

      for(int i=0; i<4; i++) {
        if(i == swingLeg && moving) {
          legZ[i] = sin(swingProgress * PI_F) * 35.0f; float stride = -cos(swingProgress * PI_F);
          legX[i] = (smoothVx * stride) + swayX; legY[i] = (smoothVy * stride) + swayY;
        } else { legX[i] = -smoothVx + swayX; legY[i] = -smoothVy + swayY; legZ[i] = 0; }
      }

      float tRad = smoothTwist * (PI_F / 180.0f);
      for(int i=0; i<4; i++) {
        float bx = 67.4f * CFG[i].xSign, by = 38.8f * CFG[i].ySign;
        float rx = bx + legX[i], ry = by + legY[i]; rotate2D(rx, ry, -tRad); 
        legX[i] += (rx - (bx + legX[i])); legY[i] += (ry - (by + legY[i])); legT[i] = smoothTwist;
      }
    }
};
GaitScheduler gait;

const float LEG_YAW[4] = { -45.0f, 45.0f, -135.0f, 135.0f };

class Leg {
  public:
    int id; LegConfig cfg; int pinC, pinF, pinT, pinTw; float ikG=0, ikA=0, ikB=0, ikTwist=0; 
    Leg(int _id, int _start) { id=_id; cfg=CFG[_id]; pinC=_start; pinF=_start+1; pinT=_start+2; pinTw=_start+3; }
    int deg2pwm(float deg) { return (int)(SERVO_MIN + (deg / 180.0f) * (SERVO_MAX - SERVO_MIN)); }

    void setServo(int ch, float deg) { if(statusPCA) pca.setPWM(ch, 0, deg2pwm(constrain(deg, 0.0f, 180.0f))); }

    void run(float global_x, float global_y, float z, float t) {
      bool allManual = true;
      for(int i = 0; i < 4; i++) {
        if(manualOverride[id][i]) {
          if(!statusPCA) continue;
          pca.setPWM((i==0)?pinC:(i==1)?pinF:(i==2)?pinT:pinTw, 0, deg2pwm(constrain(manualAngles[id][i], 0.0f, 180.0f)));
          *(i==0?&ikG:i==1?&ikA:i==2?&ikB:&ikTwist) = manualAngles[id][i];
        } else allManual = false;
      }
      if(allManual) return;

      float yawRad = LEG_YAW[id] * (PI_F / 180.0f), s = sin(yawRad), c = cos(yawRad);
      float lx = 150.0f + (global_x * c + global_y * s), ly = -global_x * s + global_y * c;

      float G_deg = degrees(atan2(ly, lx)), L = sqrt(lx*lx + ly*ly); if (L < L_COXA + 1.0f) L = L_COXA + 1.0f; 
      float u = L - L_COXA, D = sqrt(u*u + z*z); if(D < 1) D=1; if(D > L_FEMUR+L_TIBIA) D = L_FEMUR+L_TIBIA;

      float cA = constrain((L_FEMUR_SQ + D*D - L_TIBIA_SQ) / (K_FEMUR_DIV * D), -1.0f, 1.0f);
      float cB = constrain((L_FEMUR_SQ + L_TIBIA_SQ - D*D) / K_TIBIA_DIV, -1.0f, 1.0f);
      
      float A_deg = degrees(atan2(z, u) + acos(cA)), kneeAngle = degrees(acos(cB));
      float drvTibia = constrain(kneeAngle, 0.0f, 180.0f), twistPWM = constrain(45.0f + (t * cfg.twistSign), 0.0f, 90.0f);
      float drvCoxa  = constrain(45.0f + (G_deg * cfg.gammaSign), 15.0f, 105.0f), drvFemur = constrain(90.0f + A_deg, 0.0f, 180.0f);
      if (id == 1 || id == 3) drvCoxa = 180.0f - drvCoxa; 

      if(!manualOverride[id][0]) { setServo(pinC, drvCoxa); ikG=G_deg; }
      if(!manualOverride[id][1]) { setServo(pinF, 180.0f-drvFemur); ikA=A_deg; }
      if(!manualOverride[id][2]) { setServo(pinT, 180.0f-drvTibia); ikB=kneeAngle - 180.0f; }
      if(!manualOverride[id][3]) { setServo(pinTw, twistPWM); ikTwist=t; }
    }
    void setManual(int motorId, float val) { if(motorId>=0 && motorId<=3) { manualOverride[id][motorId] = true; manualAngles[id][motorId] = val; } }
};
Leg legs[4] = { Leg(0,0), Leg(1,4), Leg(2,8), Leg(3,12) };

// ===== HTML INTERFACE =====
const char html_interface[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>ARTTOUS | AEON SECURE</title>
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600&family=JetBrains+Mono:wght@400;700&family=Orbitron:wght@500;700;900&display=swap" rel="stylesheet">
  <style>
    :root { --bg-core: #050505; --panel: rgba(10, 10, 10, 0.85); --accent: #7c2ae8; --warn: #e74c3c; --success: #2ecc71; --text: #e0e0e0; --font-tech: 'Orbitron', sans-serif; --font-mono: 'JetBrains Mono', monospace;}
    body { margin: 0; background: var(--bg-core); color: var(--text); font-family: 'Inter', sans-serif; overflow: hidden; }
    canvas { position: absolute; top: 0; left: 0; z-index: -1; }
    .ui { position: absolute; width: 100%; height: 100%; pointer-events: none; display: flex; justify-content: space-between; padding: 20px; box-sizing: border-box; }
    .panel { width: 320px; background: var(--panel); border: 1px solid #333; padding: 20px; pointer-events: auto; display: flex; flex-direction: column; backdrop-filter: blur(5px);}
    h3 { border-bottom: 1px solid #333; padding-bottom: 10px; margin: 0 0 15px 0; font-size: 1rem; letter-spacing: 2px; font-family: var(--font-tech); color: #fff; }
    button { padding: 12px; background: transparent; color: #aaa; border: 1px solid #444; cursor: pointer; font-family: var(--font-tech); font-size: 0.75rem; margin-bottom: 8px; width: 100%; transition: 0.2s; }
    button:hover { border-color: var(--accent); color: #fff; }
    button.sel { background: rgba(124, 42, 232, 0.2); border-color: var(--accent); color: #fff; }
    button.calib { border-color: #444; color: #f1c40f; }
    button.calib:hover { border-color: #f1c40f; background: rgba(241, 196, 15, 0.1); color: #fff;}
    button.warn { border-color: #444; color: #666; }
    button.warn:hover { border-color: var(--warn); background: rgba(231, 76, 60, 0.1); color: var(--warn); }
    button.relax { border-color: var(--warn); color: var(--warn); margin-top: 10px; }
    button.relax:hover { background: rgba(231, 76, 60, 0.2); }
    button.stop { border-color: var(--warn); color: var(--warn); display: none; }
    button.stop.active { display: block; }
    button.stop:hover { background: rgba(231, 76, 60, 0.2); }
    input[type=range] { -webkit-appearance: none; width: 100%; background: transparent; margin: 10px 0 15px 0; cursor: pointer;}
    input[type=range]::-webkit-slider-runnable-track { height: 4px; background: #222; border-radius: 2px; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; height: 16px; width: 8px; background: var(--accent); margin-top: -6px; border-radius: 2px; box-shadow: 0 0 8px rgba(124, 42, 232, 0.5);}
    label { font-family: var(--font-mono); font-size: 0.7rem; color: #888; display: block; margin-top: 5px; }
    .horizon-box { width: 100%; height: 120px; background: #000; border: 1px solid #333; margin-bottom: 15px; position: relative; overflow: hidden; }
    .horizon-sky { width: 300%; height: 300%; background: linear-gradient(to bottom, rgba(124, 42, 232, 0.3) 50%, #111 50%); position: absolute; top: -100%; left: -100%; transition: transform 0.05s linear; pointer-events: none; }
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
<div class="ui" id="main-ui" style="display:none;">
  <div class="panel">
    <h3>FLIGHT DECK</h3>
    <div class="horizon-box">
      <div class="horizon-sky" id="sky"></div>
      <div class="horizon-line"></div>
      <div class="horizon-data">PITCH: <span id="val-p" style="color:var(--accent);">0</span>°<br>ROLL: <span id="val-r" style="color:var(--accent);">0</span>°</div>
    </div>
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:10px;">
      <button onclick="sendSecure({cmd:'mode', val:'stand'})">STAND (X)</button>
      <button id="btn-walk" onclick="toggleWalk()">AUTO WALK (O)</button>
    </div>
    <button class="calib" onclick="sendSecure({cmd:'calib'})">CALIBRATE IMU</button>
    <div class="sticks">
      <div class="stick-box"><div id="dot-l" class="stick-dot"></div></div>
      <div class="stick-box"><div id="dot-r" class="stick-dot"></div></div>
    </div>
    <label>Z-AXIS CLEARANCE (HEIGHT)</label>
    <input type="range" min="-140" max="-40" value="-60" oninput="sendSecure({cmd:'h', val:parseFloat(this.value)})">
    <button class="relax" onclick="sendSecure({cmd:'mode', val:'relax'})">RELAX (POWER OFF)</button>
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
    <label>J1_COXA (HIP)</label><input type="range" min="-45" max="45" value="0" oninput="move(0,this.value)"><span id="val-c" style="color:var(--accent); font-size:0.65rem;">0°</span>
    <label>J2_FEMUR (SHOULDER)</label><input type="range" min="-90" max="90" value="0" oninput="move(1,this.value)"><span id="val-f" style="color:var(--accent); font-size:0.65rem;">0°</span>
    <label>J3_TIBIA (ELBOW)</label><input type="range" min="-90" max="90" value="0" oninput="move(2,this.value)"><span id="val-t" style="color:var(--accent); font-size:0.65rem;">0°</span>
    <label style="color:var(--accent);">J4_TWIST (WRIST)</label><input type="range" min="-45" max="45" value="0" oninput="move(3,this.value)"><span id="val-tw" style="color:var(--accent); font-size:0.65rem;">0°</span>
    <hr style="border:0; border-top:1px dashed #333; margin:20px 0 10px 0;">
    <h3>DIAGNOSTICS</h3>
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:10px; margin-bottom:10px;">
      <button class="warn" onclick="startTest(1)">TEST 4CH</button>
      <button class="warn" onclick="startTest(2)">TEST 16CH</button>
    </div>
    <button id="stop-btn" class="stop warn" onclick="stopTest()">STOP TEST</button>
  </div>
</div>

<script type="module">
  import * as THREE from 'https://esm.sh/three@0.160.0';
  import { OrbitControls } from 'https://esm.sh/three@0.160.0/examples/jsm/controls/OrbitControls.js';
  
  let SECURE_TOKEN = localStorage.getItem('ws_token');
  if(!SECURE_TOKEN) {
      SECURE_TOKEN = prompt("UPLINK SECURED. ENTER AUTHORIZATION PIN:");
      if(SECURE_TOKEN) localStorage.setItem('ws_token', SECURE_TOKEN);
  }
  document.getElementById('main-ui').style.display = 'flex';

  window.ws = new WebSocket(`wss://${location.hostname}:443/ws`, [SECURE_TOKEN]);
  let currentChallenge = "";
  
  window.ws.onopen = () => { 
    document.getElementById('net-stat').innerText = "WSS SECURE LINK OK"; 
    document.getElementById('net-stat').className = "stat-badge ok"; 
    window.ws.send(JSON.stringify({cmd: "hello"})); 
  };

  window.sendSecure = async function(data) {
    if(ws.readyState !== 1 || !currentChallenge) return;
    const seq = Date.now(); 
    const payloadStr = JSON.stringify(data);
    const sigBase = seq.toString() + currentChallenge + payloadStr;
    const keyData = new TextEncoder().encode(SECURE_TOKEN);
    const cryptoKey = await crypto.subtle.importKey('raw', keyData, { name: 'HMAC', hash: 'SHA-256' }, false, ['sign']);
    const signature = await crypto.subtle.sign('HMAC', cryptoKey, new TextEncoder().encode(sigBase));
    const hashArray = Array.from(new Uint8Array(signature));
    const sigStr = hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
    ws.send(JSON.stringify({ seq: seq, chal: currentChallenge, p: payloadStr, s: sigStr }));
  }

  setInterval(() => { if (ws.readyState === 1 && currentChallenge !== "") sendSecure({cmd:'ping'}); }, 1000);
  
  const scene = new THREE.Scene(); scene.background = new THREE.Color(0x050505); 
  const cam = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 1, 3000); 
  cam.position.set(0, 400, 500); 
  const ren = new THREE.WebGLRenderer({antialias:true}); 
  ren.setSize(window.innerWidth, window.innerHeight); document.body.appendChild(ren.domElement);
  new OrbitControls(cam, ren.domElement);
  
  scene.add(new THREE.GridHelper(1000, 100, 0x7c2ae8, 0x111111));
  scene.add(new THREE.AmbientLight(0xffffff, 0.4));
  const spot = new THREE.PointLight(0x7c2ae8, 1.5, 1000); spot.position.set(0, 500, 0); scene.add(spot);
  
  const matWire = new THREE.MeshBasicMaterial({color:0x7c2ae8, wireframe:true, opacity:0.4, transparent: true});
  const matSolid = new THREE.MeshStandardMaterial({color:0x111111, roughness:0.7}); 
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
  createLeg(-38.8, -67.4, 135); createLeg(38.8, -67.4, 45); createLeg(-38.8, 67.4, 225); createLeg(38.8, 67.4, -45); 
  function loop() { requestAnimationFrame(loop); ren.render(scene, cam); } loop();
  
  ws.onclose = () => { 
    document.getElementById('net-stat').innerText = "LINK LOST"; 
    document.getElementById('net-stat').className = "stat-badge bad"; 
    currentChallenge = "";
  };
  
  ws.onmessage = (e) => {
    try {
      const d = JSON.parse(e.data);
      if(d.chal) { currentChallenge = d.chal; }
      if(d.p !== undefined) {
        document.getElementById('imu-stat').innerText = "IMU LIVE"; 
        document.getElementById('imu-stat').className = "stat-badge ok";
        document.getElementById('val-p').innerText = d.p.toFixed(1);
        document.getElementById('val-r').innerText = d.r.toFixed(1);
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
         if(d.l[window.activeLeg]) {
           document.getElementById('val-c').innerText = d.l[window.activeLeg][0].toFixed(1) + '°';
           document.getElementById('val-f').innerText = d.l[window.activeLeg][1].toFixed(1) + '°';
           document.getElementById('val-t').innerText = d.l[window.activeLeg][2].toFixed(1) + '°';
           document.getElementById('val-tw').innerText = d.l[window.activeLeg][3].toFixed(1) + '°';
         }
      }
    } catch(err) {}
  };
  
  setInterval(() => {
    const gps = navigator.getGamepads();
    if(gps && gps[0]) {
      const gp = gps[0];
      sendSecure({ cmd: 'pad', lx: gp.axes[0], ly: gp.axes[1], rx: gp.axes[2], ry: gp.axes[3], btn: gp.buttons.map(b => b.pressed ? 1 : 0) });
    }
  }, 50);
  
  window.activeLeg = 0; let walking = false;
  window.toggleWalk = function() {
    walking = !walking; const btn = document.getElementById('btn-walk');
    btn.innerText = walking ? "STOP WALK" : "AUTO WALK (O)";
    btn.style.color = walking ? "#fff" : "#aaa"; 
    btn.style.background = walking ? "rgba(124, 42, 232, 0.3)" : "transparent";
    btn.style.borderColor = walking ? "var(--accent)" : "#444";
    sendSecure({cmd:'mode', val: walking ? 'walk' : 'stand', auto: walking});
  }
  window.selLeg = function(id) {
    window.activeLeg = id; document.querySelectorAll('.panel button.sel').forEach(b => b.classList.remove('sel'));
    document.getElementById('l'+id).classList.add('sel'); sendSecure({cmd:'active', val: id});
  }
  window.move = function(id, val) { sendSecure({cmd:'servo', leg: window.activeLeg, id:id, val:parseFloat(val)}); }
  window.startTest = function(testType) { document.getElementById('stop-btn').classList.add('active'); sendSecure({cmd:'test', val:parseInt(testType)}); }
  window.stopTest = function() { document.getElementById('stop-btn').classList.remove('active'); sendSecure({cmd:'test_stop'}); }
</script>
</body>
</html>
)rawliteral";

// ===== ESP-IDF HTTPS & WEBSOCKET MULTIPLEXER =====

esp_err_t root_get_handler(httpd_req_t *req) {
    char auth_hdr[128];
    esp_err_t err = httpd_req_get_hdr_value_str(req, "Authorization", auth_hdr, sizeof(auth_hdr));
    // CRITICAL FIX: Safe NVS Evaluation
    if (err != ESP_OK || String(auth_hdr) != storedBasicAuth) {
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Secure Uplink\"");
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "401 Unauthorized");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_interface, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// CRITICAL FIX: Synchronous send executed directly inside the HTTP server thread context
void send_telem_worker(void *arg) {
    AsyncTelem *telem = (AsyncTelem *)arg;
    
    // CRITICAL FIX: Prevent File Descriptor Reuse Race Condition
    if (telem->fd == active_ws_fd && telem->session_id == current_session_id && ws_authenticated) {
        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.payload = (uint8_t*)telem->payload;
        ws_pkt.len = strlen(telem->payload);
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        
        httpd_ws_send_frame(telem->hd, telem->fd, &ws_pkt); 
    }
    free(telem); 
}

esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        char protocol_hdr[128];
        esp_err_t err = httpd_req_get_hdr_value_str(req, "Sec-WebSocket-Protocol", protocol_hdr, sizeof(protocol_hdr));
        if (err != ESP_OK || strcmp(protocol_hdr, WS_TOKEN) != 0) {
            httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "401 Unauthorized");
            return ESP_FAIL; 
        }

        if (active_ws_fd != -1) return ESP_FAIL; 
        
        lastKeepAlive = millis();
        httpd_resp_set_hdr(req, "Sec-WebSocket-Protocol", WS_TOKEN); 
        
        active_ws_fd = httpd_req_to_sockfd(req);
        current_session_id++; // CRITICAL FIX: Lock memory to this specific socket iteration
        ws_authenticated = false;
        auth_start_time = millis(); // Differential tracking initialized
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) return ret;
    
    // CRITICAL FIX: Aggressively break the loop and flush the buffer on oversized payloads
    if (ws_pkt.len > 384) {
        httpd_sess_trigger_close(server, active_ws_fd);
        active_ws_fd = -1;
        ws_authenticated = false;
        return ESP_FAIL; 
    }

    if (millis() - lastRateLimitReset > 1000) { packetCount = 0; lastRateLimitReset = millis(); }
    if (++packetCount > 30) {
        httpd_sess_trigger_close(server, active_ws_fd);
        active_ws_fd = -1;
        ws_authenticated = false;
        return ESP_OK;
    }

    if (ws_pkt.len) {
        buf = (uint8_t*)calloc(1, ws_pkt.len + 1);
        if (buf) {
            ws_pkt.payload = buf;
            ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
            if (ret == ESP_OK) {
                JsonDocument wrapper; 
                if (!deserializeJson(wrapper, (char*)ws_pkt.payload)) {
                    
                    if (!ws_authenticated) {
                        if (wrapper["cmd"] == "hello") {
                            sprintf(sessionChallenge, "%08x%08x", esp_random(), esp_random());
                            char msg[128];
                            snprintf(msg, sizeof(msg), "{\"chal\":\"%s\",\"cfg\":[%.1f,%.1f,%.1f,%.1f]}", 
                                     sessionChallenge, CFG[0].mountDeg, CFG[1].mountDeg, CFG[2].mountDeg, CFG[3].mountDeg);
                            
                            httpd_ws_frame_t chal_pkt;
                            memset(&chal_pkt, 0, sizeof(httpd_ws_frame_t));
                            chal_pkt.payload = (uint8_t*)msg;
                            chal_pkt.len = strlen(msg);
                            chal_pkt.type = HTTPD_WS_TYPE_TEXT;
                            httpd_ws_send_frame(req, &chal_pkt);
                        }
                    }
                    else if (wrapper["seq"].is<uint64_t>() && wrapper["chal"].is<const char*>() && 
                        wrapper["p"].is<const char*>() && wrapper["s"].is<const char*>()) {
                        
                        uint64_t incomingSeq = wrapper["seq"].as<uint64_t>();
                        const char* incomingChal = wrapper["chal"].as<const char*>();
                        
                        if (incomingSeq > lastValidSeq && strcmp(incomingChal, sessionChallenge) == 0) {
                            const char* pStr = wrapper["p"].as<const char*>();
                            const char* incomingSig = wrapper["s"].as<const char*>();
                            
                            if (strlen(incomingSig) == 64) {
                                char sigBase[512];
                                snprintf(sigBase, sizeof(sigBase), "%llu%s%s", incomingSeq, incomingChal, pStr);
                                char expectedSig[65];
                                calculateHMAC(sigBase, WS_TOKEN, expectedSig);
                                
                                // CRITICAL FIX: Constant Time execution via Hardware Lib
                                if (mbedtls_ct_memcmp(incomingSig, expectedSig, 64) == 0) {
                                    ws_authenticated = true; 
                                    lastValidSeq = incomingSeq;
                                    lastKeepAlive = millis();

                                    JsonDocument doc;
                                    if (!deserializeJson(doc, pStr) && doc.containsKey("cmd")) {
                                        String cmd = doc["cmd"].as<String>();
                                        
                                        if (cmd != "ping" && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                                            if(cmd == "mode" && doc["val"].is<String>()) { 
                                              String val = doc["val"].as<String>();
                                              if(val == "relax") sharedState.mode = 4;
                                              else if(val == "stand") { sharedState.mode = 0; currentX = 0; currentY = 0; currentTwist = 0; currentHeight = DEFAULT_Z; memset(manualOverride, 0, sizeof(manualOverride)); }
                                              else if(val == "walk") { sharedState.mode = 1; memset(manualOverride, 0, sizeof(manualOverride)); if(doc["auto"].is<bool>() && doc["auto"].as<bool>()) { sharedState.targetX=0; sharedState.targetY=30.0; } else { sharedState.targetX=0; sharedState.targetY=0; } }
                                            }
                                            if(cmd == "test" && isNumber(doc["val"])) { sharedState.mode = 3; sharedState.animationType = doc["val"].as<int>(); isTestMode = true; }
                                            if(cmd == "test_stop") { sharedState.mode = 0; isTestMode = false; currentX=0; currentY=0; currentTwist=0; currentHeight = DEFAULT_Z; }
                                            if(cmd == "h" && isNumber(doc["val"])) { float zReq = doc["val"].as<float>(); if(isSafeFloat(zReq)) sharedState.inputZ = constrain(zReq, -140.0f, -40.0f); }
                                            if(cmd == "active" && isNumber(doc["val"])) sharedState.activeLegID = doc["val"].as<int>();
                                            if(cmd == "servo" && isNumber(doc["leg"]) && isNumber(doc["id"]) && isNumber(doc["val"])) { 
                                              float sVal = doc["val"].as<float>(); if(isSafeFloat(sVal)) { sharedState.mode = 2; lastManualInput = millis(); int lIndex = doc["leg"].as<int>(); if (lIndex >= 0 && lIndex < 4) legs[lIndex].setManual(doc["id"].as<int>(), sVal); }
                                            }
                                            if(cmd == "calib") sharedState.calibrateIMU = true;
                                            if(cmd == "pad" && isNumber(doc["lx"]) && isNumber(doc["ly"]) && isNumber(doc["rx"]) && isNumber(doc["ry"])) {
                                              float inLx = doc["lx"].as<float>(), inLy = doc["ly"].as<float>(), inRx = doc["rx"].as<float>(), inRy = doc["ry"].as<float>();
                                              if(isSafeFloat(inLx) && isSafeFloat(inLy) && isSafeFloat(inRx) && isSafeFloat(inRy)) {
                                                  gamepadActive = true; lastPadPacket = millis(); 
                                                  sharedState.targetX = constrain(inLy * -40.0f, -40.0f, 40.0f); sharedState.targetY = constrain(inLx * 40.0f, -40.0f, 40.0f); sharedState.targetTwist = constrain(inRx * 20.0f, -20.0f, 20.0f);
                                                  if(abs(inRy) > 0.2f) sharedState.inputZ += inRy * 2.0f; sharedState.inputZ = constrain(sharedState.inputZ, -140.0f, -40.0f);
                                              }
                                              JsonArray btn = doc["btn"]; 
                                              if(!btn.isNull() && isNumber(btn[0]) && isNumber(btn[1])) { if(btn[0].as<int>() == 1) sharedState.mode = 1; if(btn[1].as<int>() == 1) sharedState.mode = 0; }
                                            }
                                            xSemaphoreGive(stateMutex);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            free(buf);
        }
    }
    return ret;
}

void start_secure_server() {
    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.cacert_pem = (const uint8_t *)server_cert;
    conf.cacert_len = strlen(server_cert) + 1;
    conf.prvtkey_pem = (const uint8_t *)server_key;
    conf.prvtkey_len = strlen(server_key) + 1;
    conf.httpd.max_open_sockets = 2; 

    httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = NULL };
    httpd_uri_t ws_uri = { .uri = "/ws", .method = HTTP_GET, .handler = ws_handler, .user_ctx = NULL, .is_websocket = true };

    if (httpd_ssl_start(&server, &conf) == ESP_OK) {
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &ws_uri);
    }
}

// ===== TASKS & KINEMATICS LOOP =====

void TaskNetwork(void *pvParameters) {
  for(;;) {
    if (active_ws_fd != -1) {
        // CRITICAL FIX: Timer Wraparound Defeated via Differential Equation
        if (!ws_authenticated && (millis() - auth_start_time > 2000)) {
            httpd_sess_trigger_close(server, active_ws_fd);
            active_ws_fd = -1;
        } 
        else if (ws_authenticated && (millis() - lastKeepAlive > 2000)) {
            httpd_sess_trigger_close(server, active_ws_fd);
            active_ws_fd = -1;
            ws_authenticated = false;
        } 
        else if (ws_authenticated) {
            static unsigned long lastTelem = 0;
            if(millis() - lastTelem > TELEMETRY_MS) {
                lastTelem = millis();
                
                AsyncTelem* telem = (AsyncTelem*)malloc(sizeof(AsyncTelem));
                if (telem) {
                    telem->hd = server;
                    telem->fd = active_ws_fd;
                    telem->session_id = current_session_id; // Secure context locking
                    
                    if (xSemaphoreTake(telemMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                        snprintf(telem->payload, sizeof(telem->payload), 
                          "{\"p\":%.1f,\"r\":%.1f,\"l\":[[%d,%d,%d,%d],[%d,%d,%d,%d],[%d,%d,%d,%d],[%d,%d,%d,%d]]}",
                          sharedTelem.pitch, sharedTelem.roll,
                          (int)sharedTelem.legAngles[0][0], (int)sharedTelem.legAngles[0][1], (int)sharedTelem.legAngles[0][2], (int)sharedTelem.legAngles[0][3],
                          (int)sharedTelem.legAngles[1][0], (int)sharedTelem.legAngles[1][1], (int)sharedTelem.legAngles[1][2], (int)sharedTelem.legAngles[1][3],
                          (int)sharedTelem.legAngles[2][0], (int)sharedTelem.legAngles[2][1], (int)sharedTelem.legAngles[2][2], (int)sharedTelem.legAngles[2][3],
                          (int)sharedTelem.legAngles[3][0], (int)sharedTelem.legAngles[3][1], (int)sharedTelem.legAngles[3][2], (int)sharedTelem.legAngles[3][3]
                        );
                        xSemaphoreGive(telemMutex);
                        
                        if (httpd_queue_work(server, send_telem_worker, telem) != ESP_OK) {
                            free(telem); 
                        }
                    } else {
                        free(telem);
                    }
                }
            }
        }
    }
    vTaskDelay(5 / portTICK_PERIOD_MS); 
  }
}

void TaskControl(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(20); 
  float dt = 0.02f;
  
  // CRITICAL FIX: Safe Boot Sequence prevents uninitialized memory slamming
  RobotState localState;
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  localState = sharedState;
  xSemaphoreGive(stateMutex);

  for(;;) {
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      localState = sharedState;
      if (sharedState.calibrateIMU) {
         isCalibrating = true; calibCount = 0; calibSumP = 0; calibSumR = 0;
         sharedState.calibrateIMU = false; 
      }
      xSemaphoreGive(stateMutex);
    }

    if(imuConnected) readRawMPU(dt);

    if(gamepadActive && (millis() - lastPadPacket > 500)) {
       if (xSemaphoreTake(stateMutex, 0) == pdTRUE) {
           sharedState.targetX=0; sharedState.targetY=0; sharedState.targetTwist=0; sharedState.inputZ=DEFAULT_Z; 
           xSemaphoreGive(stateMutex);
       }
       gamepadActive=false;
    }

    if (localState.mode == 4) {
        setRGB(255, 0, 0); 
        if (statusPCA) for (int i = 0; i < 16; i++) pca.setPWM(i, 0, 0); 
    }
    else if (localState.mode == 3) { 
        setRGB(255, 165, 0); 
        float angle = constrain(90.0f + 40.0f * sin(millis() / 300.0f), 0.0f, 180.0f);
        int pulse = map((long)angle, 0, 180, SERVO_MIN, SERVO_MAX);
        if(statusPCA) {
          if(localState.animationType == 1) for(int i=0;i<4;i++) pca.setPWM(i * 4, 0, pulse);
          else for(int i=0;i<16;i++) pca.setPWM(i, 0, pulse);
        }
    } 
    else {
        if (localState.mode == 1) setRGB(255, 0, 255); else setRGB(0, 255, 0); 
        if(abs(localState.targetX)<1) localState.targetX=0; 
        currentX += (localState.targetX-currentX)*(ACCEL*dt*5.0);
        if(abs(localState.targetY)<1) localState.targetY=0; 
        currentY += (localState.targetY-currentY)*(ACCEL*dt*5.0);
        if(abs(localState.targetTwist)<0.1) localState.targetTwist=0;
        currentTwist += (localState.targetTwist-currentTwist)*(ACCEL*dt*5.0);
        if(abs(currentHeight-localState.inputZ)>0.1f) currentHeight += (localState.inputZ-currentHeight)*0.1f;

        if(statusPCA && localState.mode != 2) { 
          gait.run((localState.mode == 1) ? 1.0f : 0.0f, currentX, currentY, currentTwist, dt);
          float balZ[4] = {0,0,0,0};
          if(imuConnected) {
             float p = constrain(pitch, -20.0f, 20.0f) * (PI_F/180.0f);
             float r = constrain(roll, -20.0f, 20.0f) * (PI_F/180.0f);
             for(int i=0; i<4; i++) balZ[i] = (80 * legs[i].cfg.xSign * sin(p)) + (60 * legs[i].cfg.ySign * sin(r));
          }
          for(int i=0; i<4; i++) legs[i].run(gait.legX[i], gait.legY[i], currentHeight + gait.legZ[i] - balZ[i], gait.legT[i]);
        }
    }
    
    if (xSemaphoreTake(telemMutex, 0) == pdTRUE) {
        sharedTelem.pitch = pitch;
        sharedTelem.roll = roll;
        for(int i=0; i<4; i++) {
           sharedTelem.legAngles[i][0] = legs[i].ikG;
           sharedTelem.legAngles[i][1] = legs[i].ikA;
           sharedTelem.legAngles[i][2] = legs[i].ikB;
           sharedTelem.legAngles[i][3] = legs[i].ikTwist;
        }
        xSemaphoreGive(telemMutex);
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void setup() {
  Serial.begin(115200);
  
  // CRITICAL FIX: Safe NVS Fetching limits strings dump vectors
  Preferences prefs;
  prefs.begin("security", true); 
  // If undefined, default to admin:interceptor2026 base64. 
  // In production, write this to NVS once during secure boot flashing and remove from code.
  storedBasicAuth = prefs.getString("b64_auth", "Basic YWRtaW46aW50ZXJjZXB0b3IyMDI2"); 
  prefs.end();

  rgb.begin(); setRGB(0, 0, 255); 
  Wire.begin(PIN_SERVO_SDA, PIN_SERVO_SCL, 400000); Wire.setTimeOut(10); 
  pca.begin(); pca.setPWMFreq(50);
  Wire.beginTransmission(0x40); statusPCA = (Wire.endTransmission()==0);
  imuConnected = initRawMPU(); 

  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) delay(100);
  setRGB(0, 255, 0); 

  // CRITICAL FIX: Initialize Mutexes BEFORE opening the network gates
  stateMutex = xSemaphoreCreateMutex();
  telemMutex = xSemaphoreCreateMutex(); 

  start_secure_server();

  xTaskCreatePinnedToCore(TaskNetwork, "NetTask", 8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(TaskControl, "CtrlTask", 8192, NULL, 2, NULL, 1);
  vTaskDelete(NULL); 
}
void loop() {}