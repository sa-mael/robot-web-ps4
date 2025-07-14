Art­tous Quadruped Robot Project

Overview

Art­tous is a cutting-edge quadruped robot platform designed for high-speed, stable locomotion across diverse terrains. Controlled via a PS4 DualShock controller, it features autonomous route memory to return to base if control is lost. Future applications include geological mapping, search & rescue operations, and archaeological exploration.

Key Features
	•	Quadruped Locomotion: Four-legged design for agility and balance.
	•	PS4 Remote Control: Intuitive, low-latency operation via DualShock.
	•	Autonomous Path Memory: Records waypoints and retraces steps if the link drops.
	•	Future Use Cases:
	•	Geological surface mapping
	•	Search & rescue in hazardous zones
	•	Archaeological site surveys

Project Structure

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

Getting Started
	1.	Clone the repository:

git clone https://github.com/yourusername/arttous.git
cd art­tous


	2.	Run locally (optional):

cd tera
python3 -m http.server


	3.	View at http://localhost:8000.

Deployment (GitHub Pages)
	1.	Push to GitHub:

git remote add origin https://github.com/yourusername/arttous.git
git branch -M main
git add .
git commit -m "Initial commit"
git push -u origin main


	2.	In GitHub settings, under Pages, set source to main branch /tera folder.
	3.	Your site appears at https://yourusername.github.io/arttous/.

3D Printing & Mechanical Design

This project includes comprehensive CAD files designed for direct 3D printing. Every part is modeled one-to-one with real-world dimensions and tolerances, enabling you to produce functional components that fit together seamlessly.
	•	Real-World Assembly: All mechanical elements have been engineered and stress-tested virtually to ensure reliable operation once printed.
	•	Logical Design: Each component’s geometry is optimized for strength, ease of printing, and logical assembly.
	•	Calculated Precision: Bearings, gear clearances, and joint interfaces are dimensioned to real-world standards, resulting in smooth movement and reliable performance.

Usage
	•	Customize Content: Edit HTML files in /tera to change text, images, or navigation.
	•	Adjust Styles: Tweak colors, fonts, and layouts in /bono/css/styles.css.
	•	Modify Animations: Update blink-circle settings in /bono/js/main.js.
	•	3D Assets: Open Blender file blender/robo-model.blend to revise parts and re-export renders.

Contributing

Contributions welcome! Steps:
	1.	Fork the repo.
	2.	Create a branch: git checkout -b feature-name.
	3.	Commit: git commit -m 'Add feature'.
	4.	Push: git push origin feature-name.
	5.	Open a Pull Request.

License

MIT License (SPDX: MIT)

Permissions: Commercial use, modification, distribution, private use, sublicensing allowed.
Conditions: Include original copyright and license.
Limitations: Provided “as is,” without warranty; authors not liable.