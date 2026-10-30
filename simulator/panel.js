// This file is injected only by the simulator; it is not shipped to the ESP8266.
(() => {
  const byId = id => document.getElementById(id);
  const controls = [...document.querySelectorAll('[data-sim-advance],#sim-real,#sim-reboot,#sim-time-form button')];
  const field = byId('sim-date');
  const local = new Date(Date.now() - new Date().getTimezoneOffset() * 60000);
  field.value = local.toISOString().slice(0, 16);
  let pending = false;
  function paint() {
    if (!state) return;
    byId('deviceMode').textContent = 'Mô phỏng trên máy tính';
    byId('sim-status').textContent = !state.timeValid ?
      'Đã mất giờ như khi ESP khởi động lại. Lịch vẫn còn; bấm “Lấy giờ thật” để tiếp tục.' :
      state.simulator?.testClock ?
      'Đang dùng giờ thử nghiệm. Tự đồng bộ giờ bị tạm dừng để không ghi đè giờ thử; bấm “Lấy giờ thật” để trở về bình thường.' :
      'Đang dùng giờ thật. Lịch được lưu trên máy tính, giữ lại khi tải lại trang hoặc tắt/mở máy chủ.';
  }
  async function run(route, data) {
    if (pending) return;
    pending = true;
    controls.forEach(button => { button.disabled = true; });
    try {
      // Let an in-flight firmware UI request finish before changing test time.
      while (busy || requesting) await new Promise(resolve => setTimeout(resolve, 30));
      await mutate(route, data);
      paint();
    } finally { pending = false; controls.forEach(button => { button.disabled = false; }); }
  }
  document.querySelectorAll('[data-sim-advance]').forEach(button => {
    button.onclick = () => run('/sim/advance', { seconds: Number(button.dataset.simAdvance) });
  });
  const realTime = () => run('/sim/realtime', { epoch: Math.floor(Date.now() / 1000), utcOffsetMinutes: -new Date().getTimezoneOffset() });
  byId('sim-real').onclick = realTime;
  // Explicit sync should work in test mode too; background sync remains paused.
  byId('sync').onclick = realTime;
  byId('sim-reboot').onclick = () => run('/sim/reboot', {});
  byId('sim-time-form').onsubmit = event => {
    event.preventDefault();
    const chosen = new Date(field.value);
    if (!Number.isFinite(chosen.getTime())) return;
    run('/sim/time', { epoch: Math.floor(chosen.getTime() / 1000), utcOffsetMinutes: -chosen.getTimezoneOffset() });
  };
  setInterval(paint, 500);
  paint();
})();
