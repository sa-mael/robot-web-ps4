/* ══════════════════════════════════════════════════════
   ARTTOUS — hidden-cli.js
   Hidden terminal. Open with ` (backtick) or Ё.
   Restyled to match luxury dashboard.
══════════════════════════════════════════════════════ */

document.addEventListener('DOMContentLoaded', () => {

    /* ── Inject styles ── */
    const style = document.createElement('style');
    style.innerHTML = `
        #cli-terminal {
            position: fixed; inset: 0;
            background: rgba(4, 4, 4, 0.94);
            backdrop-filter: blur(18px) saturate(0.6);
            z-index: 99998;
            padding: clamp(32px, 3.5vw, 60px) clamp(28px, 4vw, 72px);
            font-family: 'DM Mono', monospace;
            color: var(--text, #f0f0e8);
            display: none;
            flex-direction: column;
            opacity: 0;
            border-top: 1px solid var(--accent, #c8a05a);
        }
        .cli-header {
            display: flex; justify-content: space-between; align-items: center;
            border-bottom: 1px solid var(--line, #222);
            padding-bottom: clamp(10px, 1rem, 18px);
            margin-bottom: clamp(14px, 1.4rem, 24px);
            font-size: clamp(8px, 0.6rem, 11px);
            letter-spacing: 0.18em;
            color: var(--text-dim, #686860);
            text-transform: uppercase;
        }
        .cli-header-left { color: var(--accent, #c8a05a); }
        .cli-header-right { opacity: 0.6; }
        .cli-output {
            flex: 1; overflow-y: auto;
            display: flex; flex-direction: column; gap: 6px;
            font-size: clamp(10px, 0.72rem, 13px);
            margin-bottom: clamp(14px, 1.4rem, 22px);
            padding-right: 8px;
            letter-spacing: 0.04em;
        }
        .cli-output::-webkit-scrollbar { width: 3px; }
        .cli-output::-webkit-scrollbar-thumb { background: var(--line2, #2e2e2e); }
        .cli-line { line-height: 1.6; word-wrap: break-word; color: var(--text-mid, #acacA4); }
        .cli-line.sys  { color: var(--accent, #c8a05a); }
        .cli-line.err  { color: var(--neg, #8a4a48); }
        .cli-line.succ { color: var(--pos, #567a5e); }
        .cli-line.echo { color: var(--text, #f0f0e8); opacity: 0.65; }
        .cli-input-wrapper {
            display: flex; align-items: center; gap: 14px;
            font-size: clamp(10px, 0.72rem, 13px);
            border-top: 1px solid var(--line, #222);
            padding-top: clamp(10px, 1rem, 16px);
        }
        .cli-prompt {
            color: var(--accent, #c8a05a);
            letter-spacing: 0.06em;
            white-space: nowrap;
            flex-shrink: 0;
        }
        #cli-input {
            flex: 1; background: transparent; border: none;
            color: var(--text, #f0f0e8);
            font-family: 'DM Mono', monospace;
            font-size: clamp(10px, 0.72rem, 13px);
            outline: none;
            letter-spacing: 0.06em;
            caret-color: var(--accent, #c8a05a);
        }
    `;
    document.head.appendChild(style);

    /* ── Inject HTML ── */
    const terminal = document.createElement('div');
    terminal.id = 'cli-terminal';
    terminal.innerHTML = `
        <div class="cli-header">
            <span class="cli-header-left">ARTTOUS OS v75.4 — Root Access</span>
            <span class="cli-header-right">[Esc] or [\`] to close terminal</span>
        </div>
        <div class="cli-output" id="cli-output">
            <div class="cli-line sys">Arttous RTOS v75.4  ·  Core 0/1 Active</div>
            <div class="cli-line">Type <span style="color:var(--accent)">help</span> to see available commands.</div>
        </div>
        <div class="cli-input-wrapper">
            <span class="cli-prompt">operator@mesh:~$</span>
            <input type="text" id="cli-input" autocomplete="off" spellcheck="false">
        </div>
    `;
    document.body.appendChild(terminal);

    /* ── State ── */
    const input  = document.getElementById('cli-input');
    const output = document.getElementById('cli-output');
    let isOpen = false;
    const history = [];
    let histIdx = -1;

    /* ── Toggle ── */
    document.addEventListener('keydown', (e) => {
        if (e.key === '`' || e.key === '~' || e.key === 'ё' || e.key === 'Ё') {
            e.preventDefault();
            toggle();
        }
        if (e.key === 'Escape' && isOpen) toggle();
    });

    function toggle() {
        isOpen = !isOpen;
        if (isOpen) {
            terminal.style.display = 'flex';
            if (typeof gsap !== 'undefined') {
                gsap.fromTo(terminal, { opacity: 0, y: -8 }, { opacity: 1, y: 0, duration: 0.28, ease: 'power2.out' });
            } else {
                terminal.style.opacity = 1;
            }
            setTimeout(() => input.focus(), 80);
        } else {
            if (typeof gsap !== 'undefined') {
                gsap.to(terminal, { opacity: 0, y: -8, duration: 0.22, ease: 'power2.in', onComplete: () => { terminal.style.display = 'none'; } });
            } else {
                terminal.style.opacity = 0;
                terminal.style.display = 'none';
            }
        }
    }

    /* ── Input ── */
    input.addEventListener('keydown', function(e) {
        if (e.key === 'Enter') {
            const cmd = this.value.trim();
            if (cmd) {
                history.unshift(cmd);
                histIdx = -1;
                printLine(`operator@mesh:~$ ${cmd}`, 'echo');
                processCommand(cmd.toLowerCase());
            }
            this.value = '';
        }
        if (e.key === 'ArrowUp') {
            if (histIdx < history.length - 1) this.value = history[++histIdx] || '';
        }
        if (e.key === 'ArrowDown') {
            this.value = histIdx > 0 ? history[--histIdx] : (histIdx = -1, '');
        }
    });

    terminal.addEventListener('click', () => input.focus());

    /* ── Print ── */
    function printLine(text, cls = '') {
        const div = document.createElement('div');
        div.className = 'cli-line ' + cls;
        div.innerHTML = text;
        output.appendChild(div);
        output.scrollTop = output.scrollHeight;
    }

    /* ── Commands ── */
    function processCommand(cmd) {
        input.disabled = true;

        const done = () => { input.disabled = false; input.focus(); };

        switch (cmd) {
            case 'help':
                printLine('Available commands:', 'sys');
                printLine('  help          — this message');
                printLine('  whoami        — current operator identity');
                printLine('  status        — system health summary');
                printLine('  ping fleet    — ICMP sweep of active mesh nodes');
                printLine('  clear         — clear terminal output');
                printLine('  theme [name]  — switch theme (gold/sapphire/emerald/rouge/violet/platinum/obsidian/frost)');
                printLine('  reboot        — restart RTOS kernel');
                done(); break;

            case 'whoami':
                printLine(localStorage.getItem('userNick') || 'GUEST_USER', 'succ');
                done(); break;

            case 'status':
                printLine('System Health Report', 'sys');
                printLine('  UNIT_ALPHA  —  Online   14.2ms');
                printLine('  UNIT_BETA   —  Online   18.5ms');
                printLine('  UNIT_GAMMA  —  Offline  ——');
                printLine('  Control Link — Encrypted / Secure', 'succ');
                done(); break;

            case 'clear':
                output.innerHTML = '';
                done(); break;

            case 'ping fleet':
                printLine('Initiating LoRa Mesh ICMP sequence...', 'sys');
                setTimeout(() => printLine('PING 10.0.0.11 (UNIT_ALPHA): 56 bytes'), 400);
                setTimeout(() => printLine('64 bytes from 10.0.0.11: icmp_seq=1 ttl=64 time=14.2ms', 'succ'), 1000);
                setTimeout(() => printLine('PING 10.0.0.14 (UNIT_BETA): 56 bytes'), 1600);
                setTimeout(() => printLine('64 bytes from 10.0.0.14: icmp_seq=1 ttl=64 time=18.5ms', 'succ'), 2300);
                setTimeout(() => printLine('PING 10.0.0.22 (UNIT_GAMMA): 56 bytes'), 2900);
                setTimeout(() => printLine('Request timeout for icmp_seq=1', 'err'), 4200);
                setTimeout(() => {
                    printLine('--- Fleet Ping Statistics ---', 'sys');
                    printLine('3 nodes transmitted, 2 received, 33% packet loss.');
                    done();
                }, 4600);
                break;

            case 'reboot':
            case 'sudo reboot':
                printLine('Initiating kernel restart...', 'err');
                printLine('Killing Core 0 [TaskNetwork]', 'err');
                printLine('Killing Core 1 [TaskControl]', 'err');
                if (typeof gsap !== 'undefined') {
                    gsap.timeline()
                        .to('body', { skewX: 4, x: 8, duration: 0.08, yoyo: true, repeat: 5 })
                        .to('body', { filter: 'invert(1) hue-rotate(180deg)', duration: 0.1 })
                        .to('body', { opacity: 0, duration: 0.5, onComplete: () => window.location.reload() });
                } else {
                    setTimeout(() => window.location.reload(), 900);
                }
                break;

            case 'sudo':
                printLine('Permission denied. This incident has been logged.', 'err');
                done(); break;

            case 'make me a sandwich':
                printLine('This unit is a 16-DOF robotic platform, not a kitchen appliance.', 'sys');
                done(); break;

            default:
                if (cmd.startsWith('theme ')) {
                    const t = cmd.split(' ')[1];
                    const valid = ['gold','platinum','sapphire','emerald','rouge','violet','obsidian','frost'];
                    if (valid.includes(t)) {
                        if (typeof applyTheme === 'function') applyTheme(t);
                        printLine(`Theme set to: ${t}`, 'succ');
                    } else {
                        printLine(`Unknown theme: ${t}. Valid: ${valid.join(', ')}`, 'err');
                    }
                    done(); break;
                }
                printLine(`Command not found: ${cmd}`, 'err');
                printLine("Type 'help' for available commands.");
                done();
        }
    }
});