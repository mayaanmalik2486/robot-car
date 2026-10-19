'use strict';
// Makes one full robot sketch per song from the main sketch: only the part
// between "SONG START" and "SONG END" (and the title line) changes.
// usage: node make-dances.js ..\results\songs.json ..\..\Dances [onlyId]
const fs = require('fs');
const path = require('path');
const CHOREO = require('./choreo');

const MAIN = path.join(__dirname, '..', '..', 'Robot_Car_Main', 'Robot_Car_Main.ino');

// folder/file name (Arduino rules: letters, digits, _ - .), name on the screen, dance style
// keepMoves: the user said these dances are perfect - same speed, no head bob
const SONGS = {
  save_me:           { folder: 'Save_Me_Remy_Zero',          screen: 'Save Me - Remy Zero',          style: 'rock' },
  national_treasure: { folder: 'National_Treasures_Drake',   screen: 'National Treasures - Drake',   style: 'hype' },
  mentalist:         { folder: 'The_Mentalist_Theme_Blake_Neely',        screen: 'The Mentalist',                style: 'mystery' },
  my_destiny:        { folder: 'My_Destiny_Slowed_Delinquent',          screen: 'My Destiny (Slowed)',          style: 'groove' },
  confidence:        { folder: 'Confidence_Kim',          screen: 'Confidence - Kim',             style: 'groove', keepMoves: true },
  distractions:      { folder: 'Distractions_Haiti_Babii',               screen: 'Distractions',                 style: 'groove' },
  ambarsariya:       { folder: 'Ambarsariya_Sona_Mohapatra',                screen: 'Ambarsariya',                  style: 'bollywood', keepMoves: true },
  night_shine:       { folder: 'The_Night_Begins_To_Shine_BER',  screen: 'The Night Begins to Shine',    style: 'hype' },
  love_me:           { folder: 'Love_Me_JMSN',               screen: 'Love Me - JMSN',               style: 'ballad' },
  loverboy:          { folder: 'Loverboy_A_Wall',                   screen: 'Loverboy',                     style: 'groove' },
  the_kill2:         { folder: 'The_Kill_2_Lex_Amarni',      screen: 'the kill 2 - Lex Amarni',      style: 'hype' },
  gods_plan:         { folder: 'Gods_Plan_Drake',            screen: "God's Plan - Drake",           style: 'groove' },
  oh_yeah:           { folder: 'Oh_Yeah_Steve_Lacy',         screen: 'oh yeah? - Steve Lacy',        style: 'groove' },
  loser:             { folder: 'Loser_Tame_Impala',          screen: 'Loser - Tame Impala',          style: 'groove' },
  all_is_well:       { folder: 'Aal_Izz_Well_3_Idiots',      screen: 'Aal Izz Well - 3 Idiots',      style: 'bollywood', sound: 'DANCE_SOUND_OFF', power: { minEnergy: 4, style: 'explosive', speed: 250 } },
  chammak_challo:    { folder: 'Chammak_Challo_Ra_One',      screen: 'Chammak Challo - Ra.One',      style: 'explosive', energyBoost: 1, chunk: 1 },
  feet_dont_fail:    { folder: 'Feet_Dont_Fail_Me_Now_Joy_Crookes',      screen: "Feet Don't Fail Me Now - Joy Crookes", style: 'hype' },
  kabhi_kabhi_aditi: { folder: 'Kabhi_Kabhi_Aditi_Rashid_Ali',          screen: 'Kabhi Kabhi Aditi',            style: 'bollywood' },
  bolve:             { folder: 'Bolve_Madan_Maddi',          screen: 'Bolve - Madan Maddi',          style: 'bollywood', power: { minEnergy: 4, style: 'explosive', speed: 250 } },
  robot_groove:      { main: true, screen: 'Robot Groove', style: 'groove' },
  badtameez_dil:     { folder: 'Badtameez_Dil_Pritam',       screen: 'Badtameez Dil - YJHD',         style: 'bollywood', power: { minEnergy: 5, style: 'explosive', speed: 250 } },
  flashing_lights:   { folder: 'Flashing_Lights_Kanye_West',      screen: 'Flashing Lights - Kanye West', style: 'groove' },
  touch_the_sky:     { folder: 'Touch_The_Sky_Kanye_West',        screen: 'Touch The Sky - Kanye West',   style: 'hype' },
  aankhen_milaney:   { folder: 'Aankhen_Milaney_Waaley_Nazia_Hassan',     screen: 'Aankhen Milaney Waaley',       style: 'hype' },
  we_are_the_people: { folder: 'We_Are_The_People_Empire_Of_The_Sun',          screen: 'We Are The People - Empire Of The Sun', style: 'hype' },
  get_lucky:         { folder: 'Get_Lucky_Drumless_Daft_Punk',         screen: 'Get Lucky (Drumless) - Daft Punk', style: 'groove' },
  greenback_boogie:  { folder: 'Greenback_Boogie_Suits_Theme',     screen: 'Suits - Greenback Boogie',     style: 'hype' },
  copacabana:        { folder: 'Copacabana_Barry_Manilow',   screen: 'Copacabana - Barry Manilow',   style: 'hype' },
  they_dont_care:    { folder: 'They_Dont_Care_About_Us_Michael_Jackson',    screen: "They Don't Care About Us - MJ", style: 'rock' },
  upside_down:       { folder: 'Upside_Down_Diana_Ross',     screen: 'Upside Down - Diana Ross',     style: 'groove' },
  my_type:           { folder: 'My_Type_Saint_Motel',        screen: 'My Type - Saint Motel',        style: 'hype' },
  baba_bandook:      { folder: 'Baba_Bandook_Ali_Azmat',               screen: 'Baba Bandook - Ali Azmat',     style: 'hype' },
  iraaday:           { folder: 'Iraaday_Rovalio',            screen: 'Iraaday - Rovalio',            style: 'groove' },
  worry:             { folder: 'Worry_LONOWN',               screen: 'worry - LONOWN',               style: 'hype' },
  sh_boom:           { folder: 'Sh_Boom_The_Chords_Cars',               screen: 'Sh-Boom - Cars (Lightning McQueen)', style: 'cruise', halfTime: true, keepMoves: true },
  ben10_og:          { folder: 'Ben_10_1_Classic_Theme',       screen: 'Ben 10 (2005)',                style: 'rock', power: { minEnergy: 5, style: 'explosive', speed: 250 }, shout: { text: 'BEN 10!', atSec: [49.5] } },
  ben10_af:          { folder: 'Ben_10_2_Alien_Force_Theme',   screen: 'Ben 10: Alien Force',          style: 'hype', power: { minEnergy: 5, style: 'explosive', speed: 250 } },
  ben10_ua:          { folder: 'Ben_10_3_Ultimate_Alien_Theme', screen: 'Ben 10: Ultimate Alien',      style: 'hype', halfTime: true, power: { minEnergy: 5, style: 'explosive', speed: 250 } },
  ben10_ov:          { folder: 'Ben_10_4_Omniverse_Theme',     screen: 'Ben 10: Omniverse',            style: 'hype', power: { minEnergy: 5, style: 'explosive', speed: 250 }, shout: { text: 'FEEDBACK!', atSec: [1.4, 11.5, 17.5] } },
  ben10_ov_kid:      { folder: 'Ben_10_5_Omniverse_Kid_Ben',   screen: 'Ben 10: Omniverse (Kid Ben)',  style: 'groove', power: { minEnergy: 5, style: 'explosive', speed: 250 }, shout: { text: 'FEEDBACK!', atSec: [1.4, 11.5, 17.5] } },
};
const SPEED_BOOST = 20;                      // the improved dances move a bit faster and quicker

const SPEED = { ballad: 180, epic: 200, mystery: 190, groove: 190, rock: 220, bollywood: 210, hype: 220, explosive: 240, cruise: 170 };

// Songs where the name could mean more than one track: say which one was used
const NOTES = {
  save_me: 'Normal speed (the 4:01 radio edit / Smallville version). The 4:44 album version has a longer ending.',
  national_treasure: 'Tempo worked out from the timed lyrics (about 130). If the moves drift, adjust SONG_BPM.',
  my_destiny: 'The Delinquent ft. KCAT slowed release. I could NOT find this song\'s key: if the buzzer tune sounds off, change SONG_KEY (try 60-71) or set DANCE_SOUND to DANCE_SOUND_OFF.',
  distractions: 'Haiti Babii "Distractions".',
  love_me: 'JMSN "Love Me". No timed lyrics exist for it, so the song parts are an estimate.',
  chammak_challo: 'Tempo worked out from the timed lyrics (2 bars per line = 134 BPM); key from a chord site. If it drifts, tap dance on a beat. Phone X mutes the buzzer.',
  ben10_og: 'The 1:01 TV opening. Works with the English AND the Hindi/Urdu version (same music and length). The title hit flashes BEN 10! on the screen.',
  ben10_af: 'The 0:31 TV opening. Works with the English AND the Hindi/Urdu version (same music and length).',
  ben10_ua: 'The 0:21 TV opening. Works with the English AND the Hindi/Urdu version. The song is very fast (170), so the robot dances one move per beat.',
  ben10_ov: 'The 0:21 TV opening. Works with the English AND the Hindi/Urdu version. FEEDBACK! flashes (with an electric zap) where Feedback appears. The song is very fast (196), so the robot dances one move per beat.',
  ben10_ov_kid: 'Opening #1 (Season 1 to S2E7 Bros in Space) - the one built around 11-year-old Ben, same song as the later Omniverse opening. FEEDBACK! flashes three times where kid-Ben Feedback appears.',
  sh_boom: 'Slow, jazzy cruising like the Cars neon-lights scene: one move per beat (the tempo here is half the song tempo on purpose).',
  bolve: 'The big bhangra parts are POWER parts: explosive moves at extra speed.',
  badtameez_dil: 'Tempo 106 BPM (songbpm; the timed lyrics agree). The choruses and the brass dance break are POWER parts: explosive moves at extra speed.',
  all_is_well: 'Calm parts at normal speed; the party parts (choruses) are POWER parts: explosive moves at extra speed. The dance is silent (DANCE_SOUND_OFF) - set DANCE_SOUND_TUNE for the tune.',
  ambarsariya: 'Played live, so the beat wobbles a little: tap dance on a beat at each chorus.',
};

// Which chords each song part uses: 0 = verse chords, 1 = chorus chords, 2 = bridge chords
const CHORD_SET = { intro: 0, verse: 0, prechorus: 0, outro: 0, chorus: 1, postchorus: 1, hook: 1, drop: 1,
  bridge: 2, breakdown: 2, instrumental: 2, solo: 2, build: 2 };

// The buzzer tune for one bar: calm parts get slow notes, the best parts get the catchy hook
function tuneFor(style, sec, k, n) {
  if (sec.label === 'build') return 'T_RISE';
  const pick = (list, every = 1) => list[Math.floor(k / every) % list.length];
  switch (sec.energy) {
    case 1: return style === 'mystery' ? 'T_DARK' : 'T_CALM';
    case 2: return style === 'mystery' ? pick(['T_DARK', 'T_CALM']) : pick(['T_CALM', 'T_FLOAT']);
    case 3: return style === 'ballad' ? pick(['T_FLOAT', 'T_ARP']) : style === 'mystery' ? pick(['T_DARK', 'T_PULSE']) : pick(['T_GROOVE', 'T_PULSE'], 2);
    case 4: return pick(['T_HOOK_A', 'T_HOOK_B']);
    default: return /hype|rock|bollywood/.test(style) ? pick(['T_HOOK_A', 'T_HOOK_B', 'T_DRIVE', 'T_HOOK_B']) : pick(['T_HOOK_A', 'T_HOOK_B', 'T_ARP', 'T_HOOK_B']);
  }
}

const PART = { intro: 'INTRO', verse: 'VERSE', prechorus: 'PRE', chorus: 'CHORUS', postchorus: 'CHORUS', hook: 'HOOK', build: 'BUILD',
  drop: 'DROP', bridge: 'BRIDGE', breakdown: 'BREAK', instrumental: 'SOLO', solo: 'SOLO', outro: 'OUTRO' };

function guessStyle(s) {
  const m = (s.mood + ' ' + s.track.title + ' ' + s.track.artist).toLowerCase();
  if (/bollywood|punjabi|bhangra|hindi|indian/.test(m)) return 'bollywood';
  if (/mysterious|crime|sly|suspense/.test(m)) return 'mystery';
  if (/orchestral|epic|film score|cinematic/.test(m)) return 'epic';
  if (/rock|metal|grunge/.test(m)) return 'rock';
  if (s.bpm < 90 && /ballad|slow|dreamy|sad|emotional|romantic|tender/.test(m)) return 'ballad';
  if (/zouk|r&b|rnb|soul|smooth|groove|chill|lo-?fi/.test(m)) return 'groove';
  return 'hype';
}

// split a long comment line into lines of at most ~90 characters
function wrap(line) {
  const words = line.replace(/^\/\/ /, '').split(' ');
  const out = [];
  let cur = '//';
  for (const w of words) {
    if (cur.length + 1 + w.length > 90 && cur !== '//') { out.push(cur); cur = '//  '; }
    cur += (cur.endsWith(' ') ? '' : ' ') + w;
  }
  out.push(cur);
  return out;
}

const mmss = (t) => `${Math.floor(t / 60)}:${String(Math.floor(t % 60)).padStart(2, '0')}`;
const plain = (s) => s.replace(/[^\x20-\x7e]/g, '');                     // for comments and the guide
const cstr = (s) => s.replace(/\\/g, '\\\\').replace(/"/g, '\\"').replace(/[^\x20-\x7e]/g, '');

// Turn the research timeline into the choreography: { step, bars, part } entries.
// Each part of the song dances its routine from choreo.js; the routine starts again
// every time that part comes back, so repeated parts get the same moves.
function buildMap(id, s, style, cfg = {}) {
  const beatsPerBar = 4;                                       // the robot always counts 4 beats per bar
  const barSec = 60 / s.bpm * beatsPerBar;
  const first = Math.max(0, s.first_beat_s || 0);
  const totalBars = Math.max(1, Math.round((s.duration_s - first) / barSec));
  const spec = CHOREO.SONGS[id] || CHOREO.SONGS.robot_groove;
  const kit = CHOREO.KITS[spec.kit];

  // section boundaries on the bar grid
  const secs = [];
  for (const sec of s.sections) {
    const a = Math.max(0, Math.round((sec.start_s - first) / barSec));
    const b = Math.min(totalBars, Math.round((sec.end_s - first) / barSec));
    if (b > a) secs.push({ label: sec.label, energy: Math.max(1, Math.min(5, sec.energy + (cfg.energyBoost || 0))), a, b, start: sec.start_s });
  }
  // close gaps so every bar belongs to a section
  if (secs.length === 0) secs.push({ label: 'verse', energy: 3, a: 0, b: totalBars, start: 0 });
  secs[0].a = 0;
  for (let i = 1; i < secs.length; i++) secs[i].a = secs[i - 1].b;
  secs[secs.length - 1].b = totalBars;
  const clean = secs.filter((x) => x.b > x.a);

  // the routine for a part: the song's own, else the kit's, following the fallbacks (hook -> chorus ...)
  const routineFor = (label, isLast) => {
    if (isLast && spec[label + '_last']) return { name: label + '_last', list: spec[label + '_last'] };
    for (const from of [spec, kit])                              // the song's own routines first, then the kit's
      for (let l = label, n = 0; l && n < 6; l = CHOREO.FALLBACK[l], n++)
        if (from[l]) return { name: l, list: from[l] };
    return { name: 'verse', list: kit.verse };
  };
  const TRANSITION = new Set(['riser', 'fill', 'fill_hit', 'build', 'button']);
  const LEAD_IN = new Set(['riser', 'fill_hit']);

  const bars = [];
  const used = {};                                              // which routine each part used (for the description)
  clean.forEach((sec, si) => {
    const n = sec.b - sec.a;
    const next = clean[si + 1];
    const isLast = !clean.slice(si + 1).some((x) => x.label === sec.label);
    const r = routineFor(sec.label, isLast);
    const key = PART[sec.label] || 'VERSE';
    (used[key + (isLast && r.name.endsWith('_last') ? ' (last time)' : '')] ||= r.list);
    const power = cfg.power && sec.energy >= cfg.power.minEnergy;
    for (let k = 0; k < n; k++) {
      let step = r.list[k % r.list.length];
      if (k === n - 1 && step === 'strut_f') step = 'swagger';      // never leave the car out front
      if (k < n - 1 && LEAD_IN.has(step)) step = 'button';          // a lead-in move only right before the next part: mid-part it's a pose
      // the last bar before a bigger part: a lead-in move
      if (k === n - 1 && n >= 2 && next && next.energy >= 4 && next.energy > sec.energy && !TRANSITION.has(step)) step = spec.fill || 'riser';
      const chords = CHORD_SET[sec.label] !== undefined ? CHORD_SET[sec.label] : 0;
      bars.push({ step, part: key + (power ? '+POWER' : ''), sec, chords, tune: tuneFor(style, sec, k, n), power });
    }
  });
  bars[bars.length - 1].step = spec.finale || 'finale';
  bars[bars.length - 1].tune = 'T_END';

  // squash repeats into { step, bars, part, chords, tune }
  const map = [];
  for (const b of bars) {
    const last = map[map.length - 1];
    if (last && last.step === b.step && last.part === b.part && last.chords === b.chords && last.tune === b.tune && last.bars < 255) last.bars++;
    else map.push({ step: b.step, bars: 1, part: b.part, chords: b.chords, tune: b.tune });
  }
  return { map, bars, beatsPerBar, barSec, totalBars, first, clean, spec, used };
}

// This song's dance steps, in order of first use, as C: name, code and description
function songSteps(spec, bars) {
  const names = [];
  for (const b of bars) if (!names.includes(b.step)) names.push(b.step);
  return names.map((n) => {
    const [text, say] = (spec.own && spec.own[n]) || CHOREO.MOVES[n];
    return { n, cname: 'M_' + n.toUpperCase(), steps: CHOREO.parseStep(n, text), say };
  });
}

function songBlock(id, s, cfg) {
  const style = cfg.style || guessStyle(s);
  const { map, bars, beatsPerBar, barSec, totalBars, first, clean, spec, used } = buildMap(id, s, style, cfg);
  const speed = Math.min(240, SPEED[style] + (cfg.keepMoves ? 0 : SPEED_BOOST));
  const powerSpeed = cfg.power ? cfg.power.speed : speed;
  const steps = songSteps(spec, bars);
  const warnings = checkSteps(id, steps, bars, s.bpm, speed, powerSpeed);
  const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
  const keyName = NOTE_NAMES[s.key % 12] + (s.minor ? ' minor' : ' major');
  const bpm = Math.round(s.bpm * 100) / 100;
  const L = [];
  L.push('// ----- SONG START (each song file has its own version of this part) -----');
  L.push(`// Song: ${plain(s.track.title)} - ${plain(s.track.artist)}`);
  L.push(...wrap(`// Version this dance is timed to (${mmss(s.duration_s)} long): ${plain(s.track.version)}`));
  if (NOTES[id]) L.push(...wrap(`// NOTE: ${NOTES[id]}`));
  L.push(`// HOW TO START: play the song, and press dance (phone 4 / remote 5) on the`);
  L.push(`// first beat${first > 0.05 ? ` - about ${first.toFixed(1)} s after the song starts` : ' - right when the song starts'}. The dance lasts the whole song`);
  L.push(`// (${totalBars} bars, about ${mmss(totalBars * barSec)}). Tap dance again on a beat to line it up if it drifts.`);
  L.push(`// Tempo: ${bpm} BPM (${s.bpm_confidence === 'high' ? 'checked in several places' : s.bpm_confidence === 'medium' ? 'fairly sure' : 'best guess'})${s.bpm_confidence !== 'high' ? ' - if the moves get ahead of the music, lower SONG_BPM a little; if they fall behind, raise it' : ''}.`);
  L.push(...wrap(`// Song parts (${s.structure_confidence === 'high' ? 'checked' : 'estimated'} - where the dance changes): ` + clean.map((c) => `${mmss(c.start)} ${PART[c.label] || 'VERSE'}`).join(', ')));
  L.push(`const char SONG_NAME[] PROGMEM = "${cstr(cfg.screen)}";`);
  L.push(`const float SONG_BPM = ${bpm};                        // beats per minute`);
  L.push(`const unsigned long SONG_FIRST_BEAT_MS = ${Math.round(first * 1000)};        // where the first beat is in the song (so the clock on the screen matches your player)`);
  L.push(`const int DANCE_SPEED = ${speed};                       // motor speed for the dance moves (0-255)`);
  L.push(`const int DANCE_POWER_SPEED = ${cfg.power ? cfg.power.speed : speed};                 // motor speed in the POWER parts (the explosive bits)`);
  const shoutAt = cfg.shout ? [].concat(cfg.shout.atSec) : [];
  const shoutSteps = [0, 1, 2].map((i) => (i < shoutAt.length ? Math.max(0, Math.round((shoutAt[i] - first) / (barSec / 8))) : -1));
  L.push(`const char SONG_SHOUT[] PROGMEM = "${cfg.shout ? cstr(cfg.shout.text) : ''}";              // a special moment: this text on the screen + a zap sound ("" = none)`);
  L.push(`const int SONG_SHOUT_STEPS[3] = { ${shoutSteps.join(', ')} };    // ... at these steps of the dance (2 steps per beat, 0 = the first step, -1 = not used)${shoutAt.length ? ` - about ${shoutAt.map(mmss).join(', ')} in the song` : ''}`);
  L.push(`const bool DANCE_HEAD_BOB = ${cfg.keepMoves ? 'false' : 'true'};                  // the sensor head also bobs left/right on the straight moves`);
  L.push(`const byte DANCE_SOUND = ${cfg.sound || 'DANCE_SOUND_TUNE'};         // buzzer: DANCE_SOUND_TUNE, DANCE_SOUND_DRUMS or DANCE_SOUND_OFF`);
  L.push('');
  L.push(`// The song's key and chords (for the buzzer tune) - ${keyName}, ${s.keySure}`);
  L.push(`const byte SONG_KEY = ${60 + (s.key % 12)};                          // key note: 60 C, 61 C#, 62 D, 63 D#, 64 E, 65 F, 66 F#, 67 G, 68 G#, 69 A, 70 A#, 71 B`);
  L.push(`const bool SONG_MINOR = ${s.minor ? 'true' : 'false'};                     // true = minor key`);
  L.push('// Chord progressions, one chord per bar, as steps of the scale (1 = the key\'s own chord)');
  L.push('const byte SONG_CHORDS[][4] PROGMEM = {');
  L.push(`    { ${s.chords.verse.join(', ')} },                                // 0 verse`);
  L.push(`    { ${s.chords.chorus.join(', ')} },                                // 1 chorus`);
  L.push(`    { ${s.chords.bridge.join(', ')} },                                // 2 bridge`);
  L.push('};');
  L.push('');
  L.push('');
  L.push('// THE CHOREOGRAPHY');
  L.push(...wrap(`// ${spec.story}`));
  L.push('// Each part dances its routine (one step per bar) and starts it again every time that part comes back:');
  for (const [part, list] of Object.entries(used)) L.push(...wrap(`//   ${part}: ${list.join(', ')}`));
  L.push('');
  L.push('// This song\'s dance steps: one bar each, 8 half-beat steps ("1 & 2 & 3 & 4 &")');
  const w = Math.max(...steps.map((x) => x.cname.length));
  steps.forEach((x, i) => L.push(`const byte ${(x.cname + ' ').padEnd(w + 1)}= ${String(i).padStart(2)};   // ${x.say}`));
  L.push('const byte SONG_MOVES[][STEPS_PER_BAR] PROGMEM = {');
  const cw = Math.max(...steps.flatMap((x) => CHOREO.stepCode(x.steps).map((c) => c.length)));
  for (const x of steps) L.push(`    { ${CHOREO.stepCode(x.steps).map((c, i) => (c + (i < 7 ? ',' : '')).padEnd(cw + 2)).join('')}},   // ${x.cname}`);
  L.push('};');
  L.push('');
  L.push('// The song from start to end: { dance step, how many bars, part name, chords, tune pattern }');
  L.push('const byte SONG_MAP[][5] PROGMEM = {');
  const sw = Math.max(...map.map((e) => ('M_' + e.step).length)) + 1;
  for (const e of map) L.push(`    { ${('M_' + e.step.toUpperCase() + ',').padEnd(sw)} ${String(e.bars).padStart(2)}, ${(e.part + ',').padEnd(13)} ${e.chords}, ${e.tune.padEnd(8)} },`);
  L.push('};');
  L.push('// ----- SONG END -----');
  return { text: L.join('\r\n'), style, speed, map, totalBars, barSec, beatsPerBar, bpm, keyName, warnings, story: spec.story, steps: steps.length };
}

// Checks one song: every ROUND spin has time to go once round before the next move,
// and the whole dance brings the car back to where it started
function checkSteps(id, steps, bars, bpm, speed, powerSpeed) {
  const warn = [];
  const stepMs = 60000 / bpm / 2;
  const turnMs = (sp) => 455 * 2 * Math.floor(255 * 85 / 100) / sp;          // fullTurnMs() in the sketch
  const powerUse = new Set(bars.filter((b) => b.power).map((b) => b.step));
  for (const x of steps) {
    x.steps.forEach((st, i) => {
      if (!st.round) return;
      let holds = 0;
      while (i + 1 + holds < 8 && x.steps[i + 1 + holds].move === 'HOLD') holds++;
      const window = (holds + 1) * stepMs;
      for (const sp of [speed].concat(powerUse.has(x.n) ? [powerSpeed] : [])) {
        const need = turnMs(st.soft ? Math.floor(sp * 0.6) : sp) + 25;
        if (window < need) warn.push(`${id}: ${x.n} round spin needs ${need.toFixed(0)} ms, has ${window.toFixed(0)} ms`);
      }
    });
  }
  const byName = Object.fromEntries(steps.map((x) => [x.n, x.steps]));
  const end = CHOREO.travel(bars.map((b) => byName[b.step]));
  if (Math.hypot(end.x, end.y) > 0.5 || Math.abs(end.a) > 0.1) warn.push(`${id}: the dance ends ${Math.hypot(end.x, end.y).toFixed(2)} steps away, turned ${(end.a * 180 / Math.PI).toFixed(0)}`);
  return warn;
}

function main() {
  const [researchFile, outRoot, only] = process.argv.slice(2);
  // accepts a plain list of songs, or a workflow output file ({ result: { songs } })
  let research = JSON.parse(fs.readFileSync(researchFile, 'utf8').replace(/^ï»¿/, ''));
  if (!Array.isArray(research)) {
    let r = research.result !== undefined ? research.result : research;
    if (typeof r === 'string') r = JSON.parse(r);
    research = r.songs;
  }
  const template = fs.readFileSync(MAIN, 'utf8');
  const nl = template.includes('\r\n') ? '\r\n' : '\n';
  const startTag = '// ----- SONG START (each song file has its own version of this part) -----';
  const endTag = '// ----- SONG END -----';
  const i0 = template.indexOf(startTag), i1 = template.indexOf(endTag);
  if (i0 < 0 || i1 < 0) throw new Error('song markers not found in the main sketch');
  const made = [];
  const warnings = [];
  for (const s of research) {
    if (only && s.id !== only) continue;
    const cfg = SONGS[s.id];
    if (!cfg) { console.log('no config for ' + s.id); continue; }
    const blk = songBlock(s.id, cfg.halfTime ? { ...s, bpm: s.bpm / 2 } : s, cfg);
    warnings.push(...blk.warnings);
    let out = template.slice(0, i0) + blk.text.replace(/\r\n/g, nl) + template.slice(i1 + endTag.length);
    out = out.replace(/ROBOT CAR  -  v9    dance song: .*/, `ROBOT CAR  -  v9    dance song: ${plain(cfg.screen)}`);
    let file = MAIN;                                        // the main sketch's own demo dance
    if (!cfg.main) {
      const dir = path.join(outRoot, cfg.folder);
      fs.mkdirSync(dir, { recursive: true });
      file = path.join(dir, cfg.folder + '.ino');
      made.push({ id: s.id, file, folder: cfg.folder, style: blk.style, speed: blk.speed, bpm: blk.bpm, bars: blk.totalBars, seconds: +(blk.totalBars * blk.barSec).toFixed(1), entries: blk.map.length, beatsPerBar: blk.beatsPerBar,
        track: `${plain(s.track.title)} - ${plain(s.track.artist)}`, length: mmss(s.duration_s), first: +(s.first_beat_s || 0).toFixed(2), bpmConf: s.bpm_confidence, structConf: s.structure_confidence, best: plain(s.best_part || ''), note: NOTES[s.id] || '', keyName: blk.keyName, story: blk.story });
    }
    fs.writeFileSync(file, out);
    console.log(`${cfg.folder || 'Robot_Car_Main'}: ${blk.style}, ${blk.bpm} BPM, ${blk.totalBars} bars (${mmss(blk.totalBars * blk.barSec)}), ${blk.steps} dance steps, ${blk.map.length} map entries`);
  }
  fs.writeFileSync(path.join(__dirname, '..', 'results', 'songs-made.json'), JSON.stringify(made, null, 1));
  if (!only) writeReadme(outRoot, made);
  console.log(warnings.length ? 'WARNINGS:\n  ' + warnings.join('\n  ') : 'no warnings');
}

// The song list next to the song folders (Dances/README.md)
function writeReadme(outRoot, made) {
  const order = Object.keys(SONGS);
  made.sort((a, b) => order.indexOf(a.id) - order.indexOf(b.id));
  const sure = (c) => (c === 'high' ? 'checked' : c === 'medium' ? 'fairly sure' : 'best guess');
  const cell = (t) => t.replace(/\|/g, '/');
  const L = [
    '# Robot Dances',
    '',
    'Each folder here is the **complete robot program** (driving, search, chaos, eyes...).',
    'The only difference between them is the song and its **choreography**. Open the `.ino` file',
    "inside the song's folder in the Arduino IDE and upload it.",
    '',
    '## How the choreography works',
    '',
    "Every song has its own dance steps (its signature moves) and a routine for each part of the song.",
    'A routine starts again every time that part comes back, so **every chorus has the same chorus',
    'routine**, like a real dance: you will recognise it. Phrases end on a pose, the bar before a big',
    'part is a lead-in move, and the last bar is a finale. Spins marked *round* go exactly once round',
    'and stop facing the front (timed like the 180 turn), and every step brings the car back to its',
    'spot, so it dances in place. The routines are written out in plain words at the top of each file',
    '(THE CHOREOGRAPHY) and the steps are in `SONG_MOVES`, so you can change them.',
    '',
    '## How to dance in time',
    '',
    '1. Put the car on the floor with some free space around it.',
    '2. Play the song on a speaker, laptop or another phone.',
    '3. Press dance (phone app: **4**, IR remote: **5**) exactly on the **first beat**',
    '   (the table says how long after the song starts that is).',
    '4. If it slowly drifts off the beat, tap dance again exactly **on** a beat.',
    '   It lines itself up with your tap (it does not restart).',
    '5. The screen shows the song name, the part (INTRO / VERSE / CHORUS / DROP!) and the',
    '   time in the song, which should match the time on your music player.',
    '6. If the moves are a bit too fast or slow for your version of the song, change',
    '   `SONG_BPM` in section 3c of that file (a little at a time).',
    '',
    "The buzzer plays an original tune in each song's key (bass notes on the beat plus a melody",
    'on top), not the real melody. **Mute:** phone **X** (the "!" button) turns every sound off,',
    'press again to turn it back on. The dance never drives forward into anything closer than 30 cm.',
    '',
    `## The songs (${made.length})`,
    '',
    '| Folder | Song (exact version the dance is timed to) | The dance | Tempo / key | Press dance | Notes |',
    '|---|---|---|---|---|---|',
    ...made.map((m) => `| \`${m.folder}\` | ${cell(m.track)}  (${m.length}) | ${cell(m.story)} | ${m.bpm} BPM (${sure(m.bpmConf)}), ${m.keyName} | ${m.first > 0.05 ? `about ${m.first} s after the song starts` : 'right when the song starts'} | ${cell(m.note)} |`),
    '',
  ];
  fs.writeFileSync(path.join(outRoot, 'README.md'), L.join('\n'));
}

main();
