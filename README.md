# Arttous Quadruped Robot Platform

![Status](https://img.shields.io/badge/Status-Active_Development-blueviolet)
![Hardware](https://img.shields.io/badge/Hardware-ESP32--C6%20%7C%20PCA9685%20%7C%20MG90S-blue)
![License](https://img.shields.io/badge/License-MIT-green)

**Arttous** is an advanced, open-source quadruped robot platform designed for high-speed locomotion and autonomous navigation in complex terrains. Unlike traditional hobbyist projects, Arttous utilizes a **servo-driven architecture** with real-time **Inverse Kinematics (IK)** and dynamic stabilization, making it suitable for geological mapping, search & rescue operations, and robotics research.

---

## 🚀 Key Features

* **Precision Locomotion:** 12-DOF (Degrees of Freedom) driven by metal-gear servos for smooth, organic gaits (Trot, Crawl, Idle).
* **Smart Stability:** Integrated **GY-521 (MPU-6050)** IMU provides 6-axis telemetry, allowing the robot to self-level on uneven surfaces.
* **Autonomous Path Memory:** The robot records waypoints in real-time. If the connection is lost, it autonomously retraces its path to the deployment point.
* **Low-Latency Control:** Direct **PS4 DualShock** interface via Bluetooth for instant tactile feedback.
* **Modular Web Documentation:** Includes a full documentation website (`/tera`) for easy deployment and presentation.

---

## 🛠 Hardware Architecture

### Core Components
| Component | Model | Purpose |
| :--- | :--- | :--- |
| **Microcontroller** | **ESP32-C6-DevKitC-1-N8** | Main brain. Handles IK math, Bluetooth, and Logic. |
| **Actuators** | **12x MG90S** (Metal Gear) | High-torque servos (3 per leg). Replaced older DC motors. |
| **Servo Driver** | **PCA9685** (16-Channel) | Offloads PWM generation from the ESP32 via I²C. |
| **IMU Sensor** | **GY-521 / MPU-6050** | 3-Axis Gyroscope + 3-Axis Accelerometer for balance. |
| **Power System** | **5A UBEC** + 2S LiPo (7.4V) | UBEC provides clean 5V/6V to servos; ESP32 powered via 3.3V/USB. |

### Wiring Diagram (ESP32-C6)

The system uses a shared **I²C Bus** topology to communicate with both the Servo Driver and the IMU.

**I²C Connections:**
```text
ESP32-C6 (Master)      PCA9685 (Slave 0x40)      GY-521 (Slave 0x68)
-----------------      --------------------      -------------------
GPIO 6 (SDA)  <------> SDA  <------------------> SDA
GPIO 7 (SCL)  <------> SCL  <------------------> SCL
GND           -------  GND  -------------------- GND
3V3           -------  VCC  -------------------- VCC (Logic Only)

Battery (7.4V) (+) ====> UBEC Input (+)
Battery (7.4V) (-) ----> UBEC Input (-)

UBEC Output (5V/6V) ===> PCA9685 (V+ Terminal) -> Powers Servos
UBEC GND            ---> PCA9685 (GND Terminal) -> Common Ground


/arttous-root
├── /tera/                # Documentation & Marketing Website
│   ├── index.html        # Main Landing Page (Hero, Tech Stack)
│   ├── technology.html   # Detailed Hardware Specs & Deep Dives
│   ├── investors.html    # Roadmap & Business Model
│   ├── about.html        # Project Evolution & Team
│   └── contact.html      # Enterprise Contact Form
│
├── /bono/                # Web Assets
│   ├── css/              # Stylesheets (styles.css, home.css)
│   ├── js/               # Interactive scripts (main.js)
│   └── images/           # Renders (release.jpg, prototype.jpg, icons)
│
├── /blender/             # 3D CAD Files
│   ├── robo-model.blend  # Master design file (1:1 scale for printing)
│   └── renders/          # High-res marketing visuals
│
└── README.md             # This file