[README.md](https://github.com/user-attachments/files/21567124/README.md)
# Art­tous Quadruped Robot Project

## Overview

Art­tous is a cutting-edge quadruped robot platform designed for high-speed, stable locomotion across diverse terrains. Controlled via a PS4 DualShock controller, it features autonomous route memory to return to base if control is lost. Future applications include geological mapping, search & rescue operations, and archaeological exploration.

## Key Features

* **Quadruped Locomotion**: Four-legged design for agility and balance.
* **PS4 Remote Control**: Intuitive, low-latency operation via DualShock.
* **Autonomous Path Memory**: Records waypoints and retraces steps if the link drops.
* **Future Use Cases**:

  * Geological surface mapping
  * Search & rescue in hazardous zones
  * Archaeological site surveys

## Project Structure

```
/tera/                # HTML templates for the website
  ├── index.html      # Landing page with hero & visuals
  ├── about.html      # Robot overview and team info
  ├── technology.html # Hardware, AI & manufacturing details
  ├── investors.html  # Market analysis, funding, ROI projections
  └── contact.html    # Contact form page

/bono/                # Assets, styles & scripts
  ├── css/
  │   └── styles.css  # Main stylesheet
  ├── js/
  │   └── main.js     # Blink animation & interactivity
  └── images/         # Logos, icons, renders, photos

/blender/             # 3D models and renders
  ├── robo-model.blend # Blender source file
  └── renders/        # Exported images

README.md             # Project documentation
LICENSE               # MIT license text
```

## Project Structure Сonnection Wires 

```
     Battery 3–8 V
      +3–8V ==== VM Bus ====+──> VM (all DRV modules)
      –GND  ---- GND Bus ---+──> GND (ESP32, PCA9685, DRV modules)

              I²C Bus
 ESP32-DevKit v1            PCA9685 16-CH PWM Driver
 +----------------------+   +-------------------------+
 | 3V3 ==== VCC Bus -->─+-->| VCC                     |
 | GND ---- GND Bus -->─+-->| GND                     |
 |                      |   |                         |
 | D32 SDA  ----||---->─+-->| SDA                      |
 | D33 SCL  ----||---->─+-->| SCL                      |
 |                      |   +-------------------------+
 |                      |
 | D17 SLP ------------+------------------------------> SLP/EEN (all DRV modules)
 |                      |
 |        DIR Lines     |
 | D18 ---> DRV1 IN2    |   PCA9685 PWM Channels       |  DRV Module #1
 | D19 ---> DRV1 IN4    |   +----------------------+   |  +-----------------+
 | D21 ---> DRV2 IN2    |   | CH0 --> IN1 (M1 A)    |   |  | IN1 <- CH0      |
 | D22 ---> DRV2 IN4    |   | CH1 --> IN3 (M1 B)    |   |  | IN2 <- D18      |
 | D23 ---> DRV3 IN2    |   | CH2 --> IN1 (M2 A)    |   |  | IN3 <- CH1      |
 | D25 ---> DRV3 IN4    |   | CH3 --> IN3 (M2 B)    |   |  | IN4 <- D19      |
 | D26 ---> DRV4 IN2    |   | CH4 --> IN1 (M3 A)    |   |  +-----------------+
 | D27 ---> DRV4 IN4    |   | CH5 --> IN3 (M3 B)    |   |
 | D2  ---> DRV5 IN2    |   | CH6 --> IN1 (M4 A)    |   |  DRV Module #2
 | D4  ---> DRV5 IN4    |   | CH7 --> IN3 (M4 B)    |   |  +-----------------+
 | D5  ---> DRV6 IN2    |   | CH8 --> IN1 (M5 A)    |   |  | IN1 <- CH2      |
 | D12 ---> DRV6 IN4    |   | CH9 --> IN3 (M5 B)    |   |  | IN2 <- D21      |
 | D13 ---> DRV7 IN2    |   | CH10-> IN1 (M6 A)     |   |  | IN3 <- CH3      |
 | D14 ---> DRV7 IN4    |   | CH11-> IN3 (M6 B)     |   |  | IN4 <- D22      |
 | D15 ---> DRV8 IN2    |   | CH12-> IN1 (M7 A)     |   |  +-----------------+
 | D16 ---> DRV8 IN4    |   | CH13-> IN3 (M7 B)     |   |
 +----------------------+   | CH14-> IN1 (M8 A)     |   |  ... for 8 modulus
                            | CH15-> IN3 (M8 B)     |   |
                            +----------------------+   |
                                                         |
      (further DRV modules #3…#8 are identically connected to CH4…CH 15 and the corresponding DIR lines D23…D16)             |
                                                         |
        Every DRV8833:                                   |
         • IN1/IN3 — PWM (from PCA9685 CHx)                 |
         • IN2/IN4 — DIR (from ESP32 Dxx)                   |
         • EEP/SLP — general (from ESP32 D17 or VCC)        |
         • VM — motor power supply (3–8 В, from battery)    |
         • VCC — logics 3.3 В (from ESP32 3V3)              |
         • GND — general GND                                |

```
## Getting Started

1. **Clone the repository**:

   ```bash
   git clone https://github.com/yourusername/arttous.git
   cd art­tous
   ```
2. **Run locally** (optional):

   ```bash
   cd tera
   python3 -m http.server
   ```
3. **View** at [http://localhost:8000](http://localhost:8000).

## Deployment (GitHub Pages)

1. Push to GitHub:

   ```bash
   git remote add origin https://github.com/yourusername/arttous.git
   git branch -M main
   git add .
   git commit -m "Initial commit"
   git push -u origin main
   ```
2. In GitHub settings, under **Pages**, set source to `main` branch `/tera` folder.
3. Your site appears at `https://yourusername.github.io/arttous/`.

## 3D Printing & Mechanical Design

This project includes comprehensive CAD files designed for direct 3D printing. Every part is modeled **one-to-one** with real-world dimensions and tolerances, enabling you to produce functional components that fit together seamlessly.

* **Real-World Assembly**: All mechanical elements have been engineered and stress-tested virtually to ensure reliable operation once printed.
* **Logical Design**: Each component’s geometry is optimized for strength, ease of printing, and logical assembly.
* **Calculated Precision**: Bearings, gear clearances, and joint interfaces are dimensioned to real-world standards, resulting in smooth movement and reliable performance.

## Usage

* **Customize Content**: Edit HTML files in `/tera` to change text, images, or navigation.
* **Adjust Styles**: Tweak colors, fonts, and layouts in `/bono/css/styles.css`.
* **Modify Animations**: Update blink-circle settings in `/bono/js/main.js`.
* **3D Assets**: Open Blender file `blender/robo-model.blend` to revise parts and re-export renders.

## Contributing

Contributions welcome! Steps:

1. Fork the repo.
2. Create a branch: `git checkout -b feature-name`.
3. Commit: `git commit -m 'Add feature'`.
4. Push: `git push origin feature-name`.
5. Open a Pull Request.

## License

MIT License (SPDX: MIT)

**Permissions**: Commercial use, modification, distribution, private use, sublicensing allowed.
**Conditions**: Include original copyright and license.
**Limitations**: Provided "as is," without warranty; authors not liable.
