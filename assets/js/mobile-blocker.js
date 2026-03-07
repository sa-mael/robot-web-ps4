/* ══════════════════════════════════════════════════════
   ARTTOUS — mobile-blocker.js
   Blocks viewport < 1024px. Luxury aesthetic.
══════════════════════════════════════════════════════ */

document.addEventListener('DOMContentLoaded', () => {
    if (window.innerWidth > 1024) return;

    const style = document.createElement('style');
    style.innerHTML = `
        @import url('https://fonts.googleapis.com/css2?family=DM+Mono:wght@300;400&family=Cormorant+Garamond:wght@300;400&display=swap');

        #res-blocker {
            position: fixed; inset: 0;
            background: #060606;
            z-index: 999999;
            display: flex; align-items: center; justify-content: center;
            padding: 24px;
            font-family: 'DM Mono', monospace;
        }

        /* Subtle scanlines */
        #res-blocker::before {
            content: '';
            position: absolute; inset: 0;
            background: repeating-linear-gradient(
                0deg,
                transparent,
                transparent 3px,
                rgba(0,0,0,0.12) 3px,
                rgba(0,0,0,0.12) 4px
            );
            pointer-events: none;
            z-index: 0;
        }

        .blocker-card {
            position: relative; z-index: 1;
            max-width: 420px; width: 100%;
            background: #0e0e0e;
            border: 1px solid #c8a05a;
            padding: 40px 32px;
            text-align: center;
        }

        .blocker-rule {
            width: 100%; height: 1px;
            background: linear-gradient(90deg, transparent, #c8a05a, transparent);
            margin: 20px 0;
        }

        .blocker-wordmark {
            font-family: 'Cormorant Garamond', serif;
            font-size: 18px; letter-spacing: 0.35em;
            color: #f0f0e8; text-transform: uppercase;
            margin-bottom: 4px;
        }
        .blocker-wordmark em { font-style: normal; color: #c8a05a; }

        .blocker-title {
            font-size: 10px; letter-spacing: 0.22em;
            color: #8a4a48; text-transform: uppercase;
            margin-bottom: 24px;
        }

        .blocker-body {
            font-size: 11px; letter-spacing: 0.06em;
            color: #7a7a72; line-height: 1.8;
            margin-bottom: 24px;
        }

        .blocker-notice {
            font-size: 9px; letter-spacing: 0.16em;
            color: #c8a05a; text-transform: uppercase;
            border: 1px solid rgba(200,160,90,0.25);
            padding: 10px 16px;
            background: rgba(200,160,90,0.05);
            margin-bottom: 20px;
        }

        .blocker-code {
            font-size: 8px; letter-spacing: 0.1em;
            color: #333330;
        }
    `;
    document.head.appendChild(style);

    const blocker = document.createElement('div');
    blocker.id = 'res-blocker';
    blocker.innerHTML = `
        <div class="blocker-card">
            <div class="blocker-wordmark">ART<em>TOUS</em></div>
            <div class="blocker-rule"></div>
            <div class="blocker-title">Insufficient Viewport</div>
            <div class="blocker-body">
                The Arttous Command Interface requires a high-resolution terminal environment.<br><br>
                Mobile and tablet displays do not support the active WebGL rendering pipeline and dense telemetry matrices required for this uplink.
            </div>
            <div class="blocker-notice">
                Initiate connection via desktop or laptop
            </div>
            <div class="blocker-code">SYS_ERR: 0xVIEWPORT_TOO_SMALL</div>
        </div>
    `;

    document.body.appendChild(blocker);
    document.body.style.overflow = 'hidden';
});