'use strict';
// Dance engine + search "turn until open" checks.
// env SKETCH = path of the .ino that was built (the dance song is read from it)
const { parseSong, matchMotors, matchTune } = require('./dancecheck');
const { makeRoom } = require('./room');

const kind = (log, k) => log.filter((e) => e.kind === k);
const check = (name, pass, detail) => ({ name, pass: !!pass, detail });
const song = parseSong(process.env.SKETCH);
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
      const fin = kind(log, 'serial').find((e) => e.v === 'DANCE finished');
      const want = anchor + song.totalMs;
      res.push(check('stops at the end of the song', fin && fin.t >= want - 15 && fin.t <= want + 40,
        fin ? `finished ${(fin.t - want).toFixed(1)} ms after the last step's end (song ${(song.totalMs / 1000).toFixed(1)} s)` : 'never finished'));
      const after = kind(log, 'motor').filter((e) => fin && e.t > fin.t + 5);
      res.push(check('motors stay off after the end', after.length === 0, after.map((e) => e.v).join(',')));
      const st = kind(log, 'state').filter((e) => fin && e.t >= fin.t - 5);
      res.push(check('back to IDLE after the end', st.length && st[st.length - 1].v === 'IDLE', st.map((e) => e.v).join(',')));
      const sv = kind(log, 'servo');
      res.push(check('sensor head back to the centre at the end', sv.length && sv[sv.length - 1].v >= 88 && sv[sv.length - 1].v <= 92, sv.length ? 'last ' + sv[sv.length - 1].v : ''));
      const parts = kind(log, 'serial').filter((e) => /^DANCE part \d+$/.test(e.v));
      res.push(check('goes through every part of the song', parts.length === song.map.length - 1, `${parts.length + 1}/${song.map.length} parts`));
      if (song.notes.length) {
        const shouts = song.shout ? kind(log, 'serial').filter((e) => e.v === song.shout).map((e) => e.t - 5) : [];
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
      res.push(check('a new tap moves the beat (by the tap time + up to one loop)', moved && shift >= 60 && shift <= 170, moved ? moved.v : 'no tap seen'));
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
  {
    name: 'dance-ir',
    duration: DANCE_AT + 8,
    distance: () => 150,
    inputs: [{ t: DANCE_AT, ir: 0x40 }, { t: DANCE_AT + 4 * (60 / song.bpm) + 0.05, ir: 0x40 }],
    check: (log) => {
      const starts = kind(log, 'serial').filter((e) => /^DANCE to:/.test(e.v));
      const taps = kind(log, 'serial').filter((e) => /^DANCE beat moved/.test(e.v));
      return [check('remote 5 starts the dance, 5 again lines up the beat', starts.length === 1 && taps.length === 1, `${starts.length} start, ${taps.map((e) => e.v).join(',')}`)];
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
      const codes = [0x18, 0x52, 0x08, 0x5A, 0x1C, 0x45, 0x46, 0x47, 0x44, 0x40, 0x43, 0x07, 0x15, 0x09, 0x16, 0x19, 0x0D];
      const seen = new Set(kind(log, 'serial').filter((e) => /^IR button 0x/.test(e.v)).map((e) => e.v.slice(12)));
      const missing = codes.map((c) => c.toString(16).toUpperCase()).filter((h) => !seen.has(h));
      // total time spent turning left during the ~1 s hold (short gaps between repeats are fine)
      const m = kind(log, 'motor');
      let turning = 0;
      for (let i = 0; i < m.length; i++) {
        if (m[i].v !== 'LB216 RF216' || m[i].t < 3800 || m[i].t > 5000) continue;
        const end = m[i + 1] ? m[i + 1].t : m[i].t;
        turning += end - m[i].t;
      }
      return [
        check('all 17 remote buttons are decoded (NEC only)', missing.length === 0, missing.length ? 'missing ' + missing.join(',') : '17/17'),
        check('holding LEFT for ~1 s keeps turning (more than one 150 ms tap)', turning > 400, `${turning.toFixed(0)} ms of turning`),
      ];
    },
  },
  ...[
    { name: 'stop-dance-ir-up', inp: { ir: 0x18 }, want: 'MANUAL_FORWARD' },
    { name: 'stop-dance-brake', inp: { bt: 'u' }, want: 'IDLE' },
    { name: 'stop-dance-ir-180', inp: { ir: 0x45 }, want: 'IDLE' },
    { name: 'stop-search-ir-down', inp: { ir: 0x52 }, want: 'MANUAL_BACKWARD', start: 'I' },
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
    duration: DANCE_AT + 7,
    distance: () => 150,
    inputs: [{ t: 0.5, bt: 'W' }, { t: DANCE_AT, bt: '4' }, { t: DANCE_AT + 4.6, snapshot: 'feedback-screen' }],
    check: (log) => {
      const anchor = DANCE_AT * 1000 + 20;
      const bar = (60000 / song.bpm) * 4;
      const want = anchor + 2 * bar;                               // SONG_SHOUT_STEPS = {16} (= bar 2) in the test copy
      const s = kind(log, 'serial').find((e) => e.v === 'FEEDBACK!');
      const zap = kind(log, 'buzzer').filter((e) => e.t > want - 30 && e.t < want + 350 && e.v >= 1700);
      const moves = kind(log, 'motor').filter((e) => e.t > want && e.t < want + 2000);
      return [
        check('the special moment fires at the start of bar 2', s && Math.abs(s.t - want) < 80, s ? `${(s.t - want).toFixed(0)} ms from the bar start` : 'never'),
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
