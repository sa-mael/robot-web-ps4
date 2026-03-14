/* ══════════════════════════════════════════════════════
   ARTTOUS — modules.js
   All module HTML. Styled to match luxury dashboard.
══════════════════════════════════════════════════════ */

const Modules = {

    dashboard: `
        <div class="mod-eyebrow gs-item">Operator Overview</div>
        <h2 class="mod-title gs-item">Command Interface</h2>
        <span class="mod-sub gs-item">Welcome back. Systems are nominal.</span>

        <div class="stats-grid gs-item">
            <div class="stat-card">
                <div class="stat-rule"></div>
                <div class="stat-num">3</div>
                <div class="stat-label">Active Units</div>
                <div class="stat-delta">↑ 1 from last week</div>
            </div>
            <div class="stat-card">
                <div class="stat-rule"></div>
                <div class="stat-num">124</div>
                <div class="stat-label">Commits Merged</div>
                <div class="stat-delta">↑ 18 today</div>
            </div>
            <div class="stat-card">
                <div class="stat-rule"></div>
                <div class="stat-num">850</div>
                <div class="stat-label">Reputation Points</div>
                <div class="stat-delta">↑ 35 this week</div>
            </div>
        </div>

        <div class="panel gs-item">
            <div class="panel-hdr">
                <span class="panel-title">Contribution History</span>
                <button class="panel-act">52 weeks ↗</button>
            </div>
            <div class="heatmap-wrap">
                <div class="heatmap-canvas" id="heatmap"></div>
                <div class="hm-foot">
                    <span class="hm-lbl">Commit frequency</span>
                    <div class="hm-legend">
                        <div class="hm-leg-c" style="background:var(--surface)"></div>
                        <div class="hm-leg-c" style="background:var(--hm1)"></div>
                        <div class="hm-leg-c" style="background:var(--hm2)"></div>
                        <div class="hm-leg-c" style="background:var(--hm3)"></div>
                        <div class="hm-leg-c" style="background:var(--hm4)"></div>
                        <div class="hm-leg-c" style="background:var(--accent)"></div>
                    </div>
                </div>
            </div>
        </div>

        <div class="panel gs-item">
            <div class="panel-hdr">
                <span class="panel-title">Credentials</span>
                <span class="panel-act">3 / 12</span>
            </div>
            <div class="badge-row">
                <div class="badge-item" title="Early Adopter">★</div>
                <div class="badge-item earned" title="Code Contributor">⌘</div>
                <div class="badge-item earned" title="Senior Engineer">III</div>
                <div class="badge-item" title="Locked" style="opacity:.25;cursor:not-allowed;">IV</div>
                <div class="badge-item" title="Locked" style="opacity:.25;cursor:not-allowed;">V</div>
            </div>
        </div>
    `,

    fleet: `
        <div class="mod-eyebrow gs-item">Asset Registry</div>
        <h2 class="mod-title gs-item">Fleet Management</h2>
        <span class="mod-sub gs-item">Manage your registered ESP32-C6 units.</span>

        <div class="fleet-item gs-item">
            <div>
                <div class="fleet-name">Unit Alpha</div>
                <div class="fleet-meta">MAC: A0:B1:C2:D3:E4<br>FW: v53.0 — Up to date</div>
            </div>
            <div class="fleet-actions">
                <div class="status-badge">Online</div>
                <button class="action-btn" onclick="pingUnit('UNIT_ALPHA')">Ping</button>
                <button class="action-btn">Config</button>
            </div>
        </div>

        <div class="fleet-item gs-item">
            <div>
                <div class="fleet-name">Unit Beta</div>
                <div class="fleet-meta">MAC: F5:E6:D7:C8:B9<br>FW: v52.4 — Update available</div>
            </div>
            <div class="fleet-actions">
                <div class="status-badge">Online</div>
                <button class="action-btn" onclick="pingUnit('UNIT_BETA')">Ping</button>
                <button class="action-btn accent-border">Update</button>
            </div>
        </div>

        <div class="fleet-item gs-item">
            <div>
                <div class="fleet-name" style="color:var(--text-dim);">Unit Gamma</div>
                <div class="fleet-meta">MAC: 11:22:33:44:55<br>FW: Unknown</div>
            </div>
            <div class="fleet-actions">
                <div class="status-badge offline">Offline</div>
                <button class="action-btn" disabled>Ping</button>
                <button class="action-btn danger">Remove</button>
            </div>
        </div>

        <button class="action-btn dashed gs-item" style="width:100%;margin-top:4px;padding:clamp(12px,1rem,18px);">
            + Register New Device
        </button>
    `,

    missions: `
        <div id="missionHeaders">
            <div style="display:flex;justify-content:space-between;align-items:flex-end;margin-bottom:clamp(16px,1.5vw,26px);gap:14px;flex-wrap:wrap;">
                <div>
                    <div class="mod-eyebrow gs-item">Active Board</div>
                    <h2 class="mod-title gs-item">Open Contracts</h2>
                    <span class="mod-sub gs-item">Contribute computing power or hardware to earn credits.</span>
                </div>
                <button class="action-btn dashed gs-item" style="align-self:flex-end;">Refresh Board</button>
            </div>

            <div class="mission-stats-row gs-item">
                <div class="stat-box">
                    <div class="stat-val" style="color:var(--accent);">Lv. 4</div>
                    <div class="stat-label">Operator Rank</div>
                </div>
                <div class="stat-box">
                    <div class="stat-val" style="color:var(--pos);">$1,450</div>
                    <div class="stat-label">Total Earnings</div>
                </div>
                <div class="stat-box">
                    <div class="stat-val">98.2%</div>
                    <div class="stat-label">Reputation Score</div>
                </div>
            </div>

            <div class="mission-card gs-item">
                <div class="ms-icon">ARM</div>
                <div class="ms-info">
                    <h4>Project Replicator: 6-Axis Arm</h4>
                    <div class="ms-desc">Build an industrial-grade CNC robotic arm. Phase 1 Assembly.</div>
                </div>
                <div class="ms-tags">
                    <span class="tag-diff classA">Class A</span>
                    <span class="tag-reward">+$250</span>
                </div>
                <button class="btn-accept" onclick="openMissionModal()">Accept</button>
            </div>

            <div class="mission-card gs-item">
                <div class="ms-icon">GPU</div>
                <div class="ms-info">
                    <h4>Neural Training: Gait v2</h4>
                    <div class="ms-desc">Donate GPU cycles to train the new walking algorithm.</div>
                </div>
                <div class="ms-tags">
                    <span class="tag-diff classB">Class B</span>
                    <span class="tag-reward">+$400</span>
                </div>
                <button class="btn-accept">Accept</button>
            </div>

            <div class="mission-card gs-item">
                <div class="ms-icon">SRV</div>
                <div class="ms-info">
                    <h4>Stress Test: Servo Array</h4>
                    <div class="ms-desc">Execute high-load calibration sequence. Risk of overheating.</div>
                </div>
                <div class="ms-tags">
                    <span class="tag-diff classA">Class A</span>
                    <span class="tag-reward">+$950</span>
                </div>
                <button class="btn-accept">Accept</button>
            </div>

            <div class="mission-card gs-item" style="opacity:.38;pointer-events:none;">
                <div class="ms-icon">ENC</div>
                <div class="ms-info">
                    <h4>Encrypted Payload</h4>
                    <div class="ms-desc">Locked. Requires Rank: Senior Engineer.</div>
                </div>
                <div class="ms-tags">
                    <span class="tag-diff classS">Class S</span>
                    <span class="tag-reward" style="color:var(--text-dim);">Unknown</span>
                </div>
                <button class="btn-accept" disabled>Locked</button>
            </div>

            <div class="section-divider gs-item">Completed Archive</div>

            <div class="completed-card gs-item">
                <img src="assets/images/photo_2025-12-15_19-08-27.jpg" class="completed-img" alt="The Cyber-Oni">
                <div class="completed-info">
                    <h4>The Cyber-Oni</h4>
                    <div class="completed-desc">
                        "A style you can put together yourself — part of the interior is part of your vision."<br>
                        Concept design, 3D modelling, and physical fabrication.
                    </div>
                    <div class="author-tag">/// Lead Designer: Rostislav Kovalenko</div>
                </div>
                <div style="flex-shrink:0;">
                    <a href="https://www.artstation.com/artwork/oJvwaJ" target="_blank" class="action-btn accent-border" style="display:block;text-align:center;padding:clamp(8px,.65rem,12px) clamp(14px,1.2rem,20px);">
                        View on<br>ArtStation
                    </a>
                </div>
            </div>
        </div>

        <div id="activeProjectView" style="display:none;">
            <div style="display:flex;justify-content:space-between;align-items:flex-start;flex-wrap:wrap;gap:14px;margin-bottom:clamp(16px,1.5vw,26px);padding-bottom:clamp(14px,1.3vw,22px);border-bottom:1px solid var(--line);">
                <div>
                    <button class="action-btn" style="margin-bottom:8px;" onclick="closeProject()">← Back to Board</button>
                    <h2 class="mod-title" style="margin-bottom:4px;">Project "Replicator"</h2>
                    <div style="font-family:var(--font-mono);font-size:clamp(8px,.6rem,10px);letter-spacing:.12em;color:var(--text-dim);">Phase 1: V1 Mother — Objective: Precision metal milling & partial self-replication</div>
                </div>
                <div style="font-family:var(--font-mono);font-size:clamp(8px,.6rem,10px);letter-spacing:.14em;color:var(--warn);text-align:right;padding-top:4px;">Status: Phase 1 In Progress</div>
            </div>

            <div class="project-grid">
                <div>
                    <img src="assets/images/3D robot rig animation.jpg" class="project-img" alt="6-Axis Robot Arm">
                    <div class="info-panel">
                        <h4>Core BOM — Key Components</h4>
                        <ul>
                            <li><strong>Motors:</strong> Nema 23 (J1–J3, &gt;2.2Nm) &amp; Nema 17 (J4–J6 Wrist)</li>
                            <li><strong>Drives:</strong> Custom 1:100 Cycloidal (Dual-disc, 686ZZ Bearings)</li>
                            <li><strong>Spindle:</strong> RATTM MOTOR 0.8kW Water-Cooled (65mm, ER11)</li>
                            <li><strong>Payload Comp:</strong> Gas Springs (200N–300N force)</li>
                            <li><strong>Logic:</strong> 32-bit MCU (Teensy 4.1/STM32) + DM542 Drivers</li>
                        </ul>
                    </div>
                    <div class="info-panel">
                        <h4>Data &amp; References</h4>
                        <a href="https://youtu.be/cvuBCdxfGDg?si=x5nKxtRm-U4wOJH2" target="_blank" class="resource-link">↗ YouTube — Guide: 1:100 Cycloidal Drives</a>
                        <a href="#" class="resource-link">↗ Docs — Kinematic Architecture (750–800mm Reach)</a>
                        <a href="#" class="resource-link">↗ CAM — Trochoidal Milling Strategy (800+ mm/min)</a>
                    </div>
                </div>

                <div>
                    <div class="info-panel">
                        <h4>Execution Plan</h4>
                        <p style="font-family:var(--font-mono);font-size:clamp(7px,.54rem,10px);color:var(--text-dim);letter-spacing:.06em;margin-bottom:clamp(10px,.85rem,16px);line-height:1.6;">
                            Click a task to upload proof. Admin verification required for progression.
                        </p>
                        <div class="checklist-item" onclick="openUploadModal('Phase 1: 3D Print V1 Shells (PETG/ABS)')">
                            <div class="checklist-status"></div>
                            <div>1. Print V1 Shells &amp; Fill (Epoxy-Granite)</div>
                        </div>
                        <div class="checklist-item" onclick="openUploadModal('Task: J1–J3 Heavy Axis Assembly')">
                            <div class="checklist-status"></div>
                            <div>2. Assemble J1–J3 (Nema 23 + Cycloidal)</div>
                        </div>
                        <div class="checklist-item" onclick="openUploadModal('Task: Gravity Compensation')">
                            <div class="checklist-status"></div>
                            <div>3. Install 200N–300N Gas Springs</div>
                        </div>
                        <div class="checklist-item" onclick="openUploadModal('Task: J4–J6 Wrist & Spindle')">
                            <div class="checklist-status"></div>
                            <div>4. Mount Nema 17 Wrist &amp; 0.8kW Spindle</div>
                        </div>
                        <div class="checklist-item" onclick="openUploadModal('Phase 2: CNC Mill V2 Aluminum Parts')" style="border-color:var(--accent-dim);color:var(--accent);">
                            <div class="checklist-status" style="border-color:var(--accent-dim);"></div>
                            <div>5. [Phase 2] Mill V2 Daughter Parts</div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    `,

    control: `
        <div class="mod-eyebrow gs-item">Remote Access</div>
        <h2 class="mod-title gs-item">Direct Control Uplink</h2>
        <span class="mod-sub gs-item">Connect to local Arttous unit via HTTP.</span>

        <div class="control-frame-container gs-item">
            <div class="address-bar">
                <img src="assets/images/ps4.png" style="height:30px;opacity:.6;flex-shrink:0;" alt="">
                <span style="font-family:var(--font-mono);font-size:clamp(8px,.6rem,10px);letter-spacing:.1em;color:var(--text-dim);white-space:nowrap;">Target IP</span>
                <input type="text" id="targetIP" value="http://10.0.0.113" class="ip-input">
                <button class="connect-btn" onclick="connectRobot()">Establish Link</button>
            </div>
            <div class="frame-wrap" id="frameWrapper">
                <div class="frame-placeholder" id="framePlaceholder">
                    <div class="frame-placeholder-title">No Signal</div>
                    <div class="frame-placeholder-sub">Enter robot IP address above to begin FPV stream</div>
                </div>
                <iframe id="robotFrame" src=""></iframe>
            </div>
        </div>
    `,

    chat: `
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:clamp(10px,.85rem,16px);gap:14px;flex-wrap:wrap;" class="gs-item">
            <div>
                <div class="mod-eyebrow">Encrypted Channel</div>
                <h2 class="mod-title">Secure Comms</h2>
            </div>
            <button class="action-btn" onclick="addFriend()">+ Add Node ID</button>
        </div>

        <div style="display:flex;gap:8px;margin-bottom:clamp(10px,.85rem,14px);" class="gs-item">
            <button class="pill-btn" onclick="showRequests()">
                Incoming Requests <span class="badge-count" id="reqCount">2</span>
            </button>
            <button class="pill-btn" onclick="showBlocked()">
                Blocked Nodes
            </button>
        </div>

        <div class="chat-layout gs-item">
            <div class="chat-list">
                <div class="chat-search-wrap">
                    <input type="text" id="friendSearch" class="friend-search" placeholder="Filter nodes..." onkeyup="filterFriends()">
                </div>
                <div id="friendsList" class="friends-scroll">
                    <div class="friend project-member active" onclick="loadChat('Sarah_Connor', this)">
                        <span>Sarah_Connor</span>
                        <span class="proj-badge">Project</span>
                    </div>
                    <div class="friend" onclick="loadChat('Toto', this)">
                        <span>Toto</span>
                        <span style="font-family:var(--font-mono);font-size:clamp(7px,.5rem,8px);color:var(--text-faint);letter-spacing:.1em;">IDLE</span>
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
                    <button class="action-btn" onclick="sendChat()">Send</button>
                </div>
            </div>

            <div id="contextMenu" class="ctx-menu">
                <div class="ctx-item" onclick="toggleProjectAccess()">Toggle Project Access</div>
                <div class="ctx-item" onclick="muteUser()">Mute Notifications</div>
                <div class="ctx-item danger" onclick="blockUser()">Block Node</div>
                <div class="ctx-item danger" onclick="deleteUser()">Disconnect</div>
            </div>
        </div>
    `,

    investor: `
        <div class="mod-eyebrow gs-item">Financials</div>
        <h2 class="mod-title gs-item">Investor Relations</h2>
        <span class="mod-sub gs-item">Real-time unit economics and asset overview.</span>

        <div class="invest-grid gs-item">
            <div class="invest-card">
                <div class="invest-val">$850</div>
                <div class="invest-label">Unit Cost</div>
            </div>
            <div class="invest-card">
                <div class="invest-val">$1,200</div>
                <div class="invest-label">MRR / Unit</div>
            </div>
            <div class="invest-card">
                <div class="invest-val">84%</div>
                <div class="invest-label">Margin</div>
            </div>
        </div>

        <div class="panel gs-item">
            <div class="panel-hdr">
                <span class="panel-title">Software Asset — Digital Twin UI</span>
            </div>
            <div style="padding:clamp(16px,1.5rem,26px);">
                <img src="assets/images/loren.png" style="width:55%;border:1px solid var(--line2);margin-bottom:clamp(12px,1rem,20px);" alt="Flight Deck UI">
                <div style="font-family:var(--font-serif);font-size:clamp(14px,1.05rem,18px);font-weight:500;color:var(--text);letter-spacing:.04em;margin-bottom:8px;">Proprietary Mission Control Interface</div>
                <p style="font-family:var(--font-mono);font-size:clamp(8px,.62rem,11px);color:var(--text-mid);line-height:1.7;letter-spacing:.04em;">
                    Served entirely from the ESP32-S3's onboard memory, this Zero-Install Web UI provides operators with a real-time WebGL (Three.js) Digital Twin. Features 50Hz WebSocket telemetry, active IMU visualisation, and direct kinematic joint control. A major selling point for enterprise RaaS clients.
                </p>
            </div>
        </div>

        <div class="panel gs-item">
            <div class="panel-hdr">
                <span class="panel-title">Seed Round — Open</span>
                <button class="panel-act">Download Pitch Deck</button>
            </div>
            <div style="padding:clamp(14px,1.3rem,22px);">
                <p style="font-family:var(--font-mono);font-size:clamp(8px,.62rem,11px);color:var(--text-dim);letter-spacing:.06em;line-height:1.7;">
                    Fuelling the transition to injection moulding. Seeking seed capital to scale unit production and expand the RaaS operator network.
                </p>
            </div>
        </div>
    `,

    settings: `
        <div class="mod-eyebrow gs-item">Preferences</div>
        <h2 class="mod-title gs-item">System Configuration</h2>
        <span class="mod-sub gs-item">Manage identity protocols and data retention.</span>

        <div class="settings-group gs-item">
            <div class="settings-group-title">Identity Matrix</div>
            <div style="display:flex;gap:clamp(16px,1.5rem,26px);align-items:center;margin-bottom:clamp(18px,1.6rem,28px);flex-wrap:wrap;">
                <div id="settingsAvatarPreview" style="width:clamp(48px,3.6rem,64px);height:clamp(48px,3.6rem,64px);border-radius:50%;background:var(--surface2);border:1px solid var(--line2);display:flex;align-items:center;justify-content:center;font-family:var(--font-serif);font-size:clamp(16px,1.2rem,22px);color:var(--accent);flex-shrink:0;background-size:cover;background-position:center;" id="settingsAv">A</div>
                <div>
                    <input type="file" id="avatarInput" accept="image/*" onchange="handleAvatarUpload(this)" style="display:none;">
                    <button class="action-btn" onclick="document.getElementById('avatarInput').click()">Upload Avatar</button>
                    <button class="action-btn" style="margin-left:8px;" onclick="resetAvatar()">Reset</button>
                    <div style="font-family:var(--font-mono);font-size:clamp(7px,.52rem,9px);color:var(--text-dim);margin-top:8px;letter-spacing:.1em;">JPG, PNG, WEBP — Max 2MB</div>
                </div>
            </div>
            <div class="form-grid">
                <div>
                    <span class="input-label">Operator ID (Nickname)</span>
                    <input type="text" id="inputNick" value="ALEX_MERCER" class="dark-input">
                </div>
                <div>
                    <span class="input-label">Full Designation</span>
                    <input type="text" id="inputName" value="Alex Mercer" class="dark-input">
                </div>
                <div style="grid-column:span 2;">
                    <span class="input-label">Secure Comms Channel (Email)</span>
                    <input type="email" id="inputEmail" value="alex@arttous.com" class="dark-input">
                </div>
            </div>
            <button class="action-btn primary" onclick="saveIdentity()">Save Identity</button>
        </div>

        <div class="settings-group gs-item">
            <div class="settings-group-title">Security Protocols</div>
            <div class="setting-row">
                <div class="setting-desc">
                    <h4>Encryption Key (Password)</h4>
                    <p>Last updated: 3 months ago.</p>
                </div>
                <button class="action-btn" onclick="togglePasswordBox()">Update Key</button>
            </div>
            <div id="passwordBox" class="password-box">
                <div class="form-grid">
                    <div>
                        <span class="input-label">Current Key</span>
                        <input type="password" placeholder="••••••••" class="dark-input">
                    </div>
                    <div>
                        <span class="input-label">New Key</span>
                        <input type="password" placeholder="New password" class="dark-input">
                    </div>
                </div>
                <button class="action-btn accent-border" style="width:100%;" onclick="updatePassword()">Confirm New Key</button>
            </div>
            <div class="setting-row" style="margin-top:clamp(10px,.85rem,16px);">
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

        <div class="settings-group gs-item">
            <div class="settings-group-title">Asset Management</div>
            <div class="setting-row">
                <div class="setting-desc">
                    <h4>RaaS Subscription</h4>
                    <p style="color:var(--pos);">Status: Active — Tier 2</p>
                </div>
                <button class="action-btn" onclick="alert('Subscription frozen. Assets will be recalled.')">Freeze Contract</button>
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

        <div class="settings-group danger-zone gs-item">
            <div class="settings-group-title">Terminal Commands</div>
            <div class="setting-row">
                <div class="setting-desc">
                    <h4>Export Neural Logs</h4>
                    <p>Download personal data in JSON format.</p>
                </div>
                <button class="action-btn" onclick="downloadData()">Download JSON</button>
            </div>
            <div class="setting-row" style="border:none;margin-bottom:0;">
                <div class="setting-desc">
                    <h4>Burn Identity</h4>
                    <p>Irreversible deletion of account and fleet access.</p>
                </div>
                <button class="action-btn danger" onclick="deleteAccount()">Execute Burn</button>
            </div>
        </div>
    `
};