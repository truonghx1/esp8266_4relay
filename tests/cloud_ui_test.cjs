const assert = require('node:assert/strict');
const fs = require('node:fs');
const { JSDOM } = require('../.tools/ui-test/node_modules/jsdom');
let state = { relays: { 1: 0, 2: 1, 3: 0, 4: 1 }, reported: { 1: 0, 2: 1, 3: 0, 4: 1 }, lastSeen: Date.now(), names: { 1: '<b>Đèn</b>' } };
let fail = false;
const calls = [];
const dom = new JSDOM(fs.readFileSync('web/index.html', 'utf8').replace('<script defer src="app.js"></script>', ''), {
  runScripts: 'dangerously', url: 'https://example.test/', beforeParse(window) {
    window.fetch = async (url, options) => {
      calls.push({ url, ...options });
      if (fail) return { ok: false, status: 401 };
      if (options.method === 'PUT') state.relays[Number(url.match(/relays\/(\d)/)[1])] = JSON.parse(options.body);
      return { ok: true, json: async () => structuredClone(state) };
    };
  }
});
dom.window.eval(fs.readFileSync('web/app.js', 'utf8'));
const settle = () => new Promise(r => setTimeout(r, 20));
const document = dom.window.document;
(async () => {
  await settle();
  assert.equal(document.querySelectorAll('.relay').length, 4);
  assert.equal(document.querySelector('h2').textContent, '<b>Đèn</b>');
  assert.equal(document.querySelector('h2 b'), null);
  assert.equal(document.querySelector('.reported').textContent, 'ESP đã áp dụng');
  document.querySelector('.switch').click(); await settle();
  const write = calls.find(c => c.method === 'PUT');
  assert.ok(write.url.endsWith('/devices/esp8266_4relay/relays/1.json'));
  assert.equal(write.body, '1'); assert.equal(state.relays[2], 1);
  assert.equal(document.querySelector('.reported').textContent, 'Chờ ESP xác nhận');
  document.querySelector('.switch').click(); await settle();
  assert.equal(calls.filter(c => c.method === 'PUT').at(-1).body, '0');
  state.lastSeen = Date.now() - 60000;
  document.getElementById('retry').click(); await settle();
  assert.match(document.getElementById('deviceStatus').textContent, /chưa phản hồi/);
  fail = true;
  document.querySelector('.switch').click(); await settle();
  assert.equal(document.getElementById('error').hidden, false);
  assert.match(document.getElementById('error').textContent, /từ chối/);
  assert.equal(document.querySelector('.switch').disabled, true);
  fail = false; state = null;
  document.getElementById('retry').click(); await settle();
  assert.match(document.querySelector('.value').textContent, /Chưa có dữ liệu/);
  console.log('PASS: Firebase channel 0/1 writes, readback, pending acknowledgement, offline device, permission errors, empty database, escaped names');
})().catch(error => { console.error(error); process.exitCode = 1; }).finally(() => dom.window.close());
