const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { Simulator } = require('../simulator/model.cjs');
const { createServer } = require('../simulator/server.cjs');
const parent = path.resolve(__dirname, '../.simulator');
fs.mkdirSync(parent, { recursive: true });
function fixture(t) {
  const dir = fs.mkdtempSync(path.join(parent, 'test-'));
  t.after(() => fs.rmSync(dir, { recursive: true, force: true }));
  return path.join(dir, 'config.json');
}
const epoch = value => Date.parse(value) / 1000;
const add = (model, fields = {}) => model.command('/api/schedule', {
  id: -1, enabled: true, relays: 3, days: 1, start: 480, end: 600, ...fields
});

test('device names persist, migrate old config, validate atomically and preserve relay state', t => {
  const file = fixture(t);
  fs.writeFileSync(file, JSON.stringify({ version: 1, utcOffsetMinutes: 420, schedules: [] }));
  const model = new Simulator(file, () => 0);
  assert.deepEqual(model.state().relays.map(r => r.name), ['Kênh 1', 'Kênh 2', 'Kênh 3', 'Kênh 4']);
  add(model);
  model.command('/api/relay', { relay: 1, on: true });
  const names = ['Đèn phòng khách', 'Quạt trần', 'Ổ cắm', '<b>Đèn</b>'];
  model.command('/api/names', { names });
  assert.deepEqual(model.state().relays.map(r => r.name), names);
  assert.equal(model.state().relays[0].on, true);
  assert.equal(model.state().relays[0].manual, true);
  assert.equal(model.schedules.length, 1);
  assert.equal(model.schedules[0].relays, 3);
  const reloaded = new Simulator(file, () => 0);
  assert.deepEqual(reloaded.state().relays.map(r => r.name), names);
  reloaded.command('/sim/reboot', {});
  assert.deepEqual(reloaded.state().relays.map(r => r.name), names);
  for (const invalid of [[], ['One'], ['', ...names.slice(1)], ['a'.repeat(25), ...names.slice(1)], ['a\nb', ...names.slice(1)], ['a\0b', ...names.slice(1)], ['\uD800', ...names.slice(1)]]) {
    assert.throws(() => model.command('/api/names', { names: invalid }), error => error.status === 400);
    assert.deepEqual(model.state().relays.map(r => r.name), names);
  }
  const originalSave = model.save;
  model.save = () => { throw Error('Disk unavailable'); };
  assert.throws(() => model.command('/api/names', { names: ['New', ...names.slice(1)] }));
  assert.deepEqual(model.state().relays.map(r => r.name), names);
  model.save = originalSave;
});

test('firmware HTML contains no simulation controls, scripts or styles', () => {
  const source = fs.readFileSync(path.resolve(__dirname, '../firmware/esp8266_4relay/web_ui.h'), 'utf8');
  assert.doesNotMatch(source, /MÔ PHỎNG|Mô phỏng|sim-panel|\/sim\/|SIMULATOR_PANEL|state\.simulator/);
});

test('schedule boundaries, overlap, manual overrides, persistence and reboot', t => {
  const file = fixture(t);
  let now = 0;
  const model = new Simulator(file, () => now);
  model.command('/sim/time', { epoch: epoch('2026-10-26T07:59:00Z'), utcOffsetMinutes: 0 });
  add(model);
  assert.equal(model.state().relays[0].on, false);
  now += 60000;
  assert.equal(model.state().relays[0].on, true);
  model.command('/api/relay', { relay: 1, on: false });
  assert.equal(model.state().relays[0].on, false);
  model.command('/sim/advance', { seconds: 7200 });
  assert.equal(model.state().relays[0].manual, false);
  add(model, { start: 540, end: 660, relays: 6 });
  assert.deepEqual(model.state().relays.map(r => r.on), [false, true, true, false]);
  const loaded = new Simulator(file, () => now);
  assert.equal(loaded.schedules.length, 2);
  assert.equal(loaded.state().timeValid, false);
  assert.ok(loaded.state().relays.every(r => !r.on));
  loaded.command('/sim/realtime', { epoch: epoch('2026-10-26T10:30:00Z'), utcOffsetMinutes: 0 });
  assert.deepEqual(loaded.state().relays.map(r => r.on), [false, true, true, false]);
  loaded.command('/sim/reboot', {});
  assert.equal(loaded.state().timeValid, false);
  assert.equal(loaded.state().schedules.length, 2);
  loaded.command('/api/time', { epoch: epoch('2026-10-26T10:30:00Z'), utcOffsetMinutes: 0 });
  assert.equal(loaded.state().timeValid, false, 'background phone sync must not hide reboot simulation');
});

test('Sunday overnight, accelerated time crosses complete intervals, 10 slots and validation', t => {
  const model = new Simulator(fixture(t), () => 0);
  model.command('/sim/time', { epoch: epoch('2026-10-25T21:59:00Z'), utcOffsetMinutes: 0 });
  add(model, { days: 64, start: 1320, end: 120 });
  model.command('/sim/advance', { seconds: 60 });
  assert.equal(model.state().relays[0].on, true);
  model.command('/sim/advance', { seconds: 4 * 3600 });
  assert.equal(model.state().relays[0].on, false);
  model.command('/api/relay', { relay: 1, on: true });
  model.command('/sim/advance', { seconds: 7 * 86400 });
  assert.equal(model.state().relays[0].manual, false, 'time travel must visit intermediate boundaries');
  for (let i = 1; i < 10; i++) add(model);
  assert.throws(() => add(model), error => error.status === 409);
  model.command('/api/schedule/delete', { id: 4 });
  add(model);
  assert.equal(model.schedules.length, 10);
  assert.ok(model.schedules.some(s => s.id === 4));
  assert.throws(() => model.command('/api/schedule', { id: 0, enabled: true, relays: 1, days: 1, start: 60, end: 60 }), error => error.status === 400);
  assert.throws(() => model.command('/api/relay', { relay: 0, on: true }), error => error.status === 400);
});

test('real HTTP API serves firmware UI and rejects invalid requests', async t => {
  const { server } = createServer({ file: fixture(t) });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  t.after(() => { server.closeAllConnections(); return new Promise(resolve => server.close(resolve)); });
  const base = `http://127.0.0.1:${server.address().port}`;
  const page = await (await fetch(base)).text();
  assert.match(page, /MÔ PHỎNG/);
  assert.match(page, /id="scheduleForm"/);
  assert.match(page, /sim-panel.js/);
  const post = (route, data, headers = {}) => fetch(base + route, {
    method: 'POST', headers: { 'Content-Type': 'application/json', ...headers }, body: JSON.stringify(data)
  });
  let response = await post('/api/time', { epoch: epoch('2026-10-26T08:30:00Z'), utcOffsetMinutes: 0 });
  assert.equal(response.status, 200);
  assert.equal((await response.json()).timeValid, true);
  response = await post('/api/schedule', { id: -1, enabled: true, relays: 3, days: 1, start: 480, end: 600 });
  assert.deepEqual((await response.json()).relays.map(r => r.on), [true, true, false, false]);
  response = await post('/api/relay', { relay: 2, on: false });
  assert.equal((await response.json()).relays[1].on, false);
  response = await post('/api/auto', { relay: 2 });
  assert.equal((await response.json()).relays[1].on, true);
  response = await post('/api/names', { names: ['Đèn', 'Quạt', 'Ổ cắm', 'Đèn trần'] });
  assert.equal(response.status, 200);
  const renamed = await response.json();
  assert.equal(renamed.relays[0].name, 'Đèn');
  assert.equal(renamed.schedules.length, 1);
  assert.equal((await post('/api/relay', { relay: 1, on: true }, { 'Content-Type': 'text/plain' })).status, 415);
  assert.equal((await post('/api/relay', { relay: 1, on: true }, { Origin: 'http://different.invalid' })).status, 403);
  assert.equal((await post('/api/time', { padding: 'x'.repeat(1100) })).status, 413);
  assert.equal((await fetch(base + '/api/relay')).status, 404);
});
