
// Generate blinking circles in header
window.addEventListener('DOMContentLoaded', () => {
  const container = document.querySelector('.circle-blink-container');
  const count = 8;
  for (let i = 0; i < count; i++) {
    const circle = document.createElement('div');
    circle.classList.add('blink-circle');
    // random position
    circle.style.left = Math.random() * 100 + '%';
    circle.style.top = Math.random() * 100 + '%';
    // random animation timing
    circle.style.animationDuration = (2 + Math.random() * 3) + 's';
    circle.style.animationDelay = Math.random() * 5 + 's';
    container.appendChild(circle);
  }
});



window.addEventListener('DOMContentLoaded', () => {
  const stepInfo = {
    1: '🚀 Deploy the robot rapidly on-site, ready for immediate operation.',
    2: '🎮 Control with a PS4 DualShock controller, ultra-low latency.',
    3: '🗺️ Record and map each route autonomously for detailed analysis.',
    4: '🔄 If link is lost, the robot returns to base automatically.',
    5: '🆘 Gather data & assist in search-and-rescue missions safely.'
  };

  const modal = document.getElementById('step-modal');
  const modalText = document.getElementById('modal-step-text');
  const closeBtn = modal.querySelector('.modal-close');

  document.querySelectorAll('.step').forEach(stepEl => {
    stepEl.addEventListener('click', () => {
      const id = stepEl.dataset.step;
      // Сначала очищаем предыдущий текст
      modalText.innerHTML = '';

      // Берём новое описание (если нет, пишем «No info»)
      const info = stepInfo[id] || 'No information available for this step.';
      // Можно вставлять HTML, поэтому используем innerHTML
      modalText.innerHTML = `<p>${info}</p>`;

      // Показываем модалку
      modal.classList.add('active');
    });
  });

  // Закрываем
  closeBtn.addEventListener('click', () => modal.classList.remove('active'));
  modal.addEventListener('click', e => {
    if (e.target === modal) modal.classList.remove('active');
  });
});

/* 2) Style it in /bono/css/styles.css */
.scroll-btn {
  margin: 40px auto 0;
  display: inline-block;
  padding: 12px 24px;
  background: rgba(255,255,255,0.1);
  color: #fff;
  border: 2px solid #fff;
  border-radius: 4px;
  font-size: 16px;
  cursor: pointer;
  transition: background 0.3s, transform 0.2s;
}

.scroll-btn:hover {
  background: rgba(255,255,255,0.2);
  transform: translateY(-2px);
}
