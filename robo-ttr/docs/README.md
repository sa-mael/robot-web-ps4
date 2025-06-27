# RoboInvest Quadruped Robot Project

## Overview

RoboInvest is a cutting-edge quadruped robot platform designed for high-speed, stable locomotion across diverse terrains. Controlled via a PS4 DualShock controller, it features autonomous path memory to return to base if control is lost. Future applications include geological mapping, search & rescue operations, and archaeological exploration.

## Key Features

* **Quadruped Locomotion**: Four-legged design ensures stability and agility.
* **PS4 Remote Control**: Low-latency wireless operation using a DualShock controller.
* **Autonomous Memory**: Records its route and can autonomously return to the starting point if the connection drops.
* **Future Applications**:

  * Geological surface mapping
  * Search & rescue in hazardous environments
  * Archaeological site surveys

## Project Structure

```
/tera/                # Website HTML templates
  ├── index.html      # Landing page with visual presentation
  ├── about.html      # Robot overview and team info
  ├── technology.html # Hardware, AI, and manufacturing details
  ├── investors.html  # Market analysis, funding, ROI
  └── contact.html    # Contact form

/bono/                # Assets and styling
  ├── css/
  │   └── styles.css  # Main stylesheet
  ├── js/
  │   └── main.js     # JS animations (blinking circles, interactivity)
  └── images/         # Logos, icons, render screenshots

/blender/             # 3D model source and renders
  ├── robo-model.blend # Blender 4.0 scene file
  └── renders/        # Exported PNG renders

README.md             # This documentation
LICENSE               # Project license
```

## Getting Started

1. **Clone the repository**:

   ```bash
   git clone https://github.com/yourusername/roboinvest.git
   cd roboinvest
   ```
2. **Serve the website** (e.g., with VS Code Live Server or Python HTTP server):

   ```bash
   cd tera
   python3 -m http.server
   ```
3. **Open** [http://localhost:8000](http://localhost:8000) in your browser to view.

## Usage

* **Customize the logo**: Replace `bono/images/your-logo.png` and update the `<link rel="icon">` in each HTML.
* **Edit content**: Modify HTML files in `/tera` to update text, add images, or adjust the navigation.
* **Style tweaks**: Update colors, fonts, and layouts in `/bono/css/styles.css`.
* **Animation parameters**: Change blink frequency or circle count in `/bono/js/main.js`.
* **3D assets**: Open `blender/robo-model.blend` in Blender to inspect or export new renders.

## Contributing

Contributions are welcome! Please:

1. Fork the repository.
2. Create a feature branch (`git checkout -b feature-name`).
3. Commit your changes (`git commit -m 'Add feature'`).
4. Push to the branch (`git push origin feature-name`).
5. Open a Pull Request.

## License

This project is licensed under the MIT License (SPDX: MIT). See [LICENSE](LICENSE) for the full license text.

The MIT License is a permissive open-source license that grants broad permissions:

* **Commercial use**: The software may be used for commercial purposes.
* **Modification**: You may modify and adapt the software.
* **Distribution**: You may distribute original or modified copies.
* **Private use**: You may use the software privately.
* **Sublicensing**: You may grant sublicenses to third parties.

**Conditions**:

* You must include the original copyright and license notice in all copies.
* No additional restrictions may be imposed by the licensee.

**Limitations**:

* The software is provided "as is", without warranty of any kind.
* The authors or copyright holders are not liable for any claims or damages arising from its use.
