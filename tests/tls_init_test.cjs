// Run after compiling firmware. Inspect generated instructions, not just source:
// ESP8266's static initialization guard masks interrupts during construction.
const assert = require('node:assert/strict');
const { execFileSync } = require('node:child_process');
const objdump = process.env.OBJDUMP || '.tools/arduino-data/packages/esp8266/tools/xtensa-lx106-elf-gcc/3.1.0-gcc10.3-e5f9fec/bin/xtensa-lx106-elf-objdump.exe';
const elf = process.argv[2] || 'build/firmware/esp8266_4relay.ino.elf';
const output = execFileSync(objdump, ['-d', '-C', elf], { encoding: 'utf8', maxBuffer: 32 * 1024 * 1024 });
const functions = [...output.matchAll(/^([0-9a-f]+) <(.+)>:\r?\n([\s\S]*?)(?=^[0-9a-f]+ <|$(?![\s\S]))/gm)];
for (const name of ['firebaseRequest(', 'prepareFirebaseTls()']) {
  const entry = functions.find(f => f[2].startsWith(name));
  assert.ok(entry, 'Missing compiled function: ' + name);
  assert.ok(!entry[3].includes('__cxa_guard_'), 'TLS initialization masks interrupts: ' + name);
}
const prepare = functions.find(f => f[2] === 'prepareFirebaseTls()');
assert.match(prepare[3], /BearSSL::X509List::append/, 'Certificate parsing must remain in explicit preparation');
console.log('PASS: compiled TLS preparation/request contain no interrupt-masking static initialization guards');
