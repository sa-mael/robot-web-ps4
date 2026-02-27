// --- 1. GSAP SPA ROUTER ---
function loadModule(moduleId, btnElement) {
    const container = document.getElementById('dynamic-content');

    // Обновляем активную кнопку в сайдбаре
    if (btnElement) {
        document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
        btnElement.classList.add('active');
    }

    // Если модуля нет, выходим
    if (!Modules[moduleId]) {
        console.error(`Module ${moduleId} not found!`);
        return;
    }

    // Анимация ИСЧЕЗНОВЕНИЯ старого контента
    if (container.innerHTML.trim() !== '') {
        gsap.to(container, {
            opacity: 0, 
            y: -10, 
            duration: 0.2, 
            ease: "power2.in",
            onComplete: () => injectAndAnimate(moduleId, container)
        });
    } else {
        // Если это первая загрузка страницы
        injectAndAnimate(moduleId, container);
    }
}

function injectAndAnimate(moduleId, container) {
    // Вставляем HTML код модуля
    container.innerHTML = Modules[moduleId];
    
    // Сбрасываем позицию контейнера
    gsap.set(container, { opacity: 1, y: 0 });

    // Ищем элементы для анимации
    const items = container.querySelectorAll('.gs-item');
    
    // Анимация ПОЯВЛЕНИЯ нового контента (Stagger)
    if (items.length > 0) {
        gsap.fromTo(items, 
            { opacity: 0, y: 20 }, 
            { opacity: 1, y: 0, duration: 0.6, stagger: 0.05, ease: "power3.out" }
        );
    }

    // Инициализация специфичной логики после рендера HTML
    initModuleLogic(moduleId);
}

// Запуск дефолтного модуля при загрузке страницы
document.addEventListener('DOMContentLoaded', () => {
    loadIdentityGlobal(); // Грузим аватарки в сайдбар
    loadModule('dashboard', document.querySelector('.tab-btn.active'));
});

// --- 2. ИНИЦИАЛИЗАЦИЯ ЛОГИКИ МОДУЛЕЙ ---
let currentTarget = "Sarah_Connor"; 
let selectedFriendEl = null;

function initModuleLogic(moduleId) {
    if (moduleId === 'dashboard') {
        generateHeatmap();
    }
    if (moduleId === 'settings') {
        loadIdentitySettings();
        loadToggles();
    }
    if (moduleId === 'chat') {
        updateContextListeners();
        const activeFriend = document.querySelector('.friend.active');
        if(activeFriend) loadChat('Sarah_Connor', activeFriend);
        
        // Закрытие контекстного меню чата по клику мимо
        document.addEventListener('click', (e) => {
            const menu = document.getElementById('contextMenu');
            if(menu && !e.target.closest('.friend') && !e.target.closest('.ctx-menu')) {
                menu.style.display = 'none';
            }
        });
    }
}

// --- 3. ГЛОБАЛЬНЫЕ ФУНКЦИИ (Работают для инжектированного HTML) ---

// DASHBOARD
function generateHeatmap() {
    const heatContainer = document.getElementById('heatmap');
    if(!heatContainer) return;
    for(let i=0; i<50; i++) { 
        const col = document.createElement('div'); col.className = 'h-col';
        for(let j=0; j<7; j++) { 
            const cell = document.createElement('div'); cell.className = 'h-cell';
            if(Math.random() > 0.7) cell.classList.add('h-l1');
            if(Math.random() > 0.85) cell.classList.add('h-l2');
            if(Math.random() > 0.95) cell.classList.add('h-l3');
            col.appendChild(cell);
        }
        heatContainer.appendChild(col);
    }
}

// FLEET
function pingUnit(name) {
    const term = document.getElementById('terminal');
    term.style.display = 'block';
    term.innerHTML = `<div class="term-line">> PING ${name}...</div>`;
    setTimeout(() => { term.innerHTML += `<div class="term-line" style="color: #fff;">> REPLY from ${name}: bytes=32 time=12ms TTL=54</div>`; }, 500);
    setTimeout(() => { term.innerHTML += `<div class="term-line" style="color: #fff;">> REPLY from ${name}: bytes=32 time=14ms TTL=54</div>`; }, 1000);
    setTimeout(() => { 
        term.innerHTML += `<div class="term-line" style="color: #2ecc71;">> CONNECTION STABLE</div>`;
        setTimeout(() => { term.style.display = 'none'; }, 2000);
    }, 1500);
}

// IDENTITY & SETTINGS
function loadIdentityGlobal() {
    const savedNick = localStorage.getItem('userNick') || 'ALEX_MERCER';
    const savedAvatar = localStorage.getItem('userAvatar');
    document.querySelectorAll('.username').forEach(el => el.innerText = savedNick);
    if(savedAvatar) applyAvatar(savedAvatar);
}

function loadIdentitySettings() {
    const savedNick = localStorage.getItem('userNick') || 'ALEX_MERCER';
    const savedName = localStorage.getItem('userName') || 'Alex Mercer';
    const savedEmail = localStorage.getItem('userEmail') || 'alex@arttous.com';

    if(document.getElementById('inputNick')) {
        document.getElementById('inputNick').value = savedNick;
        document.getElementById('inputName').value = savedName;
        document.getElementById('inputEmail').value = savedEmail;
    }
}

function handleAvatarUpload(input) {
    if (input.files && input.files[0]) {
        const reader = new FileReader();
        reader.onload = function(e) {
            const imageData = e.target.result; 
            localStorage.setItem('userAvatar', imageData);
            applyAvatar(imageData);
            showToast("AVATAR UPLOADED");
        }
        reader.readAsDataURL(input.files[0]);
    }
}

function applyAvatar(imgSrc) {
    document.querySelectorAll('.large-avatar, .avatar-small').forEach(av => {
        av.style.backgroundImage = `url(${imgSrc})`;
        av.style.backgroundSize = 'cover';
        av.style.backgroundPosition = 'center';
        av.innerText = ''; 
    });
}

function resetAvatar() { localStorage.removeItem('userAvatar'); location.reload(); }

function saveIdentity() {
    const nick = document.getElementById('inputNick').value;
    const name = document.getElementById('inputName').value;
    const email = document.getElementById('inputEmail').value;
    localStorage.setItem('userNick', nick);
    localStorage.setItem('userName', name);
    localStorage.setItem('userEmail', email);
    document.querySelectorAll('.username').forEach(el => el.innerText = nick);
    showToast("IDENTITY MATRIX UPDATED");
}

function togglePasswordBox() { document.getElementById('passwordBox').classList.toggle('show'); }
function updatePassword() {
    showToast("ENCRYPTION KEY ROTATED");
    togglePasswordBox();
    document.querySelectorAll('#passwordBox input').forEach(i => i.value = '');
}
function showToast(message) {
    const toast = document.getElementById("toast");
    toast.innerText = message; toast.className = "show";
    setTimeout(() => { toast.className = toast.className.replace("show", ""); }, 3000);
}
function saveToggle(key, isChecked) { localStorage.setItem(key, isChecked); }
function loadToggles() {
    const t2fa = localStorage.getItem('2fa');
    const tTrack = localStorage.getItem('tracking');
    if(t2fa !== null && document.getElementById('check2FA')) document.getElementById('check2FA').checked = (t2fa === 'true');
    if(tTrack !== null && document.getElementById('checkTrack')) document.getElementById('checkTrack').checked = (tTrack === 'true');
}
function downloadData() { alert("DOWNLOADING JSON LOGS..."); }
function deleteAccount() {
    if(confirm("CRITICAL WARNING: This action will permanently erase your Operator ID. Proceed?")) {
        localStorage.clear();
        window.location.href = 'index.html';
    }
}

// CONTROL
function connectRobot() {
    const ip = document.getElementById('targetIP').value;
    const iframe = document.getElementById('robotFrame');
    const placeholder = document.querySelector('.frame-placeholder');
    if(ip.length < 5) return alert("Invalid IP Address");
    placeholder.style.display = 'none';
    iframe.style.display = 'block';
    iframe.src = ip; 
}

// MISSIONS & MODALS
function openMissionModal() {
    document.getElementById('missionModal').style.display = 'flex';
    document.getElementById('modalStep1').style.display = 'block';
    document.getElementById('modalStep2').style.display = 'none';
}
function nextModalStep() {
    document.getElementById('modalStep1').style.display = 'none';
    document.getElementById('modalStep2').style.display = 'block';
}
function closeModal(id) { document.getElementById(id).style.display = 'none'; }
function startProject() {
    closeModal('missionModal');
    document.getElementById('missionHeaders').style.display = 'none';
    document.getElementById('activeProjectView').style.display = 'block';
}
function closeProject() {
    document.getElementById('activeProjectView').style.display = 'none';
    document.getElementById('missionHeaders').style.display = 'block';
}

let currentTaskElement = null;
function openUploadModal(taskName) {
    currentTaskElement = event.currentTarget; 
    document.getElementById('uploadTaskName').innerText = "Verifying: " + taskName;
    document.getElementById('uploadModal').style.display = 'flex';
}
function submitProof() {
    closeModal('uploadModal');
    if(currentTaskElement) {
        currentTaskElement.classList.add('pending');
        currentTaskElement.innerHTML = `
            <div class="checklist-status">⌛</div>
            <div style="flex: 1;">${currentTaskElement.innerText}</div>
            <div style="color: #f1c40f; font-size: 0.6rem;">PENDING ADMIN</div>
        `;
    }
    alert("UPLOAD SUCCESSFUL.\nData transmitted to System Admin for verification.");
}

// CHAT
function filterFriends() {
    const filter = document.getElementById('friendSearch').value.toUpperCase();
    const friends = document.getElementById("friendsList").getElementsByClassName("friend");
    for (let i = 0; i < friends.length; i++) {
        const span = friends[i].querySelector("span"); 
        if (span && span.innerText.toUpperCase().indexOf(filter) > -1) {
            friends[i].style.display = "flex"; 
        } else {
            friends[i].style.display = "none";
        }
    }
}
function loadChat(name, el) {
    currentTarget = name;
    document.querySelectorAll('.friend').forEach(f => f.classList.remove('active'));
    if(el) el.classList.add('active');
    
    const area = document.getElementById('msgArea');
    area.innerHTML = ''; 

    if(name.includes('Sarah')) {
        area.innerHTML += `<div class="msg them">[${name}]: I've updated the locomotion config. Check it out.</div>`;
        area.innerHTML += `<div class="msg me">[You]: Copy that. Deploying to Unit Alpha now.</div>`;
    } else if (name === 'System_Bot') {
        area.innerHTML += `<div class="msg them">[SYSTEM]: Welcome back, Operator. Systems nominal.</div>`;
    } else {
         area.innerHTML += `<div class="msg them" style="color:#444; text-align:center; font-size:0.7rem;">--- ENCRYPTED LOG STARTED ---</div>`;
    }
}
function updateContextListeners() {
    document.querySelectorAll('.friend').forEach(f => {
        f.oncontextmenu = function(e) {
            e.preventDefault();
            openContextMenu(e, this);
        };
    });
}
function openContextMenu(e, el) {
    const menu = document.getElementById('contextMenu');
    selectedFriendEl = el;
    menu.style.display = 'block';
    menu.style.left = e.clientX + 'px'; 
    menu.style.top = e.clientY + 'px';
}
function toggleProjectAccess() {
    if (!selectedFriendEl) return;
    const nameSpan = selectedFriendEl.querySelector('span');
    const nameText = nameSpan ? nameSpan.innerText : selectedFriendEl.innerText;

    if (selectedFriendEl.classList.contains('project-member')) {
        selectedFriendEl.classList.remove('project-member');
        const badge = selectedFriendEl.querySelector('.project-badge');
        if(badge) badge.remove();
        alert(`Access REVOKED for ${nameText}`);
    } else {
        selectedFriendEl.classList.add('project-member');
        if (!selectedFriendEl.querySelector('.project-badge')) {
            const badge = document.createElement('span');
            badge.className = 'project-badge';
            badge.innerText = 'PROJECT';
            selectedFriendEl.appendChild(badge);
        }
        alert(`Access GRANTED to ${nameText}`);
    }
}
function muteUser() { alert("Notifications muted."); }
function blockUser() { 
    if(selectedFriendEl && confirm("Block this user?")) { 
        selectedFriendEl.remove(); 
        document.getElementById('msgArea').innerHTML = ''; 
    } 
}
function deleteUser() { if(selectedFriendEl && confirm("Disconnect node?")) selectedFriendEl.remove(); }
function addFriend() {
    const id = prompt("Enter Node ID to add:");
    if(id) {
        const list = document.getElementById('friendsList');
        const newFriend = document.createElement('div');
        newFriend.className = 'friend';
        newFriend.innerHTML = `<span>${id}</span>`;
        newFriend.onclick = function() { loadChat(id, this); };
        list.appendChild(newFriend);
        updateContextListeners();
    }
}
function handleChat(e) { if(e.key === 'Enter') sendChat(); }
function sendChat() {
    const input = document.getElementById('chatInput');
    const area = document.getElementById('msgArea');
    const txt = input.value;
    if(txt.trim() === "") return;

    const myNick = localStorage.getItem('userNick') || 'ME';
    area.innerHTML += `<div class="msg me">[${myNick}]: ${txt}</div>`;
    input.value = "";
    area.scrollTop = area.scrollHeight;

    if(currentTarget === 'System_Bot') {
         setTimeout(() => {
            area.innerHTML += `<div class="msg them">[SYSTEM]: Command "${txt}" processed.</div>`;
            area.scrollTop = area.scrollHeight;
        }, 600);
    }
}
function showRequests() { alert("INCOMING REQUESTS:\n1. Alex_V9 (Wants to join)\n2. Router_X (Ping)"); }
function showBlocked() { alert("BLOCKED USERS:\n- Spammer_123"); }