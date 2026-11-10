'use strict';
// Checks every Android call in the compiled app against the Android version that added it
// (from the SDK's api-versions.xml). A call newer than Android 5 (API 21) must be inside an
// "if (Build.VERSION.SDK_INT >= ...)" - this lists them, with the line, so each can be checked.
// usage: node api-check.js <classes dir> <api-versions.xml> <javap.exe>
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');
const [classes, xmlFile, javap] = process.argv.slice(2);
const MIN = 21;
const BASES = ['android/view/View', 'android/widget/TextView', 'android/view/ViewGroup', 'android/widget/LinearLayout', 'android/widget/FrameLayout',
  'android/app/Activity', 'android/view/ContextThemeWrapper', 'android/content/ContextWrapper', 'android/content/Context'];

// api-versions.xml: <class name="android/os/Vibrator" since="1"> <method name="vibrate(J)V" since="1"/> ...
const xml = fs.readFileSync(xmlFile, 'utf8');
const since = new Map();
const clsRe = /<class name="([^"]+)"([^>]*)>([\s\S]*?)<\/class>/g;
let m;
while ((m = clsRe.exec(xml))) {
  const cs = /since="(\d+)"/.exec(m[2]);
  const cSince = cs ? +cs[1] : 1;
  since.set(m[1], cSince);
  const memRe = /<(method|field) name="([^"]+)"([^>]*)\/?>/g;
  let k;
  while ((k = memRe.exec(m[3]))) {
    const s = /since="(\d+)"/.exec(k[3]);
    since.set(m[1] + '#' + k[2], s ? +s[1] : cSince);
  }
}

const files = [];
(function walk(d) { for (const f of fs.readdirSync(d)) { const p = path.join(d, f); if (fs.statSync(p).isDirectory()) walk(p); else if (p.endsWith('.class')) files.push(p); } })(classes);
const out = execFileSync(javap, ['-c', '-p', '-l', ...files], { encoding: 'utf8', maxBuffer: 256 * 1024 * 1024 });

const found = [];
let cls = '', method = '', lines = [], code = [];
const flush = () => {
  for (const c of code) {
    let line = 0;
    for (const [pc, ln] of lines) if (pc <= c.pc) line = ln;
    found.push({ ...c, where: `${cls.split('.').pop()}.${method} line ${line}` });
  }
  code = []; lines = [];
};
for (const l of out.split(/\r?\n/)) {
  let x;
  if ((x = /^(?:final |public |abstract |static )*(?:class|interface) (\S+)/.exec(l))) { flush(); cls = x[1]; continue; }
  if ((x = /^  [^ ].*?([\w$<>]+)\(.*\);$/.exec(l))) { flush(); method = x[1]; continue; }
  if ((x = /^\s+line (\d+): (\d+)$/.exec(l))) { lines.push([+x[2], +x[1]]); continue; }
  if ((x = /^\s+(\d+): \w+.*\/\/ (Method|InterfaceMethod|Field) (android\/[\w\/$]+)\.([\w$<>]+):(\S+)/.exec(l))) {
    const owner = x[3], name = x[4], desc = x[5];
    const key = x[2] === 'Field' ? `${owner}#${name}` : `${owner}#${name}${desc}`;
    code.push({ pc: +x[1], key, owner, s: since.has(key) ? since.get(key) : null });
    continue;
  }
  // a call on one of our own classes (a button calling its own View method): look it up in the Android classes they come from
  // (javap leaves the owner out when a class calls its own method: "// Method name:(...)V")
  if ((x = /^\s+(\d+): \w+.*\/\/ (Method|InterfaceMethod) (io\/github\/[\w\/$]+\.)?([\w$]+):(\(\S+)/.exec(l))) {
    for (const base of BASES) {
      const key = `${base}#${x[4]}${x[5]}`;
      if (since.has(key)) { code.push({ pc: +x[1], key, owner: base, s: since.get(key) }); break; }
    }
  }
  if ((x = /^\s+(\d+): \w+.*\/\/ class (android\/[\w\/$]+)/.exec(l)))
    code.push({ pc: +x[1], key: x[2], owner: x[2], s: since.get(x[2]) ?? null });
}
flush();

const newer = found.filter((f) => f.s !== null && f.s > MIN);
const unknown = [...new Set(found.filter((f) => f.s === null && !since.has(f.owner)).map((f) => f.key))];
const byKey = new Map();
for (const f of newer) { if (!byKey.has(f.key)) byKey.set(f.key, { s: f.s, where: new Set() }); byKey.get(f.key).where.add(f.where); }
console.log(`${found.length} Android calls checked against API ${MIN} (Android 5).`);
console.log(`Newer than Android 5 (each must be behind a version check):`);
for (const [k, v] of [...byKey].sort((a, b) => a[1].s - b[1].s)) console.log(`  API ${v.s}  ${k}\n          at ${[...v.where].join(', ')}`);
if (unknown.length) console.log('Not in the list (inherited, fine if the class is old):\n  ' + unknown.slice(0, 20).join('\n  '));
