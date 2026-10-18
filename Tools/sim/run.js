'use strict';
// usage: node run-stage2.js <scenarios.js> <outDir> <buildBase> [onlyScenario]
const fs = require('fs');
const path = require('path');
const { simulate } = require('./harness');

const STATE_NAMES = ['IDLE', 'MANUAL_FORWARD', 'MANUAL_BACKWARD', 'MANUAL_LEFT', 'MANUAL_RIGHT', 'AUTO_FORWARD', 'AUTO_BACKWARD',
  'DONUT', 'DRIFT_LEFT', 'DRIFT_RIGHT', 'DANCE', 'DEMO', 'CHAOS', 'SEARCH', 'TURN180', 'BRAKING'];
const [scFile, outDir, base, only] = process.argv.slice(2);
const build = { label: 'v8', bin: base + '.bin', nm: base + '.nm.txt', stateNames: STATE_NAMES };
const scenarios = require(path.resolve(scFile)).filter((s) => !only || s.name === only);
fs.mkdirSync(outDir, { recursive: true });

let failed = 0;
for (const sc of scenarios) {
  const r = simulate(build, sc, { snapDir: outDir });
  fs.writeFileSync(path.join(outDir, sc.name + '.json'), JSON.stringify({ log: r.log, i2cWindows: r.i2cWindows }, null, 1));
  if (sc.room) fs.writeFileSync(path.join(outDir, sc.name + '.path.json'), JSON.stringify(sc.room.state.path));
  // compact timeline
  const lines = [];
  let lastBuzz = null;
  for (const e of r.log) {
    if (e.kind === 'buzzer') { if (e.v === 0 && lastBuzz === 0) continue; lastBuzz = e.v; }
    lines.push(`${(e.t / 1000).toFixed(3).padStart(8)}s  ${e.kind.padEnd(8)} ${e.v}`);
  }
  fs.writeFileSync(path.join(outDir, sc.name + '.timeline.txt'), lines.join('\n'));
  const res = sc.check(r.log, r);
  const draws = r.i2cWindows.filter((w) => w[0] > 600).map((w) => w[1] - w[0]).sort((a, b) => a - b);
  const longest = draws.length ? draws[draws.length - 1].toFixed(0) : 0;
  const median = draws.length ? draws[Math.floor(draws.length / 2)].toFixed(0) : 0;
  console.log(`\n=== ${sc.name} (${sc.duration}s simulated in ${(r.wallMs / 1000).toFixed(1)}s, I2C bytes ${r.i2cBytes}, OLED draws after boot: ${draws.length}, median ${median} ms, longest ${longest} ms)`);
  for (const c of res) {
    if (!c.pass) failed++;
    console.log(`  ${c.pass ? 'PASS' : 'FAIL'}  ${c.name}${c.detail ? '  [' + c.detail + ']' : ''}`);
  }
}
console.log(`\n${failed === 0 ? 'ALL CHECKS PASSED' : failed + ' CHECK(S) FAILED'}`);
