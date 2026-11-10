'use strict';
// Checks the web page's brain (logic.js) against the Android app's (Java): the same layout for
// every screen size, the same wheel speeds, and exactly the same codes in long random drives.
// The Java side is written by App\tests\Golden.java (run App\tests\run-tests.ps1 first).
// Run: node Web\test-logic.js
const fs = require('fs');
const path = require('path');
const { DriveLogic, Layout, ControllerMap, CarState, Car } = require('./logic.js');

const file = path.join(__dirname, '..', 'Tools', 'build', 'golden.json');
let passed = 0, failed = 0;
const check = (name, ok, detail) => {
  if (ok) passed++; else failed++;
  console.log(`  ${ok ? 'PASS' : 'FAIL'}  ${name}${ok || !detail ? '' : '  [' + detail + ']'}`);
};
if (!fs.existsSync(file)) {
  console.log('  FAIL  Tools/build/golden.json is missing - run App\\tests\\run-tests.ps1 first');
  process.exit(1);
}
const g = JSON.parse(fs.readFileSync(file, 'utf8'));

// the layout
let bad = [];
for (const L of g.layouts) {
  const mine = Layout.screen(L.w, L.h, L.style);
  const a = JSON.stringify(Object.fromEntries(Object.entries(mine).map(([k, b]) => [k, [b.x, b.y, b.w, b.h]])));
  const b = JSON.stringify(L.boxes);
  const p = mine.pages;
  if (a !== b) bad.push(`${L.w}x${L.h} style ${L.style}: ${a} vs ${b}`);
  else if (JSON.stringify(Layout.grid(8, p.w, p.h)) !== JSON.stringify(L.grid8) || JSON.stringify(Layout.grid(6, p.w, p.h)) !== JSON.stringify(L.grid6))
    bad.push(`${L.w}x${L.h} grid`);
}
check(`the same layout as the app for ${g.layouts.length} screen sizes and styles`, bad.length === 0, bad.slice(0, 2).join(' | '));

// the wheel mixing
bad = [];
const o = [0, 0];
for (const [t, s, slow, l, r] of g.mix) {
  DriveLogic.mix(Math.fround(t / 10), Math.fround(s / 10), slow === 1, Math.fround(0.6), o);
  if (o[0] !== l || o[1] !== r) bad.push(`t ${t / 10} s ${s / 10} slow ${slow}: ${o} vs ${l},${r}`);
}
check(`the same wheel speeds as the app for ${g.mix.length} stick positions`, bad.length === 0, bad.slice(0, 3).join(' | '));

// long random drives
let seed = 0;
const rand = () => { seed = (Math.imul(seed, 1103515245) + 12345) | 0; return seed & 0x7fffffff; };
const jobs = ['4', 'I', 'T', 'Y', 'X', '3', 'k', 'u', 'Q', 'W', 'O'];
g.drives.forEach((want, run) => {
  seed = 1000 + run;
  const got = [];
  let now = 0;
  const d = new DriveLogic((c) => got.push(now, c));
  d.setSteering(run % 2 === 0);
  d.setOneStick(run >= 4);
  d.reset();
  for (let step = 0; step < 4000; step++) {
    now += 4 + rand() % 57;
    const r = rand() % 1000;
    if (r < 650) {
      const kind = rand() % 4;
      let t = kind === 0 ? 0 : Math.fround((rand() % 2001 - 1000) / 1000);
      let s = kind === 1 ? 0 : Math.fround((rand() % 2001 - 1000) / 1000);
      if (kind === 3) { t = 0; s = 0; }
      d.input(t, s, now);
    } else if (r < 900) d.tick(now);
    else if (r < 950) d.actionSent(jobs[rand() % jobs.length], now);
    else if (r < 965) d.stopAll(now);
    else if (r < 972) d.halt(now);
    else if (r < 985) d.setSlowTop(rand() % 2 === 0);
    else if (r < 992) d.carSpeedIs(rand() % 2 === 0);
    else d.setSteering(rand() % 2 === 0);
  }
  let first = -1;
  for (let i = 0; i < Math.max(got.length, want.length); i++) if (got[i] !== want[i]) { first = i; break; }
  check(`random drive ${run + 1} (${run % 2 === 0 ? 'steering' : 'letters'}${run >= 4 ? ', one stick' : ''}): the same ${want.length / 2} codes at the same times`,
    first < 0, first < 0 ? '' : `differs at code ${first >> 1}: web ${got.slice(first - (first & 1), first - (first & 1) + 6)} app ${want.slice(first - (first & 1), first - (first & 1) + 6)}`);
});

// the controller map and the car's lines
const m = new ControllerMap();
check('the same default controller map', m.toText() === ControllerMap.DEFAULT_NORMAL + ControllerMap.DEFAULT_SHIFT && m.job(ControllerMap.B.A, true) === 'M' && m.labelFor('M') === 'LB+A' && m.labelFor('T') === 'RT');
const cs = new CarState();
check('reads the car\'s lines', cs.read('~DRIVE|0010', 5) === 'status' && cs.mode === 'DRIVE' && cs.slow && cs.read('#V:2', 6) === 'version' && cs.steers
  && cs.read('#R:f42', 7) === 'radar' && cs.radarNearest(10) === 42 && cs.read('#S:Robot Groove', 8) === 'song' && cs.song === 'Robot Groove');
check('steering codes', Car.wheel(false, 0) === 160 && Car.wheel(true, 31) === 255 && Car.wheel(false, -31) === 129);

console.log(failed === 0 ? `\nALL ${passed} WEB CHECKS PASSED` : `\n${failed} WEB CHECK(S) FAILED`);
process.exit(failed === 0 ? 0 : 1);
