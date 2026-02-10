
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



// /bono/js/main.js
window.addEventListener('DOMContentLoaded', () => {
  const stepInfo = {
    1: '🚀 Deploy the robot rapidly on-site, ready for immediate operation.',
    2: '🎮 Control with a PS4 DualShock controller, ultra-low latency.',
    3: '🗺️ Record and map each route autonomously for detailed analysis.',
    4: '🔄 If link is lost, the robot returns to base automatically.',
    5: '🆘 Gather data & assist in search-and-rescue missions safely.'
  };

  // Try to grab the modal – if it doesn't exist, skip all modal logic
  const modal = document.getElementById('step-modal');
  if (!modal) {
    console.warn('No #step-modal in DOM – skipping step popups');
    return;
  }
  const modalText = document.getElementById('modal-step-text');
  const closeBtn  = modal.querySelector('.modal-close');
  if (!modalText || !closeBtn) {
    console.error('Modal structure incomplete (missing .modal-close or #modal-step-text)');
    return;
  }

  // Bind clicks on each step
  document.querySelectorAll('.step').forEach(stepEl => {
    stepEl.addEventListener('click', () => {
      const id = stepEl.dataset.step;
      modalText.innerHTML = `<p>${stepInfo[id] || 'No information available.'}</p>`;
      modal.classList.add('active');
    });
  });

  // Close handlers
  closeBtn.addEventListener('click', () => modal.classList.remove('active'));
  modal.addEventListener('click', e => {
    if (e.target === modal) modal.classList.remove('active');
  });
});


