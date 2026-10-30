// Optional end-to-end DOM check against the real local HTTP simulator.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { JSDOM } = require('../.tools/ui-test/node_modules/jsdom');
const { createServer } = require('../simulator/server.cjs');
const parent = path.resolve(__dirname, '../.simulator');
fs.mkdirSync(parent, { recursive: true });
const folder = fs.mkdtempSync(path.join(parent, 'ui-test-'));
const { server } = createServer({ file: path.join(folder, 'config.json') });
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
async function until(condition) {
  for (let i = 0; i < 100; i++) { if (condition()) return; await sleep(50); }
  throw Error('Timed out waiting for simulator UI');
}
let dom;
(async () => {
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const base = `http://127.0.0.1:${server.address().port}`;
  dom = await JSDOM.fromURL(base, {
    runScripts: 'dangerously', resources: 'usable', pretendToBeVisual: true,
    beforeParse(window) {
      window.fetch = (route, options) => fetch(new URL(route, base), options);
      window.confirm = () => true;
      window.HTMLDialogElement.prototype.showModal = function () { this.open = true; };
      window.HTMLDialogElement.prototype.close = function () { this.open = false; };
    }
  });
  const $ = id => dom.window.document.getElementById(id);
  await until(() => $('sim-status').textContent.includes('Đang dùng giờ thật'));
  await until(() => !$('add').disabled);
  dom.window.document.querySelector('#relays .switch').click();
  await until(() => dom.window.document.querySelector('#relays .switch').getAttribute('aria-checked') === 'true');
  $('add').click();
  $('start').value = '22:00'; $('end').value = '02:00';
  $('scheduleForm').dispatchEvent(new dom.window.Event('submit', { bubbles: true, cancelable: true }));
  await until(() => !$('editor').open && $('count').textContent === '1/10');
  $('sim-date').value = '2026-10-25T21:59';
  $('sim-time-form').dispatchEvent(new dom.window.Event('submit', { bubbles: true, cancelable: true }));
  await until(() => $('sim-status').textContent.includes('Đang dùng giờ thử nghiệm'));
  dom.window.document.querySelector('[data-sim-advance="60"]').click();
  await until(() => dom.window.document.querySelector('#relays .switch').getAttribute('aria-checked') === 'true');
  $('sim-reboot').click();
  await until(() => $('sim-status').textContent.includes('Đã mất giờ'));
  assert.equal($('clock').textContent, 'Chưa có giờ');
  assert.equal($('count').textContent, '1/10');
  $('sim-real').click();
  await until(() => $('sim-status').textContent.includes('Đang dùng giờ thật'));
  console.log('PASS: live HTTP + UI, relay, save schedule, set time, advance, reboot, resync');
})().catch(error => { console.error(error); process.exitCode = 1; }).finally(async () => {
  dom?.window.close();
  server.closeAllConnections();
  await new Promise(resolve => server.close(resolve));
  fs.rmSync(folder, { recursive: true, force: true });
});
