/* ══════════════════════════════════════════════════════
   ARTTOUS — app.js
   Core application logic. Works with luxury account.html.
══════════════════════════════════════════════════════ */

/* ── Module loader ── */
function loadModule(name, btnEl, projectView = false) {
    // Update active tab
    if (btnEl) {
        document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
        btnEl.classList.add('active');
    }

    const content = document.getElementById('dynamic-content');
    content.innerHTML = Modules[name] || '';

    // Animate items
    const items = content.querySelectorAll('.gs-item');
    if (typeof gsap !== 'undefined') {
        gsap.fromTo(items,
            { opacity: 0, y: 10 },
            { opacity: 1, y: 0, duration: 0.4, stagger: 0.06, ease: 'power2.out' }
        );
    } else {
        items.forEach((el, i) => {
            el.style.transition = `opacity .4s ${i * 0.06}s, transform .4s ${i * 0.06}s`;
            el.style.opacity = '1';
            el.style.transform = 'none';
        });
    }

    // Module-specific init
    if (name === 'dashboard') initHeatmap();
    if (name === 'settings') initSettings();
    if (name === 'chat') initChat();
    if (name === 'missions' && projectView) {
        setTimeout(() => {
            const headers = document.getElementById('missionHeaders');
            const project = document.getElementById('activeProjectView');
            if (headers) headers.style.display = 'none';
            if (project) project.style.display = 'block';
        }, 100);
    }
}

/* ── Heatmap ── */
function initHeatmap() {
    const el = document.getElementById('heatmap');
    if (!el) return;
    el.innerHTML = '';
    for (let w = 0; w < 52; w++) {
        const col = document.createElement('div');
        col.className = 'h-col';
        for (let d = 0; d < 7; d++) {
            const cell = document.createElement('div');
            const r = Math.random();
            let cls = '';
            if (r > 0.55) cls = 'l1';
            if (r > 0.70) cls = 'l2';
            if (r > 0.82) cls = 'l3';
            if (r > 0.91) cls = 'l4';
            if (r > 0.97) cls = 'l5';
            cell.className = 'h-cell' + (cls ? ' ' + cls : '');
            col.appendChild(cell);
        }
        el.appendChild(col);
    }
}

/* ── Settings init ── */
function initSettings() {
    const nick = localStorage.getItem('userNick') || 'ALEX_MERCER';
    const name = localStorage.getItem('userName') || 'Alex Mercer';
    const email = localStorage.getItem('userEmail') || 'alex@arttous.com';
    const av = localStorage.getItem('userAvatar');

    const nickEl = document.getElementById('inputNick');
    const nameEl = document.getElementById('inputName');
    const emailEl = document.getElementById('inputEmail');
    if (nickEl) nickEl.value = nick;
    if (nameEl) nameEl.value = name;
    if (emailEl) emailEl.value = email;

    if (av) {
        const preview = document.getElementById('settingsAv');
        if (preview) {
            preview.style.backgroundImage = `url(${av})`;
            preview.textContent = '';
        }
    }

    const check2FA = document.getElementById('check2FA');
    const checkTrack = document.getElementById('checkTrack');
    if (check2FA) check2FA.checked = localStorage.getItem('2fa') === 'true';
    if (checkTrack) checkTrack.checked = localStorage.getItem('tracking') !== 'false';
}

function saveIdentity() {
    const nick = document.getElementById('inputNick')?.value || 'ALEX_MERCER';
    const name = document.getElementById('inputName')?.value || 'Alex Mercer';
    const email = document.getElementById('inputEmail')?.value || '';
    localStorage.setItem('userNick', nick);
    localStorage.setItem('userName', name);
    localStorage.setItem('userEmail', email);
    updateSidebarUser();
    showToast('Identity saved');
}

function handleAvatarUpload(input) {
    const file = input.files[0];
    if (!file || file.size > 2 * 1024 * 1024) { showToast('File too large — max 2MB'); return; }
    const reader = new FileReader();
    reader.onload = (e) => {
        const data = e.target.result;
        localStorage.setItem('userAvatar', data);
        const preview = document.getElementById('settingsAv');
        if (preview) { preview.style.backgroundImage = `url(${data})`; preview.textContent = ''; }
        const sb = document.getElementById('sidebarAvatar');
        if (sb) { sb.style.backgroundImage = `url(${data})`; const l = sb.querySelector('.prof-av-letter'); if (l) l.style.display = 'none'; }
        showToast('Avatar updated');
    };
    reader.readAsDataURL(file);
}

function resetAvatar() {
    localStorage.removeItem('userAvatar');
    const preview = document.getElementById('settingsAv');
    if (preview) { preview.style.backgroundImage = ''; preview.textContent = (localStorage.getItem('userNick') || 'A').charAt(0).toUpperCase(); }
    const sb = document.getElementById('sidebarAvatar');
    if (sb) { sb.style.backgroundImage = ''; const l = sb.querySelector('.prof-av-letter'); if (l) l.style.display = ''; }
    showToast('Avatar reset');
}

function togglePasswordBox() {
    const box = document.getElementById('passwordBox');
    if (box) box.classList.toggle('show');
}

function updatePassword() { showToast('Encryption key updated'); }

function saveToggle(key, val) {
    localStorage.setItem(key, val);
    showToast(`${key.toUpperCase()} ${val ? 'enabled' : 'disabled'}`);
}

function downloadData() {
    const data = {
        nick: localStorage.getItem('userNick'),
        email: localStorage.getItem('userEmail'),
        level: 4,
        reputation: 850,
        theme: localStorage.getItem('theme'),
    };
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url; a.download = 'arttous-data.json'; a.click();
    URL.revokeObjectURL(url);
    showToast('Download started');
}

function deleteAccount() {
    if (confirm('Execute Burn? This is irreversible.')) {
        localStorage.clear();
        window.location.href = 'index.html';
    }
}

/* ── Fleet ── */
function pingUnit(name) {
    showToast(`Pinging ${name}...`);
    setTimeout(() => showToast(`${name}: 14.2ms — ONLINE`), 1200);
}

/* ── Control link ── */
function connectRobot() {
    const ip = document.getElementById('targetIP')?.value?.trim();
    if (!ip) return;
    const frame = document.getElementById('robotFrame');
    const placeholder = document.getElementById('framePlaceholder');
    if (frame) { frame.src = ip; frame.style.display = 'block'; }
    if (placeholder) placeholder.style.display = 'none';
}

/* ── Chat ── */
const chatHistories = {
    Sarah_Connor: [
        { me: false, text: 'Phase 1 shell is ready for inspection.' },
        { me: true,  text: 'Acknowledged. Uploading verification photo now.' },
    ],
    Toto: [{ me: false, text: 'Hey, check the LoRa signal range doc.' }],
    System_Bot: [{ me: false, text: 'System nominal. All units reporting healthy.' }],
};
let currentChat = 'Sarah_Connor';

function initChat() {
    loadChat('Sarah_Connor', document.querySelector('.friend.active'));
    document.getElementById('contextMenu')?.addEventListener('contextmenu', e => e.preventDefault());
    document.getElementById('friendsList')?.addEventListener('contextmenu', e => {
        e.preventDefault();
        const menu = document.getElementById('contextMenu');
        if (menu) { menu.style.display = 'block'; menu.style.left = e.clientX + 'px'; menu.style.top = e.clientY + 'px'; }
    });
    document.addEventListener('click', () => {
        const menu = document.getElementById('contextMenu');
        if (menu) menu.style.display = 'none';
    }, { once: false });
}

function loadChat(name, el) {
    currentChat = name;
    if (el) {
        document.querySelectorAll('.friend').forEach(f => f.classList.remove('active'));
        el.classList.add('active');
    }
    const area = document.getElementById('msgArea');
    if (!area) return;
    area.innerHTML = '';
    (chatHistories[name] || []).forEach(m => appendMsg(m.text, m.me));
}

function appendMsg(text, isMe) {
    const area = document.getElementById('msgArea');
    if (!area) return;
    const div = document.createElement('div');
    div.className = 'msg ' + (isMe ? 'me' : 'them');
    div.textContent = text;
    area.appendChild(div);
    area.scrollTop = area.scrollHeight;
}

function sendChat() {
    const input = document.getElementById('chatInput');
    if (!input || !input.value.trim()) return;
    const text = input.value.trim();
    if (!chatHistories[currentChat]) chatHistories[currentChat] = [];
    chatHistories[currentChat].push({ me: true, text });
    appendMsg(text, true);
    input.value = '';
}

function handleChat(e) { if (e.key === 'Enter') sendChat(); }

function filterFriends() {
    const q = document.getElementById('friendSearch')?.value?.toLowerCase() || '';
    document.querySelectorAll('.friend').forEach(f => {
        f.style.display = f.textContent.toLowerCase().includes(q) ? '' : 'none';
    });
}

function addFriend() {
    const id = prompt('Enter Node ID:');
    if (!id) return;
    const list = document.getElementById('friendsList');
    if (!list) return;
    const el = document.createElement('div');
    el.className = 'friend';
    el.textContent = id;
    el.onclick = () => loadChat(id, el);
    list.appendChild(el);
    chatHistories[id] = [];
}

function showRequests() { showToast('2 pending connection requests'); }
function showBlocked()  { showToast('No blocked nodes'); }

function toggleProjectAccess() { showToast('Project access toggled'); }
function muteUser()            { showToast('Notifications muted'); }
function blockUser()           { showToast('Node blocked'); }
function deleteUser()          { showToast('Node disconnected'); }

/* ── Missions ── */
function closeProject() {
    const headers = document.getElementById('missionHeaders');
    const project = document.getElementById('activeProjectView');
    if (headers) headers.style.display = 'block';
    if (project) project.style.display = 'none';
}

/* ── Init on load ── */
document.addEventListener('DOMContentLoaded', () => {
    loadModule('dashboard', document.querySelector('.tab-btn.active'));
});