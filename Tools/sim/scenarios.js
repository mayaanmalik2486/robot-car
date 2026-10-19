'use strict';
// Dance engine + search "turn until open" checks.
// env SKETCH = path of the .ino that was built (the dance song is read from it)
const { parseSong, matchMotors, matchTune } = require('./dancecheck');
const { makeRoom } = require('./room');

const kind = (log, k) => log.filter((e) => e.kind === k);
// how long (ms) the motors were in this state between from and to
function motorMs(log, v, from, to) {
  const m = kind(log, 'motor');
  let total = 0;
  for (let i = 0; i < m.length; i++) {
    const a = Math.max(m[i].t, from), b = Math.min(m[i + 1] ? m[i + 1].t : to, to);
    if (m[i].v === v && b > a) total += b - a;
  }
  return total;
}
// did the buzzer play about this note between from and to? (low notes can only be measured to about 25 Hz)
function buzzAt(log, hz, from, to) {
  const b = kind(log, 'buzzer');
  const near = (e) => e && Math.abs(e.v - hz) <= Math.max(hz * 0.05, 30);
  return near(b.filter((e) => e.t <= from).pop()) || b.some((e) => e.t >= from && e.t <= to && near(e));
}
// silent all the way from 'from' to 'to'
function silentFrom(log, from, to) {
  const b = kind(log, 'buzzer');
  const before = b.filter((e) => e.t <= from).pop();
  return (!before || before.v === 0) && !b.some((e) => e.t > from && e.t <= to && e.v > 0);
}
const check = (name, pass, detail) => ({ name, pass: !!pass, detail });
const song = parseSong(process.env.SKETCH);
const footballPings = [];                                // when the sensor was pinged (football test)
const DANCE_AT = 4.0;                                  // seconds: when '4' is sent
const songSec = song.totalMs / 1000;

// find when the dance really started: the first predicted motor change, matched in the log
function anchorOf(log, from) {
  const first = song.changes[0];
  const m = kind(log, 'motor').find((e) => e.t >= from && e.v === first.motor);
  return m ? m.t - first.tMs : null;
}

function danceChecks(log, opts = {}) {
  const res = [];
  const sent = DANCE_AT * 1000;
  const anchor = anchorOf(log, sent);
  res.push(check('dance starts within 70 ms of the button', anchor !== null && anchor - sent >= 0 && anchor - sent <= 70,
    anchor === null ? 'no start' : `${(anchor - sent).toFixed(1)} ms after the byte`));
  if (anchor === null) return { res, anchor };
  const until = opts.untilMs || Infinity;
  const r = matchMotors(song, log, anchor, 30, opts.shiftAfter || null, until);
  res.push(check(`every move on the beat grid (${song.bpm} BPM, step ${(song.stepUs / 1000).toFixed(1)} ms)`, r.matched === r.total && r.total > 0,
    `${r.matched}/${r.total} moves, worst ${r.worst.toFixed(1)} ms, mean ${r.mean.toFixed(1)} ms${r.missing.length ? ' missing: ' + r.missing.join('; ') : ''}`));
  return { res, anchor };
}

const scenarios = [
  {
    name: 'dance-full-song',
    duration: DANCE_AT + songSec + 3,
    distance: () => 150,
    inputs: [
      { t: 0.5, bt: 'W' },
      { t: DANCE_AT, bt: '4' },
      { t: DANCE_AT + 0.6, snapshot: 'dance-start' },
      { t: DANCE_AT + Math.min(songSec * 0.55, songSec - 2), snapshot: 'dance-middle' },
      { t: DANCE_AT + songSec + 2.5, snapshot: 'after-end' },
    ],
    check: (log) => {
      const { res, anchor } = danceChecks(log);
      if (anchor === null) return res;
      const states = kind(log, 'state');
      const fin = states.find((e, i) => i > 0 && e.t > anchor && states[i - 1].v === 'DANCE' && e.v !== 'DANCE');
      const want = anchor + song.totalMs;
      res.push(check('stops at the end of the song', fin && fin.t >= want - 15 && fin.t <= want + 40,
        fin ? `finished ${(fin.t - want).toFixed(1)} ms after the last step's end (song ${(song.totalMs / 1000).toFixed(1)} s)` : 'never finished'));
      const after = kind(log, 'motor').filter((e) => fin && e.t > fin.t + 5);
      res.push(check('motors stay off after the end', after.length === 0, after.map((e) => e.v).join(',')));
      const st = kind(log, 'state').filter((e) => fin && e.t >= fin.t - 5);
      res.push(check('back to IDLE after the end', st.length && st[st.length - 1].v === 'IDLE', st.map((e) => e.v).join(',')));
      const sv = kind(log, 'servo');
      res.push(check('sensor head back to the centre at the end', sv.length && sv[sv.length - 1].v >= 88 && sv[sv.length - 1].v <= 92, sv.length ? 'last ' + sv[sv.length - 1].v : ''));
      const parts = new Set(kind(log, 'var').filter((e) => e.t > anchor && /^dancePart=\d+$/.test(e.v)).map((e) => +e.v.split('=')[1]).filter((n) => n >= 1 && n < song.map.length));
      res.push(check('goes through every part of the song', parts.size === song.map.length - 1, `${parts.size + 1}/${song.map.length} parts`));
      if (song.notes.length) {
        const shouts = song.shout ? kind(log, 'var').filter((e) => e.t > anchor && /^shoutUntil=/.test(e.v) && e.v !== 'shoutUntil=0').map((e) => e.t - 2) : [];
        if (song.shout && song.shoutSteps.length) {
          const want = song.shoutSteps.map((k) => anchor + k * song.stepUs / 1000);
          const ok = want.every((w) => shouts.some((s) => Math.abs(s - w) < 60));
          res.push(check(`the special moment "${song.shout}" fires at each planned time`, ok && shouts.length === want.length,
            `planned ${want.map((w) => ((w - anchor) / 1000).toFixed(2) + 's').join(', ')}; fired ${shouts.map((s) => ((s - anchor) / 1000).toFixed(2) + 's').join(', ')}`));
        }
        const tn = matchTune(song, log, anchor, Infinity, shouts);
        res.push(check('the buzzer plays the tune: right note at the right time', tn.ok >= tn.total * 0.97,
          `${tn.ok}/${tn.total} notes${tn.bad.length ? ' e.g. ' + tn.bad.join('; ') : ''}`));
      }
      return res;
    },
  },
  {
    name: 'dance-tap-resync',
    duration: DANCE_AT + 16,
    distance: () => 150,
    inputs: [
      { t: 0.5, bt: 'W' },
      { t: DANCE_AT, bt: '4' },
      { t: DANCE_AT + 0.3, bt: 'S' },                                  // let go of the button: ignored
      { t: DANCE_AT + 8 * (60 / song.bpm) + 0.09, bt: '4' },           // tapped 90 ms late (about) on beat 8
    ],
    check: (log) => {
      const res = [];
      const moved = kind(log, 'serial').find((e) => /^DANCE beat moved by ms: -?\d+$/.test(e.v));
      const shift = moved ? +moved.v.split(': ')[1] : null;
      // the tap came this late after the robot's own beat 8 (its beat started when the dance started, a little after the byte)
      const late = DANCE_AT * 1000 + 90 - (anchorOf(log, DANCE_AT * 1000) || 0);
      res.push(check('a new tap moves the beat (by the tap time + up to one loop)', moved && shift >= late - 10 && shift <= late + 110,
        moved ? `${moved.v} (tap ${late.toFixed(0)} ms after the beat)` : 'no tap seen'));
      const st = kind(log, 'state').filter((e) => e.t > DANCE_AT * 1000 + 100);
      res.push(check('dance keeps going after the button is let go and tapped', st.every((e) => e.v === 'DANCE'), st.map((e) => e.v).join(',')));
      if (moved) {
        const anchor = anchorOf(log, DANCE_AT * 1000);
        const tapAt = moved.t - anchor;
        const r = matchMotors(song, log, anchor, 32, { fromMs: tapAt + 30, shiftMs: shift }, (DANCE_AT + 15.5) * 1000);
        res.push(check('moves after the tap follow the new beat', r.matched === r.total, `${r.matched}/${r.total}, worst ${r.worst.toFixed(1)} ms ${r.missing.join('; ')}`));
      }
      return res;
    },
  },
  {
    name: 'dance-hold-button',
    duration: DANCE_AT + 6,
    distance: () => 150,
    inputs: [{ t: 0.5, bt: 'W' }, ...Array.from({ length: 12 }, (_, i) => ({ t: DANCE_AT + i * 0.1, bt: '4' })), { t: DANCE_AT + 1.25, bt: 'S' }],
    check: (log) => {
      const taps = kind(log, 'serial').filter((e) => /^DANCE beat moved/.test(e.v));
      const starts = kind(log, 'serial').filter((e) => /^DANCE to:/.test(e.v));
      const { res } = danceChecks(log, { untilMs: (DANCE_AT + 5.5) * 1000 });
      return [check('holding dance (letter repeats) does not restart or shift it', taps.length === 0 && starts.length === 1, `${starts.length} start(s), ${taps.length} shift(s)`), ...res];
    },
  },
  // ---- these two need the IR add-on pasted in (see IR_Remote_Addon) ----
  {
    name: 'dance-ir',
    duration: DANCE_AT + 8,
    distance: () => 150,
    inputs: [{ t: DANCE_AT, ir: 0x40 }, { t: DANCE_AT + 4 * (60 / song.bpm) + 0.05, ir: 0x40 }],
    check: (log) => {
      const st = kind(log, 'state');
      const into = st.filter((e) => e.v === 'DANCE').length;
      const last = st.length ? st[st.length - 1].v : '-';
      return [check('remote 5 starts the dance, 5 again does not restart it', into === 1 && last === 'DANCE', st.map((e) => e.v).join(','))];
    },
  },
  {
    name: 'ir-all-buttons',
    duration: 1 + 17 * 1.4 + 2,
    distance: () => 150,
    inputs: (() => {
      const codes = [0x18, 0x52, 0x08, 0x5A, 0x1C, 0x45, 0x46, 0x47, 0x44, 0x40, 0x43, 0x07, 0x15, 0x09, 0x16, 0x19, 0x0D];
      const out = [];
      codes.forEach((c, i) => { out.push({ t: 1 + i * 1.4, ir: c, repeats: c === 0x08 ? 9 : 0 }); out.push({ t: 1 + i * 1.4 + (c === 0x08 ? 1.2 : 0.6), ir: 0x1C }); });
      return out;
    })(),
    check: (log) => {
      const want = ['MANUAL_FORWARD', 'MANUAL_BACKWARD', 'MANUAL_LEFT', 'MANUAL_RIGHT', 'TURN180', 'BRAKING', 'TURBO', 'DANCE', 'CHAOS',
        'CURVE_LEFT', 'CURVE_RIGHT', 'SEARCH', 'AUTO_FORWARD', 'DONUT', 'AUTO_BACKWARD'];
      const seen = new Set(kind(log, 'state').map((e) => e.v));
      const missing = want.filter((w) => !seen.has(w));
      const turning = motorMs(log, 'LB216 RF216', 3800, 5300);
      const upMs = motorMs(log, 'LF255 RF255', 1000, 2000);
      return [
        check('every remote button does its job', missing.length === 0, missing.length ? 'missing ' + missing.join(',') : want.length + ' modes seen'),
        check('holding LEFT for ~1 s keeps turning', turning > 900, `${turning.toFixed(0)} ms of turning`),
        check('a tap on UP drives briefly, then stops when the remote goes quiet', upMs > 150 && upMs < 450, `${upMs.toFixed(0)} ms`),
      ];
    },
  },
  {
    name: 'curve-sharper',
    duration: 9,
    distance: () => 150,
    inputs: [{ t: 0.5, bt: 'W' }, { t: 4.0, bt: 'G' }, { t: 4.8, snapshot: 'curve-left-screen' }, { t: 5.5, bt: 'S' }, { t: 6.0, bt: 'H' }, { t: 7.5, bt: 'S' }],
    check: (log) => {
      const m = kind(log, 'motor').map((e) => e.v);
      const st = kind(log, 'state').map((e) => e.v);
      const inner = Math.floor(255 * 30 / 100);
      return [
        check('G curves left: left wheels backward at 30 %, right wheels full', m.includes(`LB${inner} RF255`), m.filter((v) => v !== 'L-0 R-0').join(', ')),
        check('H curves right: right wheels backward at 30 %, left wheels full', m.includes(`LF255 RB${inner}`), ''),
        check('the robot is in CURVE_LEFT, then CURVE_RIGHT', st.includes('CURVE_LEFT') && st.includes('CURVE_RIGHT'), st.join(',')),
      ];
    },
  },
  {
    name: 'football-mode',
    duration: 16,
    distance: (t) => { footballPings.push(t * 1000); return 15; },        // a wall 15 cm in front the whole time
    inputs: [{ t: 0.5, bt: 'W' }, { t: 4.0, bt: 'F' }, { t: 5.0, bt: 'S' }, { t: 5.5, bt: 'V' }, { t: 6.0, bt: 'F' }, { t: 6.5, snapshot: 'football-screen' },
      { t: 7.5, bt: 'S' }, { t: 8.0, bt: 'I' }, { t: 10.0, bt: 'S' }, { t: 11.0, bt: 'v' }, { t: 13.0, bt: 'F' }, { t: 14.5, bt: 'S' }],
    check: (log) => {
      const m = kind(log, 'motor');
      const fwdMs = (a, b) => m.reduce((sum, e, i) => (e.t > a && e.t < b && e.v === 'LF255 RF255' ? sum + ((m[i + 1] ? m[i + 1].t : b) - e.t) : sum), 0);
      const pings = (a, b) => footballPings.filter((t) => t > a && t < b).length;
      const ser = kind(log, 'serial');
      const searchSaw = ser.some((e) => e.t > 8000 && e.t < 10000 && /^SEARCH/.test(e.v) && e.v !== 'SEARCH');
      return [
        check('normal: F with a wall 15 cm ahead stops at once', fwdMs(4000, 5000) < 100, `drove ${fwdMs(4000, 5000).toFixed(0)} ms`),
        check('V = football mode: the sensor is completely off (no pings) and F keeps driving', pings(5700, 7500) === 0 && fwdMs(6000, 7600) > 1300, `${pings(5700, 7500)} pings, drove ${fwdMs(6000, 7600).toFixed(0)} ms`),
        check('search still uses the sensor in football mode', pings(8100, 10000) > 5 && searchSaw, `${pings(8100, 10000)} pings, ${ser.filter((e) => e.t > 8000 && e.t < 10000 && /^SEARCH/.test(e.v)).map((e) => e.v).slice(0, 2).join(' | ')}`),
        check('V again = sensor back on: F stops at once', pings(11200, 13000) > 5 && fwdMs(13000, 14500) < 100, `${pings(11200, 13000)} pings, drove ${fwdMs(13000, 14500).toFixed(0)} ms`),
      ];
    },
  },
  ...[
    { name: 'stop-dance-forward', inp: { bt: 'F' }, want: 'MANUAL_FORWARD' },
    { name: 'stop-dance-brake', inp: { bt: 'u' }, want: 'IDLE' },
    { name: 'stop-dance-180', inp: { bt: 'U' }, want: 'IDLE' },
    { name: 'stop-search-backward', inp: { bt: 'B' }, want: 'MANUAL_BACKWARD', start: 'I' },
  ].map((c) => ({
    name: c.name,
    duration: DANCE_AT + 7,
    distance: () => 150,
    inputs: [{ t: 0.5, bt: 'W' }, { t: DANCE_AT, bt: c.start || '4' }, { t: DANCE_AT + 3, ...c.inp }],
    check: (log) => {
      const after = (DANCE_AT + 3) * 1000;
      const st = kind(log, 'state').filter((e) => e.t > after);
      const parts = kind(log, 'serial').filter((e) => e.t > after + 300 && /^DANCE part/.test(e.v));
      const last = st.length ? st[st.length - 1].v : '-';
      const back = st.some((e) => e.v === 'DANCE' || e.v === 'SEARCH');
      const sv = kind(log, 'servo').filter((e) => e.t > after + 400);
      const head = sv.length ? sv[sv.length - 1].v : 90;
      return [check(`the button stops the ${c.start ? 'search' : 'dance'} for good`, !back && parts.length === 0 && last === c.want && head >= 88 && head <= 92,
        `states after: ${st.map((e) => e.v).join(',')}; head ${head}`)];
    },
  })),
  // ---------------- play modes (no debug messages needed: they read motors, servo, buzzer, variables) ----------------
  {
    name: 'turbo-horn-status',
    duration: 15,
    // a wall 20 cm ahead from 8 s on
    distance: (t) => (t >= 8 ? 20 : 150),
    inputs: [{ t: 0.5, bt: '?' }, { t: 2.0, bt: 'W' }, { t: 3.0, bt: 'Y' }, { t: 4.0, bt: 'T' }, { t: 6.5, bt: 'X' }, { t: 7.2, bt: 'X' },
      { t: 8.5, bt: 'T' }, { t: 10.0, bt: 'V' }, { t: 10.5, bt: 'T' }, { t: 12.5, bt: 'v' }, { t: 13.0, bt: 'M' }],
    check: (log) => {
      const bt = kind(log, 'bt').map((e) => e.v);
      const vars = kind(log, 'var').map((e) => e.v);
      const start = vars.find((v) => v.startsWith('batteryStartMv=') && v !== 'batteryStartMv=0');
      return [
        check('the car answers "?" with its status line', bt[0] === '~READY|0000', bt.slice(0, 3).join(' / ')),
        check('Y = horn (880 Hz)', buzzAt(log, 880, 3000, 3600), ''),
        check('T = turbo: full power for 1.5 s, then stop', Math.abs(motorMs(log, 'LF255 RF255', 4000, 6000) - 1500) < 80, `${motorMs(log, 'LF255 RF255', 4000, 6000).toFixed(0)} ms`),
        check('the app hears TURBO! then READY', bt.includes('~TURBO!|0000') && bt.indexOf('~READY|0000', bt.indexOf('~TURBO!|0000')) > 0, bt.join(' / ')),
        check('mute shows in the status (X)', bt.includes('~READY|1000'), ''),
        check('turbo stops at once for a wall 20 cm ahead', motorMs(log, 'LF255 RF255', 8400, 9500) < 60, `${motorMs(log, 'LF255 RF255', 8400, 9500).toFixed(0)} ms`),
        check('in football mode (V) turbo drives the full 1.5 s', Math.abs(motorMs(log, 'LF255 RF255', 10400, 12400) - 1500) < 80 && bt.includes('~TURBO!|0100'),
          `${motorMs(log, 'LF255 RF255', 10400, 12400).toFixed(0)} ms`),
        check('follow me (M) shows in the status', bt.includes('~FOLLOW|0000'), ''),
        check('the battery check reads the power supply (about 5 V) and does not warn', start && +start.split('=')[1] > 4500 && !vars.includes('batteryLow=1'), start || 'no reading'),
      ];
    },
  },
  {
    name: 'radar',
    duration: 8,
    // something 40 cm away on the left (servo 110 degrees and more), nothing anywhere else
    distance: (t, angle) => (angle >= 110 ? 40 : 999),
    inputs: [{ t: 0.5, bt: '?' }, { t: 1.0, bt: 'O' }, { t: 5.0, snapshot: 'screen' }, { t: 6.5, bt: 'S' }],
    check: (log) => {
      const bt = kind(log, 'bt').map((e) => e.v);
      const radar = bt.filter((v) => v.startsWith('#R:'));
      const sv = kind(log, 'servo').filter((e) => e.t > 1000 && e.t < 6500).map((e) => e.v);
      const last = kind(log, 'servo').filter((e) => e.t > 6500).map((e) => e.v);
      return [
        check('the car tells the app its song (?)', bt.includes('#S:Robot Groove'), bt.slice(0, 3).join(' / ')),
        check('O = radar: the app hears RADAR', bt.includes('~RADAR|0000'), ''),
        check('it sends a radar dot about every 0.2 s', radar.length >= 22 && radar.length <= 36, radar.length + ' dots'),
        check('the sensor sweeps all 11 directions (40 ... 140 degrees)', [40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140].every((a) => sv.includes(a)), [...new Set(sv)].join(',')),
        check('it sees the thing on the left at 40 cm', radar.some((v) => /^#R:[ijk](39|40|41)$/.test(v)), radar.filter((v) => /^#R:[ijk]/.test(v)).slice(0, 4).join(' ')),
        check('nothing on the right', radar.filter((v) => /^#R:[a-e]/.test(v)).every((v) => v.endsWith('999')), ''),
        check('the car stands still while the radar runs', motorMs(log, 'L-0 R-0', 1100, 6400) > 5200, motorMs(log, 'L-0 R-0', 1100, 6400).toFixed(0) + ' ms'),
        check('S stops it: the sensor looks ahead again, the app hears READY', last.at(-1) === 90 && bt.indexOf('~READY|0000', bt.indexOf('~RADAR|0000')) > 0, last.join(',')),
      ];
    },
  },
  {
    name: 'app-session',
    duration: 16,
    // exactly what the phone app sends: '?' on connect, slow, fast, a move, a play mode, radar, stop
    distance: (t) => 120,
    inputs: [{ t: 0.5, bt: '?' }, { t: 1.5, bt: 'Q' }, { t: 2.5, bt: 'W' }, { t: 3.5, bt: 'I' }, { t: 6.0, bt: 'S' }, { t: 7.0, bt: 'M' },
      { t: 9.5, bt: 'S' }, { t: 10.5, bt: 'O' }, { t: 13.0, bt: 'S' }, { t: 14.0, bt: 'P' }],
    check: (log) => {
      const bt = kind(log, 'bt').map((e) => e.v);
      require('fs').writeFileSync(require('path').join(__dirname, '..', 'build', 'app-session-lines.txt'), bt.join('\n') + '\n');
      return [
        check('status after ?', bt.includes('~READY|0000'), bt.slice(0, 4).join(' / ')),
        check('Q shows slow', bt.includes('~READY|0010'), ''),
        check('W shows fast again', bt.lastIndexOf('~READY|0000') > bt.indexOf('~READY|0010'), ''),
        check('I = search', bt.some((v) => v.startsWith('~SEARCH') || v.startsWith('~LOOK')), ''),
        check('M = follow', bt.includes('~FOLLOW|0000'), ''),
        check('O = radar + dots', bt.includes('~RADAR|0000') && bt.some((v) => v.startsWith('#R:')), ''),
        check('P = pet', bt.some((v) => v.startsWith('~PET') || v.startsWith('~BORED')), ''),
      ];
    },
  },
  {
    name: 'follow-me',
    duration: 16,
    // you: straight ahead at 60 cm, then 25 cm, then 10 cm; from 11 s you step out to the left
    distance: (t, angle) => (t < 7 ? 60 : t < 9 ? 25 : t < 11 ? 10 : angle >= 110 ? 50 : 999),
    inputs: [{ t: 0.5, bt: 'W' }, { t: 4.0, bt: 'M' }],
    check: (log) => {
      const sv = kind(log, 'servo').filter((e) => e.t > 11000).map((e) => e.v);
      return [
        check('you are far (60 cm): it comes after you', motorMs(log, 'LF165 RF165', 4100, 7000) > 2500, `${motorMs(log, 'LF165 RF165', 4100, 7000).toFixed(0)} ms forward`),
        check('25 cm: it stops', motorMs(log, 'L-0 R-0', 7100, 9000) > 1700, ''),
        check('10 cm: it backs away', motorMs(log, 'LB165 RB165', 9100, 11000) > 1700, ''),
        check('it lost you: it looks around (5 angles)', [40, 65, 90, 115, 140].every((a) => sv.includes(a)), sv.join(',')),
        check('it turns toward where it saw you (left)', motorMs(log, 'LB216 RF216', 12000, 16000) > 30, `${motorMs(log, 'LB216 RF216', 12000, 16000).toFixed(0)} ms`),
      ];
    },
  },
  {
    name: 'goalkeeper',
    duration: 15,
    // the ball: on the right (40-65 degrees) at 100 cm; after it turned: straight ahead 80 cm, then 25 cm (a shot!), then gone
    distance: (t, angle) => (t < 7 ? (angle <= 70 ? 100 : 999) : t < 8.5 ? 80 : t < 8.65 ? 25 : 999),
    inputs: [{ t: 0.5, bt: 'W' }, { t: 3.0, bt: 'V' }, { t: 4.0, bt: 'K' }, { t: 12.0, bt: 'k' }],
    check: (log) => {
      const sv = kind(log, 'servo').filter((e) => e.t > 4000 && e.t < 7000).map((e) => e.v);
      const fwd = motorMs(log, 'LF255 RF255', 8400, 9600), back = motorMs(log, 'LB255 RB255', 8400, 9800);
      return [
        check('it looks around for the ball (even in football mode)', [40, 65, 90, 115, 140].every((a) => sv.includes(a)), sv.join(',')),
        check('it turns to face the ball (right)', motorMs(log, 'LF216 RB216', 5000, 7500) > 60, `${motorMs(log, 'LF216 RB216', 5000, 7500).toFixed(0)} ms`),
        check('the shot (25 cm): it punches the ball away and comes back', Math.abs(fwd - 220) < 25 && Math.abs(back - 220) < 25, `forward ${fwd.toFixed(0)} ms, back ${back.toFixed(0)} ms`),
        check('k = kick right away', Math.abs(motorMs(log, 'LF255 RF255', 12000, 12600) - 220) < 25, `${motorMs(log, 'LF255 RF255', 12000, 12600).toFixed(0)} ms`),
      ];
    },
  },
  {
    name: 'guard',
    duration: 30,
    // the room: a wall 200 cm away all round. From 14 s someone stands at the right (65 degrees) 80 cm away
    // (once the car has turned to them, they're straight ahead); gone at 19 s
    distance: (t, angle, read) => (t >= 14 && t < 19 && (angle === 65 || (read('playStep') >= 3 && angle === 90)) ? 80 : 200),
    inputs: [{ t: 0.5, bt: 'W' }, { t: 3.0, bt: 'A' }],
    vars: ['playLook'],
    check: (log) => {
      const steps = kind(log, 'var').filter((e) => e.v.startsWith('playStep='));
      const alarm = steps.find((e) => e.v === 'playStep=4' && e.t > 14000);
      const relearn = alarm && steps.find((e) => e.v === 'playStep=1' && e.t > alarm.t);
      const beeps = kind(log, 'buzzer').filter((e) => e.t > 3500 && e.t < 9500 && Math.abs(e.v - 880) < 50).length;
      const bt = kind(log, 'bt').map((e) => e.v);
      return [
        check('it counts down with 5 beeps (time to walk away)', beeps >= 5, `${beeps} beeps`),
        check('an intruder sets off the alarm', alarm && alarm.t < 17000, alarm ? `at ${(alarm.t / 1000).toFixed(1)} s` : 'no alarm'),
        check('siren (wee-woo)', alarm && buzzAt(log, 1320, alarm.t - 500, alarm.t + 3000) && buzzAt(log, 880, alarm.t, alarm.t + 3000), ''),
        check('it turns to face the intruder (right)', alarm && motorMs(log, 'LF216 RB216', alarm.t - 600, alarm.t + 200) > 30, ''),
        check('the app hears INTRUDER', bt.some((v) => v.startsWith('~INTRUDER|')), bt.join(' / ')),
        check('5 s after they left it calms down and learns the room again', relearn && relearn.t > 19000 + 4800 && relearn.t < 19000 + 7000,
          relearn ? `at ${(relearn.t / 1000).toFixed(1)} s` : 'never'),
      ];
    },
  },
  {
    name: 'music',
    duration: 10,
    // your hand: none, 50 cm, 20 cm, 5 cm, none
    distance: (t) => (t < 5 ? 999 : t < 6 ? 50 : t < 7 ? 20 : t < 8 ? 5 : 999),
    inputs: [{ t: 0.5, bt: 'W' }, { t: 4.0, bt: 'N' }],
    check: (log) => [
      check('hand at 50 cm: a low note (E5, 659 Hz)', buzzAt(log, 659, 5050, 5950), ''),
      check('hand at 20 cm: a higher note (G6, 1568 Hz)', buzzAt(log, 1568, 6050, 6950), ''),
      check('hand at 5 cm: the top note (C7, 2093 Hz)', buzzAt(log, 2093, 7050, 7950), ''),
      check('no hand: silence', silentFrom(log, 8200, 10000), ''),
      check('the car stays still', motorMs(log, 'L-0 R-0', 4100, 10000) > 5800, ''),
    ],
  },
  {
    name: 'gestures',
    duration: 19,
    // tap (5.0-5.3), double tap (8.0-8.25 and 8.45-8.7), push (8 cm, 11.0-12.5), "come here" (25 cm, 14.0-16.5)
    distance: (t) => ((t >= 5 && t < 5.3) || (t >= 8 && t < 8.25) || (t >= 8.45 && t < 8.7) ? 20
      : t >= 11 && t < 12.5 ? 8 : t >= 14 && t < 16.5 ? 25 : 999),
    inputs: [{ t: 0.5, bt: 'W' }, { t: 4.0, bt: 'E' }],
    check: (log) => {
      const spin = motorMs(log, 'LB216 RF216', 5300, 7500);
      const wiggle = motorMs(log, 'LB178 RF178', 8700, 10500) + motorMs(log, 'LF178 RB178', 8700, 10500);
      return [
        check('one tap: it spins round once (910 ms)', Math.abs(spin - 910) < 25, `${spin.toFixed(0)} ms`),
        check('two taps: a happy wiggle', wiggle > 600, `${wiggle.toFixed(0)} ms of wiggling`),
        check('hand held close: it backs away', Math.abs(motorMs(log, 'LB140 RB140', 11000, 13000) - 500) < 60, `${motorMs(log, 'LB140 RB140', 11000, 13000).toFixed(0)} ms`),
        check('hand held further away: it comes to you', motorMs(log, 'LF140 RF140', 14000, 17000) > 800, `${motorMs(log, 'LF140 RF140', 14000, 17000).toFixed(0)} ms`),
      ];
    },
  },
  {
    name: 'pet',
    duration: 64,
    // an empty room; from 56 s to 59 s a hand right in front of it (only seen when it looks straight ahead)
    distance: (t, angle) => (t >= 56 && t < 59 && angle === 90 ? 15 : 150),
    inputs: [{ t: 0.5, bt: 'W' }, { t: 4.0, bt: 'P' }],
    check: (log) => {
      const steps = kind(log, 'var').filter((e) => e.v.startsWith('playStep='));
      const at = (v, from, to) => steps.find((e) => e.v === 'playStep=' + v && e.t >= from && e.t <= to);
      return [
        check('it wanders about slowly first', motorMs(log, 'LF127 RF127', 4100, 12000) > 7000, ''),
        check('then sits and looks around', at(2, 11000, 13500) && kind(log, 'servo').some((e) => e.t > 12500 && e.t < 18000 && e.v !== 90), ''),
        check('nobody plays with it: it gets bored', at(3, 28000, 51000), steps.map((e) => `${(e.t / 1000).toFixed(0)}s:${e.v.slice(9)}`).join(' ')),
        check('... and falls asleep (snoring)', at(4, 48000, 52000) && buzzAt(log, 196, 48000, 56000), ''),
        check('a hand wakes it up: happy, purring', at(5, 56000, 57000) && buzzAt(log, 262, 56000, 60000), ''),
      ];
    },
  },
  {
    name: 'faces-donut-dizzy-180',
    duration: 16,
    distance: () => 150,
    inputs: [
      { t: 0.5, bt: 'W' },
      { t: 4.0, bt: '3' }, { t: 5.2, snapshot: 'donut-dizzy-eyes' },
      { t: 6.5, bt: 'S' }, { t: 6.95, snapshot: 'after-donut-very-dizzy' },
      { t: 9.0, bt: 'U' }, { t: 9.06, snapshot: 'turn180-focused' },
      { t: 11.0, bt: '1' }, { t: 12.0, snapshot: 'chaos-mad-serious' },
      { t: 14.0, bt: 'S' },
    ],
    check: (log) => {
      const sv = kind(log, 'servo').filter((e) => e.t > 6500 && e.t < 8200);
      const wobbles = sv.filter((e) => Math.abs(e.v - 90) >= 20).length;
      const back = sv.length && sv[sv.length - 1].v >= 88 && sv[sv.length - 1].v <= 92;
      const m = kind(log, 'motor').filter((e) => e.t > 9000 && e.t < 10500);
      const spin = m.find((e) => e.v === 'LF255 RB255');
      const after = spin ? m.find((e) => e.t > spin.t && e.v !== 'LF255 RB255') : null;
      const ms = spin && after ? after.t - spin.t : 0;
      return [
        check('after stopping a donut: the head wobbles for ~1.3 s, then looks straight', wobbles >= 6 && back, `${wobbles} wobbles, last ${sv.length ? sv[sv.length - 1].v : '-'}`),
        check('180 turn stops on time: 455 ms (+-15)', ms >= 445 && ms <= 470, `${ms.toFixed(0)} ms`),
      ];
    },
  },
  {
    name: 'shout-moment',
    duration: DANCE_AT + 13,
    distance: () => 150,
    inputs: [{ t: 0.5, bt: 'W' }, { t: DANCE_AT, bt: '4' }, { t: DANCE_AT + 8.6, snapshot: 'feedback-screen' }],
    check: (log) => {
      const anchor = DANCE_AT * 1000 + 20;
      const bar = (60000 / song.bpm) * 4;
      const want = anchor + 4 * bar;                               // SONG_SHOUT_STEPS = {32} (= bar 4) in the test copy
      const s = kind(log, 'serial').find((e) => e.v === 'FEEDBACK!');
      const zap = kind(log, 'buzzer').filter((e) => e.t > want - 30 && e.t < want + 350 && e.v >= 1700);
      const moves = kind(log, 'motor').filter((e) => e.t > want && e.t < want + 4000);
      return [
        check('the special moment fires at the start of bar 4', s && Math.abs(s.t - want) < 80, s ? `${(s.t - want).toFixed(0)} ms from the bar start` : 'never'),
        check('it plays the electric zap', zap.length >= 3, `${zap.length} zap notes`),
        check('the dance keeps going during it', moves.length >= 2, `${moves.length} moves`),
      ];
    },
  },
  {
    name: 'mute-button',
    duration: DANCE_AT + 11,
    distance: () => 150,
    inputs: [{ t: 0.5, bt: 'W' }, { t: DANCE_AT, bt: '4' }, { t: DANCE_AT + 3, bt: 'X' }, { t: DANCE_AT + 3.1, bt: 'X' }, { t: DANCE_AT + 7, bt: 'x' },
      { t: DANCE_AT + 5, snapshot: 'muted-screen' }],
    check: (log) => {
      const buz = kind(log, 'buzzer').filter((e) => e.v > 0);
      const t0 = (DANCE_AT + 3) * 1000;
      const during = buz.filter((e) => e.t > t0 + 120 && e.t < t0 + 3900);   // the X byte takes up to ~100 ms to be read
      const before = buz.filter((e) => e.t > DANCE_AT * 1000 && e.t < t0);
      const after = buz.filter((e) => e.t > t0 + 4100);
      const st = kind(log, 'state').filter((e) => e.t > DANCE_AT * 1000 + 100);
      return [
        check('X mutes every sound (a quick double press counts once), x turns it back on', before.length > 0 && during.length === 0 && after.length > 0,
          `notes before ${before.length}, while muted ${during.length}, after ${after.length}`),
        check('muting does not stop the dance', st.every((e) => e.v === 'DANCE'), st.map((e) => e.v).join(',')),
      ];
    },
  },
  {
    name: 'search-stuck-no-echo',
    duration: 14,
    distance: () => 250,                                // nothing within the sensor's range at all
    inputs: [{ t: 0.5, bt: 'W' }, { t: 2, bt: 'I' }],
    check: (log) => {
      const stuck = kind(log, 'serial').find((e) => /^SEARCH stuck/.test(e.v));
      const dt = stuck ? (stuck.t - 2000) / 1000 : null;
      return [check('driving with no echo for ~6 s counts as stuck (backs out and looks)', stuck && dt > 5.5 && dt < 8.5, stuck ? `after ${dt.toFixed(1)} s` : 'never')];
    },
  },
  {
    name: 'dance-wall-in-front',
    duration: DANCE_AT + Math.min(songSec, 40),
    distance: () => 20,
    inputs: [{ t: 0.5, bt: 'W' }, { t: DANCE_AT, bt: '4' }],
    check: (log) => {
      const fwd = kind(log, 'motor').filter((e) => e.t > DANCE_AT * 1000 && /^LF\d+ RF\d+$/.test(e.v));
      const any = kind(log, 'motor').filter((e) => e.t > DANCE_AT * 1000 && e.v !== 'L-0 R-0');
      return [check('never dances forward into a wall 20 cm away (but still dances)', fwd.length === 0 && any.length > 0, `${fwd.length} forward moves, ${any.length} other moves`)];
    },
  },
];

// SEARCH in a room. The car turns at half the speed the code expects
// (like a real 4WD car skidding), so a fixed short turn would be too small.
function roomScenario(name, opts) {
  const room = makeRoom(opts);
  return {
    name,
    duration: opts.duration || 70,
    oled: false,
    distance: (t, a) => room.distance(t, a),
    onSample: (t, m) => room.onSample(t, m),
    inputs: [{ t: 0.5, bt: 'W' }, { t: 4.0, bt: 'I' }],
    room,
    check: (log) => {
      const st = room.state;
      const blocked = kind(log, 'serial').filter((e) => /^SEARCH blocked/.test(e.v));
      const turns = kind(log, 'serial').filter((e) => /^SEARCH -> |^SEARCH open way|^SEARCH no open/.test(e.v));
      // "went straight back": blocked again after driving less than 30 cm on the chosen way
      // (when it starts driving again: new code says "open way found", the old code drove right after "-> turn left/right")
      const opens = kind(log, 'serial').filter((e) => /^SEARCH open way found|^SEARCH -> (straight|turn left|turn right)$/.test(e.v));
      const runs = [];
      for (const o of opens) {
        const b = blocked.find((x) => x.t > o.t);
        if (b) runs.push(room.odoAt(b.t) - room.odoAt(o.t));
      }
      const quickReblocks = runs.filter((d) => d < 30).length;
      const avgRun = runs.length ? runs.reduce((a, b) => a + b, 0) / runs.length : 0;
      const again = kind(log, 'serial').filter((e) => /^SEARCH no open way/.test(e.v)).length;
      return [
        check('no crashes', st.crashes === 0, `crashes at ${st.crashTimes.join(', ') || '-'}`),
        check('keeps exploring (drove at least 6 m)', st.odo >= 600, `${(st.odo / 100).toFixed(1)} m driven, ${blocked.length} obstacles, longest without driving ${st.maxStill.toFixed(1)} s`),
        check('does not drive straight back into the obstacle (blocked again after < 30 cm)', quickReblocks <= Math.max(1, runs.length / 10),
          `${quickReblocks} of ${runs.length} ways chosen were blocked again within 30 cm; average run ${avgRun.toFixed(0)} cm; looked again ${again}x; first decisions: ${turns.slice(0, 4).map((e) => e.v.replace('SEARCH ', '')).join(' | ')}`),
      ];
    },
  };
}

const ROOM = [[0, 0, 300, 0], [300, 0, 300, 220], [300, 220, 0, 220], [0, 220, 0, 0],   // walls
  [120, 80, 180, 80], [180, 80, 180, 130], [180, 130, 120, 130], [120, 130, 120, 80],   // a box in the middle
  [0, 160, 60, 220]];                                                                     // a corner cut
if (process.env.ROOM !== '0') {
  scenarios.push(roomScenario('search-room-slow-turning', { walls: ROOM, x: 60, y: 40, th: 0, turnEff: 0.5, duration: 75 }));
  scenarios.push(roomScenario('search-room-fast-turning', { walls: ROOM, x: 240, y: 170, th: 200, turnEff: 1.0, duration: 60 }));
  scenarios.push(roomScenario('search-room-medium-turning', { walls: ROOM, x: 150, y: 40, th: 90, turnEff: 0.75, duration: 60 }));
  scenarios.push(roomScenario('search-room-mirrored-servo', { walls: ROOM, x: 60, y: 40, th: 0, turnEff: 0.5, mirror: true, duration: 60 }));
  const CLUTTER = [[0, 0, 260, 0], [260, 0, 260, 200], [260, 200, 0, 200], [0, 200, 0, 0],
    [70, 60, 90, 60], [90, 60, 90, 80], [90, 80, 70, 80], [70, 80, 70, 60],            // chair legs / boxes
    [170, 40, 200, 40], [200, 40, 200, 70], [200, 70, 170, 70], [170, 70, 170, 40],
    [120, 130, 160, 130], [160, 130, 160, 160], [160, 160, 120, 160], [120, 160, 120, 130],
    [200, 140, 260, 120]];
  scenarios.push(roomScenario('search-cluttered-room', { walls: CLUTTER, x: 30, y: 30, th: 45, turnEff: 0.6, duration: 60 }));
  // A dead end: a 60 cm corridor closed in front. It must turn 90 degrees at a time until it faces the way out.
  {
    const DEAD_END = [[0, -150, 0, 200], [60, -150, 60, 200], [0, 200, 60, 200], [0, -150, 60, -150]];
    const sc = roomScenario('search-dead-end', { walls: DEAD_END, x: 30, y: 120, th: 90, turnEff: 1.04, duration: 20 });
    const base = sc.check;
    sc.check = (log) => {
      const s = kind(log, 'serial').map((e) => e.v);
      const boxed = s.filter((v) => /^SEARCH boxed in/.test(v)).length;
      const opening = s.findIndex((v) => /^SEARCH opening found/.test(v));
      const st = sc.room.state;
      const southLater = st.path.some((p) => p[2] < 40);   // drove back out of the dead end
      return [
        check('boxed in: turns 90 degrees at a time, then finds the way out and drives out', boxed >= 1 && opening >= 0 && southLater && st.crashes === 0,
          `${boxed} x 90-degree turns, opening ${opening >= 0 ? 'found' : 'not found'}, ${southLater ? 'drove out' : 'still inside'}, crashes ${st.crashes}`),
        check('never went straight back at the wall', !s.some((v) => /-> straight/.test(v)), ''),
      ];
    };
    scenarios.push(sc);
  }
}

module.exports = scenarios;
