'use strict';
const DATABASE_URL = 'https://iot-thx-default-rtdb.asia-southeast1.firebasedatabase.app';
const DEVICE_PATH = '/devices/esp8266_4relay';
const $ = id => document.getElementById(id);
let authToken = '', state = null, connected = false, busy = false;
const cards = [1, 2, 3, 4].map(id => {
  const card = document.createElement('article'); card.className = 'relay';
  card.innerHTML = `<span class="channel">KÊNH 0${id}</span><h2></h2><div class="value">Chưa có dữ liệu</div><button class="switch" type="button" role="switch" aria-checked="false" disabled></button><p class="reported">Chờ kết nối Firebase</p>`;
  card.querySelector('button').onclick = () => writeRelay(id);
  $('relays').append(card); return card;
});
async function request(path, method = 'GET', value) {
  const controller = new AbortController(), timer = setTimeout(() => controller.abort(), 8000);
  try {
    const url = DATABASE_URL + DEVICE_PATH + path + '.json' + (authToken ? '?auth=' + encodeURIComponent(authToken) : '');
    const response = await fetch(url, { method, cache: 'no-store', signal: controller.signal,
      headers: value === undefined ? {} : { 'Content-Type': 'application/json' },
      body: value === undefined ? undefined : JSON.stringify(value) });
    if (!response.ok) throw Error(response.status === 401 || response.status === 403 ? 'Firebase từ chối truy cập. Kiểm tra Rules hoặc ID token.' : 'Firebase trả lỗi ' + response.status);
    return await response.json();
  } finally { clearTimeout(timer); }
}
function paint() {
  const age = typeof state?.lastSeen === 'number' ? Date.now() - state.lastSeen : Infinity;
  const deviceOnline = connected && age >= -60000 && age < 20000;
  $('dot').classList.toggle('live', connected);
  $('status').textContent = connected ? 'Đã kết nối Firebase' : 'Chưa kết nối Firebase';
  $('deviceStatus').textContent = deviceOnline ? 'ESP đang online · Đã nhận phản hồi gần đây' : 'ESP chưa phản hồi gần đây · Lệnh sẽ chờ thiết bị';
  cards.forEach((card, i) => {
    const id = i + 1, value = state?.relays?.[id], valid = value === 0 || value === 1;
    const name = typeof state?.names?.[id] === 'string' ? state.names[id] : 'Kênh ' + id;
    card.classList.toggle('on', value === 1);
    card.querySelector('h2').textContent = name;
    card.querySelector('.value').textContent = !valid ? 'Chưa có dữ liệu 0/1' : value ? 'Đang bật' : 'Đang tắt';
    const button = card.querySelector('button');
    button.disabled = busy || !connected;
    button.setAttribute('aria-label', (value === 1 ? 'Tắt ' : 'Bật ') + name);
    button.setAttribute('aria-checked', String(value === 1));
    card.querySelector('.reported').textContent = !connected ? 'Chưa xác minh dữ liệu' : deviceOnline && valid && state?.reported?.[id] === value ? 'ESP đã áp dụng' : 'Chờ ESP xác nhận';
  });
  $('retry').disabled = busy;
}
function showError(error) {
  $('error').textContent = error.name === 'AbortError' ? 'Kết nối quá thời gian. Hãy thử lại.' : error.message;
  $('error').hidden = false;
}
async function readState() {
  const next = await request('');
  if (next !== null && (typeof next !== 'object' || Array.isArray(next))) throw Error('Dữ liệu thiết bị trên Firebase không hợp lệ.');
  state = next; connected = true; $('error').hidden = true;
}
async function refresh() {
  if (busy) return;
  busy = true; paint();
  try { await readState(); } catch (error) { connected = false; showError(error); }
  finally { busy = false; paint(); }
}
async function writeRelay(id) {
  if (busy || !connected) return;
  const value = state?.relays?.[id] === 1 ? 0 : 1;
  busy = true; paint();
  try {
    // Write only this channel, so other channels are never overwritten.
    await request('/relays/' + id, 'PUT', value);
    await readState();
  } catch (error) { connected = false; showError(error); }
  finally { busy = false; paint(); }
}
$('retry').onclick = refresh;
$('authForm').onsubmit = async event => {
  event.preventDefault(); if (busy) return;
  authToken = $('token').value.trim(); $('token').value = ''; state = null; connected = false;
  await refresh();
};
document.addEventListener('visibilitychange', () => { if (!document.hidden) refresh(); });
setInterval(() => { if (!document.hidden) refresh(); }, 2000);
paint(); refresh();
