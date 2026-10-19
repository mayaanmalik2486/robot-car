'use strict';
// Reads the dance song out of a sketch and predicts the exact motor states and
// their times, so a simulation log can be checked step by step.
const fs = require('fs');

function parseSong(sketchPath) {
  const src = fs.readFileSync(sketchPath, 'utf8');
  const num = (re, what) => { const m = src.match(re); if (!m) throw new Error('cannot find ' + what); return parseFloat(m[1]); };
  const block = (re, what) => { const m = src.match(re); if (!m) throw new Error('cannot find ' + what); return m[1]; };
  const bpm = num(/const float SONG_BPM = ([\d.]+)/, 'SONG_BPM');
  const bpb = src.match(/const byte BEATS_PER_BAR = (\d+)/);
  const beatsPerBar = bpb ? +bpb[1] : 4;
  const speed = num(/const int DANCE_SPEED = (\d+)/, 'DANCE_SPEED');
  const twistPct = num(/DANCE_TWIST_PERCENT = (\d+)/, 'DANCE_TWIST_PERCENT');
  const curvePct = num(/DANCE_CURVE_PERCENT = (\d+)/, 'DANCE_CURVE_PERCENT');
  const softPct = num(/DANCE_SOFT_PERCENT = (\d+)/, 'DANCE_SOFT_PERCENT');
  const turn180 = num(/const unsigned long TURN180_TIME = (\d+)/, 'TURN180_TIME');
  const name = src.match(/const char SONG_NAME\[\] PROGMEM = "([^"]*)"/)[1];
  const sm = src.match(/const char SONG_SHOUT\[\] PROGMEM = "([^"]*)"/);
  const shout = sm ? sm[1] : '';
  const ss = src.match(/SONG_SHOUT_STEPS\[3\] = \{([^}]*)\}/);
  const shoutSteps = ss ? ss[1].split(',').map((x) => +x.trim()).filter((x) => x >= 0) : [];

  const consts = {};
  for (const m of src.matchAll(/const byte (\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+);/g)) consts[m[1]] = parseInt(m[2]);
  const evalExpr = (e) => e.trim().split('+').reduce((a, t) => {
    const k = t.trim();
    if (!(k in consts)) throw new Error('unknown name in dance table: ' + k);
    return a + consts[k];
  }, 0);

  // the song's own dance steps (SONG_MOVES) and the choreography (SONG_MAP)
  const pb = block(/SONG_MOVES\[\]\[STEPS_PER_BAR\] PROGMEM = \{([\s\S]*?)\r?\n\};/, 'SONG_MOVES');
  const patterns = [...pb.matchAll(/\{([^}]*)\}/g)].map((m) => m[1].split(',').map(evalExpr));
  const mb = block(/SONG_MAP\[\]\[\d\] PROGMEM = \{([\s\S]*?)\r?\n\};/, 'SONG_MAP');
  const map = [...mb.matchAll(/\{\s*(\w+),\s*(\d+),\s*([\w+]+)(?:,\s*(\w+),\s*(\w+))?\s*\}/g)].map((m) => ({
    pattern: m[1], p: consts[m[1]], bars: +m[2], part: m[3].replace(/\+POWER$/, ''), power: /\+POWER$/.test(m[3]),
    chords: m[4] !== undefined ? +m[4] : null, tune: m[5] !== undefined ? consts[m[5]] : null }));

  // the buzzer tune (if the sketch has one)
  let tune = null;
  const tp = src.match(/TUNE_PATTERNS\[\]\[STEPS_PER_BAR\] PROGMEM = \{([\s\S]*?)\r?\n\};/);
  const sc = src.match(/SONG_CHORDS\[\]\[4\] PROGMEM = \{([\s\S]*?)\r?\n\};/);
  if (tp && sc && map[0].tune !== null) {
    tune = {
      patterns: [...tp[1].matchAll(/\{([^}]*)\}/g)].map((m) => m[1].split(',').map(evalExpr)),
      chords: [...sc[1].matchAll(/\{([^}]*)\}/g)].map((m) => m[1].split(',').map((x) => +x.trim())),
      key: num(/const byte SONG_KEY = (\d+)/, 'SONG_KEY'),
      minor: /const bool SONG_MINOR = true/.test(src),
      octaves: num(/DANCE_TUNE_OCTAVES = (\d+)/, 'DANCE_TUNE_OCTAVES'),
      bassOctaves: num(/DANCE_BASS_OCTAVES = (\d+)/, 'DANCE_BASS_OCTAVES'),
      highest: num(/DANCE_TUNE_HIGHEST = (\d+)/, 'DANCE_TUNE_HIGHEST'),
      gap: num(/DANCE_NOTE_GAP_MS = (\d+)/, 'DANCE_NOTE_GAP_MS'),
      bassMax: num(/DANCE_BASS_MAX_MS = (\d+)/, 'DANCE_BASS_MAX_MS'),
      soundTune: /const byte DANCE_SOUND = DANCE_SOUND_TUNE/.test(src),
    };
  }

  const stepUs = Math.floor(60000000.0 / (bpm * 2));       // same as the sketch (float math at compile time, then truncated)
  const stepsPerBar = beatsPerBar * 2;
  const pm = src.match(/const int DANCE_POWER_SPEED = (\d+)/);
  const powerSpeed = pm ? +pm[1] : speed;
  const C = consts;
  const fullTurnMs = (sp) => Math.floor(turn180 * 2 * Math.floor(255 * 85 / 100) / sp);    // fullTurnMs() in the sketch
  const motorAt = (move, s, backFull) => { const t = Math.floor(s * twistPct / 100), i = Math.floor(s * curvePct / 100); return ({
    [C.POSE]: 'L-0 R-0', [C.LOOK_L]: 'L-0 R-0', [C.LOOK_R]: 'L-0 R-0', [C.FREEZE]: 'L-0 R-0',
    [C.FWD]: `LF${s} RF${s}`, [C.BACK]: backFull ? 'LB255 RB255' : `LB${s} RB${s}`, [C.DASH]: 'LF255 RF255',
    [C.SPIN_L]: `LB${s} RF${s}`, [C.SPIN_R]: `LF${s} RB${s}`,
    [C.TWIST_L]: `LB${t} RF${t}`, [C.TWIST_R]: `LF${t} RB${t}`,
    [C.SWAY_L]: `LF${i} RF${s}`, [C.SWAY_R]: `LF${s} RF${i}`,
    [C.BACK_L]: `LB${i} RB${s}`, [C.BACK_R]: `LB${s} RB${i}`,
  })[move]; };

  // every step in order, with its time (ms from the start of the dance)
  const steps = [];
  let k = 0;
  map.forEach((e, partIndex) => {
    if (!patterns[e.p]) throw new Error('unknown dance step ' + e.pattern);
    for (let b = 0; b < e.bars; b++) {
      for (let st = 0; st < stepsPerBar; st++) {
        const code = patterns[e.p][st];
        const sp = e.power ? powerSpeed : speed;
        const soft = !!(code & C.SOFT);
        steps.push({ k, tMs: k * stepUs / 1000, move: code & 0x0f, soft, round: !!(code & C.ROUND), partIndex, part: e.part,
          speed: soft ? Math.floor(sp * softPct / 100) : sp });
        k++;
      }
    }
  });

  // expected motor changes (a ROUND spin stops by itself after one turn)
  const changes = [];
  let prev = 'L-0 R-0', afterDash = false, stopAt = null;
  for (const st of steps) {
    if (st.move === C.HOLD) continue;
    if (stopAt !== null && stopAt < st.tMs && prev !== 'L-0 R-0') { changes.push({ tMs: stopAt, motor: 'L-0 R-0', k: -1, roundStop: true }); prev = 'L-0 R-0'; }
    stopAt = null;
    const backFull = st.move === C.BACK && afterDash && !st.soft;
    if (![C.POSE, C.FREEZE, C.LOOK_L, C.LOOK_R].includes(st.move)) afterDash = st.move === C.DASH;
    const m = motorAt(st.move, st.speed, backFull);
    if (m !== prev) { changes.push({ tMs: st.tMs, motor: m, k: st.k }); prev = m; }
    if (st.round && (st.move === C.SPIN_L || st.move === C.SPIN_R)) stopAt = st.tMs + fullTurnMs(st.speed);
  }
  const totalMs = steps.length * stepUs / 1000;
  if (stopAt !== null && stopAt < totalMs && prev !== 'L-0 R-0') changes.push({ tMs: stopAt, motor: 'L-0 R-0', k: -1, roundStop: true });
  const partStarts = [];
  { let acc = 0; map.forEach((e, idx) => { partStarts.push({ idx, tMs: acc * stepUs / 1000 }); acc += e.bars * stepsPerBar; }); }

  // expected tune notes: same rules as playTuneStep() in the sketch
  const notes = [];
  if (tune && tune.soundTune) {
    const TOP = [4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902];
    const MAJ = [0, 2, 4, 5, 7, 9, 11], MIN = [0, 2, 3, 5, 7, 8, 10];
    let kk = 0, sectionBar = 0;
    map.forEach((e, pi) => {
      if (pi > 0) {
        const pe = map[pi - 1];
        sectionBar = (pe.part !== e.part || pe.chords !== e.chords) ? 0 : sectionBar + 1;
      }
      for (let b = 0; b < e.bars; b++) {
        if (b > 0) sectionBar++;
        const chord = tune.chords[e.chords][sectionBar % 4];
        const pat = tune.patterns[e.tune];
        for (let st = 0; st < stepsPerBar; st++, kk++) {
          const code = pat[st];
          if (code === C.TIE || code === C.SIL) continue;
          let n = 1;
          while (st + n < stepsPerBar && pat[st + n] === C.TIE) n++;
          const bass = (code & 0xf0) === C.BASS;
          const ss = (chord - 1) + (code & 0x0f);
          let midi = 60 + tune.key % 12 + 12 * Math.floor(ss / 7) + (tune.minor ? MIN : MAJ)[ss % 7];
          midi += (bass ? tune.bassOctaves : tune.octaves) * 12;
          while (midi > tune.highest) midi -= 12;
          const freq = TOP[midi % 12] >> (9 - Math.floor(midi / 12));
          let ms = n * Math.floor(stepUs / 1000) - tune.gap;
          if (bass && ms > tune.bassMax) ms = tune.bassMax;
          notes.push({ tMs: kk * stepUs / 1000, freq, ms, midi, bass });
        }
      }
    });
  }
  return { name, shout, shoutSteps, bpm, beatsPerBar, speed, stepUs, stepsPerBar, patterns, map, steps, changes, totalMs, partStarts, consts, notes };
}

// Match the predicted motor changes against a simulation log.
// anchorMs: when the dance started in the sim. shiftAfter: optional {fromMs, shiftMs}
function matchMotors(song, log, anchorMs, tolMs = 30, shiftAfter = null, untilMs = Infinity) {
  const motors = log.filter((e) => e.kind === 'motor' && e.t >= anchorMs - 5);
  let j = 0, matched = 0, worst = 0, missing = [];
  const errs = [];
  for (const c of song.changes) {
    let want = anchorMs + c.tMs;
    if (shiftAfter && c.tMs >= shiftAfter.fromMs) want += shiftAfter.shiftMs;
    if (want > untilMs) break;
    let found = -1;
    for (let q = j; q < motors.length && motors[q].t <= want + tolMs; q++) {
      if (motors[q].v === c.motor && Math.abs(motors[q].t - want) <= tolMs) { found = q; break; }
    }
    if (found >= 0) { matched++; const e = motors[found].t - want; errs.push(e); worst = Math.max(worst, Math.abs(e)); j = found + 1; }
    else missing.push(`${(want / 1000).toFixed(3)}s ${c.motor}`);
  }
  const total = matched + missing.length;
  const mean = errs.length ? errs.reduce((a, b) => a + b, 0) / errs.length : 0;
  return { matched, total, worst, mean, missing: missing.slice(0, 5) };
}

// Check the buzzer tune: around each predicted note start, the buzzer must be
// playing that pitch (the harness measures the pitch in 20 ms windows, +-5%).
function matchTune(song, log, anchorMs, untilMs = Infinity, skip = []) {
  const buz = log.filter((e) => e.kind === 'buzzer');
  let ok = 0, total = 0;
  const bad = [];
  for (const n of song.notes) {
    const t = anchorMs + n.tMs;
    if (t + n.ms > untilMs) break;
    if (skip.some((s) => t >= s - 40 && t < s + 360)) continue;     // the special-moment zap plays here instead
    total++;
    // the pitch the buzzer has a little after the note started (the log only notes changes)
    const probe = t + Math.min(45, n.ms - 5);
    let cur = null;
    for (const e of buz) { if (e.t <= probe) cur = e; else break; }
    if (cur && cur.v > 0 && Math.abs(cur.v - n.freq) / n.freq <= 0.06 && cur.t >= t - 25) ok++;
    else if (cur && cur.v > 0 && Math.abs(cur.v - n.freq) / n.freq <= 0.06) ok++;          // same note repeated: no new log entry
    else bad.push(`${(t / 1000).toFixed(3)}s want ${n.freq} got ${cur ? cur.v : '-'}`);
  }
  return { ok, total, bad: bad.slice(0, 5) };
}

module.exports = { parseSong, matchMotors, matchTune };
