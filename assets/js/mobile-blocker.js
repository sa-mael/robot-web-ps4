document.addEventListener("DOMContentLoaded", () => {
    // Проверяем ширину экрана. Если больше 1024px, скрипт просто останавливается и ничего не делает.
    if (window.innerWidth > 1024) return;

    // Если экран маленький, создаем стили для блокировщика
    const style = document.createElement('style');
    style.innerHTML = `
        #resolution-blocker {
            position: fixed; top: 0; left: 0; width: 100vw; height: 100vh;
            background: #050505; z-index: 999999;
            display: flex; flex-direction: column; align-items: center; justify-content: center;
            padding: 20px; text-align: center; color: #e0e0e0; font-family: 'Inter', sans-serif;
        }
        #resolution-blocker::before {
            content: " "; display: block; position: absolute; top: 0; left: 0; bottom: 0; right: 0;
            background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.25) 50%), linear-gradient(90deg, rgba(255, 0, 0, 0.06), rgba(0, 255, 0, 0.02), rgba(0, 0, 255, 0.06));
            background-size: 100% 2px, 3px 100%; pointer-events: none; z-index: 0;
        }
        .blocker-content {
            position: relative; z-index: 1; max-width: 400px; background: rgba(14, 14, 14, 0.9);
            border: 1px solid #e74c3c; padding: 40px 30px; box-shadow: 0 0 50px rgba(231, 76, 60, 0.2);
            backdrop-filter: blur(10px); animation: blockerPulse 2s infinite alternate;
        }
        .blocker-icon { font-size: 3rem; margin-bottom: 15px; }
        .blocker-title { font-family: 'Orbitron', sans-serif; color: #e74c3c; font-size: 1.5rem; letter-spacing: 2px; margin-bottom: 15px; }
        .blocker-desc { color: #aaa; font-size: 0.9rem; line-height: 1.6; margin-bottom: 30px; }
        .blocker-sys-msg { font-family: 'JetBrains Mono', monospace; color: #fff; font-size: 0.8rem; background: rgba(231, 76, 60, 0.1); padding: 10px; border-left: 2px solid #e74c3c; margin-bottom: 20px; }
        @keyframes blockerPulse { from { box-shadow: 0 0 20px rgba(231, 76, 60, 0.1); } to { box-shadow: 0 0 40px rgba(231, 76, 60, 0.3); } }
    `;
    document.head.appendChild(style);

    // Создаем сам HTML блокировщика
    const blocker = document.createElement('div');
    blocker.id = 'resolution-blocker';
    blocker.innerHTML = `
        <div class="blocker-content">
            <div class="blocker-icon">⚠️</div>
            <h2 class="blocker-title">INSUFFICIENT VIEWPORT</h2>
            <div style="height: 1px; width: 100%; background: linear-gradient(90deg, transparent, #e74c3c, transparent); margin-bottom: 20px;"></div>
            <p class="blocker-desc">
                The Arttous Mission Control interface requires a high-resolution terminal. Mobile and tablet displays do not support the active WebGL rendering and dense telemetry matrices required for this uplink.
            </p>
            <div class="blocker-sys-msg">/// PLEASE INITIATE CONNECTION VIA DESKTOP OR LAPTOP ///</div>
            <div style="font-family: 'JetBrains Mono'; color: #555; font-size: 0.65rem;">SYS_ERR: 0xVIEWPORT_TOO_SMALL</div>
        </div>
    `;
    
    // Вставляем в body и блокируем скролл
    document.body.appendChild(blocker);
    document.body.style.overflow = 'hidden';
});