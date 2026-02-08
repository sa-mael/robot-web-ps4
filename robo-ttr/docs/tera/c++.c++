/*
 * PROJECT:  IK4=49 "TURBO INTERCEPTOR"
 * VERSION:  v49.1 (STABLE / DECOUPLED TWIST)
 * MODULE:   MAIN FIRMWARE
 * AUTHOR:   Gemini & User
 *
 * FEATURES:
 * - Decoupled 4-DOF Solver (Twist independent of geometric reach)
 * - ArduinoJson v7 Compatibility
 * - 400kHz Fast I2C Bus
 * - Artificial Horizon Flight Deck
 * - Watchdog Safety System
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <math.h>

// ---------------------------------------------------------------------------
// 1. SYSTEM CONFIGURATION
// ---------------------------------------------------------------------------
#define WIFI_SSID "BoomHouse"
#define WIFI_PASS "d0uBL3Tr0ubl3"

void socketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

constexpr float PI_F = 3.14159265f;
constexpr int TELEMETRY_MS = 40; // 25 Hz Update Rate
constexpr float GAIT_SPEED = 7.0f;
constexpr int SERVO_MIN = 50;
constexpr int SERVO_MAX = 490;

// Mechanical Dimensions (mm)
constexpr float L_COXA  = 67.00f;
constexpr float L_FEMUR = 69.16f;
const float L_TIBIA = 123.59f;

// ---------------------------------------------------------------------------
// 2. PHYSICS ENGINE & CONSTANTS
// ---------------------------------------------------------------------------
// Optimization: Pre-calculate squares and denominators for Law of Cosines
const float L_COXA_SQ  = L_COXA * L_COXA;
const float L_FEMUR_SQ = L_FEMUR * L_FEMUR;
const float L_TIBIA_SQ = L_TIBIA * L_TIBIA;
const float K_FEMUR_DIV = 2.0f * L_FEMUR;
const float K_TIBIA_DIV = 2.0f * L_FEMUR * L_TIBIA;

struct LegConfig {
  int xSign; int ySign; int gammaSign; float mountDeg; 
};

// "Spider-X" Calibration Matrix
const LegConfig CFG[4] = {
  { +1, +1, +1,  135.0f }, // Leg 0: FL (Front-Left)
  { +1, -1, -1,  45.0f },  // Leg 1: FR (Front-Right)
  { -1, +1, +1,  45.0f },  // Leg 2: BL (Back-Left)
  { -1, -1, -1, -225.0f }  // Leg 3: BR (Back-Right)
};

// ---------------------------------------------------------------------------
// 3. GLOBAL STATE
// ---------------------------------------------------------------------------
enum RobotMode { MODE_STAND, MODE_WALK, MODE_MANUAL };
RobotMode currentMode = MODE_STAND;

// Flight Vectors
float targetX = 0, targetY = 0, targetTwist = 0;   
float currentX = 0, currentY = 0, currentTwist = 0; 
float inputZ = -60;
float currentHeight = -60;

// Safety & Dynamics
unsigned long lastPadPacket = 0;
bool gamepadActive = false;
const float ACCEL = 1.5f; // Inertia Factor

// IMU Fusion
float pitch = 0, roll = 0;
float pitchOffset = 0, rollOffset = 0;
bool isCalibrating = false;
int calibCount = 0;
float calibSumP = 0, calibSumR = 0;

// ---------------------------------------------------------------------------
// 4. DRIVER CLASSES (HAL)
// ---------------------------------------------------------------------------
class AxialRotator {
  public:
    int channel; float mountOffset; bool inverted;
    
    void attach(int _ch, float _mountDeg, bool _inv) { 
        channel = _ch; mountOffset = _mountDeg; inverted = _inv; 
    }
    
    // SPECIFICATION COMPLIANCE: 
    // "Neutral position 45°... Used range 90°"
    int getPulse(float geometryAngle, float manualOffset) {
        // 1. Calculate Target Angle relative to mount
        // geometryAngle comes from IK (e.g., heading towards target)
        float target = mountOffset + geometryAngle + manualOffset;
        
        // 2. Normalize to -180 to +180 range
        target = fmod(target, 360.0f);
        if (target < 0.0f) target += 360.0f;
        float relativeAngle = target - mountOffset;
        if (relativeAngle > 180.0f) relativeAngle -= 360.0f;
        if (relativeAngle < -180.0f) relativeAngle += 360.0f;

        // 3. Map to Servo constraints (Section 4 & 5 of spec)
        // 0° Logical (Neutral) = 45° Physical Servo Angle
        // Range is constrained to [15° ... 105°] (The -30/+60 limit)
        float driveAngle = 45.0f + relativeAngle; 
        
        // Hard Safety Limits (prevent hitting chassis)
        driveAngle = constrain(driveAngle, 15.0f, 105.0f); 

        if (inverted) driveAngle = 180.0f - driveAngle;
        return map((long)(driveAngle * 100), 0, 18000, SERVO_MIN, SERVO_MAX);
    }
};

class TwistRotator {
  public:
    int channel;
    void attach(int _ch) { channel = _ch; }
    
    // SPECIFICATION COMPLIANCE (Section 8):
    // "Neutral position 45°... Twist Range 0..90"
    int getPulse(float twistAngle) {
        // twistAngle is usually small (-20 to +20 degrees)
        // 0° Twist Input = 45° Servo Position
        float driveAngle = 45.0f + twistAngle;
        
        // Clamp to working range (0 to 90 per spec)
        driveAngle = constrain(driveAngle, 0.0f, 90.0f);
        
        return map((long)(driveAngle * 100), 0, 18000, SERVO_MIN, SERVO_MAX);
    }
};

class VerticalLifter {
  public:
    int channel; bool inverted;
    void attach(int _ch, bool _inv) { channel=_ch; inverted=_inv; }
    
    int getPulse(float ikAngle, float manualOffset, bool isKnee) {
        float totalAngle = ikAngle + manualOffset;
        // Standard mapping: 90 is center
        float driveAngle = 90.0f + totalAngle;
        if(isKnee) driveAngle = 90.0f - totalAngle; 
        
        driveAngle = constrain(driveAngle, 0.0f, 180.0f);
        if (inverted) driveAngle = 180.0f - driveAngle;
        return map((long)(driveAngle * 100), 0, 18000, SERVO_MIN, SERVO_MAX);
    }
};

// ---------------------------------------------------------------------------
// 5. LEG SYSTEM
// ---------------------------------------------------------------------------
class Leg {
  public:
    int id; LegConfig cfg;
    AxialRotator Hip; 
    VerticalLifter Femur; 
    VerticalLifter Tibia; 
    TwistRotator Twist; // Renamed from "Roll" to match spec
    
    // Telemetry Buffers
    float ikG=0, ikA=0, ikB=0, ikTwist=0;
    float manG=0, manA=0, manB=0, manTwist=0;

    Leg(int _id, int _startPin) {
      id = _id; cfg = CFG[_id]; 
      Hip.attach(_startPin + 0, cfg.mountDeg, false);
      Femur.attach(_startPin + 1, true);
      Tibia.attach(_startPin + 2, true);
      Twist.attach(_startPin + 3);
    }

    void run(float gX, float gY, float gZ, float inputTwist) {
      // 1. Coordinate Transformation (World -> Local Leg Frame)
      // Apply Symmetry Rules (Section 7)
      float lx = (150 + gX) * cfg.xSign; 
      float ly = (0 + gY) * cfg.ySign; 
      float lz = gZ; // Z is vertical, not affected by symmetry in this model

      // 2. Planar Inverse Kinematics (COXA -> FEMUR -> TIBIA)
      // Calculate Yaw (Gamma)
      float G_rad = atan2(ly, lx); 
      float G_deg = degrees(G_rad);
      
      // Calculate Horizontal Reach (Hypotenuse on ground)
      // "L_COXA" is subtracted because it is a rigid offset before the Femur joint
      float u = sqrt(lx*lx + ly*ly) - L_COXA; 
      
      // Calculate Vertical Diagonal (Femur-to-Tip direct line)
      // FIXED: Removed "Twist" from this calculation. 
      // Twist is axial and does not shorten the vertical reach D.
      float D = sqrt(u*u + lz*lz);
      
      // Safety: Prevent extending beyond physical bone length
      if (D < 1.0f) D = 1.0f; 
      if (D > (L_FEMUR + L_TIBIA)) D = L_FEMUR + L_TIBIA; 

      // Law of Cosines for Femur (Alpha) and Tibia (Beta)
      float a1 = atan2(lz, u); // Elevation angle of the diagonal
      float a2 = acos((L_FEMUR_SQ + D*D - L_TIBIA_SQ) / (K_FEMUR_DIV * D));
      float b_rad = acos((L_FEMUR_SQ + L_TIBIA_SQ - D*D) / K_TIBIA_DIV);

      float A_deg = degrees(a1 + a2); 
      float B_deg = degrees(b_rad) - 180.0f; // Knee bends relative to Femur

      // 3. Twist Pass (Decoupled 4th DOF)
      // Simply passes the twist command to the 4th servo
      float finalTwist = inputTwist + manTwist;

      // 4. Hardware Execution
      ikG = G_deg + manG; 
      ikA = A_deg + manA; 
      ikB = B_deg + manB; 
      ikTwist = finalTwist;

      extern Adafruit_PWMServoDriver pca;
      
      // Apply Gamma Sign correction for Right-side legs
      pca.setPWM(Hip.channel, 0, Hip.getPulse(G_deg * cfg.gammaSign, manG));
      pca.setPWM(Femur.channel, 0, Femur.getPulse(A_deg, manA, false));
      pca.setPWM(Tibia.channel, 0, Tibia.getPulse(B_deg, manB, true));
      
      // Apply Twist directly
      pca.setPWM(Twist.channel, 0, Twist.getPulse(finalTwist)); 
    }
    
    void setManual(int motorId, float val) {
      if(motorId == 0) manG = val; 
      if(motorId == 1) manA = val; 
      if(motorId == 2) manB = val; 
      if(motorId == 3) manTwist = val; 
    }
};

// ---------------------------------------------------------------------------
// 6. MAIN CONTROLLER
// ---------------------------------------------------------------------------
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver();
Adafruit_MPU6050 mpu; 
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

Leg legs[4] = { Leg(0, 0), Leg(1, 4), Leg(2, 8), Leg(3, 12) };
int activeLegID = 0; float phase = 0; unsigned long lastT = 0;

// ---------------------------------------------------------------------------
// 7. USER INTERFACE (HTML/JS)
// ---------------------------------------------------------------------------
const char html_interface[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>IK4=49 FLIGHT DECK</title>
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
    button.calib:hover { background: #fc0; color: #000; }
    input[type=range] { width: 100%; accent-color: var(--accent); cursor: pointer; height: 5px; background: #333; margin: 10px 0; }
    
    .horizon-box {
      width: 100%; height: 120px; background: #000; border: 1px solid var(--accent); 
      margin-bottom: 10px; position: relative; overflow: hidden; border-radius: 4px;
    }
    .horizon-sky {
      width: 300%; height: 300%; background: linear-gradient(to bottom, #005566 50%, #333 50%);
      position: absolute; top: -100%; left: -100%; transition: transform 0.05s linear; pointer-events: none; opacity: 0.6;
    }
    .horizon-line {
      width: 100%; height: 2px; background: var(--accent); position: absolute; top: 50%; left: 0;
      box-shadow: 0 0 5px var(--accent);
    }
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
  </div>
</div>

<script type="module">
  import * as THREE from 'https://esm.sh/three@0.160.0';
  import { OrbitControls } from 'https://esm.sh/three@0.160.0/examples/jsm/controls/OrbitControls.js';
  window.ws = new WebSocket(`ws://${location.hostname}:81/`);
  
  const scene = new THREE.Scene(); scene.background = new THREE.Color(0x0b0b0b);
  const cam = new THREE.PerspectiveCamera(45, window.innerWidth/innerHeight, 1, 3000); cam.position.set(0, 400, 500); 
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
// 8. KERNEL INITIALIZATION
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200); 
  
  // TURBO I2C
  #if defined(CONFIG_IDF_TARGET_ESP32C6)
    Wire.begin(6, 7); 
  #else
    Wire.begin();     
  #endif
  Wire.setClock(400000); // 400kHz Fast Mode
  
  pca.begin(); pca.setPWMFreq(50);
  
  // ROBUST MPU INIT
  if (!mpu.begin()) { 
    Serial.println("MPU FAIL! CHECK WIRING!"); 
  } else { 
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) delay(100);
  
  server.on("/", [](){ server.send(200, "text/html", html_interface); });
  server.begin(); webSocket.begin(); webSocket.onEvent(socketEvent);
  lastT = millis();
}

// ---------------------------------------------------------------------------
// 9. REAL-TIME KERNEL LOOP
// ---------------------------------------------------------------------------
void loop() {
  webSocket.loop(); server.handleClient();
  unsigned long now = millis(); 
  float dt = (now - lastT) / 1000.0f; 
  lastT = now;

  // 1. WATCHDOG (AUTO STOP)
  if (gamepadActive && (now - lastPadPacket > 500)) {
     targetX = 0; targetY = 0; targetTwist = 0;
     gamepadActive = false;
  }

  // 2. SMOOTH ACCELERATION
  if(abs(targetX) < 1.0) targetX = 0;
  if(abs(targetY) < 1.0) targetY = 0;
  
  if(abs(currentX - targetX) > 0.1) currentX += (targetX - currentX) * (ACCEL * dt * 5.0); else currentX = targetX;
  if(abs(currentY - targetY) > 0.1) currentY += (targetY - currentY) * (ACCEL * dt * 5.0); else currentY = targetY;
  if(abs(currentTwist - targetTwist) > 0.1) currentTwist += (targetTwist - currentTwist) * (ACCEL * dt * 5.0); else currentTwist = targetTwist;
  if(abs(currentHeight - inputZ) > 0.1f) currentHeight += (inputZ - currentHeight) * 0.1f;
  
  // 3. SENSOR FUSION
  sensors_event_t a, g, temp; 
  bool imuOk = mpu.getEvent(&a, &g, &temp); 
  
  if(imuOk) {
    float rawP = atan2(a.acceleration.y, a.acceleration.z) * 180.0/PI;
    float rawR = atan2(-a.acceleration.x, sqrt(a.acceleration.y*a.acceleration.y + a.acceleration.z*a.acceleration.z)) * 180.0/PI;

    if(isCalibrating) {
      if(calibCount < 50) { calibSumP += rawP; calibSumR += rawR; calibCount++; } 
      else { pitchOffset = calibSumP / 50.0; rollOffset = calibSumR / 50.0; isCalibrating = false; }
    }
    pitch = rawP - pitchOffset; roll = rawR - rollOffset;
  }

  // 4. GAIT GENERATION
  float gaitdX[4] = {0,0,0,0}; float gaitdY[4] = {0,0,0,0}; 
  float gaitZ[4] = {currentHeight, currentHeight, currentHeight, currentHeight};

  if(currentMode == MODE_WALK && (abs(currentX) > 1.0 || abs(currentY) > 1.0 || abs(currentTwist) > 1.0)) {
    phase += dt * GAIT_SPEED; if(phase > 2*PI_F) phase -= 2*PI_F;
    float offsets[4] = {0, PI_F, PI_F, 0}; 
    for(int i=0; i<4; i++) {
      float p = phase + offsets[i]; if(p > 2*PI_F) p -= 2*PI_F;
      if(p < PI_F) { 
        float prog = p/PI_F; 
        gaitdX[i] = currentX * cos(prog * PI_F); gaitdY[i] = currentY * cos(prog * PI_F);
        gaitZ[i] = currentHeight + sin(prog * PI_F) * 30.0f; 
      } else { 
        float prog = (p-PI_F)/PI_F;
        gaitdX[i] = -currentX * cos(prog * PI_F); gaitdY[i] = -currentY * cos(prog * PI_F);
        gaitZ[i] = currentHeight;
      }
    }
  }

  // 5. STABILIZATION
  float balanceZ[4]; 
  float damp = (abs(currentX) > 5) ? 0.2 : 0.6; 
  
  for(int i=0; i<4; i++) {
     float lX = 80.0 * legs[i].cfg.xSign; float lY = 60.0 * legs[i].cfg.ySign;
     float rotZ = (lX * tan(pitch * PI/180.0)) + (lY * tan(roll * PI/180.0));
     balanceZ[i] = rotZ * damp; 
  }

  // 6. EXECUTION
  for(int i=0; i<4; i++) {
    float mRad = legs[i].cfg.mountDeg * PI_F / 180.0f;
    float localdX = gaitdX[i] * cos(-mRad) - gaitdY[i] * sin(-mRad);
    float localdY = gaitdX[i] * sin(-mRad) + gaitdY[i] * cos(-mRad);
    float totalZ = gaitZ[i] - balanceZ[i]; 
    legs[i].run(localdX, localdY, totalZ, currentTwist);
  }

  // 7. TELEMETRY (OPTIMIZED)
  static unsigned long lastTelem = 0;
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

// ---------------------------------------------------------------------------
// 10. NETWORK INTERRUPT
// ---------------------------------------------------------------------------
void socketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_TEXT) {
    // ARDUINOJSON v7 COMPATIBLE
    JsonDocument doc; 
    DeserializationError error = deserializeJson(doc, payload);
    if(error) return;

    const char* cmdRaw = doc["cmd"]; String cmd = String(cmdRaw ? cmdRaw : "");

    if(cmd == "mode") { 
      const char* valRaw = doc["val"]; String val = String(valRaw ? valRaw : "");
      if(val == "stand") currentMode = MODE_STAND; 
      if(val == "walk") {
        currentMode = MODE_WALK;
        if(doc["auto"]) { targetX = 0; targetY = 30.0; } // Auto-walk forward
        else { targetX = 0; targetY = 0; }
      }
    }
    if(cmd == "h") inputZ = doc["val"].as<float>();
    if(cmd == "active") activeLegID = doc["val"].as<int>();
    if(cmd == "servo") { currentMode = MODE_MANUAL; int l=doc["leg"]; int i=doc["id"]; int v=doc["val"]; legs[l].setManual(i,(float)v); }
    if(cmd == "calib") { isCalibrating = true; calibCount = 0; calibSumP = 0; calibSumR = 0; }
    if(cmd == "pad") {
      gamepadActive = true; lastPadPacket = millis(); 
      targetX = doc["ly"].as<float>() * -40.0; targetY = doc["lx"].as<float>() * 40.0; targetTwist = doc["rx"].as<float>() * 20.0;
      float ry = doc["ry"].as<float>(); if(abs(ry) > 0.2) inputZ += ry * 2.0; inputZ = constrain(inputZ, -140, -40);
      JsonArray btn = doc["btn"]; 
      if(!btn.isNull()) { if(btn[0] == 1) currentMode = MODE_WALK; if(btn[1] == 1) currentMode = MODE_STAND; }
    }
  }
  else if(type == WStype_CONNECTED) {
    String msg = "{\"cfg\":["; for(int i=0; i<4; i++) { msg += String(CFG[i].mountDeg); if(i<3) msg += ","; } msg += "]}"; webSocket.sendTXT(num, msg);
  }
}