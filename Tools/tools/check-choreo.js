'use strict';
// Checks the choreography: every step is valid and brings the car back to its spot,
// and every routine only uses steps that exist.
const { MOVES, KITS, SONGS, parseStep, travel } = require('./choreo');
let bad = 0;
const balanced = (name, text) => {
  const bars = text.trim().split(/\s+/);
  if (bars.length % 8) { console.log(`${name}: ${bars.length} tokens - not whole bars of 8`); return false; }
  const end = travel(Array.from({ length: bars.length / 8 }, (_, i) => parseStep(name, bars.slice(i * 8, i * 8 + 8).join(' '))));
  const d = Math.hypot(end.x, end.y), deg = end.a * 180 / Math.PI * 1.0;
  if (d > 0.25 || Math.abs(deg) > 3) { console.log(`NOT BALANCED ${name}: moves ${d.toFixed(2)} steps, turns ${deg.toFixed(0)}  (${text})`); return false; }
  return true;
};
const PAIRS = { strut_f: 'strut_b' };
for (const [n, [t]] of Object.entries(MOVES)) if (!PAIRS[n] && !Object.values(PAIRS).includes(n) && !balanced(n, t)) bad++;
for (const [a, b] of Object.entries(PAIRS)) if (!balanced(a + '+' + b, MOVES[a][0] + ' ' + MOVES[b][0]) ) bad++;
for (const [id, s] of Object.entries(SONGS)) {
  for (const [n, [t]] of Object.entries(s.own || {})) { if (MOVES[n]) { console.log(`${id}: own step ${n} hides a dictionary step`); bad++; } if (!balanced(`${id}.${n}`, t)) bad++; }
  const known = (n) => MOVES[n] || (s.own && s.own[n]);
  for (const [label, list] of Object.entries(s)) {
    if (!Array.isArray(list)) continue;
    list.forEach((n, i) => { if (!known(n)) { console.log(`${id}.${label}[${i}]: unknown step ${n}`); bad++; } });
    // a pair must stay together
    list.forEach((n, i) => { if (PAIRS[n] && list[i + 1] !== PAIRS[n]) { console.log(`${id}.${label}: ${n} must be followed by ${PAIRS[n]}`); bad++; } });
  }
  for (const k of ['finale', 'fill']) if (s[k] && !known(s[k])) { console.log(`${id}.${k}: unknown step ${s[k]}`); bad++; }
  if (!KITS[s.kit]) { console.log(`${id}: unknown kit ${s.kit}`); bad++; }
}
for (const [k, kit] of Object.entries(KITS)) for (const [label, list] of Object.entries(kit)) list.forEach((n) => { if (!MOVES[n]) { console.log(`kit ${k}.${label}: unknown ${n}`); bad++; } });
console.log(bad ? `${bad} problem(s)` : `all ${Object.keys(MOVES).length} dictionary steps + every song's own steps are balanced; all routines OK`);
process.exit(bad ? 1 : 0);
