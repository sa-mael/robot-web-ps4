/*
 * PROJECT:  IK4=38 (NEON SHIFT & DIAGRAM FIX)
 * PLATFORM: ESP32-C6 / ESP32
 * UPDATE:   Implements User's Angular Diagram
 * - Default Position (Red Cross) = 45° Physical
 * - Servo Center (90°) maps to 45° Physical
 * - Hard Limits: -30° to +60°
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <ArduinoJson.h>
#include <math.h>

// 1. CONFIGURATION
#define WIFI_SSID "BoomHouse"
#define WIFI_PASS "d0uBL3Tr0ubl3"

void socketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

constexpr float PI_F = 3.14159265f;
constexpr int JSON_BUFFER_SIZE = 512;
constexpr int TELEMETRY_MS = 50;
constexpr float GAIT_SPEED = 5.0f;

// PWM LIMITS
constexpr int SERVO_MIN = 50;  // Min Pulse
constexpr int SERVO_MAX = 490; // Max Pulse

// ROBOT DIMENSIONS (MM)
constexpr float L_COXA  = 67.00f;
constexpr float L_FEMUR = 69.16f;
const float L_TIBIA = 123.59f;

// 2. PHYSICS MATRIX (SPIDER STANCE)
struct LegConfig {
  int xSign;      
  int ySign;      
  int gammaSign;  
  float mountDeg; 
};

const LegConfig CFG[4] = {
  { +1, +1, +1,  225.0f }, // FL
  { +1, -1, -1, -45.0f },  // FR
  { -1, +1, +1, -45.0f },  // BL
  { -1, -1, -1,  225.0f }  // BR
};

// 3. STATE
enum RobotMode { MODE_STAND, MODE_WALK, MODE_MANUAL };
RobotMode currentMode = MODE_STAND;

// 4. SERVO CLASS
class ServoMotor {
  public:
    float angle = 90.0f;
    bool inverted = false;
    void init(bool inv) { inverted = inv; }
    int getPulse() {
      float driveAngle = inverted ? (180.0f - angle) : angle;
      driveAngle = constrain(driveAngle, 0.0f, 180.0f);
      return map((long)driveAngle, 0, 180, SERVO_MIN, SERVO_MAX);
    }
    void setAbs(float val) { angle = val; }
};

// 5. LEG CLASS
class Leg {
  public:
    int id; 
    int pcaBase;
    LegConfig cfg;
    
    ServoMotor mG, mA, mB, mT;
    
    // Target Coordinates (Local)
    float x=150, y=0, z=-60; 
    float twistInput = 0; 
    
    // Telemetry storage
    float ikG=0, ikA=0, ikB=0, ikT=0;

    Leg(int _id, int _pcaBase) {
      id = _id; pcaBase = _pcaBase; cfg = CFG[_id]; 
      // Inversions based on mechanics
      mG.init(false); mA.init(true); mB.init(true); mT.init(false);
    }

    void run(RobotMode mode, float gX, float gY, float gZ, bool isActiveManual) {
      if (mode == MODE_MANUAL) {
        if (isActiveManual) return; 
        x = 150; y = 0; z = gZ; twistInput = 0;
        solveIK();
        return;
      }
      if (mode == MODE_WALK) {
        // Base X=150 is the "Zero" reach. 
        // Note: With new 45deg offset logic, X=150 will result in Gamma=0 (local) -> Servo=45
        // If you want "Default" to be the Red Cross, Y must be such that atan2(y,x) = 45 deg?
        // NO: The diagram implies the servo is centered at 45. 
        // So standard X=150, Y=0 input results in Gamma=0.
        // Gamma=0 -> Servo moves to 45 (Down from center).
        // This is correct based on the math shift below.
        x = 150 + gX; y = 0 + gY; z = gZ; twistInput = 0; 
        solveIK();
      }
      else if (mode == MODE_STAND) {
        // Stand defaults
        x = 150; y = 0; z = gZ; twistInput = 0; solveIK();
      }
    }

    void solveIK() {
      float lx = x * cfg.xSign;
      float ly = y * cfg.ySign;
      float lz = z;

      // --- GAMMA SOLVER (HIP) ---
      float G_rad = atan2(ly, lx);
      float G_deg = degrees(G_rad);

      // --- DIAGRAM LOGIC APPLICATION ---
      // 1. Constrain to Diagram Limits (-30 to +60 relative to X-axis)
      if (G_deg < -30.0f) G_deg = -30.0f;
      if (G_deg > 60.0f)  G_deg = 60.0f;

      // 2. Shift Center
      // Diagram Requirement: "Default" position is +45 degrees from X-axis.
      // We want Servo Center (90) to equal Physical 45.
      // Formula: Servo = 90 + (TargetAngle - 45)
      // If Target=45 (Red Cross) -> Servo = 90 (Center).
      // If Target=0 (X Axis) -> Servo = 45.
      float servoG = 90.0f + ((G_deg - 45.0f) * cfg.gammaSign);

      // --- ALPHA / BETA SOLVER ---
      float u_total = sqrt(lx*lx + ly*ly);
      float u = u_total - L_COXA; 
      float v = lz;

      float alpha = twistInput * PI_F / 180.0f;
      float v_new = v * cos(alpha); 

      float D = sqrt(u*u + v_new*v_new);
      if (D < 1.0f) D = 1.0f;
      if (D > L_FEMUR+L_TIBIA) D = L_FEMUR+L_TIBIA; 

      float a1 = atan2(v_new, u);
      float a2 = acos(constrain(((L_FEMUR*L_FEMUR)+(D*D)-(L_TIBIA*L_TIBIA))/(2*L_FEMUR*D), -1.0f, 1.0f));
      float b_ang = acos(constrain(((L_FEMUR*L_FEMUR)+(L_TIBIA*L_TIBIA)-(D*D))/(2*L_FEMUR*L_TIBIA), -1.0f, 1.0f));

      float A_deg = degrees(a1 + a2);
      float B_deg = degrees(b_ang) - 180.0f; 

      // Telemetry
      ikG = G_deg; ikA = A_deg; ikB = B_deg; ikT = twistInput;

      // Final Constrain
      servoG = constrain(servoG, 0.0f, 180.0f);

      float servoA = 90.0f + A_deg; 
      float servoB = 90.0f - B_deg; 
      float servoT = 90.0f + twistInput;

      mG.setAbs(servoG); mA.setAbs(servoA); mB.setAbs(servoB); mT.setAbs(servoT);
    }
    
    void setManual(int motorId, float val) {
      if(motorId==0) { mG.setAbs(90.0f + val); ikG = 90.0f + val; }
      if(motorId==1) { mA.setAbs(90.0f + val); ikA = 90.0f + val; }
      if(motorId==2) { mB.setAbs(90.0f + val); ikB = 90.0f + val; }
      if(motorId==3) { mT.setAbs(90.0f + val); ikT = val; twistInput = val; }
    }
};

// 6. GLOBALS
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver();
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

Leg legs[4] = { Leg(0, 0), Leg(1, 4), Leg(2, 8), Leg(3, 12) };
int activeLegID = 0; 
float targetHeight = -60; float currentHeight = -60;
float phase = 0; unsigned long lastT = 0;

// 7. WEB UI (COMPRESSED)
const char html_interface[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>IK4=38 NEON</title>
<style>:root{--n:#00f3ff;--b:#050505;--p:rgba(0,20,30,0.85);}body{margin:0;background:var(--b);color:var(--n);font-family:monospace;overflow:hidden;}canvas{position:absolute;z-index:-1;}
.ui{position:absolute;width:100%;height:100%;pointer-events:none;display:flex;justify-content:space-between;padding:10px;box-sizing:border-box;}
.panel{width:260px;background:var(--p);border:1px solid var(--n);padding:15px;pointer-events:auto;display:flex;flex-direction:column;}
button{padding:15px;background:rgba(0,243,255,0.1);color:var(--n);border:1px solid var(--n);cursor:pointer;margin-bottom:5px;width:100%;}
button:hover,button.sel{background:var(--n);color:#000;}.grid{display:grid;grid-template-columns:1fr 1fr;gap:5px;}
input[type=range]{width:100%;accent-color:var(--n);}</style></head><body>
<div class="ui"><div class="panel"><h2>CMD CENTER v38</h2><button onclick="send({cmd:'mode',val:'stand'})">STAND</button><button id="btn-walk" onclick="toggleWalk()">WALK</button>
<div style="margin-top:20px"><label>Z-HEIGHT</label><input type="range" min="-140" max="-40" value="-60" oninput="send({cmd:'h',val:this.value})"></div>
<div style="margin-top:auto;text-align:center" id="conn">DISCONNECTED</div></div>
<div class="panel"><h2>TUNING</h2><div class="grid"><button id="l0" class="sel" onclick="selLeg(0)">FL</button><button id="l1" onclick="selLeg(1)">FR</button><button id="l2" onclick="selLeg(2)">BL</button><button id="l3" onclick="selLeg(3)">BR</button></div>
<div style="margin-top:15px"><label>GAMMA</label><input type="range" min="-45" max="45" value="0" oninput="move(0,this.value)"><label>ALPHA</label><input type="range" min="-90" max="90" value="0" oninput="move(1,this.value)"><label>BETA</label><input type="range" min="-90" max="90" value="0" oninput="move(2,this.value)"><label>TWIST</label><input type="range" min="-45" max="45" value="0" oninput="move(3,this.value)"></div></div></div>
<script type="module">import*as THREE from 'https://esm.sh/three@0.160.0';import{OrbitControls}from 'https://esm.sh/three@0.160.0/examples/jsm/controls/OrbitControls.js';
window.ws=new WebSocket(`ws://${location.hostname}:81/`);const scene=new THREE.Scene();scene.background=new THREE.Color(0x050505);
const cam=new THREE.PerspectiveCamera(45,innerWidth/innerHeight,1,3000);cam.position.set(0,400,500);
const ren=new THREE.WebGLRenderer({antialias:true});ren.setSize(innerWidth,innerHeight);document.body.appendChild(ren.domElement);new OrbitControls(cam,ren.domElement);
scene.add(new THREE.GridHelper(1000,100,0x00f3ff,0x111111));scene.add(new THREE.AmbientLight(0xffffff,0.6));
const body=new THREE.Group();body.add(new THREE.Mesh(new THREE.BoxGeometry(77,46,134),new THREE.MeshBasicMaterial({color:0x00f3ff,wireframe:true})));body.position.y=23;scene.add(body);
const legsVis=[];function cLeg(x,z,deg){const r=new THREE.Group();r.position.set(x,23,z);const m=new THREE.Group();m.rotation.y=deg*(Math.PI/180);r.add(m);
m.add(new THREE.Mesh(new THREE.BoxGeometry(66,10,10).translate(33,0,0),new THREE.MeshBasicMaterial({color:0xaa00ff,wireframe:true})));
r.userData={m:m,off:deg};scene.add(r);legsVis.push(r);}
cLeg(-38,-67,225);cLeg(38,-67,-45);cLeg(-38,67,-45);cLeg(38,67,225);
function loop(){requestAnimationFrame(loop);ren.render(scene,cam);}loop();
ws.onmessage=(e)=>{try{const d=JSON.parse(e.data);if(d.l)d.l.forEach((a,i)=>{
legsVis[i].userData.m.rotation.y=(legsVis[i].userData.off+a[0])*0.0174533;});}catch(e){}};
</script><script>let aL=0,w=0;window.toggleWalk=function(){w=!w;send({cmd:'mode',val:w?'walk':'stand'});};
window.selLeg=function(id){aL=id;document.querySelectorAll('.grid button').forEach(b=>b.classList.remove('sel'));document.getElementById('l'+id).classList.add('sel');send({cmd:'active',val:id});};
window.send=function(d){if(ws.readyState===1)ws.send(JSON.stringify(d));};window.move=function(i,v){send({cmd:'servo',leg:aL,id:i,val:parseInt(v)});};
</script></body></html>
)rawliteral";

// 8. MAIN LOOP
void setup() {
  Serial.begin(115200); 
  #if defined(CONFIG_IDF_TARGET_ESP32C6)
    Wire.begin(6, 7); 
  #else
    Wire.begin();     
  #endif
  pca.begin(); pca.setPWMFreq(50);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) delay(100);
  
  server.on("/", [](){ server.send(200, "text/html", html_interface); });
  server.begin(); webSocket.begin(); webSocket.onEvent(socketEvent);
  lastT = millis();
}

void loop() {
  webSocket.loop(); server.handleClient();
  unsigned long now = millis(); float dt = (now - lastT) / 1000.0f; lastT = now;
  if(abs(currentHeight - targetHeight) > 0.1f) currentHeight += (targetHeight - currentHeight) * 0.1f;
  
  float gaitdX[4] = {0}; float gaitdY[4] = {0}; 
  float gaitZ[4] = {currentHeight, currentHeight, currentHeight, currentHeight};

  if(currentMode == MODE_WALK) {
    phase += dt * GAIT_SPEED; if(phase > 2*PI_F) phase -= 2*PI_F;
    float offsets[4] = {0, PI_F, PI_F, 0}; // Trot Gait
    for(int i=0; i<4; i++) {
      float p = phase + offsets[i]; if(p > 2*PI_F) p -= 2*PI_F;
      if(p < PI_F) { 
        float prog = p/PI_F; 
        gaitdX[i] = 30.0f * cos(prog * PI_F); 
        gaitZ[i] = currentHeight + sin(prog * PI_F) * 30.0f; 
      } else { 
        float prog = (p-PI_F)/PI_F;
        gaitdX[i] = -30.0f * cos(prog * PI_F); 
      }
    }
  }

  for(int i=0; i<4; i++) {
    // Vector Rotation (Global -> Local)
    float mRad = legs[i].cfg.mountDeg * PI_F / 180.0f;
    float localdX = gaitdX[i] * cos(-mRad) - gaitdY[i] * sin(-mRad);
    float localdY = gaitdX[i] * sin(-mRad) + gaitdY[i] * cos(-mRad);
    
    legs[i].run(currentMode, localdX, localdY, gaitZ[i], (i == activeLegID));
    
    int base = legs[i].pcaBase;
    pca.setPWM(base + 0, 0, legs[i].mG.getPulse());
    pca.setPWM(base + 1, 0, legs[i].mA.getPulse());
    pca.setPWM(base + 2, 0, legs[i].mB.getPulse());
    pca.setPWM(base + 3, 0, legs[i].mT.getPulse());
  }

  static unsigned long lastTelem = 0;
  if(now - lastTelem > TELEMETRY_MS) {
    lastTelem = now;
    String json = "{\"l\":[";
    for(int i=0; i<4; i++) {
      json += "[" + String(legs[i].ikG) + "," + String(legs[i].ikA) + "," + String(legs[i].ikB) + "," + String(legs[i].ikT) + "]";
      if(i<3) json += ",";
    }
    json += "]}"; webSocket.broadcastTXT(json);
  }
}

void socketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_TEXT) {
    StaticJsonDocument<JSON_BUFFER_SIZE> doc; 
    DeserializationError error = deserializeJson(doc, (char*)payload);
    if (!error) {
      String cmd = doc["cmd"];
      if(cmd == "mode") { String val = doc["val"]; if(val == "stand") currentMode = MODE_STAND; if(val == "walk") currentMode = MODE_WALK; }
      if(cmd == "h") targetHeight = (float)doc["val"];
      if(cmd == "active") activeLegID = doc["val"];
      if(cmd == "servo") { 
        currentMode = MODE_MANUAL; 
        legs[(int)doc["leg"]].setManual((int)doc["id"], (float)doc["val"]); 
      }
    }
  }
}