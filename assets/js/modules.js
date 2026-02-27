const Modules = {
    
    dashboard: `
        <h2 class="gs-item">OPERATOR OVERVIEW</h2>
        <span class="subtitle gs-item">Welcome back. Systems are nominal.</span>

        <div class="stats-grid">
            <div class="stat-card gs-item">
                <div class="stat-num">3</div>
                <div class="stat-label">Active Units</div>
            </div>
            <div class="stat-card gs-item">
                <div class="stat-num" style="color: var(--success);">124</div>
                <div class="stat-label">Commits Merged</div>
            </div>
            <div class="stat-card gs-item">
                <div class="stat-num">850</div>
                <div class="stat-label">Reputation Points</div>
            </div>
        </div>

        <h3 class="gs-item">CONTRIBUTION MATRIX</h3>
        <div class="heatmap-box gs-item">
            <div class="heatmap-grid" id="heatmap"></div>
            <div style="text-align: right; font-family: var(--font-mono); font-size: 0.7rem; color: #444; margin-top: 10px;">
                LAST 30 DAYS ACTIVITY
            </div>
        </div>

        <h3 class="gs-item">BADGE COLLECTION</h3>
        <div style="display: flex; gap: 15px; margin-top: 15px;" class="gs-item">
            <div style="width: 50px; height: 50px; background: #111; border: 1px solid #333; display: flex; align-items: center; justify-content: center; color: #444; font-size: 1.5rem;" title="Early Adopter">★</div>
            <div style="width: 50px; height: 50px; background: #111; border: 1px solid var(--accent); display: flex; align-items: center; justify-content: center; color: var(--accent); font-size: 1.5rem;" title="Code Contributor">⌘</div>
            <div style="width: 50px; height: 50px; background: #111; border: 1px dashed #333; opacity: 0.5;"></div>
        </div>
    `,

    fleet: `
        <h2 class="gs-item">FLEET MANAGEMENT</h2>
        <span class="subtitle gs-item">Manage your registered ESP32-C6 units.</span>

        <div class="fleet-list">
            <div class="fleet-item gs-item">
                <div class="fleet-info">
                    <h4>UNIT_ALPHA</h4>
                    <div class="fleet-meta">MAC: A0:B1:C2:D3:E4</div>
                    <div class="fleet-meta">FW: v53.0 (Up to date)</div>
                </div>
                <div style="display: flex; align-items: center; gap: 20px;">
                    <div class="status-badge">ONLINE</div>
                    <button class="action-btn" onclick="pingUnit('UNIT_ALPHA')">PING</button>
                    <button class="action-btn">CONFIG</button>
                </div>
            </div>

            <div class="fleet-item gs-item">
                <div class="fleet-info">
                    <h4>UNIT_BETA</h4>
                    <div class="fleet-meta">MAC: F5:E6:D7:C8:B9</div>
                    <div class="fleet-meta">FW: v52.4 (Update Available)</div>
                </div>
                <div style="display: flex; align-items: center; gap: 20px;">
                    <div class="status-badge">ONLINE</div>
                    <button class="action-btn" onclick="pingUnit('UNIT_BETA')">PING</button>
                    <button class="action-btn" style="color: var(--accent); border-color: var(--accent);">UPDATE</button>
                </div>
            </div>

            <div class="fleet-item gs-item">
                <div class="fleet-info">
                    <h4 style="color: #666;">UNIT_GAMMA</h4>
                    <div class="fleet-meta">MAC: 11:22:33:44:55</div>
                    <div class="fleet-meta">FW: UNKNOWN</div>
                </div>
                <div style="display: flex; align-items: center; gap: 20px;">
                    <div class="status-badge offline">OFFLINE</div>
                    <button class="action-btn" disabled style="opacity: 0.5;">PING</button>
                    <button class="action-btn">REMOVE</button>
                </div>
            </div>
        </div>
        <button class="action-btn gs-item" style="margin-top: 20px; width: 100%; border-style: dashed;">+ REGISTER NEW DEVICE</button>
    `,

    missions: `
        <div class="mission-header-section" id="missionHeaders">
            <div style="display: flex; justify-content: space-between; align-items: flex-end; margin-bottom: 20px;">
                <div class="gs-item">
                    <h2>OPEN CONTRACTS</h2>
                    <span class="subtitle">Contribute computing power or hardware to earn credits.</span>
                </div>
                <button class="action-btn gs-item" style="border-style: dashed;">REFRESH BOARD</button>
            </div>

            <div class="mission-stats-row">
                <div class="stat-box gs-item">
                    <div class="stat-val" style="color: var(--accent);">LVL 4</div>
                    <div class="stat-label">OPERATOR RANK</div>
                </div>
                <div class="stat-box gs-item">
                    <div class="stat-val" style="color: #2ecc71;">$1,450</div>
                    <div class="stat-label">TOTAL EARNINGS</div>
                </div>
                <div class="stat-box gs-item">
                    <div class="stat-val">98.2%</div>
                    <div class="stat-label">REPUTATION SCORE</div>
                </div>
            </div>

            <div class="mission-list">
                <div class="mission-card gs-item">
                    <div class="ms-icon">🦾</div>
                    <div class="ms-info">
                        <h4>PROJECT REPLICATOR: 6-AXIS ARM</h4>
                        <div class="ms-desc">Build an industrial-grade CNC robotic arm. Phase 1 Assembly.</div>
                    </div>
                    <div class="ms-tags">
                        <span class="tag-diff" style="color:#e74c3c; border-color:#e74c3c;">CLASS A</span>
                        <span class="tag-reward">+$250 CREDITS</span>
                    </div>
                    <button class="btn-accept" onclick="openMissionModal()">ACCEPT</button>
                </div>

                <div class="mission-card gs-item">
                    <div class="ms-icon">🧬</div>
                    <div class="ms-info">
                        <h4>NEURAL TRAINING: GAIT V2</h4>
                        <div class="ms-desc">Donate GPU cycles to train the new walking algorithm.</div>
                    </div>
                    <div class="ms-tags">
                        <span class="tag-diff" style="color:#f1c40f; border-color:#f1c40f;">CLASS B</span>
                        <span class="tag-reward">+$400 CREDITS</span>
                    </div>
                    <button class="btn-accept">ACCEPT</button>
                </div>

                <div class="mission-card gs-item">
                    <div class="ms-icon">⚡</div>
                    <div class="ms-info">
                        <h4>STRESS TEST: SERVO ARRAY</h4>
                        <div class="ms-desc">Execute high-load calibration sequence (Risk of overheating).</div>
                    </div>
                    <div class="ms-tags">
                        <span class="tag-diff" style="color:#e74c3c; border-color:#e74c3c;">CLASS A</span>
                        <span class="tag-reward">+$950 CREDITS</span>
                    </div>
                    <button class="btn-accept">ACCEPT</button>
                </div>

                <div class="mission-card gs-item" style="opacity: 0.5;">
                    <div class="ms-icon">🔒</div>
                    <div class="ms-info">
                        <h4>ENCRYPTED PAYLOAD</h4>
                        <div class="ms-desc">Locked. Requires Rank: SENIOR ENGINEER.</div>
                    </div>
                    <div class="ms-tags">
                        <span class="tag-diff">CLASS S</span>
                        <span class="tag-reward">UNKNOWN</span>
                    </div>
                    <button class="btn-accept" disabled style="cursor: not-allowed; border-color: #333; background: #111; color:#333;">LOCKED</button>
                </div>
                
                <h3 class="completed-section-title gs-item">COMPLETED PROJECTS (ARCHIVE)</h3>
                <div class="completed-card gs-item">
                    <img src="assets/images/photo_2025-12-15_19-08-27.jpg" class="completed-img" alt="The Cyber-Oni">
                    <div class="completed-info">
                        <h4>THE CYBER-ONI</h4>
                        <div class="completed-desc">
                            "A style you can put together yourself - part of the interior is part of your vision."<br>
                            Concept design, 3D modeling, and physical fabrication.
                        </div>
                        <div class="author-tag">/// LEAD DESIGNER: ROSTISLAV KOVALENKO</div>
                    </div>
                    <div>
                        <a href="https://www.artstation.com/artwork/oJvwaJ" target="_blank" class="btn-view-artstation">
                            VIEW ON<br>ARTSTATION
                        </a>
                    </div>
                </div>
            </div>
        </div>

        <div id="activeProjectView" style="display:none;">
            <div class="project-header-row">
                <div>
                    <button class="btn-back" onclick="closeProject()">← ABORT / BACK TO BOARD</button>
                    <h2 style="margin: 10px 0 0 0; color: #fff; text-transform: uppercase;">PROJECT "REPLICATOR" | 6-AXIS CNC ARM</h2>
                    <div style="color: #888; font-family: var(--font-mono); font-size: 0.8rem; margin-top: 5px;">
                        Phase 1: V1 Mother  ///  Objective: Precision metal milling & partial self-replication
                    </div>
                </div>
                <div style="text-align: right;">
                    <span style="color: #f1c40f; font-family: var(--font-mono); font-size: 0.8rem;">STATUS: PHASE 1 IN PROGRESS</span>
                </div>
            </div>

            <div class="project-grid">
                <div>
                    <img src="assets/images/3D robot rig animation.jpg" class="project-main-img" alt="6-Axis Robot Arm">
                    
                    <div class="info-panel">
                        <h4>CORE BOM (KEY COMPONENTS)</h4>
                        <ul style="color: #aaa; font-family: var(--font-mono); font-size: 0.85rem; line-height: 1.6; padding-left: 20px;">
                            <li><strong style="color:#ccc;">Motors:</strong> Nema 23 (J1-J3, >2.2Nm) & Nema 17 (J4-J6 Wrist)</li>
                            <li><strong style="color:#ccc;">Drives:</strong> Custom 1:100 Cycloidal (Dual-disc, 686ZZ Bearings)</li>
                            <li><strong style="color:#ccc;">Spindle:</strong> RATTM MOTOR 0.8kW Water-Cooled (65mm, ER11)</li>
                            <li><strong style="color:#ccc;">Payload Comp:</strong> Gas Springs (200N-300N force)</li>
                            <li><strong style="color:#ccc;">Logic:</strong> 32-bit MCU (Teensy 4.1/STM32) + DM542 Drivers</li>
                        </ul>
                    </div>

                    <div class="info-panel">
                        <h4>DATA & REFERENCES</h4>
                        <a href="https://youtu.be/cvuBCdxfGDg?si=x5nKxtRm-U4wOJH2" target="_blank" class="resource-link">YOUTUBE-Guide for making: 1:100 Cycloidal Drives</a>
                        <a href="#" class="resource-link">📄 DOCS Kinematic Architecture (750-800mm Reach)</a>
                        <a href="#" class="resource-link">⚙️ CAM CNC Strategy: Trochoidal Milling (800+ mm/min)</a>
                    </div>
                </div>

                <div>
                    <div class="info-panel">
                        <h4>EXECUTION PLAN (VON NEUMANN)</h4>
                        <p style="color: #666; font-family: var(--font-mono); font-size: 0.7rem; margin-bottom: 15px;">
                            Click a task to upload photo/screenshot proof. Admin verification required for progression.
                        </p>
                        
                        <div class="checklist-item" onclick="openUploadModal('Phase 1: 3D Print V1 Shells (PETG/ABS)')">
                            <div class="checklist-status"></div>
                            <div>1. Print V1 Shells & Fill (Epoxy-Granite)</div>
                        </div>
                        
                        <div class="checklist-item" onclick="openUploadModal('Task: J1-J3 Heavy Axis Assembly')">
                            <div class="checklist-status"></div>
                            <div>2. Assemble J1-J3 (Nema 23 + Cycloidal)</div>
                        </div>

                        <div class="checklist-item" onclick="openUploadModal('Task: Gravity Compensation')">
                            <div class="checklist-status"></div>
                            <div>3. Install 200N-300N Gas Springs</div>
                        </div>

                        <div class="checklist-item" onclick="openUploadModal('Task: J4-J6 Wrist & Spindle')">
                            <div class="checklist-status"></div>
                            <div>4. Mount Nema 17 Wrist & 0.8kW Spindle</div>
                        </div>

                        <div class="checklist-item" onclick="openUploadModal('Phase 2: CNC Mill V2 Aluminum Parts')">
                            <div class="checklist-status" style="border-color: var(--accent);"></div>
                            <div style="color: var(--accent);">5. [PHASE 2] Mill V2 Daughter Parts</div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    `,

    control: `
        <h2 class="gs-item">DIRECT CONTROL UPLINK</h2>
        <span class="subtitle gs-item">Connect to local Arttous unit via HTTP.</span>

        <div class="control-frame-container gs-item">
            <div class="address-bar">
                <img src="assets/images/ps4.png" style="height: 34px; opacity: 0.7;">
                <span style="color: #666; font-family: monospace; padding-top: 8px;">TARGET IP:</span>
                <input type="text" id="targetIP" value="http://10.0.0.113" class="ip-input">
                <button class="connect-btn" onclick="connectRobot()">ESTABLISH LINK</button>
            </div>
            
            <div id="frameWrapper" style="flex: 1; position: relative;">
                <div class="frame-placeholder">
                    <div style="font-size: 3rem; color: #222;">NO SIGNAL</div>
                    <p>Enter Robot IP Address above to begin FPV stream.</p>
                </div>
                <iframe id="robotFrame" src="" style="display:none;"></iframe>
            </div>
        </div>
    `,

    chat: `
        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px;" class="gs-item">
            <h2>ENCRYPTED CHANNEL</h2>
            <button class="action-btn" onclick="addFriend()">+ ADD NODE ID</button>
        </div>

        <div class="chat-top-controls gs-item">
            <button class="top-pill-btn" onclick="showRequests()">
                <span>INCOMING REQUESTS</span>
                <span class="badge-count" id="reqCount">2</span>
            </button>
            <button class="top-pill-btn" onclick="showBlocked()">
                <span>BLOCKED NODES</span>
            </button>
        </div>

        <div class="chat-layout gs-item">
            <div class="chat-list">
                <div class="search-bar-container">
                    <input type="text" id="friendSearch" class="friend-search" placeholder="Filter Nodes..." onkeyup="filterFriends()">
                </div>
                
                <div id="friendsList" class="friends-scroll">
                    <div class="friend project-member active" onclick="loadChat('Sarah_Connor', this)">
                        <span>Sarah_Connor</span>
                        <span class="project-badge">PROJECT</span>
                    </div>
                    <div class="friend" onclick="loadChat('Toto', this)">
                        <span>Toto</span>
                        <span style="font-size:9px; color:#444;">IDLE</span>
                    </div>
                    <div class="friend" onclick="loadChat('System_Bot', this)">
                        <span>System_Bot</span>
                    </div>
                </div>
            </div>

            <div class="chat-window">
                <div class="messages" id="msgArea"></div>
                <div class="chat-input-area">
                    <input type="text" id="chatInput" class="chat-input" placeholder="Enter message..." onkeypress="handleChat(event)">
                    <button class="action-btn" onclick="sendChat()">SEND</button>
                </div>
            </div>

            <div id="contextMenu" class="ctx-menu">
                <div class="ctx-item" onclick="toggleProjectAccess()">📡 Toggle Project Access</div>
                <div class="ctx-item" onclick="muteUser()">🔕 Mute Notifications</div>
                <div class="ctx-item" style="color:#e74c3c;" onclick="blockUser()">🚫 Block Node</div>
                <div class="ctx-item" style="color:#e74c3c;" onclick="deleteUser()">❌ Disconnect</div>
            </div>
        </div>
    `,

    investor: `
        <h2 class="gs-item">INVESTOR RELATIONS</h2>
        <div class="invest-grid">
            <div class="invest-card gs-item">
                <div class="metric-val">$850</div>
                <div class="metric-label">UNIT COST</div>
            </div>
            <div class="invest-card gs-item">
                <div class="metric-val">$1,200</div>
                <div class="metric-label">MRR / UNIT</div>
            </div>
            <div class="invest-card gs-item">
                <div class="metric-val">84%</div>
                <div class="metric-label">MARGIN</div>
            </div>
        </div>
        <h3 class="gs-item" style="color: var(--accent); margin-top: 40px; margin-bottom: 20px; font-family: var(--font-tech);">SOFTWARE ASSET: DIGITAL TWIN UI</h3>
        <div class="gs-item" style="background: #0a0a0a; border: 1px solid #222; padding: 20px; margin-bottom: 40px;">
            <img src="assets/images/Screenshot 2026-02-22 124034.png" style="width: 50%; border: 1px solid #333; border-radius: 4px; margin-bottom: 15px;" alt="Flight Deck UI">
            <h4 style="color: #fff; font-family: var(--font-tech); margin-bottom: 10px;">PROPRIETARY MISSION CONTROL INTERFACE</h4>
            <p style="color: #888; font-family: var(--font-mono); font-size: 0.85rem; line-height: 1.6;">
                Served entirely from the ESP32-S3's onboard memory, this Zero-Install Web UI provides operators with a real-time WebGL (Three.js) Digital Twin. Features 50Hz WebSocket telemetry, active IMU visualization, and direct kinematic joint control. A major selling point for enterprise RaaS (Robotics-as-a-Service) clients.
            </p>
        </div>
        
        <div class="gs-item" style="background: #111; border: 1px solid #333; padding: 20px;">
            <h3>SEED ROUND (OPEN)</h3>
            <p style="color: #666;">Fueling the transition to injection molding.</p>
            <button class="action-btn" style="margin-top: 10px;">DOWNLOAD PITCH DECK</button>
        </div>
    `,

    settings: `
        <h2 class="gs-item">SYSTEM CONFIGURATION</h2>
        <span class="subtitle gs-item">Manage identity protocols and data retention.</span>

        <div class="settings-group gs-item">
            <h4 style="color: #fff; margin-bottom: 20px; font-family: var(--font-tech);">IDENTITY MATRIX</h4>
        
            <div style="display: flex; gap: 25px; align-items: center; margin-bottom: 30px;">
                <div id="settingsAvatarPreview" class="large-avatar" style="width: 80px; height: 80px; font-size: 1.2rem; margin: 0;">OP</div>
                <div>
                    <input type="file" id="avatarInput" accept="image/*" onchange="handleAvatarUpload(this)">
                    <button class="action-btn" onclick="document.getElementById('avatarInput').click()">UPLOAD NEW AVATAR</button>
                    <button class="action-btn" style="border-color: #333; color: #666; margin-left: 10px;" onclick="resetAvatar()">RESET</button>
                    <div style="font-size: 0.7rem; color: #666; margin-top: 8px; font-family: var(--font-mono);">
                        SUPPORTED: JPG, PNG, WEBP (MAX 2MB)
                    </div>
                </div>
            </div>

            <div class="form-grid">
                <div>
                    <span class="input-label">OPERATOR ID (NICKNAME)</span>
                    <input type="text" id="inputNick" value="ALEX_MERCER" class="dark-input">
                </div>
                <div>
                    <span class="input-label">FULL DESIGNATION</span>
                    <input type="text" id="inputName" value="Alex Mercer" class="dark-input">
                </div>
                <div style="grid-column: span 2;">
                    <span class="input-label">SECURE COMMS CHANNEL (EMAIL)</span>
                    <input type="email" id="inputEmail" value="alex@arttous.com" class="dark-input">
                </div>
            </div>
        
            <button class="action-btn" style="background: var(--accent); color: #fff; border: none; padding: 12px 30px;" onclick="saveIdentity()">SAVE IDENTITY</button>
        </div>

        <div class="settings-group gs-item">
            <h4 style="color: #fff; margin-bottom: 20px; font-family: var(--font-tech);">SECURITY PROTOCOLS</h4>

            <div class="setting-row">
                <div class="setting-desc">
                    <h4>Encryption Key (Password)</h4>
                    <p>Last updated: 3 months ago.</p>
                </div>
                <button class="action-btn" onclick="togglePasswordBox()">UPDATE KEY</button>
            </div>

            <div id="passwordBox" class="password-box">
                <div class="form-grid">
                    <div>
                        <span class="input-label">CURRENT KEY</span>
                        <input type="password" placeholder="•••••••" class="dark-input">
                    </div>
                    <div>
                        <span class="input-label">NEW KEY</span>
                        <input type="password" placeholder="New Password" class="dark-input">
                    </div>
                </div>
                <button class="action-btn" style="width: 100%; border-color: var(--success); color: var(--success);" onclick="updatePassword()">CONFIRM NEW KEY</button>
            </div>

            <div class="setting-row">
                <div class="setting-desc">
                    <h4>Two-Factor Authentication</h4>
                    <p>Hardware token requirement.</p>
                </div>
                <label class="switch">
                    <input type="checkbox" id="check2FA" onchange="saveToggle('2fa', this.checked)">
                    <span class="slider"></span>
                </label>
            </div>
        </div>

        <div class="settings-group gs-item" style="border-color: #333;">
            <h4 style="color: var(--success); margin-bottom: 20px; font-family: var(--font-tech);">ASSET MANAGEMENT</h4>
            <div class="setting-row">
                <div class="setting-desc">
                    <h4>RaaS Subscription</h4>
                    <p style="color: var(--success);">STATUS: ACTIVE (Tier 2)</p>
                </div>
                <button class="action-btn" style="border-color: #666; color: #888;" onclick="alert('SUBSCRIPTION FROZEN. Assets will be recalled.')">FREEZE CONTRACT</button>
            </div>
            <div class="setting-row">
                <div class="setting-desc">
                    <h4>Telemetry Tracking</h4>
                    <p>Transmit Lidar/GPS data to HQ for diagnostics.</p>
                </div>
                <label class="switch">
                    <input type="checkbox" id="checkTrack" checked onchange="saveToggle('tracking', this.checked)">
                    <span class="slider"></span>
                </label>
            </div>
        </div>

        <div class="settings-group gs-item" style="border: 1px solid var(--alert); padding: 20px; background: rgba(231, 76, 60, 0.05);">
            <h4 style="color: var(--alert); margin-bottom: 20px; font-family: var(--font-tech);">TERMINAL COMMANDS</h4>
            <div class="setting-row">
                <div class="setting-desc">
                    <h4 style="color: #fff;">Export Neural Logs</h4>
                    <p>Download personal data in JSON format.</p>
                </div>
                <button class="action-btn" onclick="downloadData()">DOWNLOAD JSON</button>
            </div>
            <div class="setting-row" style="border-bottom: none; margin-bottom: 0;">
                <div class="setting-desc">
                    <h4 style="color: #fff;">Burn Identity</h4>
                    <p>Irreversible deletion of account and fleet access.</p>
                </div>
                <button class="action-btn" style="background: var(--alert); color: #fff; border: none;" onclick="deleteAccount()">EXECUTE BURN</button>
            </div>
        </div>
    `
};