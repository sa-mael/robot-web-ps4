document.addEventListener('DOMContentLoaded', () => {
    // 1. ИНЪЕКЦИЯ СТИЛЕЙ
    const style = document.createElement('style');
    style.innerHTML = `
        #cli-terminal {
            position: fixed; top: 0; left: 0; width: 100vw; height: 100vh;
            background: rgba(5, 5, 5, 0.9); backdrop-filter: blur(15px);
            z-index: 99998; /* Чуть ниже мобильного блокировщика */
            padding: 40px 5%; font-family: 'JetBrains Mono', monospace;
            color: #e0e0e0; display: none; flex-direction: column;
            opacity: 0;
        }
        .cli-header {
            display: flex; justify-content: space-between; border-bottom: 1px solid #333;
            padding-bottom: 15px; margin-bottom: 20px; font-size: 0.8rem; color: #888;
        }
        .cli-output {
            flex: 1; overflow-y: auto; display: flex; flex-direction: column; gap: 8px;
            font-size: 0.9rem; margin-bottom: 20px; padding-right: 10px;
        }
        .cli-output::-webkit-scrollbar { width: 5px; }
        .cli-output::-webkit-scrollbar-track { background: transparent; }
        .cli-output::-webkit-scrollbar-thumb { background: #333; }
        .cli-line { line-height: 1.5; word-wrap: break-word; }
        .cli-line.sys { color: #7c2ae8; }
        .cli-line.err { color: #e74c3c; }
        .cli-line.succ { color: #2ecc71; }
        .cli-input-wrapper { display: flex; align-items: center; gap: 15px; font-size: 1rem; }
        .cli-prompt { color: #7c2ae8; font-weight: bold; }
        #cli-input {
            flex: 1; background: transparent; border: none; color: #fff;
            font-family: 'JetBrains Mono', monospace; font-size: 1rem; outline: none;
        }
    `;
    document.head.appendChild(style);

    // 2. ИНЪЕКЦИЯ HTML-ИНТЕРФЕЙСА
    const terminalDiv = document.createElement('div');
    terminalDiv.id = 'cli-terminal';
    terminalDiv.innerHTML = `
        <div class="cli-header">
            <span>ARTTOUS OS v75.4 // ROOT ACCESS</span>
            <span style="color: #7c2ae8;">[ESC] TO CLOSE TERMINAL</span>
        </div>
        <div class="cli-output" id="cli-output">
            <div class="cli-line sys">Arttous RTOS v75.4 (Core 0/1 Active)</div>
            <div class="cli-line">Type 'help' to see a list of available commands.</div>
        </div>
        <div class="cli-input-wrapper">
            <span class="cli-prompt">operator@mesh:~$</span>
            <input type="text" id="cli-input" autocomplete="off" spellcheck="false">
        </div>
    `;
    document.body.appendChild(terminalDiv);

    // 3. ЛОГИКА И СОБЫТИЯ
    const terminal = document.getElementById('cli-terminal');
    const input = document.getElementById('cli-input');
    const output = document.getElementById('cli-output');
    let isTerminalOpen = false;

    // Слушаем клавиши ~ и ESC
    document.addEventListener('keydown', (e) => {
        if (e.key === '\`' || e.key === '~' || e.key === 'ё' || e.key === 'Ё') {
            e.preventDefault(); 
            toggleTerminal();
        }
        if (e.key === 'Escape' && isTerminalOpen) {
            toggleTerminal();
        }
    });

    function toggleTerminal() {
        isTerminalOpen = !isTerminalOpen;
        if (isTerminalOpen) {
            terminal.style.display = 'flex';
            if (typeof gsap !== 'undefined') {
                gsap.to(terminal, { opacity: 1, duration: 0.3, ease: "power2.out" });
            } else {
                terminal.style.opacity = 1;
            }
            setTimeout(() => input.focus(), 100);
        } else {
            if (typeof gsap !== 'undefined') {
                gsap.to(terminal, { 
                    opacity: 0, duration: 0.3, ease: "power2.in", 
                    onComplete: () => { terminal.style.display = 'none'; }
                });
            } else {
                terminal.style.opacity = 0;
                terminal.style.display = 'none';
            }
            input.blur();
        }
    }

    // Обработка команд при нажатии Enter
    input.addEventListener('keydown', function(e) {
        if (e.key === 'Enter') {
            const cmd = this.value.trim();
            if (cmd) {
                printLine(`operator@mesh:~$ ${cmd}`, '');
                processCommand(cmd.toLowerCase());
            }
            this.value = ''; 
        }
    });

    function printLine(text, className = '') {
        const div = document.createElement('div');
        div.className = `cli-line ${className}`;
        div.innerHTML = text;
        output.appendChild(div);
        output.scrollTop = output.scrollHeight; 
    }

    function processCommand(cmd) {
        input.disabled = true;

        switch (cmd) {
            case 'help':
                printLine(`Available commands:`);
                printLine(`  help       - Shows this message`);
                printLine(`  whoami     - Displays current operator identity`);
                printLine(`  ping fleet - Pings active mesh network nodes`);
                printLine(`  clear      - Clears terminal output`);
                printLine(`  reboot     - Restarts RTOS Kernel`);
                input.disabled = false; input.focus();
                break;

            case 'whoami':
                const user = localStorage.getItem('userNick') || 'GUEST_USER';
                printLine(user, 'succ');
                input.disabled = false; input.focus();
                break;

            case 'clear':
                output.innerHTML = '';
                input.disabled = false; input.focus();
                break;

            case 'ping fleet':
                printLine('Initiating LoRa Mesh ICMP sequence...', 'sys');
                setTimeout(() => { printLine('PING 10.0.0.11 (UNIT_ALPHA): 56 data bytes'); }, 500);
                setTimeout(() => { printLine('64 bytes from 10.0.0.11: icmp_seq=1 ttl=64 time=14.2 ms', 'succ'); }, 1200);
                setTimeout(() => { printLine('PING 10.0.0.14 (UNIT_BETA): 56 data bytes'); }, 1800);
                setTimeout(() => { printLine('64 bytes from 10.0.0.14: icmp_seq=1 ttl=64 time=18.5 ms', 'succ'); }, 2600);
                setTimeout(() => { printLine('PING 10.0.0.22 (UNIT_GAMMA): 56 data bytes'); }, 3200);
                setTimeout(() => { printLine('Request timeout for icmp_seq=1', 'err'); }, 4500);
                setTimeout(() => { 
                    printLine('--- FLEET PING STATISTICS ---', 'sys');
                    printLine('3 nodes transmitted, 2 received, 33% packet loss.');
                    input.disabled = false; input.focus();
                }, 4800);
                break;

            case 'reboot':
            case 'sudo reboot':
                printLine('INITIATING KERNEL PANIC...', 'err');
                printLine('KILLING CORE 0 [TaskNetwork]', 'err');
                printLine('KILLING CORE 1 [TaskControl]', 'err');
                
                if (typeof gsap !== 'undefined') {
                    const tl = gsap.timeline();
                    tl.to("body", { x: 10, skewX: 5, duration: 0.1, yoyo: true, repeat: 5 })
                      .to("body", { filter: "invert(1) hue-rotate(180deg)", duration: 0.1 })
                      .to("body", { opacity: 0, duration: 0.5, onComplete: () => {
                          window.location.reload(); 
                      }});
                } else {
                    setTimeout(() => window.location.reload(), 1000);
                }
                break;
                
            case 'sudo':
                printLine('Nice try. This incident will be reported.', 'err');
                input.disabled = false; input.focus();
                break;

            case 'make me a sandwich':
                printLine('I am a 16-DOF robotic platform, not a kitchen appliance.', 'sys');
                input.disabled = false; input.focus();
                break;

            default:
                printLine(`Command not found: ${cmd}`, 'err');
                printLine(`Type 'help' for available commands.`);
                input.disabled = false; input.focus();
        }
    }
    
    // Возвращаем фокус в инпут при клике в любое место терминала
    terminal.addEventListener('click', () => {
        input.focus();
    });
});