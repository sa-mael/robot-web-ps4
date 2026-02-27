/*
 * ПРОЕКТ: OSMIUM S3 - 3D VISUALIZER
 * ----------------------------------------
 * ФУНКЦИИ: Чтение RAW + Математика углов + 3D Куб в браузере
 * ПОДКЛЮЧЕНИЕ:
 * - MPU6050: SDA->4, SCL->5  (Wire 0)
 * - PCA9685: SDA->6, SCL->7  (Wire 1)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <math.h>

// --- WIFI ---
#define WIFI_SSID "BoomHouse"
#define WIFI_PASS "d0uBL3Tr0ubl3"

// --- ОБЪЕКТЫ ---
#define PIN_RGB 38
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40, Wire1); // PCA на шине 1

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// --- ПЕРЕМЕННЫЕ ФИЗИКИ ---
unsigned long lastTime = 0;
float pitch = 0;
float roll = 0;
float yaw = 0; // Yaw (рысканье) будет дрейфовать без магнитометра, но добавим для теста

// Калибровка (простая)
float gyroXoffset = 0, gyroYoffset = 0, gyroZoffset = 0;

// ==========================================
// 1. РАБОТА С MPU6050 (RAW)
// ==========================================
void writeMPU(byte reg, byte data) {
  Wire.beginTransmission(0x68); Wire.write(reg); Wire.write(data); Wire.endTransmission();
}

void setupMPU() {
  writeMPU(0x6B, 0);    // Пробуждение
  writeMPU(0x1C, 0x00); // Акселерометр +/- 2g (чувствительный)
  writeMPU(0x1B, 0x08); // Гироскоп 500 deg/s
  
  // Быстрая калибровка гироскопа (берем 50 значений при старте)
  long gx_sum=0, gy_sum=0, gz_sum=0;
  for(int i=0; i<50; i++) {
    Wire.beginTransmission(0x68); Wire.write(0x43); Wire.endTransmission(false);
    Wire.requestFrom(0x68, 6, true);
    gx_sum += (int16_t)(Wire.read()<<8|Wire.read());
    gy_sum += (int16_t)(Wire.read()<<8|Wire.read());
    gz_sum += (int16_t)(Wire.read()<<8|Wire.read());
    delay(5);
  }
  gyroXoffset = gx_sum / 50.0;
  gyroYoffset = gy_sum / 50.0;
  gyroZoffset = gz_sum / 50.0;
}

void getIMUData(float dt) {
  Wire.beginTransmission(0x68); Wire.write(0x3B); Wire.endTransmission(false);
  Wire.requestFrom(0x68, 14, true);

  // Читаем сырые данные
  int16_t accX = Wire.read()<<8|Wire.read();
  int16_t accY = Wire.read()<<8|Wire.read();
  int16_t accZ = Wire.read()<<8|Wire.read();
  int16_t temp = Wire.read()<<8|Wire.read();
  int16_t gyrX = Wire.read()<<8|Wire.read();
  int16_t gyrY = Wire.read()<<8|Wire.read();
  int16_t gyrZ = Wire.read()<<8|Wire.read();

  // 1. Конвертируем акселерометр в углы (градусы)
  // atan2 выдает радианы, умножаем на 57.29 для градусов
  float accPitch = atan2(accY, accZ) * 57.2958;
  float accRoll  = atan2(-accX, accZ) * 57.2958;

  // 2. Конвертируем гироскоп в скорость (градусы в секунду)
  // 65.5 - это делитель для режима 500 deg/s
  float gyroXrate = (gyrX - gyroXoffset) / 65.5;
  float gyroYrate = (gyrY - gyroYoffset) / 65.5;
  float gyroZrate = (gyrZ - gyroZoffset) / 65.5;

  // 3. КОМПЛЕМЕНТАРНЫЙ ФИЛЬТР (Магия слияния)
  // Угол = 96% (Старый Угол + Скорость*Время) + 4% (Угол Акселерометра)
  pitch = 0.96 * (pitch + gyroXrate * dt) + 0.04 * accPitch;
  roll  = 0.96 * (roll  + gyroYrate * dt) + 0.04 * accRoll;
  yaw   = yaw + gyroZrate * dt; // Yaw просто интегрируем (будет дрейфовать)
}

// ==========================================
// 2. ВЕБ ИНТЕРФЕЙС (HTML + CSS 3D)
// ==========================================
const char html_3d[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>OSMIUM 3D</title>
  <style>
    body { background: #111; color: #0f0; font-family: sans-serif; overflow: hidden; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0; }
    h2 { margin-top: 0; text-shadow: 0 0 10px #0f0; }
    
    /* СЦЕНА 3D */
    .scene {
      width: 200px; height: 200px;
      perspective: 600px;
      margin-bottom: 50px;
    }
    .cube {
      width: 100%; height: 100%;
      position: relative;
      transform-style: preserve-3d;
      transition: transform 0.05s linear; /* Плавность */
    }
    .face {
      position: absolute;
      width: 196px; height: 196px;
      border: 2px solid #0f0;
      background: rgba(0, 255, 0, 0.1);
      line-height: 196px;
      text-align: center;
      font-size: 20px;
      font-weight: bold;
      box-shadow: 0 0 15px rgba(0,255,0,0.2);
    }
    /* Позиционирование граней куба */
    .front  { transform: rotateY(  0deg) translateZ(100px); }
    .right  { transform: rotateY( 90deg) translateZ(100px); }
    .back   { transform: rotateY(180deg) translateZ(100px); }
    .left   { transform: rotateY(-90deg) translateZ(100px); }
    .top    { transform: rotateX( 90deg) translateZ(100px); }
    .bottom { transform: rotateX(-90deg) translateZ(100px); }

    .data-panel { background: #222; padding: 20px; border-radius: 10px; border: 1px solid #444; width: 300px; }
    .row { display: flex; justify-content: space-between; font-family: monospace; font-size: 1.2em; }
  </style>
</head>
<body>

  <h2>OSMIUM TELEMETRY</h2>

  <div class="scene">
    <div class="cube" id="cube">
      <div class="face front">FRONT</div>
      <div class="face back">BACK</div>
      <div class="face right">RIGHT</div>
      <div class="face left">LEFT</div>
      <div class="face top">TOP</div>
      <div class="face bottom">BOTTOM</div>
    </div>
  </div>

  <div class="data-panel">
    <div class="row"><span>PITCH (X):</span> <span id="val-p">0.0</span></div>
    <div class="row"><span>ROLL (Y):</span> <span id="val-r">0.0</span></div>
    <div class="row"><span>YAW (Z):</span> <span id="val-y">0.0</span></div>
  </div>

<script>
  const ws = new WebSocket(`ws://${location.hostname}:81/`);
  const cube = document.getElementById('cube');
  
  ws.onmessage = (event) => {
    const d = JSON.parse(event.data);
    
    // Обновляем цифры
    document.getElementById('val-p').innerText = d.p.toFixed(1);
    document.getElementById('val-r').innerText = d.r.toFixed(1);
    document.getElementById('val-y').innerText = d.y.toFixed(1);

    // Вращаем куб
    // CSS rotateX = Pitch, rotateZ = -Roll (инверсия для правильного вида), rotateY = Yaw
    cube.style.transform = `rotateX(${-d.p}deg) rotateZ(${d.r}deg) rotateY(${-d.y}deg)`;
  };
</script>
</body>
</html>
)rawliteral";

void setRGB(uint8_t r, uint8_t g, uint8_t b) { rgb.setPixelColor(0, rgb.Color(r, g, b)); rgb.show(); }

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  rgb.begin(); rgb.setBrightness(50); setRGB(255, 100, 0);

  // 1. Запуск шин
  Wire.begin(4, 5);  // MPU
  Wire1.begin(6, 7); // PCA
  
  // 2. Настройка MPU
  Serial.print("MPU Init...");
  setupMPU();
  Serial.println("Done.");
  
  // 3. Настройка PCA
  Serial.print("PCA Init...");
  pca.begin(); pca.setPWMFreq(50);
  Serial.println("Done.");

  // 4. WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) { delay(200); }
  setRGB(0, 255, 0);
  
  Serial.println(WiFi.localIP());

  server.on("/", [](){ server.send(200, "text/html", html_3d); });
  server.begin();
  webSocket.begin();
  
  lastTime = millis();
}

// ==========================================
// LOOP
// ==========================================
void loop() {
  webSocket.loop();
  server.handleClient();

  // Обновление физики ~50 раз в секунду (20мс)
  if (millis() - lastTime > 20) {
    float dt = (millis() - lastTime) / 1000.0;
    lastTime = millis();

    // Читаем датчик и считаем углы
    getIMUData(dt);
    
    // Отправляем JSON на сайт
    JsonDocument doc;
    doc["p"] = pitch; // Наклон вперед/назад
    doc["r"] = roll;  // Наклон влево/вправо
    doc["y"] = yaw;   // Поворот вокруг оси (будет дрейфовать)
    
    String s; serializeJson(doc, s);
    webSocket.broadcastTXT(s);
  }
}