'use strict';
// Replay harness: runs a compiled UNO sketch (.bin) in avr8js and feeds it
// HC-05 bytes (D3), NEC IR frames (D8) and an HC-SR04 echo (TRIG D11 -> ECHO D12).
// Records motor pins, servo angle, Serial lines, robotState, buzzer tone (A1) and OLED (I2C).
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const avrPath = process.env.AVR8JS;
if (!avrPath) throw new Error('Set AVR8JS to the absolute path of the avr8js package folder');
const avr = require(avrPath);

const F_CPU = 16000000;
const CYC_US = 16;
const R = { PINB: 0x23, PORTB: 0x25, PORTC: 0x28, PORTD: 0x2b, TCCR0A: 0x44, OCR0A: 0x47, OCR0B: 0x48 };
const SAMPLE_CYC = 4000;          // 250 us
const MOTOR_STABLE = 4;           // state must hold 1 ms to count
const STATE_STABLE = 2;

function loadProgram(binPath) {
  const prog = new Uint16Array(0x4000);
  new Uint8Array(prog.buffer).set(fs.readFileSync(binPath));
  return prog;
}

function readSymbols(nmPath) {
  const syms = {};
  if (!fs.existsSync(nmPath)) return syms;
  for (const line of fs.readFileSync(nmPath, 'utf8').split(/\r?\n/)) {
    const m = line.match(/^([0-9a-f]+)\s+[bBdD]\s+(\S+)$/);
    if (m) syms[m[2].replace(/\.lto_priv\.\d+$/, '')] = parseInt(m[1], 16) - 0x800000;
  }
  return syms;
}

// ---------- tiny PNG writer (grayscale) ----------
const CRC_TABLE = (() => { const t = new Uint32Array(256); for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; t[n] = c >>> 0; } return t; })();
function crc32(buf) { let c = 0xffffffff; for (const b of buf) c = CRC_TABLE[(c ^ b) & 0xff] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; }
function pngChunk(type, data) {
  const len = Buffer.alloc(4); len.writeUInt32BE(data.length);
  const td = Buffer.concat([Buffer.from(type, 'ascii'), data]);
  const crc = Buffer.alloc(4); crc.writeUInt32BE(crc32(td));
  return Buffer.concat([len, td, crc]);
}
function writePng(file, w, h, pixel /* (x,y)=>0..255 */) {
  const raw = Buffer.alloc((w + 1) * h);
  for (let y = 0; y < h; y++) { raw[y * (w + 1)] = 0; for (let x = 0; x < w; x++) raw[y * (w + 1) + 1 + x] = pixel(x, y); }
  const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 0; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
  fs.writeFileSync(file, Buffer.concat([Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]), pngChunk('IHDR', ihdr), pngChunk('IDAT', zlib.deflateSync(raw)), pngChunk('IEND', Buffer.alloc(0))]));
}

// ---------- SSD1306 model on the I2C bus ----------
class Ssd1306 {
  // avr8js finishes I2C steps instantly; we delay each step by the real bus time
  // (9 clock pulses per byte at the speed set by the sketch) so drawing costs real time.
  constructor(cpu, twi, onTraffic) {
    this.cpu = cpu; this.twi = twi; this.onTraffic = onTraffic;
    this.fb = new Uint8Array(128 * 8);
    this.addr = -1; this.first = true; this.ctrl = 0;
    this.mode = 2; this.col = 0; this.page = 0; this.c0 = 0; this.c1 = 127; this.p0 = 0; this.p1 = 7;
    this.pending = []; this.need = 0; this.bytes = 0;
  }
  bits(n) { return Math.max(1, Math.round(n * F_CPU / this.twi.sclFrequency)); }
  later(n, fn) { this.cpu.addClockEvent(fn, this.bits(n)); }
  start() { this.first = true; this.later(1, () => this.twi.completeStart()); }
  stop() { this.later(1, () => this.twi.completeStop()); }
  connectToSlave(addr) { this.addr = addr; this.first = true; this.later(9, () => this.twi.completeConnect(addr === 0x3c)); }
  readByte() { this.later(9, () => this.twi.completeRead(0xff)); }
  writeByte(v) {
    this.bytes++; this.onTraffic();
    if (this.addr !== 0x3c) { this.later(9, () => this.twi.completeWrite(false)); return; }
    if (this.first) { this.ctrl = v; this.first = false; }
    else if (this.ctrl & 0x40) this.data(v);
    else this.command(v);
    this.later(9, () => this.twi.completeWrite(true));
  }
  data(v) {
    this.fb[this.page * 128 + this.col] = v;
    if (this.mode === 2) { this.col = (this.col + 1) & 127; }
    else if (this.mode === 0) { if (++this.col > this.c1) { this.col = this.c0; if (++this.page > this.p1) this.page = this.p0; } }
    else { if (++this.page > this.p1) { this.page = this.p0; if (++this.col > this.c1) this.col = this.c0; } }
  }
  command(v) {
    if (this.need > 0) {
      this.pending.push(v); this.need--;
      if (this.need === 0) this.finish();
      return;
    }
    const withArgs = { 0x20: 1, 0x21: 2, 0x22: 2, 0x81: 1, 0x8d: 1, 0xa8: 1, 0xd3: 1, 0xd5: 1, 0xd9: 1, 0xda: 1, 0xdb: 1, 0x26: 6, 0x27: 6, 0x29: 5, 0x2a: 5, 0xa3: 2 };
    if (withArgs[v]) { this.pending = [v]; this.need = withArgs[v]; return; }
    if (v >= 0xb0 && v <= 0xb7) this.page = v & 7;
    else if (v <= 0x0f) this.col = (this.col & 0xf0) | v;
    else if (v >= 0x10 && v <= 0x1f) this.col = ((v & 0x0f) << 4) | (this.col & 0x0f);
  }
  finish() {
    const [c, a, b] = this.pending;
    if (c === 0x20) this.mode = a & 3;
    else if (c === 0x21) { this.c0 = a & 127; this.c1 = b & 127; this.col = this.c0; }
    else if (c === 0x22) { this.p0 = a & 7; this.p1 = b & 7; this.page = this.p0; }
  }
  pixel(x, y) { return (this.fb[(y >> 3) * 128 + x] >> (y & 7)) & 1; }
  savePng(file) {
    const S = 4;
    writePng(file, 128 * S + 8, 64 * S + 8, (x, y) => {
      const px = Math.floor((x - 4) / S), py = Math.floor((y - 4) / S);
      if (px < 0 || py < 0 || px >= 128 || py >= 64) return 40;
      return this.pixel(px, py) ? 235 : 10;
    });
  }
}

// ---------- the simulation ----------
function simulate(build, scenario, opts = {}) {
  const cpu = new avr.CPU(loadProgram(build.bin), 2048);
  new avr.AVRTimer(cpu, avr.timer0Config);
  new avr.AVRTimer(cpu, avr.timer1Config);
  new avr.AVRTimer(cpu, avr.timer2Config);
  const portB = new avr.AVRIOPort(cpu, avr.portBConfig);
  const portC = new avr.AVRIOPort(cpu, avr.portCConfig);
  const portD = new avr.AVRIOPort(cpu, avr.portDConfig);
  const usart = new avr.AVRUSART(cpu, avr.usart0Config, F_CPU);
  const twi = new avr.AVRTWI(cpu, avr.twiConfig, F_CPU);
  new avr.AVRADC(cpu, avr.adcConfig);   // analogRead() (used for randomSeed)
  const syms = readSymbols(build.nm);

  const log = [];
  const now = () => cpu.cycles / F_CPU;
  const add = (kind, v, cyc = cpu.cycles) => log.push({ t: +(cyc / F_CPU * 1000).toFixed(3), kind, v });

  // event queue (cycles)
  const q = [];
  const at = (t, fn) => { let lo = 0, hi = q.length; while (lo < hi) { const m = (lo + hi) >> 1; if (q[m].t <= t) lo = m + 1; else hi = m; } q.splice(lo, 0, { t, fn }); };

  // idle input levels
  portB.setPin(0, true);    // IR receiver output idles HIGH
  portD.setPin(3, true);    // HC-05 TXD idles HIGH
  portB.setPin(4, false);   // ECHO idles LOW
  if (scenario.statePin !== undefined) portC.setPin(2, !!scenario.statePin);   // optional HC-05 STATE on A2

  // Serial Monitor text
  let line = '';
  usart.onByteTransmit = (b) => {
    if (b === 10) { add('serial', line.replace(/\r$/, '')); line = ''; }
    else line += String.fromCharCode(b);
  };

  // OLED
  let i2cBytes = 0, i2cLastLogged = -1;
  const oled = new Ssd1306(cpu, twi, () => { i2cBytes++; });
  if (scenario.oled !== false) twi.eventHandler = oled;

  // servo: commanded angle read from the Servo library's RAM (servos[0].ticks, 0.5 us per tick, +2 us trim)
  const servoAddr = syms.servos;
  let servoLogged = null;
  const servoAngleNow = () => {
    if (servoAddr === undefined) return 90;
    const ticks = cpu.data[servoAddr + 1] | (cpu.data[servoAddr + 2] << 8);
    if (ticks === 0) return 90;
    return Math.round((ticks / 2 + 2 - 544) * 180 / (2400 - 544));
  };
  // trigger (D11 = PB3)
  let trigRise = null, sonarBusy = false;
  portB.addListener((value, old) => {
    const ch = value ^ old;
    if (ch & 0x08) {
      if (value & 0x08) trigRise = cpu.cycles;
      else if (trigRise !== null && cpu.cycles - trigRise >= 8 * CYC_US && !sonarBusy) {
        sonarBusy = true;
        const d = scenario.distance(now(), servoAngleNow(), (name) => (syms[name] !== undefined ? cpu.data[syms[name]] : undefined), motor());
        const width = (d === null || d >= 400) ? 38000 : d * 58.31;
        const start = cpu.cycles + 460 * CYC_US;
        at(start, () => portB.setPin(4, true));
        at(start + Math.round(width * CYC_US), () => { portB.setPin(4, false); sonarBusy = false; });
      }
    }
  });

  // buzzer (A1 = PC1): frequency from edge count per 20 ms window
  let buzEdges = 0, buzWinStart = 0, buzLogged = 0;
  portC.addListener((value, old) => { if ((value ^ old) & 0x02) buzEdges++; });
  const BUZ_WIN = 20 * 16000;

  // inputs
  const BIT = F_CPU / 9600;
  for (const inp of scenario.inputs) {
    const t0 = Math.round(inp.t * F_CPU);
    if (inp.bt !== undefined) {
      let tc = t0;
      for (const chr of inp.bt) {
        const b = chr.charCodeAt(0);
        const lv = [0, ...[0, 1, 2, 3, 4, 5, 6, 7].map((i) => (b >> i) & 1), 1];
        lv.forEach((l, i) => at(tc + Math.round(i * BIT), () => portD.setPin(3, !!l)));
        tc += Math.round(11 * BIT);
      }
    }
    if (inp.ir !== undefined) {
      const frame = (ts, seq) => { let tc = ts; for (const [l, us] of seq) { const lvl = !!l; at(tc, () => portB.setPin(0, lvl)); tc += Math.round(us * CYC_US); } at(tc, () => portB.setPin(0, true)); };
      const seq = [[0, 9000], [1, 4500]];
      for (const by of [0x00, 0xff, inp.ir & 0xff, ~inp.ir & 0xff]) for (let i = 0; i < 8; i++) seq.push([0, 560], [1, (by >> i) & 1 ? 1690 : 560]);
      seq.push([0, 560]);
      frame(t0, seq);
      for (let r = 1; r <= (inp.repeats || 0); r++) frame(t0 + Math.round(r * 108000 * CYC_US), [[0, 9000], [1, 2250], [0, 560]]);
    }
    if (inp.statePin !== undefined) at(t0, () => portC.setPin(2, !!inp.statePin));
    if (inp.snapshot !== undefined) {
      // wait until the screen has been quiet for 15 ms (a frame is complete), then take the picture
      let lastBytes = -1, quietSince = 0;
      const tryShot = () => {
        if (i2cBytes !== lastBytes) { lastBytes = i2cBytes; quietSince = cpu.cycles; }
        if (cpu.cycles - quietSince >= 15 * 16000) {
          if (opts.snapDir) oled.savePng(path.join(opts.snapDir, `${build.label}-${scenario.name}-${inp.snapshot}.png`));
          add('snapshot', inp.snapshot);
        } else at(cpu.cycles + 16000, tryShot);
      };
      at(t0, tryShot);
    }
  }

  // motor + state sampling
  let mCand = null, mN = 0, mSince = 0, mLogged = null;
  let svCand = null, svN = 0, svSince = 0;
  let sCand = null, sN = 0, sSince = 0, sLogged = null;
  const stateAddr = syms.robotState;
  const stateNames = scenario.stateNames || build.stateNames;
  const motor = () => {
    const d = cpu.data, pd = d[R.PORTD], pb = d[R.PORTB], tc = d[R.TCCR0A];
    const in1 = (pd >> 2) & 1, in2 = (pd >> 4) & 1, in3 = (pd >> 7) & 1, in4 = (pb >> 2) & 1;
    const ena = tc & 0x20 ? d[R.OCR0B] : ((pd >> 5) & 1) * 255;
    const enb = tc & 0x80 ? d[R.OCR0A] : ((pd >> 6) & 1) * 255;
    const L = in1 === 0 && in2 === 1 ? 'F' : in1 === 1 && in2 === 0 ? 'B' : in1 === 0 && in2 === 0 ? '-' : 'X';
    const Rr = in3 === 1 && in4 === 0 ? 'F' : in3 === 0 && in4 === 1 ? 'B' : in3 === 0 && in4 === 0 ? '-' : 'X';
    return `L${L}${ena} R${Rr}${enb}`;
  };
  const sample = () => {
    if (servoAddr !== undefined && cpu.data[servoAddr] & 0x40) {   // only once attach() marked it active
      const ang = servoAngleNow();
      if (ang === svCand) svN++; else { svCand = ang; svN = 1; svSince = cpu.cycles; }
      if (svN === STATE_STABLE && ang !== servoLogged) { servoLogged = ang; add('servo', ang, svSince); }
    }
    const m = motor();
    if (scenario.onSample) scenario.onSample(now(), m, servoAngleNow());   // e.g. move a virtual car around a room
    if (m === mCand) mN++; else { mCand = m; mN = 1; mSince = cpu.cycles; }
    if (mN === MOTOR_STABLE && m !== mLogged) { mLogged = m; add('motor', m, mSince); }
    if (stateAddr !== undefined) {
      const s = cpu.data[stateAddr];
      if (s === sCand) sN++; else { sCand = s; sN = 1; sSince = cpu.cycles; }
      if (sN === STATE_STABLE && s !== sLogged) { sLogged = s; add('state', stateNames ? (stateNames[s] || s) : s, sSince); }
    }
    if (cpu.cycles - buzWinStart >= BUZ_WIN) {
      const f = Math.round(buzEdges / 2 / ((cpu.cycles - buzWinStart) / F_CPU));
      const q10 = f < 60 ? 0 : f;
      if ((q10 === 0) !== (buzLogged === 0) || (q10 && Math.abs(q10 - buzLogged) / buzLogged > 0.04)) { buzLogged = q10; add('buzzer', q10, buzWinStart); }
      buzEdges = 0; buzWinStart = cpu.cycles;
    }
    if (i2cBytes !== i2cLastLogged && opts.logI2c) { /* aggregated below */ }
  };

  // I2C activity windows (for "no OLED traffic during the 180 turn" checks)
  const i2cWindows = [];
  let lastI2c = i2cBytes, winStart = null;

  const end = Math.round(scenario.duration * F_CPU);
  let nextSample = 0, nextI2cCheck = 0;
  const t0 = Date.now();
  let minSP = 0xffff, minSPAt = 0, minSPpc = 0;
  const trace = [];
  const watch = [];
  const traceFrom = opts.traceFrom ? opts.traceFrom * F_CPU : Infinity, traceTo = opts.traceTo ? opts.traceTo * F_CPU : 0;
  while (cpu.cycles < end) {
    // Real AVR rule (avr8js doesn't model it): after an instruction turns
    // interrupts ON (SEI, OUT SREG, RETI), one more instruction runs before
    // any interrupt. GCC's stack-pointer update relies on this.
    const iBefore = cpu.data[95] & 0x80;
    avr.avrInstruction(cpu);
    if (!iBefore && (cpu.data[95] & 0x80)) {
      cpu.data[95] &= 0x7f;
      cpu.tick();
      cpu.data[95] |= 0x80;
    } else {
      cpu.tick();
    }
    const sp = cpu.SP;
    if (sp < minSP) { minSP = sp; minSPAt = cpu.cycles; minSPpc = cpu.pc * 2; }
    if (opts.watchPc && cpu.pc * 2 === opts.watchPc && watch.length < 5000) {
      const w = [cpu.cycles / F_CPU, sp];
      for (const nm of opts.watchVars || []) { const a = syms[nm]; w.push(a === undefined ? null : cpu.data[a] | (cpu.data[a + 1] << 8)); }
      watch.push(w);
    }
    if (cpu.cycles >= traceFrom && cpu.cycles <= traceTo && (trace.length === 0 || trace[trace.length - 1][1] !== cpu.pc * 2)) {
      if (trace.length < 400000) trace.push([cpu.cycles, cpu.pc * 2, sp]);
    }
    if (q.length !== 0 && q[0].t <= cpu.cycles) { while (q.length !== 0 && q[0].t <= cpu.cycles) q.shift().fn(); }
    if (cpu.cycles >= nextSample) {
      sample();
      nextSample += SAMPLE_CYC;
      if (cpu.cycles >= nextI2cCheck) {
        if (i2cBytes !== lastI2c) { if (winStart === null) winStart = cpu.cycles; lastI2c = i2cBytes; }
        else if (winStart !== null) { i2cWindows.push([+(winStart / 16000).toFixed(1), +(cpu.cycles / 16000).toFixed(1)]); winStart = null; }
        nextI2cCheck += 16000; // 1 ms
      }
    }
  }
  if (opts.snapDir && scenario.oled !== false) oled.savePng(path.join(opts.snapDir, `${build.label}-${scenario.name}-end.png`));
  // where is the CPU at the end? (byte address, stack pointer, a few return addresses on the stack)
  const stack = [];
  for (let a = cpu.SP + 1; a < Math.min(cpu.SP + 40, cpu.data.length - 1); a += 1) stack.push(cpu.data[a]);
  return { log, i2cWindows, i2cBytes, wallMs: Date.now() - t0, simMs: scenario.duration * 1000, pc: cpu.pc * 2, sp: cpu.SP, sreg: cpu.data[95], stack, minSP, minSPAt, minSPpc, trace, watch };
}

module.exports = { simulate, readSymbols };
