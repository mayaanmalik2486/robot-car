'use strict';
// Minimal Arduino UNO build that mirrors arduino:avr 1.8.8 platform.txt recipes.
// usage: node build.js --sketch <file.ino> --out <dir> [--lib <libdir>]... [--define NAME[=VAL]]...
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');

const A15 = path.join(process.env.LOCALAPPDATA, 'Arduino15');
const BIN = path.join(A15, 'packages', 'arduino', 'tools', 'avr-gcc', '7.3.0-atmel3.6.1-arduino7', 'bin');
const PLAT = path.join(A15, 'packages', 'arduino', 'hardware', 'avr', '1.8.8');
const CORE = path.join(PLAT, 'cores', 'arduino');
const VARIANT = path.join(PLAT, 'variants', 'standard');
const CACHE = path.join(__dirname, '..', 'build-cache');

const MCU = ['-mmcu=atmega328p', '-DF_CPU=16000000L', '-DARDUINO=10607', '-DARDUINO_AVR_UNO', '-DARDUINO_ARCH_AVR'];
const CFLAGS = ['-c', '-g', '-Os', '-w', '-std=gnu11', '-ffunction-sections', '-fdata-sections', '-MMD', '-flto', '-fno-fat-lto-objects'];
const CPPFLAGS = ['-c', '-g', '-Os', '-w', '-std=gnu++11', '-fpermissive', '-fno-exceptions', '-ffunction-sections',
  '-fdata-sections', '-fno-threadsafe-statics', '-Wno-error=narrowing', '-MMD', '-flto'];
const SFLAGS = ['-c', '-g', '-x', 'assembler-with-cpp', '-flto', '-MMD'];

function parseArgs(argv) {
  const a = { libs: [], defines: [] };
  for (let i = 0; i < argv.length; i++) {
    const k = argv[i];
    if (k === '--sketch') a.sketch = argv[++i];
    else if (k === '--out') a.out = argv[++i];
    else if (k === '--lib') a.libs.push(argv[++i]);
    else if (k === '--define') a.defines.push(argv[++i]);
    else throw new Error('unknown arg ' + k);
  }
  if (!a.sketch || !a.out) throw new Error('need --sketch and --out');
  return a;
}

function walk(dir, out = []) {
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const p = path.join(dir, e.name);
    if (e.isDirectory()) { if (!/^(examples|extras|\.)/.test(e.name)) walk(p, out); }
    else out.push(p);
  }
  return out;
}

const isSrc = (f) => /\.(c|cpp|S)$/.test(f);

function libInfo(dir) {
  const src = path.join(dir, 'src');
  if (fs.existsSync(src)) return { name: path.basename(dir), inc: [src], srcs: walk(src).filter(isSrc) };
  const util = path.join(dir, 'utility');
  const srcs = fs.readdirSync(dir).map((f) => path.join(dir, f)).filter(isSrc);
  if (fs.existsSync(util)) srcs.push(...walk(util).filter(isSrc));
  return { name: path.basename(dir), inc: [dir], srcs };
}

function tool(name) { return path.join(BIN, name + '.exe'); }

function run(cmd, args) {
  try {
    return execFileSync(cmd, args, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'], maxBuffer: 64 * 1024 * 1024 });
  } catch (e) {
    process.stderr.write((e.stdout || '') + (e.stderr || '') + (e.message ? '\n' + e.message.slice(0, 400) + '\n' : ''));
    throw new Error('FAILED: ' + path.basename(cmd) + ' ' + args.filter((x) => !x.startsWith('-')).slice(-3).join(' '));
  }
}

function compile(src, obj, includes, extra = []) {
  const ext = path.extname(src);
  let cmd, flags;
  if (ext === '.c') { cmd = 'avr-gcc'; flags = CFLAGS; }
  else if (ext === '.S') { cmd = 'avr-gcc'; flags = SFLAGS; }
  else { cmd = 'avr-g++'; flags = CPPFLAGS; }
  fs.mkdirSync(path.dirname(obj), { recursive: true });
  const r = run(tool(cmd), [...flags, ...MCU, ...extra, ...includes.map((i) => '-I' + i), src, '-o', obj]);
  return r;
}

function buildCore() {
  const dir = path.join(CACHE, 'core');
  const ar = path.join(dir, 'core.a');
  if (fs.existsSync(ar)) return ar;
  const inc = [CORE, VARIANT];
  const objs = [];
  for (const s of walk(CORE).filter(isSrc)) {
    const o = path.join(dir, path.basename(s) + '.o');
    compile(s, o, inc);
    objs.push(o);
  }
  for (const o of objs) run(tool('avr-gcc-ar'), ['rcs', ar, o]);
  return ar;
}

// Each library is packed into one archive (lib<name>.a) so the link command stays short.
function buildLib(lib, allIncludes) {
  const dir = path.join(CACHE, 'lib-' + lib.name);
  const archive = path.join(dir, 'lib' + lib.name + '.a');
  const stamp = path.join(dir, 'archive.json');
  if (lib.srcs.length === 0) return [];                 // header-only library (e.g. IRremote)
  if (fs.existsSync(stamp)) return JSON.parse(fs.readFileSync(stamp, 'utf8'));
  fs.mkdirSync(dir, { recursive: true });
  if (fs.existsSync(archive)) fs.unlinkSync(archive);
  const objs = [];
  lib.srcs.forEach((s, i) => {
    const o = path.join(dir, i + '_' + path.basename(s) + '.o');
    compile(s, o, allIncludes);
    objs.push(o);
  });
  for (const o of objs) run(tool('avr-gcc-ar'), ['rcs', archive, o]);
  fs.writeFileSync(stamp, JSON.stringify([archive]));
  return [archive];
}

function main() {
  const a = parseArgs(process.argv.slice(2));
  fs.mkdirSync(a.out, { recursive: true });
  const libs = a.libs.map(libInfo);
  const includes = [CORE, VARIANT, ...libs.flatMap((l) => l.inc)];
  const coreA = buildCore();
  const libObjs = libs.flatMap((l) => buildLib(l, includes));

  // .ino -> .cpp the way the Arduino builder does (sketch already declares functions before use)
  const name = path.basename(a.sketch, '.ino');
  const cppPath = path.join(a.out, name + '.ino.cpp');
  const body = fs.readFileSync(a.sketch, 'utf8');
  fs.writeFileSync(cppPath, '#include <Arduino.h>\n#line 1 "' + a.sketch.replace(/\\/g, '\\\\') + '"\n' + body);
  const sketchObj = path.join(a.out, name + '.ino.cpp.o');
  const defs = a.defines.map((d) => '-D' + d);
  // compile the sketch with warnings ON so we see real problems (library header noise is filtered later)
  const warnFlags = ['-Wall', '-Wextra'];
  const flagsNoW = CPPFLAGS.filter((f) => f !== '-w');
  fs.mkdirSync(path.dirname(sketchObj), { recursive: true });
  const { spawnSync } = require('child_process');
  const sc = spawnSync(tool('avr-g++'), [...flagsNoW, ...warnFlags, ...MCU, ...defs, '-I' + path.dirname(path.resolve(a.sketch)), ...includes.map((i) => '-I' + i), cppPath, '-o', sketchObj],
    { encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
  // keep only diagnostics that point into the sketch itself (library header noise is dropped)
  const sketchDiag = (sc.stderr || '').split(/\r?\n/).filter((l) => l.includes(path.basename(a.sketch)));
  fs.writeFileSync(path.join(a.out, 'compile-stderr.txt'), sc.stderr || '');
  if (sc.status !== 0) {
    process.stderr.write(sc.stderr || '');
    throw new Error('Sketch compile FAILED');
  }
  if (sketchDiag.length) console.log('Sketch warnings:\n' + sketchDiag.join('\n'));

  const elf = path.join(a.out, name + '.elf');
  // object list goes into a response file: Windows limits command lines to 32 KB
  const rsp = path.join(a.out, 'link.rsp');
  const q = (p) => '"' + p.replace(/\\/g, '/') + '"';
  fs.writeFileSync(rsp, ['-o', q(elf), q(sketchObj), ...libObjs.map(q), q(coreA), '-L' + q(a.out), '-lm'].join('\n'));
  const linkOut = run(tool('avr-gcc'), ['-w', '-Os', '-g', '-flto', '-fuse-linker-plugin', '-Wl,--gc-sections', '-mmcu=atmega328p', '@' + rsp]);
  run(tool('avr-objcopy'), ['-O', 'ihex', '-R', '.eeprom', elf, path.join(a.out, name + '.hex')]);
  run(tool('avr-objcopy'), ['-O', 'binary', '-R', '.eeprom', elf, path.join(a.out, name + '.bin')]);
  const sizeTxt = run(tool('avr-size'), ['-A', elf]);
  const sec = {};
  for (const line of sizeTxt.split(/\r?\n/)) {
    const m = line.match(/^(\.\w+)\s+(\d+)/);
    if (m) sec[m[1]] = +m[2];
  }
  const flash = (sec['.text'] || 0) + (sec['.data'] || 0);
  const ram = (sec['.data'] || 0) + (sec['.bss'] || 0) + (sec['.noinit'] || 0);
  const syms = run(tool('avr-nm'), ['-C', '-S', '--size-sort', elf]);
  fs.writeFileSync(path.join(a.out, name + '.syms.txt'), syms);
  const symsAll = run(tool('avr-nm'), ['-C', elf]);
  fs.writeFileSync(path.join(a.out, name + '.nm.txt'), symsAll);
  const result = { flash, flashMax: 32256, flashPct: +(100 * flash / 32256).toFixed(1), ram, ramMax: 2048, ramPct: +(100 * ram / 2048).toFixed(1), sections: sec };
  fs.writeFileSync(path.join(a.out, 'size.json'), JSON.stringify(result, null, 2));
  console.log(`OK ${name}: flash ${flash} B (${result.flashPct}% of 32256), static RAM ${ram} B (${result.ramPct}% of 2048)`);
  if (linkOut.trim()) console.log(linkOut.trim());
}

try { main(); } catch (e) { console.error(e.message); process.exit(1); }
