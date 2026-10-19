'use strict';
// UPDATE ALL SONGS: copies your changes from Robot_Car_Main into every song file in Dances
// (each song keeps its own song part: tempo, dance steps, choreography), then checks that
// every program still compiles.
// Run by double-clicking UPDATE_ALL_SONGS.bat (in the robot-car folder).
const fs = require('fs');
const path = require('path');
const { execFileSync, spawnSync } = require('child_process');

const repo = path.join(__dirname, '..', '..');
const tools = path.join(repo, 'Tools');
const dances = path.join(repo, 'Dances');
const home = process.env.USERPROFILE || '';
const userLibs = path.join(home, 'Documents', 'Arduino', 'libraries');
const coreLibs = path.join(process.env.LOCALAPPDATA || '', 'Arduino15', 'packages', 'arduino', 'hardware', 'avr', '1.8.8', 'libraries');

console.log('');
console.log('=== 1. Copying Robot_Car_Main into every song file ===');
execFileSync(process.execPath, [path.join(tools, 'tools', 'make-dances.js'), path.join(tools, 'results', 'songs.json'), dances], { stdio: 'inherit' });

console.log('');
console.log('=== 2. Checking every program compiles ===');
const programs = [path.join(repo, 'Robot_Car_Main', 'Robot_Car_Main.ino')];
for (const d of fs.readdirSync(dances)) {
  const ino = path.join(dances, d, d + '.ino');
  if (fs.existsSync(ino)) programs.push(ino);
}
const outRoot = path.join(require('os').tmpdir(), 'robot-car-check');
let ok = 0;
const bad = [];
let biggest = { name: '', pct: 0 };
for (const ino of programs) {
  const name = path.basename(ino, '.ino');
  const r = spawnSync(process.execPath, [path.join(tools, 'tools', 'build.js'), '--sketch', ino, '--out', path.join(outRoot, String(ok + bad.length)),
    '--lib', path.join(userLibs, 'Servo'), '--lib', path.join(userLibs, 'U8g2'),
    '--lib', path.join(coreLibs, 'SoftwareSerial'), '--lib', path.join(coreLibs, 'Wire'), '--lib', path.join(coreLibs, 'SPI')], { encoding: 'utf8' });
  const line = (r.stdout + r.stderr).split(/\r?\n/).find((l) => l.startsWith('OK ')) || '';
  const m = line.match(/flash (\d+) B \(([\d.]+)%/);
  if (m && +m[2] <= 100) {
    ok++;
    if (+m[2] > biggest.pct) biggest = { name, pct: +m[2] };
    process.stdout.write('.');
  } else {
    bad.push(name + (m ? ` - too big (${m[2]} %)` : ' - does not compile'));
    process.stdout.write('X');
  }
}
console.log('');
console.log('');
console.log(`${ok} of ${programs.length} programs compile and fit. (The fullest: ${biggest.name}, ${biggest.pct} %)`);
if (bad.length) {
  console.log('PROBLEMS:');
  for (const b of bad) console.log('  ' + b);
  console.log('Open that program in the Arduino IDE and press Verify to see what is wrong.');
  process.exitCode = 1;
} else {
  console.log('All good! Upload whichever song file you want.');
}
