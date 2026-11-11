'use strict';
// The Robot Car's brain for the web page: the same rules as the Android app (DriveLogic,
// ControllerMap, PadState, CarState, Layout in App\source\java), written again in JavaScript.
// Web\test-logic.js checks that both give exactly the same codes and the same layout.
(function (root) {

  // ------------------------------------------------------------------ the car's language
  const Car = {
    STOP: 'S', SLOW: 'Q', FULL: 'W', TURBO: 'T', HORN: 'Y', DANCE: '4', SEARCH: 'I', CHAOS: '1', DONUT: '3', TURN180: 'U',
    BRAKE: 'u', AUTO_FORWARD: '2', AUTO_BACKWARD: 'J', FOLLOW: 'M', KEEPER: 'K', KICK: 'k', GUARD: 'A', MUSIC: 'N',
    GESTURE: 'E', PET: 'P', RADAR: 'O', MUTE: 'X', FOOTBALL: 'V', ASK: '?', SPEED_TOGGLE: '%', WHEEL_MAX: 31,
    wheel(rightSide, speed) {
      speed = Math.max(-31, Math.min(31, speed));
      return 0x80 | (rightSide ? 0x40 : 0) | (speed + 32);
    },
    takesTheWheels(c) { return 'I12J34UMKANEPOkuT'.indexOf(c) >= 0; },
    direction(x, y) {
      if (Math.hypot(x, y) < 0.35) return 'S';
      const a = Math.atan2(-y, x) * 180 / Math.PI;
      if (a >= 67.5 && a < 112.5) return 'F';
      if (a >= 112.5 && a < 157.5) return 'G';
      if (a >= 22.5 && a < 67.5) return 'H';
      if (a >= -22.5 && a < 22.5) return 'R';
      if (a >= 157.5 || a < -157.5) return 'L';
      return 'B';
    },
  };
  const code = (c) => (typeof c === 'string' ? c.charCodeAt(0) : c);
  const f32 = Math.fround;
  const clamp = (v) => Math.max(-1, Math.min(1, v));

  // ------------------------------------------------------------------ the driving brain (DriveLogic.java)
  const WEAKEST = 11, SLOW_TOP = 18, SPIN_GAIN = 0.85, LOCK_GAIN = 1.857, WHEELS_GAP_MS = 60, LETTER_GAP_MS = 55,
    BEAT_MS = 200, STOP_AGAIN_MS = 60, MODE_DEAF_MS = 1500, TURBO_MS = 1600, FAST_ABOVE = 0.75, SLOW_BELOW = 0.65,
    RETAKE = 0.35, UNKNOWN = 99;

  // Java's Math.round for floats (rounds .5 up)
  const jround = (v) => Math.floor(f32(v) + 0.5);

  function mix(throttle, steer, slowTop, feel, out) {
    const t = f32(clamp(throttle));
    let s = f32(clamp(steer));
    s = f32(f32((1 - feel) * s) + f32(f32(f32(feel * s) * s) * s));
    const gain = f32(SPIN_GAIN + f32(f32(LOCK_GAIN - SPIN_GAIN) * Math.abs(t)));
    const l = f32(t + f32(s * gain));
    const r = f32(t - f32(s * gain));
    const big = Math.max(Math.abs(l), Math.abs(r));
    if (big < f32(0.02)) { out[0] = out[1] = 0; return; }
    const power = Math.min(1, big);
    const top = slowTop ? SLOW_TOP : 31;
    const level = f32(WEAKEST + f32((top - WEAKEST) * power));
    out[0] = jround(f32(f32(l / big) * level));
    out[1] = jround(f32(f32(r / big) * level));
  }

  function deadZone2(x, y, zone) {
    const len = f32(Math.hypot(x, y));
    if (len <= zone || len === 0) return 0;
    const scaled = Math.min(1, f32(f32(len - zone) / f32(1 - zone)));
    return f32(scaled / len);
  }

  function deadZone1(v, zone) {
    const a = Math.abs(v);
    if (a <= zone) return 0;
    return Math.sign(v) * Math.min(1, f32(f32(a - zone) / f32(1 - zone)));
  }

  function letterFor(x, y, current) {
    const plain = Car.direction(x, y);
    if (!current || current === 'S' || plain === current) return plain;
    if (plain === 'S') return Car.direction(f32(x * f32(1.17)), f32(y * f32(1.17))) === 'S' ? 'S' : current;
    const turn = 6 * Math.PI / 180;
    for (let sign = -1; sign <= 1; sign += 2) {
      const c = Math.cos(sign * turn), s = Math.sin(sign * turn);
      if (Car.direction(f32(x * c - y * s), f32(x * s + y * c)) === current) return current;
    }
    return plain;
  }

  class DriveLogic {
    constructor(send) {
      this.out = send;                 // (code: a number 0..255)
      this.steering = false;
      this.slowTop = false;
      this.feel = f32(0.6);
      this.oneStick = false;
      this.want = [0, 0];
      this.stopAt = [0, 0];
      this.reset();
    }

    setSteering(on) { if (on === this.steering) return; this.steering = on; this.forget(); }
    setSlowTop(v) { this.slowTop = v; }
    setOneStick(v) { this.oneStick = v; }
    setFeel(v) { this.feel = f32(Math.max(0, Math.min(1, v))); }
    carSpeedIs(slow) { this.sentSlow = slow ? 1 : 0; }

    reset() {
      this.throttle = this.steer = 0;
      this.sentLeft = this.sentRight = 0;
      this.sentLetter = 'S';
      this.sentSlow = UNKNOWN;
      this.fastZone = false;
      this.parked = false;
      this.stopAt[0] = this.stopAt[1] = 0;
      this.lastSendAt = this.lastBeatAt = this.lastModeAt = -100000;
    }

    driving() { return this.steering ? this.sentLeft !== 0 || this.sentRight !== 0 : this.sentLetter !== 'S' && this.sentLetter !== 0; }
    left() { return this.sentLeft === UNKNOWN ? 0 : this.sentLeft; }
    right() { return this.sentRight === UNKNOWN ? 0 : this.sentRight; }

    input(throttle, steer, now) { this.throttle = f32(clamp(throttle)); this.steer = f32(clamp(steer)); this.pump(now); }
    tick(now) { this.pump(now); }

    actionSent(letter, now) {
      if (letter === 'Q' || letter === 'W') { this.sentSlow = letter === 'Q' ? 1 : 0; return; }
      if (!Car.takesTheWheels(letter)) return;
      this.stopAt[0] = this.stopAt[1] = 0;
      if (letter !== 'u' && letter !== 'k') this.lastModeAt = now;
      this.park(letter === 'T' ? now + TURBO_MS : 0);
    }

    stopAll(now) {
      if (this.steering) { this.out(Car.wheel(false, 0)); this.out(Car.wheel(true, 0)); }
      this.out(code('S'));
      this.lastSendAt = now;
      this.stopAt[0] = now + STOP_AGAIN_MS;
      this.stopAt[1] = !this.steering && now - this.lastModeAt < MODE_DEAF_MS ? this.lastModeAt + MODE_DEAF_MS + 50 : 0;
      this.markStopped();
      this.park(0);
    }

    halt(now) {
      const was = this.driving();
      this.throttle = this.steer = 0;
      this.stopAt[0] = this.stopAt[1] = 0;
      this.parked = false;
      if (!was) { this.markStopped(); return; }
      if (this.steering) { this.out(Car.wheel(false, 0)); this.out(Car.wheel(true, 0)); }
      this.out(code('S'));
      this.out(code('S'));
      this.lastSendAt = now;
      this.markStopped();
    }

    park(until) { this.parked = true; this.parkedUntil = until; this.parkThrottle = this.throttle; this.parkSteer = this.steer; }
    markStopped() { this.sentLeft = this.sentRight = 0; this.sentLetter = 'S'; }
    forget() { this.sentLeft = this.sentRight = UNKNOWN; this.sentLetter = 0; }

    pump(now) {
      for (let i = 0; i < 2; i++)
        if (this.stopAt[i] !== 0 && now >= this.stopAt[i]) { this.stopAt[i] = 0; this.out(code('S')); this.lastSendAt = now; }
      const centred = this.throttle === 0 && this.steer === 0;
      if (this.parked) {
        if (centred) { this.parked = false; this.markStopped(); return; }
        if (this.parkedUntil !== 0 && now >= this.parkedUntil) { this.parked = false; this.forget(); }
        else if (Math.abs(f32(this.throttle - this.parkThrottle)) >= f32(RETAKE) || Math.abs(f32(this.steer - this.parkSteer)) >= f32(RETAKE)) {
          this.parked = false; this.forget();
        } else return;
      }
      if (this.steering) this.pumpWheels(now); else this.pumpLetters(now);
    }

    pumpWheels(now) {
      mix(this.throttle, this.steer, this.slowTop, this.feel, this.want);
      const l = this.want[0], r = this.want[1];
      if (l === 0 && r === 0) {
        if (this.sentLeft !== 0 || this.sentRight !== 0) {
          this.out(Car.wheel(false, 0)); this.out(Car.wheel(true, 0));
          this.sentLeft = this.sentRight = 0;
          this.lastSendAt = now;
          this.stopAt[0] = now + STOP_AGAIN_MS;
        }
        return;
      }
      this.stopAt[0] = this.stopAt[1] = 0;
      if (now - this.lastSendAt < WHEELS_GAP_MS) return;
      const beat = now - this.lastBeatAt >= BEAT_MS;
      if (!beat && l === this.sentLeft && r === this.sentRight) return;
      if (beat || l !== this.sentLeft) this.out(Car.wheel(false, l));
      if (beat || r !== this.sentRight) this.out(Car.wheel(true, r));
      if (beat) this.lastBeatAt = now;
      this.sentLeft = l; this.sentRight = r;
      this.lastSendAt = now;
    }

    pumpLetters(now) {
      const wanted = letterFor(this.steer, f32(-this.throttle), this.sentLetter);
      if (wanted === 'S') {
        this.fastZone = false;
        if (this.sentLetter !== 'S') {
          this.out(code('S')); this.sentLetter = 'S'; this.lastSendAt = now; this.stopAt[0] = now + STOP_AGAIN_MS;
        }
        return;
      }
      this.stopAt[0] = this.stopAt[1] = 0;
      if (now - this.lastSendAt < LETTER_GAP_MS) return;
      const push = this.oneStick ? Math.min(1, f32(Math.hypot(this.throttle, this.steer)))
        : wanted === 'L' || wanted === 'R' ? Math.abs(this.steer) : Math.abs(this.throttle);
      this.fastZone = push > f32(FAST_ABOVE) || this.fastZone && push > f32(SLOW_BELOW);
      const slow = this.slowTop || !this.fastZone ? 1 : 0;
      if (slow !== this.sentSlow) { this.out(code(slow === 1 ? 'Q' : 'W')); this.sentSlow = slow; this.lastSendAt = now; return; }
      if (wanted !== this.sentLetter) { this.out(code(wanted)); this.sentLetter = wanted; this.lastSendAt = now; }
    }
  }
  DriveLogic.mix = mix;
  DriveLogic.deadZone2 = deadZone2;
  DriveLogic.deadZone1 = deadZone1;

  // ------------------------------------------------------------------ the controller map (ControllerMap.java)
  const B = { A: 0, B: 1, X: 2, Y: 3, LB: 4, RB: 5, LT: 6, RT: 7, L3: 8, R3: 9, START: 10, SELECT: 11, UP: 12, RIGHT: 13, DOWN: 14, LEFT: 15 };
  const BUTTON_NAMES = ['A', 'B', 'X', 'Y', 'LB', 'RB', 'LT', 'RT', 'L3', 'R3', 'START', 'SELECT', 'D-pad up', 'D-pad right', 'D-pad down', 'D-pad left'];
  const SHORT_NAMES = ['A', 'B', 'X', 'Y', 'LB', 'RB', 'LT', 'RT', 'L3', 'R3', 'START', 'SELECT', '▲', '▶', '▼', '◀'];
  const JOBS = ['S', '4', 'k', 'Y', 'T', 'u', 'V', 'X', 'I', '1', '3', 'U', '2', 'J', 'M', 'A', 'K', 'P', 'N', 'E', 'O', 'Q', 'W', '%', '^', '-'];
  const JOB_NAMES = ['STOP', 'Dance', 'Kick', 'Horn', 'Turbo', 'Smooth brake', 'Football mode', 'Mute', 'Search', 'Chaos', 'Donut', '180 turn',
    'Auto forward', 'Auto backward', 'Follow me', 'Guard', 'Goalkeeper', 'Pet', 'Music', 'Gestures', 'Radar', 'Slow', 'Fast', 'Slow / fast', 'SHIFT (hold)', 'Nothing'];
  const DEFAULT_NORMAL = '4SkY^VuT2JOXI13U', DEFAULT_SHIFT = 'MAKP^-------NWEQ';

  class ControllerMap {
    constructor() { this.reset(); }
    reset() { this.jobs = [DEFAULT_NORMAL.split(''), DEFAULT_SHIFT.split('')]; }
    job(b, shift) {
      if (b < 0 || b >= 16) return '-';
      if (this.jobs[0][b] === '^') return '^';
      return this.jobs[shift ? 1 : 0][b];
    }
    set(b, shift, job) {
      if (b < 0 || b >= 16 || JOBS.indexOf(job) < 0) return;
      if (job === '^') { this.jobs[0][b] = this.jobs[1][b] = '^'; return; }
      if (this.jobs[0][b] === '^') this.jobs[0][b] = this.jobs[1][b] = '-';
      this.jobs[shift ? 1 : 0][b] = job;
    }
    toText() { return this.jobs[0].join('') + this.jobs[1].join(''); }
    fromText(t) {
      this.reset();
      if (typeof t !== 'string' || t.length !== 32 || [...t].some((c) => JOBS.indexOf(c) < 0)) return;
      this.jobs = [t.slice(0, 16).split(''), t.slice(16).split('')];
      for (let b = 0; b < 16; b++) if (this.jobs[0][b] === '^' || this.jobs[1][b] === '^') this.jobs[0][b] = this.jobs[1][b] = '^';
    }
    buttonFor(job) { return this.jobs[0].indexOf(job); }
    labelFor(job) {
      const b = this.buttonFor(job);
      if (b >= 0) return SHORT_NAMES[b];
      const shift = this.buttonFor('^');
      for (let i = 0; i < 16; i++) if (this.job(i, true) === job && shift >= 0) return SHORT_NAMES[shift] + '+' + SHORT_NAMES[i];
      return null;
    }
    static jobName(j) { const i = JOBS.indexOf(j); return i < 0 ? '?' : JOB_NAMES[i]; }
  }
  Object.assign(ControllerMap, { B, BUTTON_NAMES, SHORT_NAMES, JOBS, JOB_NAMES, DEFAULT_NORMAL, DEFAULT_SHIFT });

  // ------------------------------------------------------------------ what the car says (CarState.java)
  class CarState {
    constructor() { this.reset(); }
    reset() {
      this.mode = ''; this.song = ''; this.steers = false; this.muted = this.football = this.slow = this.batteryLow = false;
      this.radarCm = new Array(11).fill(999); this.radarAt = new Array(11).fill(0); this.radarLook = 5; this.modeSince = 0;
    }
    read(line, now) {
      if (line.startsWith('~')) {
        const bar = line.indexOf('|');
        if (bar < 0 || line.length < bar + 5) return 'nothing';
        const m = line.slice(1, bar).trim();
        if (m !== this.mode) this.modeSince = now;
        this.mode = m;
        [this.muted, this.football, this.slow, this.batteryLow] = [1, 2, 3, 4].map((i) => line[bar + i] === '1');
        return 'status';
      }
      if (line.startsWith('#S:')) { this.song = line.slice(3).trim(); return 'song'; }
      if (line.startsWith('#V:')) { const v = parseInt(line.slice(3), 10); if (isNaN(v)) return 'nothing'; this.steers = v >= 2; return 'version'; }
      if (line.startsWith('#R:') && line.length >= 5) {
        const k = line.charCodeAt(3) - 97;
        const cm = parseInt(line.slice(4), 10);
        if (k < 0 || k >= 11 || isNaN(cm)) return 'nothing';
        this.radarCm[k] = cm; this.radarAt[k] = now; this.radarLook = k;
        return 'radar';
      }
      return 'nothing';
    }
    radarNearest(now) {
      let best = -1;
      for (let k = 0; k < 11; k++)
        if (this.radarAt[k] && now - this.radarAt[k] < 4000 && this.radarCm[k] < 150 && (best < 0 || this.radarCm[k] < best)) best = this.radarCm[k];
      return best;
    }
  }

  // ------------------------------------------------------------------ where everything sits (Layout.java)
  const GAP = 8;
  const snap = (v) => Math.trunc(Math.fround(v) / 8) * 8;
  const box = (x, y, w, h) => ({ x, y, w, h });
  const div = (a, b) => Math.trunc(a / b);

  function screen(w, h, style) {
    const out = {};
    const two = style === 1;
    const m = w < 720 || h < 400 ? 8 : 16;
    const topH = h < 420 ? 40 : 48;
    topBar(out, m, m, w - 2 * m, topH);
    const y0 = m + topH + GAP, bodyH = h - y0 - m;
    if (w >= f32(h * f32(1.25))) wide(out, m, y0, w - 2 * m, bodyH, two); else stacked(out, m, y0, w - 2 * m, bodyH, two);
    return out;
  }

  function topBar(out, x, y, w, h) {
    const roomy = w >= 784, tight = w < 600;
    const connect = roomy ? 168 : tight ? 120 : 136, pad = roomy ? 168 : 48, chip = roomy ? 104 : tight ? 64 : 80;
    const icon = Math.max(h, 44), ih = icon;
    const song = w - (connect + pad + 2 * chip + 2 * icon + 6 * GAP);
    let right = x + w;
    const cy = y + div(h - ih, 2);
    out.connect = box(x, cy, connect, ih);
    right -= icon; out.help = box(right, cy, icon, ih);
    right -= GAP + icon; out.settings = box(right, cy, icon, ih);
    right -= GAP + chip; out.style = box(right, cy, chip, ih);
    right -= GAP + chip; out.speed = box(right, cy, chip, ih);
    right -= GAP + pad; out.pad = box(right, cy, pad, ih);
    if (song >= 96) out.song = box(x + connect + GAP, cy, song, ih);
  }

  function wide(out, x, y, w, h, two) {
    let side = snap(Math.min(f32(w * f32(0.28)), f32(h * f32(1.1))));
    side = Math.max(152, Math.min(360, side));
    let cw = w - 2 * side - 2 * GAP;
    if (cw < 256) { side = snap(f32((w - 2 * GAP - 256) / 2)); cw = w - 2 * side - 2 * GAP; }
    const cx = x + side + GAP;
    const rows = (two ? 56 : 0) + 56 + GAP;
    const pagesH = Math.max(104, Math.min(264, snap(f32(h * f32(0.36)))));
    const lcdH = Math.max(64, Math.min(400, h - rows - pagesH));
    let cy = y;
    out.lcd = box(cx, cy, cw, lcdH); cy += lcdH + GAP;
    if (two) { actionRow(out, cx, cy, cw, 48); cy += 48 + GAP; }
    tabs(out, cx, cy, cw, 48); cy += 48 + GAP;
    out.pages = box(cx, cy, cw, y + h - cy);
    const rx = x + w - side;
    out.stick = box(x, y, side, h);
    if (two) { out.stick2 = box(rx, y, side, h); return; }
    out.stop = box(rx, y, side, 48);
    diamond(out, rx, y + 48 + GAP, side, h - 48 - GAP);
  }

  function stacked(out, x, y, w, h, two) {
    const lcdH = Math.max(64, Math.min(240, snap(f32(h * f32(0.22)))));
    const stickH = Math.max(136, Math.min(320, snap(Math.min(f32(w * f32(0.5)), f32(h * f32(0.38))))));
    let cy = y;
    out.lcd = box(x, cy, w, lcdH); cy += lcdH + GAP;
    actionRow(out, x, cy, w, 48); cy += 48 + GAP;
    tabs(out, x, cy, w, 48); cy += 48 + GAP;
    const by = y + h - stickH;
    out.pages = box(x, cy, w, by - GAP - cy);
    if (two) { out.stick = box(x, by, stickH, stickH); out.stick2 = box(x + w - stickH, by, stickH, stickH); }
    else out.stick = box(x + div(w - stickH, 2), by, stickH, stickH);
  }

  function actionRow(out, x, y, w, h) {
    const unit = f32((w - 4 * GAP) / f32(5.4));
    let stop = jround(f32(unit * f32(1.4)));
    const each = div(w - 4 * GAP - stop, 4);
    stop = w - 4 * GAP - 4 * each;
    out.stop = box(x, y, stop, h);
    let bx = x + stop + GAP;
    for (const n of ['dance', 'kick', 'horn', 'turbo']) { out[n] = box(bx, y, each, h); bx += each + GAP; }
  }

  function tabs(out, x, y, w, h) {
    const each = div(w - 2 * GAP, 3);
    out.tab0 = box(x, y, each, h);
    out.tab1 = box(x + each + GAP, y, each, h);
    out.tab2 = box(x + 2 * (each + GAP), y, w - 2 * (each + GAP), h);
  }

  function diamond(out, x, y, w, h) {
    const sq = Math.min(w, h), cell = div(sq, 3), b = Math.min(cell - 4, 96);
    const cx = x + div(w, 2), cy = y + div(h, 2);
    out.horn = box(cx - div(b, 2), cy - cell - div(b, 2), b, b);
    out.kick = box(cx - cell - div(b, 2), cy - div(b, 2), b, b);
    out.turbo = box(cx + cell - div(b, 2), cy - div(b, 2), b, b);
    out.dance = box(cx - div(b, 2), cy + cell - div(b, 2), b, b);
  }

  function grid(n, w, h) {
    let bestCols = 1, best = -1;
    for (let cols = 1; cols <= n; cols++) {
      const rows = div(n + cols - 1, cols);
      const cw = f32((w - (cols - 1) * GAP) / cols), ch = f32((h - (rows - 1) * GAP) / rows);
      const score = Math.min(f32(cw / f32(1.3)), ch);
      if (score > best) { best = score; bestCols = cols; }
    }
    const cols = bestCols, rows = div(n + cols - 1, cols);
    const cw = Math.min(240, div(w - (cols - 1) * GAP, cols)), ch = Math.min(160, div(h - (rows - 1) * GAP, rows));
    return [cols, rows, cw, ch, div(w - (cols * cw + (cols - 1) * GAP), 2), div(h - (rows * ch + (rows - 1) * GAP), 2)];
  }

  const Layout = { screen, grid, GAP, ONE_STICK: 0, TWO_STICKS: 1, TILT: 2, RADAR_BUTTON: 112 };

  const api = { Car, DriveLogic, ControllerMap, CarState, Layout };
  if (typeof module !== 'undefined' && module.exports) module.exports = api; else Object.assign(root, api);
})(typeof window !== 'undefined' ? window : globalThis);
