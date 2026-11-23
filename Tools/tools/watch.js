'use strict';
// WATCH THE CAR: runs your real Robot_Car_Main in the simulator, lets it drive around a pretend
// room with real walls (a tilted wall sends no echo back), and opens a little film of it.
// Run by double-clicking WATCH_THE_CAR.bat - or: node Tools/tools/watch.js [letter] [room] [seconds]
//   letter: I search, 1 chaos, P pet, 2 auto forward, O radar ...   room: room, empty, clutter, corridor
const fs = require('fs');
const path = require('path');
const { spawnSync, execFile } = require('child_process');

const tools = path.join(__dirname, '..');
const repo = path.join(tools, '..');
const [, , letter = 'I', roomName = 'room', seconds = '45'] = process.argv;
const userLibs = path.join(process.env.USERPROFILE || '', 'Documents', 'Arduino', 'libraries');
const coreLibs = path.join(process.env.LOCALAPPDATA || '', 'Arduino15', 'packages', 'arduino', 'hardware', 'avr', '1.8.8', 'libraries');
process.env.AVR8JS = process.env.AVR8JS || path.join(tools, 'pkg-avr8js', 'node_modules', 'avr8js');

const box = (a, b, c, d) => [[a, b, c, b], [c, b, c, d], [c, d, a, d], [a, d, a, b]];
const ROOMS = {
  room: { walls: [...box(0, 0, 300, 220), ...box(120, 80, 180, 130), [0, 160, 60, 220]], x: 60, y: 40, th: 0 },
  empty: { walls: box(0, 0, 300, 220), x: 150, y: 110, th: 30 },
  clutter: { walls: [...box(0, 0, 260, 200), ...box(70, 60, 90, 80), ...box(170, 40, 200, 70), ...box(120, 130, 160, 160)], x: 30, y: 30, th: 45 },
  corridor: { walls: [[0, 0, 0, 400], [75, 0, 75, 325], [75, 325, 400, 325], [0, 400, 400, 400], [0, 0, 75, 0], [400, 325, 400, 400]], x: 35, y: 60, th: 80 },
};
const R = ROOMS[roomName];
if (!R) { console.log('Rooms: ' + Object.keys(ROOMS).join(', ')); process.exit(1); }
if (!fs.existsSync(process.env.AVR8JS)) { console.log('One-time setup: open a terminal in Tools\\pkg-avr8js and run  npm install'); process.exit(1); }

console.log('1. Building your program ...');
const out = path.join(tools, 'build', 'watch');
const b = spawnSync(process.execPath, [path.join(__dirname, 'build.js'), '--sketch', path.join(repo, 'Robot_Car_Main', 'Robot_Car_Main.ino'), '--out', out,
  '--lib', path.join(userLibs, 'Servo'), '--lib', path.join(userLibs, 'U8g2'), '--lib', path.join(coreLibs, 'SoftwareSerial'), '--lib', path.join(coreLibs, 'Wire'),
  '--lib', path.join(coreLibs, 'SPI')], { encoding: 'utf8' });
const ok = (b.stdout + b.stderr).split(/\r?\n/).find((l) => l.startsWith('OK '));
if (!ok) { console.log(b.stdout + b.stderr); process.exit(1); }
console.log('   ' + ok);

console.log(`2. Driving for ${seconds} seconds (takes about as long in real life) ...`);
const { simulate } = require('../sim/harness');
const { makeRoom } = require('../sim/room');
const room = makeRoom({ ...R, echoLimit: 30 });
const film = [];
let next = 0, head = 90;
const log = simulate({ bin: path.join(out, 'Robot_Car_Main.bin'), nm: path.join(out, 'Robot_Car_Main.nm.txt'), label: 'watch' }, {
  name: 'watch', duration: +seconds, inputs: [{ t: 0.5, bt: 'W' }, { t: 4, bt: letter }],
  distance: (t, a) => room.distance(t, a),
  onSample: (t, m, a) => {
    room.onSample(t, m, a);
    if (t < next) return;
    next = t + 0.05;
    head = room.state.head;
    film.push([+t.toFixed(2), +room.state.x.toFixed(1), +room.state.y.toFixed(1), +room.state.th.toFixed(3), Math.round(head), room.state.crashes]);
  },
}, {}).log;
const says = log.filter((e) => e.kind === 'bt' && e.v.startsWith('~')).map((e) => [+(e.t / 1000).toFixed(2), e.v.slice(1).split('|')[0]]);
console.log(`   drove ${(room.state.odo / 100).toFixed(1)} m, bumped into things ${room.state.crashes} x`);

const html = `<!doctype html><meta charset="utf-8"><title>Robot car film</title><style>
body{margin:0;background:#10151c;color:#e8edf2;font:16px system-ui;display:flex;flex-direction:column;align-items:center;gap:10px;padding:16px}
canvas{background:#1b232d;border-radius:10px;max-width:96vw;max-height:74vh}input{width:min(760px,90vw)}button{font:inherit;padding:6px 16px}</style>
<div id="i"></div><canvas id="c"></canvas><input id="s" type="range" min="0" value="0"><div><button id="p">pause</button>
<button id="f">speed x1</button></div><script>
const W=${JSON.stringify(R.walls)},F=${JSON.stringify(film)},S=${JSON.stringify(says)};
const xs=W.flatMap(w=>[w[0],w[2]]),ys=W.flatMap(w=>[w[1],w[3]]),mx=Math.min(...xs),my=Math.min(...ys),k=Math.min(900/(Math.max(...xs)-mx),640/(Math.max(...ys)-my)),P=20;
const c=document.getElementById('c'),g=c.getContext('2d'),s=document.getElementById('s');c.width=(Math.max(...xs)-mx)*k+2*P;c.height=(Math.max(...ys)-my)*k+2*P;s.max=F.length-1;
const X=x=>P+(x-mx)*k,Y=y=>c.height-P-(y-my)*k;let i=0,run=true,sp=1,last=0;
document.getElementById('p').onclick=e=>{run=!run;e.target.textContent=run?'pause':'play'};document.getElementById('f').onclick=e=>{sp=sp>=4?1:sp*2;e.target.textContent='speed x'+sp};s.oninput=()=>{i=+s.value};
function draw(){const n=Math.min(Math.floor(i),F.length-1),f=F[n];g.clearRect(0,0,c.width,c.height);
g.strokeStyle='#3d6ea5';g.lineWidth=2;g.beginPath();for(let a=0;a<=n;a++)g.lineTo(X(F[a][1]),Y(F[a][2]));g.stroke();
g.strokeStyle='#e8edf2';g.lineWidth=4;for(const w of W){g.beginPath();g.moveTo(X(w[0]),Y(w[1]));g.lineTo(X(w[2]),Y(w[3]));g.stroke();}
for(let a=1;a<=n;a++)if(F[a][5]>F[a-1][5]){g.fillStyle='#ff5c5c';g.beginPath();g.arc(X(F[a][1]),Y(F[a][2]),8,0,7);g.fill();}
g.save();g.translate(X(f[1]),Y(f[2]));g.rotate(-f[3]);g.fillStyle='rgba(90,220,200,.25)';const h=-(f[4]-90)*Math.PI/180;g.beginPath();g.moveTo(6*k,0);g.arc(6*k,0,60*k,h-.26,h+.26);g.fill();
g.fillStyle='#ff8a2a';g.fillRect(-9*k,-7*k,18*k,14*k);g.fillStyle='#10151c';g.fillRect(4*k,-5*k,4*k,10*k);g.restore();
let m='';for(const q of S)if(q[0]<=f[0])m=q[1];document.getElementById('i').textContent=f[0].toFixed(1)+' s    '+m+'    bumps: '+f[5];s.value=n;}
requestAnimationFrame(function t(now){if(run&&last)i=Math.min(i+(now-last)/50*sp,F.length-1);last=now;draw();requestAnimationFrame(t);});
</script>`;
const file = path.join(tools, 'build', 'watch.html');
fs.writeFileSync(file, html);
console.log('3. Opening the film: ' + file);
if (!process.env.NO_OPEN) execFile('cmd', ['/c', 'start', '', file]);
