'use strict';
// The Robot Car web remote: the same rules and controller map as the Android app, for Chrome
// or Edge on a computer (and Chrome on Android). It talks to the car's HC-05 with Web Serial;
// the GameSir Cyclone 2 (or any controller) comes in through the Gamepad API, and the
// keyboard works too. The picture: a live 3D car (scene.js), a steering wheel and a throttle
// lever, glossy bubble buttons.
/* global Car, DriveLogic, ControllerMap, CarState, Layout, Scene */
(() => {
  const $ = (id) => document.getElementById(id);
  const SPP = '00001101-0000-1000-8000-00805f9b34fb';
  const APP_LINK = 'https://github.com/mayaanmalik2486/robot-car/raw/main/App/RobotCar.apk';
  const TAU = Math.PI * 2;
  const now = () => performance.now();
  const store = {
    get(k, d) { try { const v = localStorage.getItem('rc:' + k); return v === null ? d : JSON.parse(v); } catch (e) { return d; } },
    set(k, v) { try { localStorage.setItem('rc:' + k, JSON.stringify(v)); } catch (e) { /* private window */ } },
  };
  const S = {
    style: store.get('style2', 1), slowTop: store.get('slowTop', false), feel: store.get('feel', 0.6), touchDead: store.get('touchDead', 0.08),
    padDead: store.get('padDead', 0.12), leftSteers: store.get('leftSteers', false), overlay: store.get('overlay', true),
  };
  const save = (k, v) => { S[k] = v; store.set(k === 'style' ? 'style2' : k, v); };
  const css = (name) => getComputedStyle(document.documentElement).getPropertyValue(name).trim();
  const FONT = css('--font'), MONO = css('--mono');
  const C = { text: '#F4F6FF', label: '#9AA6D0', faint: '#5B678F', accent: '#FF7A1A', accent2: '#FFB52E', cyan: '#2EE6D6', stop: '#FF3B5C' };

  // ------------------------------------------------------------------ the background bubbles
  (() => {
    const bg = $('bg');
    for (let i = 0; i < 14; i++) {
      const b = document.createElement('i'), size = 6 + Math.random() * 18;
      b.style.cssText = `left:${Math.random() * 100}%;width:${size}vmin;height:${size}vmin;--h:${[24, 174, 205, 325, 45][i % 5]};--x:${(Math.random() * 16 - 8).toFixed(1)}vw;`
        + `--d:${(16 + Math.random() * 20).toFixed(1)}s;--w:${(-Math.random() * 30).toFixed(1)}s`;
      bg.appendChild(b);
    }
  })();

  // ------------------------------------------------------------------ the car link (Web Serial)
  const car = new CarState();
  let port = null, writer = null, reader = null, connecting = false, connected = false, heardStatus = false, heardSong = false, connectedAt = 0;
  let lastSent = '', lastWheels = '';
  const log = [];
  const addLog = (s) => { log.push(s); while (log.length > 60) log.shift(); };
  let writing = Promise.resolve();
  const sendByte = (b) => {
    if (!writer) return;
    writing = writing.then(() => writer.write(new Uint8Array([b]))).catch(() => {});
  };
  const sign = (v) => (v > 0 ? '+' : '') + v;
  const code = (c) => c.charCodeAt(0);

  const drive = new DriveLogic((c) => {
    if (!connected) return;
    sendByte(c);
    if (c >= 0x80) {
      const w = `L${sign(drive.left())} R${sign(drive.right())}`;
      if (w !== lastWheels) { lastWheels = w; addLog('-> ' + w); }
      lastSent = w;
    } else { lastWheels = ''; addLog('-> ' + String.fromCharCode(c)); lastSent = String.fromCharCode(c); }
  });
  drive.setFeel(S.feel);
  drive.setSlowTop(S.slowTop);

  async function connect(pick) {
    if (!('serial' in navigator)) {
      message('This browser can\'t talk to the car', '<p>Use <b>Chrome</b> or <b>Edge</b> on a computer (Windows, Mac, Linux or a Chromebook), '
        + 'or Chrome on an Android phone. Safari and Firefox can\'t do Bluetooth serial, and neither can an iPhone.</p>'
        + '<p>You can still try everything here: the 3D car follows the wheel, the keyboard and a controller.</p>'
        + '<p>On a phone, the Android app is the best way: <a style="color:var(--accent2)" href="' + APP_LINK + '">download it here</a>.</p>');
      return;
    }
    if (connected || connecting) return;
    connecting = true; render();
    try {
      const ports = await navigator.serial.getPorts();
      port = !pick && ports.length ? ports[0] : await navigator.serial.requestPort({ allowedBluetoothServiceClassIds: [SPP] });
      await port.open({ baudRate: 9600 });
      writer = port.writable.getWriter();
      connected = true; connecting = false; heardStatus = heardSong = false; connectedAt = now();
      car.reset(); drive.reset(); drive.setSteering(false);
      addLog('(connected)');
      sendByte(code('?'));
      setTimeout(() => connected && sendLetter(S.slowTop ? 'Q' : 'W'), 400);
      setTimeout(checkCarProgram, 3000);
      readLoop();
    } catch (e) {
      connecting = false; connected = false;
      if (e && e.name !== 'NotFoundError')            // (NotFoundError = the list was closed without picking)
        message('Couldn\'t connect', '<p>' + esc(e.message || String(e)) + '</p><p>Is the car switched on and close by? On Windows, pair <b>HC-05</b> '
          + 'first in Settings &gt; Bluetooth (PIN 1234 or 0000), then pick it here (it may show as a COM port).</p>');
    }
    render();
  }

  async function readLoop() {
    let line = '';
    try {
      reader = port.readable.getReader();
      for (;;) {
        const { value, done } = await reader.read();
        if (done) break;
        for (const b of value) {
          if (b === 10) { onLine(line.trim()); line = ''; } else if (line.length < 64) line += String.fromCharCode(b);
        }
      }
    } catch (e) { /* lost */ }
    const wasOn = connected;
    await disconnect(true);
    if (wasOn) message('Connection lost', '<p>The Bluetooth link to the car broke - maybe it is out of range, or its battery is empty. The car stops by itself when the app goes quiet.</p>',
      [['Reconnect', () => connect(false)]]);
  }

  async function disconnect(lost) {
    if (!lost && connected) drive.halt(now());
    connected = false; connecting = false;
    try { await writing; } catch (e) { /* */ }
    try { if (reader) await reader.cancel(); } catch (e) { /* */ }
    try { if (writer) writer.releaseLock(); } catch (e) { /* */ }
    try { if (port) await port.close(); } catch (e) { /* */ }
    reader = writer = null;
    drive.reset();
    render();
  }

  function checkCarProgram() {
    if (!connected || heardStatus) return;
    render();
    message('Old car program', '<p>The car is connected, but it doesn\'t answer - so it is running an older program. Driving works (with letters), '
      + 'but smooth steering, the play modes, the radar and the speed button need the newest one.</p>'
      + '<p>Upload <b>Robot_Car_Main</b> (or a song from the Dances folder) from Documents\\Arduino\\robot-car.</p>');
  }

  function onLine(line) {
    const oldMode = car.mode, wasLow = car.batteryLow;
    const kind = car.read(line, now());
    if (kind !== 'radar') addLog('<- ' + line);
    if (kind === 'radar' || kind === 'nothing') return;
    if (kind === 'status') { heardStatus = true; drive.carSpeedIs(car.slow); }
    if (kind === 'song') heardSong = true;
    if (kind === 'version') drive.setSteering(car.steers);
    if (car.mode === 'RADAR' && oldMode !== 'RADAR') showPage(2);
    if (car.mode === 'INTRUDER' && oldMode !== 'INTRUDER') { message('🚨 Intruder!', '<p>Guard mode saw someone.</p>'); rumble(1200, 1); }
    if (car.batteryLow && !wasLow) { message('🪫 Low battery', '<p>The car\'s battery is nearly empty - charge it soon.</p>'); rumble(500, 0.7); }
    if (car.mode === 'BLOCKED' && oldMode !== 'BLOCKED') rumble(60, 0.4);
    render();
  }

  function sendLetter(c) {
    if (!connected) { notConnected(); return; }
    sendByte(code(c));
    addLog('-> ' + c); lastSent = c; lastWheels = '';
    drive.actionSent(c, now());
    if (c === 'T') rumble(120, 0.8);
    render();
  }

  let warnedAt = 0;
  function notConnected() {
    if (now() - warnedAt < 4000) return;
    warnedAt = now();
    toast('Not connected yet - the 3D car shows what would happen. Click Connect to drive the real one.');
  }

  // a mode tried without a car: the 3D car plays it for a moment, so you see what the button does
  let demoMode = '', demoUntil = 0;
  const DEMO = { 4: 'DANCE', 3: 'DONUT', T: 'TURBO!', I: 'SEARCH', 1: 'CHAOS!', U: 'TURN 180', 2: 'AUTO FWD', J: 'AUTO BCK', O: 'RADAR', M: 'FOLLOW', P: 'PET' };

  function doJob(job) {
    if (!connected && DEMO[job]) { demoMode = DEMO[job]; demoUntil = now() + (job === 'T' ? 1500 : job === 'U' ? 700 : 3500); }
    if (!connected && job === 'S') { demoUntil = 0; }
    switch (job) {
      case 'S': if (!connected) return notConnected(); drive.stopAll(now()); break;
      case '%': setSlowTop(!S.slowTop); break;
      case 'Q': setSlowTop(true); break;
      case 'W': setSlowTop(false); break;
      case 'O':
        if (car.mode === 'RADAR') { doJob('S'); return; }
        sendLetter('O'); showPage(2); break;
      default: sendLetter(job);
    }
    render();
  }

  function setSlowTop(slow) {
    save('slowTop', slow);
    drive.setSlowTop(slow);
    if (connected) sendLetter(slow ? 'Q' : 'W');
    render();
  }

  // ------------------------------------------------------------------ driving: touch / mouse, keyboard, controller
  const SRC_TOUCH = 0, SRC_KEYS = 1, SRC_PAD = 2;
  const source = [[0, 0], [0, 0], [0, 0]];
  let active = -1;
  const wanted = [0, 0];                                     // throttle, steer of whoever drives now
  function setSource(src, t, s) {
    source[src] = [t, s];
    if (t !== 0 || s !== 0) active = src;
    else if (active === src) { active = -1; source.forEach((v, i) => { if (v[0] !== 0 || v[1] !== 0) active = i; }); }
    const v = active < 0 ? [0, 0] : source[active];
    wanted[0] = v[0]; wanted[1] = v[1];
    drive.setOneStick(active === SRC_TOUCH && S.style !== 1);
    drive.input(v[0], v[1], now());
  }

  // the keyboard: WASD or the arrows (they ramp up smoothly), space = STOP
  const keys = new Set();
  let keyT = 0, keyS = 0;
  const KEYJOBS = { ' ': 'S', Enter: '4', h: 'Y', k: 'k', t: 'T', b: 'u', r: 'O', m: 'X', f: 'V', q: '%' };
  addEventListener('keydown', (e) => {
    if (document.querySelector('dialog[open]')) return;
    const k = e.key.length === 1 ? e.key.toLowerCase() : e.key;
    if (['w', 'a', 's', 'd', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(k)) { keys.add(k); e.preventDefault(); return; }
    if (KEYJOBS[k] && !e.repeat) { e.preventDefault(); flashJob(KEYJOBS[k]); doJob(KEYJOBS[k]); }
  });
  addEventListener('keyup', (e) => keys.delete(e.key.length === 1 ? e.key.toLowerCase() : e.key));
  addEventListener('blur', () => { keys.clear(); drive.halt(now()); setSource(SRC_KEYS, 0, 0); });
  document.addEventListener('visibilitychange', () => { if (document.hidden) { drive.halt(now()); for (let i = 0; i < 3; i++) source[i] = [0, 0]; active = -1; wanted[0] = wanted[1] = 0; } });
  function keyboardTick(dt) {
    const wantT = (keys.has('w') || keys.has('ArrowUp') ? 1 : 0) - (keys.has('s') || keys.has('ArrowDown') ? 1 : 0);
    const wantS = (keys.has('d') || keys.has('ArrowRight') ? 1 : 0) - (keys.has('a') || keys.has('ArrowLeft') ? 1 : 0);
    const step = dt / 350;                                   // 0 -> full in 0.35 s
    const ramp = (v, w) => (w === 0 ? 0 : Math.abs(w - v) <= step ? w : v + Math.sign(w - v) * step);
    keyT = ramp(keyT, wantT); keyS = ramp(keyS, wantS);
    if (keyT !== source[SRC_KEYS][0] || keyS !== source[SRC_KEYS][1]) setSource(SRC_KEYS, keyT, keyS);
  }

  // the game controller (Gamepad API "standard" layout: the Cyclone 2 in XInput mode, Xbox, PlayStation ...)
  const map = new ControllerMap();
  map.fromText(store.get('map', null));
  const STD = [0, 1, 2, 3, 4, 5, 6, 7, 11, 10, 8, 9, 12, 14, 15, 13];   // our button -> the Gamepad API's button index
  const pad = { index: -1, id: '', down: new Array(16).fill(false), trig: [0, 0], lx: 0, ly: 0, rx: 0, ry: 0, home: false, t: 0, s: 0 };
  let remapJob = null, heldShift = -1, otherPressed = false;
  addEventListener('gamepadconnected', (e) => { toast('🎮 ' + shortName(e.gamepad.id) + ' connected'); render(); });
  addEventListener('gamepaddisconnected', (e) => {
    if (e.gamepad.index === pad.index) { pad.index = -1; pad.down.fill(false); setSource(SRC_PAD, 0, 0); drive.halt(now()); toast('Controller disconnected - the car stopped'); }
    render();
  });
  const shortName = (id) => (id || 'Controller').replace(/\s*\(.*$/, '').slice(0, 22) || 'Controller';
  const shiftHeld = () => pad.down.some((d, b) => d && map.job(b, false) === '^');

  function padTick() {
    const list = navigator.getGamepads ? [...navigator.getGamepads()].filter(Boolean) : [];
    const gp = list.find((g) => g.index === pad.index) || list.find((g) => g.mapping === 'standard') || list[0];
    if (!gp) { if (pad.index >= 0) { pad.index = -1; render(); } return; }
    if (gp.index !== pad.index) { pad.index = gp.index; pad.id = gp.id; render(); }
    const btn = (i) => gp.buttons[i] || { pressed: false, value: 0 };
    for (let b = 0; b < 16; b++) {
      let d;
      if (b === 6 || b === 7) {                              // triggers: on past 60 %, off under 30 %
        const v = btn(STD[b]).value || (btn(STD[b]).pressed ? 1 : 0);
        pad.trig[b - 6] = v;
        d = pad.down[b] ? v > 0.3 : v >= 0.6;
      } else d = btn(STD[b]).pressed;
      if (d !== pad.down[b]) { pad.down[b] = d; onPadButton(b, d); }
    }
    pad.home = btn(16).pressed;
    pad.lx = gp.axes[0] || 0; pad.ly = gp.axes[1] || 0; pad.rx = gp.axes[2] || 0; pad.ry = gp.axes[3] || 0;
    const kl = DriveLogic.deadZone2(pad.lx, pad.ly, S.padDead), kr = DriveLogic.deadZone2(pad.rx, pad.ry, S.padDead);
    const t = Math.fround(-pad.ly * kl);
    let s = Math.fround(pad.rx * kr);
    if (s === 0 && S.leftSteers) s = Math.fround(pad.lx * kl);
    if (t !== pad.t || s !== pad.s) {
      pad.t = t; pad.s = s;
      if (!$('controller').open) setSource(SRC_PAD, t, s);
    }
    if ($('controller').open) drawPad();
  }

  function onPadButton(b, down) {
    if ($('controller').open) {
      if (!remapJob) return;
      const isShift = map.job(b, false) === '^';
      if (down) {
        if (isShift && remapJob !== '^' && heldShift < 0) { heldShift = b; otherPressed = false; return; }
        otherPressed = true;
        assign(b, heldShift >= 0 && b !== heldShift);
      } else if (b === heldShift) { if (!otherPressed) assign(b, false); heldShift = -1; }
      return;
    }
    if (!down) return;
    const job = map.job(b, shiftHeld());
    if (job === '^' || job === '-') return;
    flashJob(job);
    doJob(job);
  }

  function rumble(ms, strength) {
    const list = navigator.getGamepads ? [...navigator.getGamepads()].filter(Boolean) : [];
    const gp = list.find((g) => g.index === pad.index);
    try { gp && gp.vibrationActuator && gp.vibrationActuator.playEffect('dual-rumble', { duration: ms, strongMagnitude: strength, weakMagnitude: strength }); } catch (e) { /* */ }
  }

  // ------------------------------------------------------------------ the screen
  const screen = $('screen');
  const el = {};
  const jobButtons = {};
  const icons = {
    gear: '<svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"/><path d="M12 2.8v2.4M12 18.8v2.4M2.8 12h2.4M18.8 12h2.4M5.5 5.5l1.7 1.7M16.8 16.8l1.7 1.7M5.5 18.5l1.7-1.7M16.8 7.2l1.7-1.7"/><circle cx="12" cy="12" r="6.4"/></svg>',
    help: '<svg class="icon" viewBox="0 0 24 24"><path d="M9 9a3 3 0 1 1 4.2 2.8c-.8.4-1.2 1-1.2 1.9V14"/><circle cx="12" cy="18" r=".6" fill="currentColor"/></svg>',
    pad: '<svg class="icon" viewBox="0 0 24 24"><rect x="2.5" y="7" width="19" height="11" rx="5.5"/><path d="M7 12.5h3M8.5 11v3"/><circle cx="15.5" cy="11.5" r=".8" fill="currentColor"/><circle cx="17.5" cy="13.5" r=".8" fill="currentColor"/></svg>',
  };
  let order = 0;
  function put(name, node) {
    node.classList.add('rise');
    node.style.setProperty('--i', order++);
    screen.appendChild(node);
    el[name] = node;
    return node;
  }
  function key(name, html, onClick, cls, hue) {
    const b = document.createElement('button');
    b.className = 'key' + (cls ? ' ' + cls : '');
    if (hue !== undefined) b.style.setProperty('--h', hue);
    b.innerHTML = html;
    b.addEventListener('click', onClick);
    return put(name, b);
  }
  function addJob(job, b) { (jobButtons[job] = jobButtons[job] || []).push(b); }
  const two = (big, small) => `<span>${big}</span><small>${small}</small>`;
  // every key: a ripple from where it was pressed
  document.addEventListener('pointerdown', (e) => {
    const k = e.target.closest && e.target.closest('.key');
    if (!k) return;
    const r = k.getBoundingClientRect(), s = document.createElement('span');
    s.className = 'ripple';
    s.style.left = (e.clientX - r.left) + 'px'; s.style.top = (e.clientY - r.top) + 'px';
    k.appendChild(s);
    setTimeout(() => s.remove(), 600);
  });

  key('connect', '', () => (connected ? confirmDisconnect() : connect(false)), 'c', 24);
  put('song', Object.assign(document.createElement('div'), { id: 'song' }));
  key('pad', '', () => openController());
  key('speed', '', () => doJob('%'));
  key('style', '', () => setStyle(S.style === 1 ? 0 : 1));
  key('settings', icons.gear, () => openSettings()).title = 'Settings';
  key('help', icons.help, () => openHelp()).title = 'Help';

  put('lcd', Object.assign(document.createElement('canvas'), { id: 'lcd' }));
  el.lcd.addEventListener('click', () => { if (!connected && !connecting) connect(false); });
  el.lcd.addEventListener('contextmenu', (e) => { e.preventDefault(); message('Link log (-> sent, <- from the car)', '<pre>' + esc(log.join('\n') || 'Nothing yet.') + '</pre>'); });
  el.lcd.title = 'Right-click: the link log';

  put('tabtrack', Object.assign(document.createElement('div'), { id: 'tabtrack' }));
  put('tabglow', Object.assign(document.createElement('div'), { id: 'tabglow' }));
  ['MOVES', 'PLAY', 'RADAR'].forEach((n, i) => key('tab' + i, `<span>${n}</span>`, () => showPage(i), 'tab'));
  put('pages', document.createElement('div'));
  const PAGES = [
    [['🔍', 'Search', 'I', 210], ['😈', 'Chaos', '1', 275], ['🍩', 'Donut', '3', 330], ['↩️', '180°', 'U', 186], ['⏫', 'Auto', '2', 145], ['⏬', 'Reverse', 'J', 38], ['🅿️', 'Brake', 'u', 355], ['📯', 'Horn', 'Y', 48]],
    [['🚶', 'Follow me', 'M', 150], ['🥅', 'Goalkeeper', 'K', 215], ['🚨', 'Guard', 'A', 2], ['🎵', 'Music', 'N', 265], ['✋', 'Gestures', 'E', 30], ['🐶', 'Pet', 'P', 320]],
  ];
  const pageEls = PAGES.map((items) => {
    const p = document.createElement('div');
    p.className = 'page';
    items.forEach(([emoji, word, job, hue], k) => {
      const b = document.createElement('button');
      b.className = 'key mode';
      b.style.setProperty('--h', hue); b.style.setProperty('--k', k);
      b.innerHTML = `<span class="chip">${emoji}</span><span>${word}</span>`;
      b.addEventListener('click', () => doJob(job));
      addJob(job, b);
      p.appendChild(b);
    });
    el.pages.appendChild(p);
    return p;
  });
  const radarPage = document.createElement('div');
  radarPage.className = 'page';
  el.radar = Object.assign(document.createElement('canvas'), { id: 'radar' });
  radarPage.appendChild(el.radar);
  el.radarB = document.createElement('button');
  el.radarB.className = 'key c'; el.radarB.style.setProperty('--h', 174); el.radarB.style.setProperty('--k', 1);
  el.radarB.addEventListener('click', () => doJob('O'));
  addJob('O', el.radarB);
  radarPage.appendChild(el.radarB);
  el.pages.appendChild(radarPage);
  pageEls.push(radarPage);

  put('stick', document.createElement('canvas'));
  put('stick2', document.createElement('canvas'));
  [['stop', 'STOP', 'S', 'c stop', 350], ['dance', 'Dance', '4', 'c', 325], ['kick', 'Kick', 'k', 'c', 215], ['horn', 'Horn', 'Y', 'c dark-text', 46], ['turbo', 'Turbo', 'T', 'c dark-text', 172]]
    .forEach(([n, label, job, cls, hue]) => addJob(job, key(n, `<span>${label}</span>`, () => doJob(job), cls, hue)));

  let page = 0;
  function showPage(i) {
    page = i;
    pageEls.forEach((p, k) => p.classList.toggle('show', k === i));
    [0, 1, 2].forEach((k) => el['tab' + k].classList.toggle('on', k === i));
    layout();
  }

  function setStyle(s) {
    drive.halt(now());
    for (let i = 0; i < 3; i++) source[i] = [0, 0];
    active = -1; wanted[0] = wanted[1] = 0;
    save('style', s === 1 ? 1 : 0);                            // (tilt is the phone app's)
    ctl[0].x = ctl[0].y = ctl[1].x = ctl[1].ang = 0;
    layout();
    render();
  }

  const dpr = () => window.devicePixelRatio || 1;
  function place(node, b) {
    if (!b) { node.style.display = 'none'; return; }
    node.style.display = '';
    Object.assign(node.style, { left: b.x + 'px', top: b.y + 'px', width: b.w + 'px', height: b.h + 'px' });
    if (node.tagName === 'CANVAS') { node.width = Math.round(b.w * dpr()); node.height = Math.round(b.h * dpr()); }
  }

  let boxes = {};
  function layout() {
    const W = window.innerWidth, H = window.innerHeight;
    boxes = Layout.screen(W, H, S.style);
    for (const n of ['connect', 'song', 'pad', 'speed', 'style', 'settings', 'help', 'lcd', 'tab0', 'tab1', 'tab2', 'pages', 'stick', 'stick2', 'stop', 'dance', 'kick', 'horn', 'turbo'])
      place(el[n], boxes[n]);
    const t0 = boxes.tab0, t2 = boxes.tab2, tc = boxes['tab' + page];
    place(el.tabtrack, { x: t0.x, y: t0.y, w: t2.x + t2.w - t0.x, h: t0.h });
    place(el.tabglow, { x: tc.x + 4, y: tc.y + 4, w: tc.w - 8, h: tc.h - 8 });
    for (const n of ['dance', 'kick', 'horn', 'turbo']) el[n].classList.toggle('round', Math.abs(boxes[n].w - boxes[n].h) < 4);
    const P = boxes.pages;
    pageEls.forEach((p, i) => {
      Object.assign(p.style, { position: 'absolute', inset: '0' });
      if (i < 2) {
        const g = Layout.grid(p.children.length, P.w, P.h);
        [...p.children].forEach((b, k) => {
          place(b, { x: g[4] + (k % g[0]) * (g[2] + 8), y: g[5] + Math.floor(k / g[0]) * (g[3] + 8), w: g[2], h: g[3] });
          const row = g[3] < 66 && g[2] >= 124;              // low and wide: the picture beside the name
          b.classList.toggle('row', row);
          b.style.setProperty('--chip', Math.round(Math.max(18, Math.min(46, row ? g[3] * 0.6 : g[3] * 0.42))) + 'px');
          b.style.fontSize = (g[2] < 84 ? 10.5 : g[2] < 110 ? 12.5 : 15) + 'px';
          b.style.padding = g[2] < 100 ? '0 2px 3px' : '';
        });
      } else {
        const side = P.w > P.h * 1.6;
        const bw = side ? Layout.RADAR_BUTTON : P.w, bh = side ? P.h : 48;
        place(el.radar, { x: 0, y: 0, w: side ? P.w - bw - 8 : P.w, h: side ? P.h : P.h - bh - 8 });
        place(el.radarB, { x: P.w - bw, y: P.h - bh, w: bw, h: bh });
      }
    });
    el.pad.innerHTML = boxes.pad && boxes.pad.w > 100 ? '' : icons.pad;
    render();
  }
  addEventListener('resize', layout);

  // what the keys show
  function render() {
    el.connect.innerHTML = connected ? two('● HC-05', 'connected') : connecting ? two('Connecting', 'pick the car') : two('Connect', 'to the car');
    el.connect.classList.toggle('c', !connected);
    el.connect.classList.toggle('on', connected);
    el.song.textContent = connected && car.song ? '♪ ' + car.song : '';
    const gp = pad.index >= 0;
    if (boxes.pad && boxes.pad.w > 100) el.pad.innerHTML = gp ? two(esc(shortName(pad.id)), 'controller') : two('No controller', 'plug one in');
    el.pad.classList.toggle('on', gp);
    el.speed.innerHTML = two(S.slowTop ? 'Slow' : 'Fast', 'top speed');
    el.speed.classList.toggle('on', S.slowTop);
    el.style.innerHTML = two(S.style === 1 ? 'Wheel' : '1 stick', 'driving');
    const scanning = car.mode === 'RADAR';
    el.radarB.innerHTML = `<span>${scanning ? 'Stop radar' : 'Start radar'}</span>`;
    const running = runningJob();
    const show = S.overlay && gp;
    for (const [job, list] of Object.entries(jobButtons))
      for (const b of list) {
        if (b !== el.stop && b !== el.radarB) b.classList.toggle('on', connected && job === running);
        let badge = b.querySelector('.badge');
        const label = show ? map.labelFor(job) : null;
        if (label && !badge) { badge = document.createElement('span'); badge.className = 'badge'; b.appendChild(badge); }
        if (badge) { if (label) { badge.textContent = label; badge.style.color = padColor(label); } else badge.remove(); }
      }
  }

  function runningJob() {
    const m = car.mode;
    if (/^LOOK|^TURN [LR]$|^SEARCH$|^BOXED IN$/.test(m)) return 'I';
    return { 'CHAOS!': '1', DONUT: '3', 'TURN 180': 'U', 'AUTO FWD': '2', 'AUTO BCK': 'J', BRAKING: 'u', DANCE: '4', 'TURBO!': 'T', FOLLOW: 'M', KEEPER: 'K', 'SAVE!': 'K',
      GUARD: 'A', ARMING: 'A', INTRUDER: 'A', MUSIC: 'N', GESTURE: 'E', PET: 'P', 'Zzz...': 'P', BORED: 'P', 'PURR <3': 'P' }[m] || '';
  }

  const padColor = (l) => ({ A: '#5BE37D', B: '#FF6B81', X: '#5AA8FF', Y: '#FFD23F' }[l] || '#9AA6D0');

  function flashJob(job) {
    const list = job === '%' || job === 'Q' || job === 'W' ? [el.speed] : jobButtons[job] || [];
    for (const b of list) { b.classList.add('flash'); setTimeout(() => b.classList.remove('flash'), 160); }
  }

  // ------------------------------------------------------------------ the 3D scene with its head-up display
  const scene = new Scene();
  const mixOut = [0, 0];
  let headline = '', headlineOld = '', headlineAt = 0;
  function rrect(g, x, y, w, h, r) { r = Math.min(r, w / 2, h / 2); g.beginPath(); g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r); g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath(); }
  function ellipsize(g, s, w) { if (g.measureText(s).width <= w) return s; while (s.length > 1 && g.measureText(s + '…').width > w) s = s.slice(0, -1); return s + '…'; }

  function sceneMode() {
    if (connected) return car.mode;
    return now() < demoUntil ? demoMode : '';
  }

  function drawScene(dt) {
    const c = el.lcd, g = c.getContext('2d'), d = dpr(), w = c.width / d, h = c.height / d;
    if (!w) return;
    // what the wheels do: the real speeds when the car steers, otherwise what the controls ask for
    let l, r;
    if (connected && drive.steering) { l = drive.left() / 31; r = drive.right() / 31; }
    else { DriveLogic.mix(wanted[0], wanted[1], S.slowTop, S.feel, mixOut); l = mixOut[0] / 31; r = mixOut[1] / 31; }
    const mode = sceneMode();
    const t = now();
    scene.update(dt / 1000, l, r, mode, (car.radarLook - 5) / 5);

    g.setTransform(d, 0, 0, d, 0, 0);
    g.clearRect(0, 0, w, h);
    g.save();
    rrect(g, 0, 0, w, h, 22); g.clip();
    // the sky: night blue, with a warm glow where the floor ends
    const sky = g.createLinearGradient(0, 0, 0, h);
    sky.addColorStop(0, '#0A1030'); sky.addColorStop(0.5, '#141F52'); sky.addColorStop(0.56, '#2A2F6E'); sky.addColorStop(1, '#070B1A');
    g.fillStyle = sky; g.fillRect(0, 0, w, h);
    const glow = g.createRadialGradient(w / 2, h * 0.5, 0, w / 2, h * 0.5, w * 0.6);
    glow.addColorStop(0, mode === 'BLOCKED' ? 'rgba(255,60,100,.30)' : 'rgba(255,122,26,.26)'); glow.addColorStop(1, 'rgba(255,122,26,0)');
    g.fillStyle = glow; g.fillRect(0, 0, w, h);
    let radarDots = null;
    if (mode === 'RADAR') {
      radarDots = [];
      for (let k = 0; k < 11; k++) {
        const age = t - car.radarAt[k];
        if (connected && car.radarAt[k] && age < 6000 && car.radarCm[k] < 150) radarDots.push({ a: (k - 5) / 5, d: car.radarCm[k] / 150, f: Math.max(0.15, 1 - age / 6000) });
      }
    }
    scene.draw(g, w, h, mode, radarDots, 174);

    // ----- the head-up display
    const pad = Math.max(10, Math.min(18, h * 0.12));
    let top, big, flags = [], warn = false;
    if (!connected) {
      top = connecting ? 'CONNECTING…' : 'NOT CONNECTED · CLICK TO CONNECT';
      big = now() < demoUntil ? demoMode : (wanted[0] || wanted[1]) ? 'TEST DRIVE' : 'ROBOT CAR';
    } else {
      const old = !heardStatus && t - connectedAt > 3000;
      top = 'CONNECTED' + (!heardStatus ? '' : drive.steering ? ' · STEERING' : ' · LETTERS (OLD CAR PROGRAM)');
      if (old) { top = 'OLD CAR PROGRAM · UPLOAD ROBOT_CAR_MAIN'; warn = true; } else if (heardStatus && !heardSong) top += ' · NO RADAR';
      big = modeText();
      flags = [[car.muted ? 'MUTED' : 'SOUND', !car.muted], [car.football ? 'SENSOR OFF' : 'SENSOR', !car.football],
        [((drive.steering || !heardStatus) ? S.slowTop : car.slow) ? 'SLOW' : 'FAST', true], [car.batteryLow ? 'LOW BATTERY' : 'BATTERY', !car.batteryLow]];
    }
    // the status chip (top left)
    const small = Math.max(9.5, Math.min(12, h * 0.085));
    g.font = `600 ${small}px ${MONO}`; g.textBaseline = 'middle'; g.textAlign = 'left';
    const chipText = ellipsize(g, top, w - pad * 2 - 34);
    const cw = g.measureText(chipText).width + small * 2.6, chh = small * 2.1;
    rrect(g, pad, pad, cw, chh, chh / 2); g.fillStyle = 'rgba(4,7,20,.55)'; g.fill();
    g.fillStyle = warn ? '#FFC857' : connected ? '#3BE38B' : connecting ? C.accent2 : '#FF6B81';
    const blink = connected ? 1 : 0.55 + 0.45 * Math.sin(t / 260);
    g.globalAlpha = blink; g.beginPath(); g.arc(pad + small * 1.05, pad + chh / 2, small * 0.38, 0, TAU); g.fill(); g.globalAlpha = 1;
    g.fillStyle = warn ? '#FFC857' : C.text; g.fillText(chipText, pad + small * 1.9, pad + chh / 2 + 0.5);
    if (connected && lastSent && w > 420) {
      g.textAlign = 'right'; g.fillStyle = C.label; g.fillText('sent ' + lastSent, w - pad - (drive.steering ? 22 : 0), pad + chh / 2 + 0.5); g.textAlign = 'left';
    }
    // the headline (bottom left): slides in when it changes
    if (big !== headline) { headlineOld = headline; headline = big; headlineAt = t; }
    const bigSize = Math.max(22, Math.min(h * 0.30, w * 0.11, 84));
    const flagH = flags.length && h > 110 ? small * 2.0 + 8 : 0;
    const baseY = h - pad - flagH;
    const k = Math.min(1, (t - headlineAt) / 260), ease = 1 - Math.pow(1 - k, 3);
    g.textBaseline = 'alphabetic';
    const drawBig = (text, dy, alpha) => {
      if (!text || alpha <= 0) return;
      g.font = `900 ${bigSize}px ${FONT}`;
      try { g.fontStretch = 'extra-condensed'; } catch (e) { /* older browsers */ }
      g.globalAlpha = alpha;
      const m = /^(.*?)(\s+[\d:%]+.*)?$/.exec(text), a = m[1], b = m[2] || '';
      const maxW = w - pad * 2 - (drive.steering && connected ? 26 : 0);
      let size = bigSize;
      const total = g.measureText(a + b).width;
      if (total > maxW) { size *= maxW / total; g.font = `900 ${size}px ${FONT}`; }
      g.fillStyle = 'rgba(0,0,0,.35)'; g.fillText(a + b, pad + 1, baseY + dy + 2);
      g.fillStyle = C.text; g.fillText(a, pad, baseY + dy);
      g.fillStyle = C.accent2; g.fillText(b, pad + g.measureText(a).width, baseY + dy);
      g.globalAlpha = 1;
    };
    drawBig(headlineOld, -bigSize * 0.5 * ease, 1 - ease);
    drawBig(headline, bigSize * 0.5 * (1 - ease), ease);
    try { g.fontStretch = 'normal'; } catch (e) { /* */ }
    // the little switches under it
    if (flagH) {
      g.font = `600 ${small}px ${MONO}`; g.textBaseline = 'middle';
      let x = pad;
      for (const [name, good] of flags) {
        const fw = g.measureText(name).width + small * 2.3;
        if (x + fw > w - pad) break;
        rrect(g, x, h - pad - small * 2.0, fw, small * 2.0, small); g.fillStyle = 'rgba(4,7,20,.5)'; g.fill();
        g.fillStyle = good ? C.cyan : '#FF6B81'; g.beginPath(); g.arc(x + small * 0.95, h - pad - small, small * 0.3, 0, TAU); g.fill();
        g.fillStyle = good ? C.label : '#FFB3C0'; g.fillText(name, x + small * 1.6, h - pad - small + 0.5);
        x += fw + 6;
      }
    }
    // how hard each side of the car works (right edge)
    if (h > 84) {
      const bw = 7, bh = Math.min(h - pad * 2 - chh - 12, 150), by = h - pad - bh, bx = w - pad - bw * 2 - 5;
      [[l, bx, 'L'], [r, bx + bw + 5, 'R']].forEach(([v, x]) => {
        rrect(g, x, by, bw, bh, bw / 2); g.fillStyle = 'rgba(4,7,20,.55)'; g.fill();
        const len = bh / 2 * Math.min(1, Math.abs(v));
        if (len > 1) {
          const grad = g.createLinearGradient(0, by, 0, by + bh);
          grad.addColorStop(0, C.accent2); grad.addColorStop(0.5, C.accent); grad.addColorStop(1, '#7C8CFF');
          g.fillStyle = grad; g.shadowColor = v > 0 ? C.accent : '#7C8CFF'; g.shadowBlur = 10;
          rrect(g, x, v > 0 ? by + bh / 2 - len : by + bh / 2, bw, len, bw / 2); g.fill(); g.shadowBlur = 0;
        }
        g.fillStyle = 'rgba(255,255,255,.5)'; g.fillRect(x - 2, by + bh / 2 - 0.5, bw + 4, 1);
      });
    }
    g.restore();
  }

  function modeText() {
    if (!car.mode) return '…';
    if (car.mode === 'DANCE') { const s = Math.floor((now() - car.modeSince) / 1000); return `DANCE ${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`; }
    if (car.mode === 'RADAR') { const n = car.radarNearest(now()); return 'RADAR' + (n >= 0 ? ` ${n} cm` : ''); }
    if (car.mode === 'DRIVE' && drive.steering) return `DRIVE ${Math.round(100 * Math.max(Math.abs(drive.left()), Math.abs(drive.right())) / 31)}%`;
    return car.mode;
  }

  // ------------------------------------------------------------------ the radar page (seen from above)
  function drawRadar() {
    const c = el.radar, g = c.getContext('2d'), d = dpr(), w = c.width / d, h = c.height / d;
    if (!w || page !== 2) return;
    g.setTransform(d, 0, 0, d, 0, 0);
    g.clearRect(0, 0, w, h);
    rrect(g, 0, 0, w, h, 18); g.fillStyle = 'rgba(4,7,20,.6)'; g.fill();
    g.strokeStyle = 'rgba(255,255,255,.1)'; g.lineWidth = 1; g.stroke();
    const pad = 10, cx = w / 2, cy = h - pad, r = Math.min(w / 2 / Math.sin(70 * Math.PI / 180), h - pad * 2.5);
    g.font = `600 10px ${MONO}`; g.textAlign = 'left'; g.textBaseline = 'alphabetic';
    for (let k = 1; k <= 3; k++) {
      g.strokeStyle = 'rgba(46,230,214,.35)'; g.beginPath(); g.arc(cx, cy, r * k / 3, -160 * Math.PI / 180, -20 * Math.PI / 180); g.stroke();
      g.fillStyle = 'rgba(46,230,214,.7)'; g.fillText(k * 50 + ' cm', cx + 4, cy - r * k / 3 + 12);
    }
    const fan = g.createRadialGradient(cx, cy, 0, cx, cy, r);
    fan.addColorStop(0, 'rgba(46,230,214,.30)'); fan.addColorStop(1, 'rgba(46,230,214,.04)');
    g.fillStyle = fan; g.beginPath(); g.moveTo(cx, cy); g.arc(cx, cy, r, -140 * Math.PI / 180, -40 * Math.PI / 180); g.fill();
    g.fillStyle = C.accent; g.shadowColor = C.accent; g.shadowBlur = 12; g.beginPath(); g.arc(cx, cy, 6, 0, TAU); g.fill(); g.shadowBlur = 0;
    const t = now();
    let live = false;
    for (let k = 0; k < 11; k++) {
      const age = t - car.radarAt[k];
      if (!car.radarAt[k] || age > 6000) continue;
      live = live || age < 1000;
      if (car.radarCm[k] >= 150) continue;
      const a = (5 - k) * 10 * Math.PI / 180, dd = r * car.radarCm[k] / 150;
      g.globalAlpha = Math.max(0.15, 1 - age / 6000);
      g.fillStyle = '#9FF5EA'; g.shadowColor = C.cyan; g.shadowBlur = 14;
      g.beginPath(); g.arc(cx + dd * Math.sin(a), cy - dd * Math.cos(a), 5.5, 0, TAU); g.fill();
    }
    g.globalAlpha = 1; g.shadowBlur = 0;
    if (live) {
      const a = (5 - car.radarLook) * 10 * Math.PI / 180;
      g.strokeStyle = C.cyan; g.lineWidth = 2; g.beginPath(); g.moveTo(cx, cy); g.lineTo(cx + r * Math.sin(a), cy - r * Math.cos(a)); g.stroke();
    } else if (car.mode !== 'RADAR') {
      g.fillStyle = C.label; g.textAlign = 'center'; g.font = `600 11px ${MONO}`;
      g.fillText('START THE RADAR: THE CAR LOOKS AROUND', cx, pad + 14);
    }
  }

  // ------------------------------------------------------------------ the controls: a throttle lever + a steering wheel, or one stick
  const WHEEL_MAX = 2.2;                                     // the wheel turns 126 degrees each way
  const ctl = [
    { c: el.stick, x: 0, y: 0, id: null, shownY: 0, shownX: 0 },
    { c: el.stick2, x: 0, ang: 0, id: null, last: 0 },
  ];
  let touchT = 0, touchS = 0;
  const wheelMode = () => S.style === 1;

  // the lever (wheel mode) or the stick (one-stick mode)
  (() => {
    const st = ctl[0];
    const move = (e) => {
      const r = st.c.getBoundingClientRect();
      if (wheelMode()) {
        const geo = leverGeo(st.c);
        st.y = Math.max(-1, Math.min(1, (e.clientY - r.top - geo.cy) / geo.travel));
        st.x = 0;
        touchT = -DriveLogic.deadZone1(st.y, S.touchDead);
      } else {
        const geo = stickGeo(st.c);
        let x = (e.clientX - r.left - geo.cx) / (geo.r * 0.62), y = (e.clientY - r.top - geo.cy) / (geo.r * 0.62);
        const len = Math.hypot(x, y);
        if (len > 1) { x /= len; y /= len; }
        st.x = x; st.y = y;
        const k = DriveLogic.deadZone2(x, y, S.touchDead);
        touchT = Math.fround(-y * k); touchS = Math.fround(x * k);
      }
      setSource(SRC_TOUCH, touchT, touchS);
    };
    st.c.addEventListener('pointerdown', (e) => { st.id = e.pointerId; try { st.c.setPointerCapture(e.pointerId); } catch (x) { /* */ } move(e); });
    st.c.addEventListener('pointermove', (e) => { if (st.id === e.pointerId) move(e); });
    const up = (e) => {
      if (st.id !== e.pointerId) return;
      st.id = null; st.x = st.y = 0; touchT = 0;
      if (!wheelMode()) touchS = 0;
      setSource(SRC_TOUCH, touchT, touchS);
    };
    st.c.addEventListener('pointerup', up);
    st.c.addEventListener('pointercancel', up);
  })();

  // the steering wheel: grab it anywhere and turn
  (() => {
    const st = ctl[1];
    const angleAt = (e) => { const r = st.c.getBoundingClientRect(), geo = wheelGeo(st.c); return Math.atan2(e.clientY - r.top - geo.cy, e.clientX - r.left - geo.cx); };
    st.c.addEventListener('pointerdown', (e) => { st.id = e.pointerId; try { st.c.setPointerCapture(e.pointerId); } catch (x) { /* */ } st.last = angleAt(e); });
    st.c.addEventListener('pointermove', (e) => {
      if (st.id !== e.pointerId) return;
      const a = angleAt(e);
      let d = a - st.last;
      if (d > Math.PI) d -= TAU; if (d < -Math.PI) d += TAU;
      st.last = a;
      st.ang = Math.max(-WHEEL_MAX, Math.min(WHEEL_MAX, st.ang + d));
      touchS = DriveLogic.deadZone1(st.ang / WHEEL_MAX, S.touchDead);
      setSource(SRC_TOUCH, touchT, touchS);
    });
    const up = (e) => { if (st.id !== e.pointerId) return; st.id = null; touchS = 0; setSource(SRC_TOUCH, touchT, touchS); };
    st.c.addEventListener('pointerup', up);
    st.c.addEventListener('pointercancel', up);
  })();

  function stickGeo(c) { const w = c.clientWidth, h = c.clientHeight - 22; return { cx: w / 2, cy: h / 2, r: Math.min(w, h) / 2 * 0.9 }; }
  function leverGeo(c) {
    const w = c.clientWidth, h = c.clientHeight - 22;
    const knobH = Math.max(40, Math.min(58, h * 0.16));
    const trackH = Math.min(h - 12, 420);
    return { cx: w / 2, cy: h / 2, trackH, knobH, travel: (trackH - knobH) / 2 - 4, trackW: Math.max(40, Math.min(62, w * 0.3)), knobW: Math.max(84, Math.min(132, w * 0.72)) };
  }
  function wheelGeo(c) { const w = c.clientWidth, h = c.clientHeight - 22; return { cx: w / 2, cy: h / 2, r: Math.min(w, h) / 2 * 0.86 }; }

  // each frame: things that are let go spring back, and the wheel / lever follow the keyboard and the controller
  function animateControls(dt) {
    const k = 1 - Math.exp(-dt / 60);
    const lever = ctl[0], wheel = ctl[1];
    const other = active === SRC_KEYS || active === SRC_PAD;
    if (wheelMode()) {
      if (lever.id === null) lever.y += ((other ? -wanted[0] : 0) - lever.y) * k;
      if (wheel.id === null) wheel.ang += ((other ? wanted[1] * WHEEL_MAX : 0) - wheel.ang) * k;
    } else if (lever.id === null) {
      lever.x += ((other ? wanted[1] : 0) - lever.x) * k;
      lever.y += ((other ? -wanted[0] : 0) - lever.y) * k;
    }
  }

  function drawControls() {
    if (wheelMode()) { drawLever(); drawWheel(); } else drawStick();
  }
  function begin(c) {
    const g = c.getContext('2d'), d = dpr(), w = c.width / d, h = c.height / d;
    g.setTransform(d, 0, 0, d, 0, 0);
    g.clearRect(0, 0, w, h);
    return { g, w, h };
  }
  function label(g, text, value, cx, y) {
    g.font = `600 10.5px ${MONO}`; g.textAlign = 'center'; g.textBaseline = 'alphabetic';
    g.fillStyle = C.label; g.fillText(text + (value ? '  ' : ''), cx - (value ? g.measureText(value).width / 2 : 0), y);
    if (value) { g.fillStyle = C.accent2; g.fillText(value, cx + g.measureText(text + '  ').width / 2, y); }
  }

  function drawLever() {
    const st = ctl[0], { g, w, h } = begin(st.c);
    if (!w) return;
    const geo = leverGeo(st.c), { cx, cy, trackH, trackW, knobW, knobH } = geo;
    const top = cy - trackH / 2;
    // the slot
    rrect(g, cx - trackW / 2, top, trackW, trackH, trackW / 2);
    const slot = g.createLinearGradient(cx - trackW / 2, 0, cx + trackW / 2, 0);
    slot.addColorStop(0, 'rgba(2,4,14,.85)'); slot.addColorStop(0.5, 'rgba(10,16,40,.75)'); slot.addColorStop(1, 'rgba(2,4,14,.85)');
    g.fillStyle = slot; g.fill();
    g.strokeStyle = 'rgba(255,255,255,.10)'; g.lineWidth = 1; g.stroke();
    // the marks beside it
    for (let i = -5; i <= 5; i++) {
      const y = cy + i * geo.travel / 5, long = i % 5 === 0;
      g.strokeStyle = i === 0 ? 'rgba(255,255,255,.55)' : 'rgba(255,255,255,.18)'; g.lineWidth = long ? 2 : 1;
      const a = trackW / 2 + 7, b = a + (long ? 12 : 6);
      g.beginPath(); g.moveTo(cx - b, y); g.lineTo(cx - a, y); g.moveTo(cx + a, y); g.lineTo(cx + b, y); g.stroke();
    }
    g.font = `600 9.5px ${MONO}`; g.textAlign = 'center'; g.fillStyle = C.faint;
    if (trackH > 150) { g.fillText('GAS', cx, top + 16); g.fillText('REV', cx, top + trackH - 9); }
    // the glowing bar from the middle to the handle
    const ky = cy + st.y * geo.travel;
    if (Math.abs(st.y) > 0.01) {
      const up = st.y < 0, grad = g.createLinearGradient(0, cy, 0, ky);
      grad.addColorStop(0, up ? 'rgba(255,122,26,.25)' : 'rgba(124,140,255,.25)'); grad.addColorStop(1, up ? C.accent2 : '#8E9BFF');
      g.fillStyle = grad; g.shadowColor = up ? C.accent : '#7C8CFF'; g.shadowBlur = 18;
      const bw = trackW * 0.42;
      rrect(g, cx - bw / 2, Math.min(cy, ky), bw, Math.abs(ky - cy), bw / 2); g.fill(); g.shadowBlur = 0;
    }
    // the handle: a glossy 3D grip
    const active = st.id !== null || Math.abs(st.y) > 0.02;
    g.fillStyle = 'rgba(0,0,0,.45)'; rrect(g, cx - knobW / 2 + 2, ky - knobH / 2 + 8, knobW - 4, knobH, knobH / 2); g.fill();
    rrect(g, cx - knobW / 2, ky - knobH / 2 + 5, knobW, knobH, knobH / 2); g.fillStyle = active ? '#B4480A' : '#1A2248'; g.fill();
    const face = g.createLinearGradient(0, ky - knobH / 2, 0, ky + knobH / 2);
    if (active) { face.addColorStop(0, '#FFB56A'); face.addColorStop(1, '#FF6A13'); } else { face.addColorStop(0, '#4A5AA6'); face.addColorStop(1, '#2A3572'); }
    rrect(g, cx - knobW / 2, ky - knobH / 2, knobW, knobH, knobH / 2); g.fillStyle = face; g.fill();
    rrect(g, cx - knobW / 2 + 5, ky - knobH / 2 + 3, knobW - 10, knobH * 0.42, knobH * 0.3); g.fillStyle = 'rgba(255,255,255,.28)'; g.fill();
    g.strokeStyle = 'rgba(0,0,0,.28)'; g.lineWidth = 2; g.lineCap = 'round';
    for (let i = -1; i <= 1; i++) { g.beginPath(); g.moveTo(cx + i * 10, ky + 2); g.lineTo(cx + i * 10, ky + knobH * 0.28); g.stroke(); }
    label(g, 'THROTTLE', Math.abs(st.y) > 0.02 ? sign(Math.round(-st.y * 100)) + '%' : '', cx, h - 4);
  }

  function drawWheel() {
    const st = ctl[1], { g, w, h } = begin(st.c);
    if (!w) return;
    const { cx, cy, r } = wheelGeo(st.c);
    const active = st.id !== null || Math.abs(st.ang) > 0.03;
    // the dial behind the wheel: how far it is turned
    g.lineCap = 'round'; g.lineWidth = Math.max(4, r * 0.05);
    g.strokeStyle = 'rgba(255,255,255,.09)'; g.beginPath(); g.arc(cx, cy, r * 1.12, -Math.PI / 2 - WHEEL_MAX, -Math.PI / 2 + WHEEL_MAX); g.stroke();
    if (active) {
      g.strokeStyle = C.accent2; g.shadowColor = C.accent; g.shadowBlur = 16;
      g.beginPath(); g.arc(cx, cy, r * 1.12, -Math.PI / 2 + Math.min(0, st.ang), -Math.PI / 2 + Math.max(0, st.ang)); g.stroke(); g.shadowBlur = 0;
    }
    g.fillStyle = 'rgba(255,255,255,.6)'; g.beginPath(); g.arc(cx, cy - r * 1.12, g.lineWidth * 0.7, 0, TAU); g.fill();
    // its shadow on the dashboard
    const sh = g.createRadialGradient(cx, cy + r * 0.12, r * 0.5, cx, cy + r * 0.12, r * 1.15);
    sh.addColorStop(0, 'rgba(0,0,0,.55)'); sh.addColorStop(1, 'rgba(0,0,0,0)');
    g.fillStyle = sh; g.beginPath(); g.arc(cx, cy + r * 0.12, r * 1.15, 0, TAU); g.fill();

    g.save(); g.translate(cx, cy); g.rotate(st.ang);
    // the spokes
    const spoke = (a) => {
      g.save(); g.rotate(a);
      const sg = g.createLinearGradient(0, -r * 0.1, 0, r * 0.1);
      sg.addColorStop(0, '#5262B0'); sg.addColorStop(1, '#1E2757');
      g.fillStyle = sg; rrect(g, r * 0.2, -r * 0.085, r * 0.66, r * 0.17, r * 0.06); g.fill();
      g.restore();
    };
    spoke(0); spoke(Math.PI); spoke(Math.PI / 2);
    // the rim: a thick ring, lighter on top
    const rim = g.createLinearGradient(0, -r, 0, r);
    rim.addColorStop(0, '#5A6BC0'); rim.addColorStop(0.45, '#2B3672'); rim.addColorStop(1, '#131A3E');
    g.strokeStyle = '#0A0F28'; g.lineWidth = r * 0.26; g.beginPath(); g.arc(0, r * 0.03, r * 0.87, 0, TAU); g.stroke();
    g.strokeStyle = rim; g.lineWidth = r * 0.22; g.beginPath(); g.arc(0, 0, r * 0.87, 0, TAU); g.stroke();
    // the two grips, and the mark on top
    const grip = g.createLinearGradient(0, -r, 0, r);
    grip.addColorStop(0, '#FFB56A'); grip.addColorStop(1, '#F25A0A');
    g.strokeStyle = active ? grip : '#FF7A1A'; g.lineWidth = r * 0.225; g.lineCap = 'butt';
    if (active) { g.shadowColor = C.accent; g.shadowBlur = 18; }
    g.beginPath(); g.arc(0, 0, r * 0.87, Math.PI - 0.42, Math.PI + 0.42); g.stroke();
    g.beginPath(); g.arc(0, 0, r * 0.87, -0.42, 0.42); g.stroke();
    g.shadowBlur = 0;
    g.strokeStyle = '#F4F6FF'; g.lineWidth = r * 0.225; g.beginPath(); g.arc(0, 0, r * 0.87, -Math.PI / 2 - 0.05, -Math.PI / 2 + 0.05); g.stroke();
    // the hub
    const hub = g.createRadialGradient(-r * 0.08, -r * 0.1, r * 0.02, 0, 0, r * 0.3);
    hub.addColorStop(0, '#6E80DA'); hub.addColorStop(1, '#1A2250');
    g.fillStyle = '#0A0F28'; g.beginPath(); g.arc(0, r * 0.025, r * 0.3, 0, TAU); g.fill();
    g.fillStyle = hub; g.beginPath(); g.arc(0, 0, r * 0.28, 0, TAU); g.fill();
    g.fillStyle = C.accent; g.beginPath(); g.arc(0, 0, r * 0.13, 0, TAU); g.fill();
    g.fillStyle = '#fff'; g.font = `900 ${r * 0.15}px ${FONT}`; g.textAlign = 'center'; g.textBaseline = 'middle'; g.fillText('RC', 0, r * 0.012);
    g.restore();
    // the shine (it stays where the light is, while the wheel turns under it)
    g.strokeStyle = 'rgba(255,255,255,.30)'; g.lineWidth = r * 0.035; g.lineCap = 'round';
    g.beginPath(); g.arc(cx, cy, r * 0.94, -Math.PI * 0.86, -Math.PI * 0.58); g.stroke();
    label(g, 'STEER', Math.abs(st.ang) > 0.03 ? sign(Math.round(st.ang * 180 / Math.PI)) + '°' : '', cx, h - 4);
  }

  function drawStick() {
    const st = ctl[0], { g, w, h } = begin(st.c);
    if (!w) return;
    const { cx, cy, r } = stickGeo(st.c), kr = r * 0.4;
    const active = st.id !== null || Math.abs(st.x) > 0.02 || Math.abs(st.y) > 0.02;
    const base = g.createRadialGradient(cx, cy - r * 0.3, r * 0.1, cx, cy, r);
    base.addColorStop(0, 'rgba(40,54,120,.55)'); base.addColorStop(1, 'rgba(4,7,20,.75)');
    g.fillStyle = base; g.beginPath(); g.arc(cx, cy, r, 0, TAU); g.fill();
    g.strokeStyle = active ? C.accent : 'rgba(255,255,255,.14)'; g.lineWidth = active ? 2 : 1;
    if (active) { g.shadowColor = C.accent; g.shadowBlur = 18; }
    g.beginPath(); g.arc(cx, cy, r, 0, TAU); g.stroke(); g.shadowBlur = 0;
    g.strokeStyle = 'rgba(255,255,255,.08)'; g.lineWidth = 1; g.beginPath(); g.arc(cx, cy, r * 0.58, 0, TAU); g.stroke();
    g.strokeStyle = 'rgba(255,255,255,.35)'; g.lineWidth = 2; g.lineCap = 'round';
    const t = 5, m = r - 12, chev = (x1, y1, x2, y2, x3, y3) => { g.beginPath(); g.moveTo(x1, y1); g.lineTo(x2, y2); g.lineTo(x3, y3); g.stroke(); };
    chev(cx - t, cy - m + t, cx, cy - m, cx + t, cy - m + t); chev(cx - t, cy + m - t, cx, cy + m, cx + t, cy + m - t);
    chev(cx - m + t, cy - t, cx - m, cy, cx - m + t, cy + t); chev(cx + m - t, cy - t, cx + m, cy, cx + m - t, cy + t);
    const px = cx + st.x * (r - kr), py = cy + st.y * (r - kr);
    if (active) { g.strokeStyle = 'rgba(255,122,26,.35)'; g.lineWidth = kr * 0.9; g.beginPath(); g.moveTo(cx, cy); g.lineTo(px, py); g.stroke(); }
    g.fillStyle = 'rgba(0,0,0,.5)'; g.beginPath(); g.arc(px, py + 7, kr, 0, TAU); g.fill();
    const knob = g.createRadialGradient(px - kr * 0.35, py - kr * 0.4, kr * 0.05, px, py, kr);
    if (active) { knob.addColorStop(0, '#FFE0B8'); knob.addColorStop(0.3, '#FF9A3D'); knob.addColorStop(1, '#E85A08'); }
    else { knob.addColorStop(0, '#93A2EE'); knob.addColorStop(0.3, '#4A5AA6'); knob.addColorStop(1, '#222C62'); }
    g.fillStyle = knob; g.beginPath(); g.arc(px, py, kr, 0, TAU); g.fill();
    g.fillStyle = 'rgba(255,255,255,.22)'; g.beginPath(); g.ellipse(px, py - kr * 0.45, kr * 0.62, kr * 0.34, 0, 0, TAU); g.fill();
    label(g, 'DRIVE', '', cx, h - 4);
  }

  // ------------------------------------------------------------------ dialogs
  document.querySelectorAll('[data-close]').forEach((b) => b.addEventListener('click', () => b.closest('dialog').close()));
  function esc(s) { return String(s).replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c])); }
  function message(title, html, buttons) {
    $('msgTitle').textContent = title;
    $('msgBody').innerHTML = html;
    if (buttons) {
      const row = document.createElement('div'); row.className = 'seg';
      for (const [text, fn] of buttons) { const b = document.createElement('button'); b.className = 'key c'; b.style.setProperty('--h', 24); b.textContent = text; b.onclick = () => { $('message').close(); fn(); }; row.appendChild(b); }
      $('msgBody').appendChild(row);
    }
    if (!$('message').open) $('message').showModal();
  }
  let toastTimer;
  function toast(s) { const t = $('toast'); t.textContent = s; t.classList.add('show'); clearTimeout(toastTimer); toastTimer = setTimeout(() => t.classList.remove('show'), 2600); }
  function confirmDisconnect() { message('Disconnect?', '<p>The car stops.</p>', [['Disconnect', () => disconnect(false)]]); }

  function openHelp() {
    const lines = [];
    for (let b = 0; b < 16; b++) { const j = map.job(b, false); if (j !== '-') lines.push(`${ControllerMap.BUTTON_NAMES[b]} = ${j === '^' ? 'SHIFT (hold)' : ControllerMap.jobName(j)}`); }
    const sh = ControllerMap.BUTTON_NAMES[map.buttonFor('^')] || 'SHIFT';
    for (let b = 0; b < 16; b++) { const j = map.job(b, true); if (j !== '-' && j !== '^') lines.push(`${sh} + ${ControllerMap.BUTTON_NAMES[b]} = ${ControllerMap.jobName(j)}`); }
    $('helpBody').innerHTML = `
      <p><b>Connect:</b> pair the car's <b>HC-05</b> with this computer first (Windows: Settings &gt; Bluetooth &amp; devices &gt; Add device, PIN 1234 or 0000).
      Then click <b>Connect</b> and pick it (it may be called "HC-05" or a COM port). Works in Chrome and Edge.</p>
      <p><b>Drive:</b> the <b>lever</b> on the left is the throttle (up = forward, down = backward), the <b>steering wheel</b> on the right steers - grab it and turn.
      Let go and both spring back. Or pick <b>1 stick</b> (top). The 3D car shows what the real one does.</p>
      <p><b>Keyboard:</b> <kbd>W</kbd><kbd>A</kbd><kbd>S</kbd><kbd>D</kbd> or the arrows, <kbd>Space</kbd> STOP, <kbd>Enter</kbd> dance, <kbd>H</kbd> horn, <kbd>K</kbd> kick,
      <kbd>T</kbd> turbo, <kbd>B</kbd> brake, <kbd>R</kbd> radar, <kbd>M</kbd> mute, <kbd>F</kbd> football, <kbd>Q</kbd> fast / slow.</p>
      <p><b>GameSir Cyclone 2 on a computer:</b> plug in its 2.4 GHz dongle (Home light <b>green</b> = XInput), or connect it by cable.
      Left stick = throttle, right stick = steering. Click the controller button (top) to see every button live.</p>
      <pre>${esc(lines.join('\n'))}</pre>
      <p style="margin-top:12px">With the newest car program the steering is smooth (each side of the car gets its own speed); older programs get the 6 driving letters.
      Right-click the 3D picture for the link log. On an Android phone, the app is better: <a style="color:var(--accent2)" href="${APP_LINK}">download it</a>.</p>`;
    $('help').showModal();
  }

  function openSettings() {
    const body = $('settingsBody');
    body.innerHTML = '';
    const sec = (t) => body.insertAdjacentHTML('beforeend', `<div class="section">${t}</div>`);
    const card = () => { const c = document.createElement('div'); c.className = 'card'; body.appendChild(c); return c; };
    const seg = (title, opts, nowI, pick) => {
      const c = card(); c.innerHTML = `<div>${title}</div><div class="seg"></div>`;
      opts.forEach((o, i) => {
        const b = document.createElement('button'); b.className = 'key' + (i === nowI ? ' on' : ''); b.textContent = o;
        b.onclick = () => { pick(i); [...c.querySelectorAll('.key')].forEach((x, k) => x.classList.toggle('on', k === i)); render(); };
        c.lastChild.appendChild(b);
      });
    };
    const slider = (title, low, high, value, max, set, unit) => {
      const c = card();
      c.innerHTML = `<div class="row"><div>${title}</div><div class="val">${unit ? value + ' ' + unit : ''}</div></div><input type="range" min="0" max="${max}" value="${value}"><div class="row sub"><div>${low}</div><div>${high}</div></div>`;
      const inp = c.querySelector('input');
      inp.oninput = () => { if (unit) c.querySelector('.val').textContent = inp.value + ' ' + unit; set(+inp.value); };
    };
    const toggle = (title, sub, value, set) => {
      const c = card();
      c.innerHTML = `<div class="row"><div><div>${title}</div><div class="sub">${sub}</div></div><input type="checkbox" class="switch" ${value ? 'checked' : ''}></div>`;
      c.querySelector('input').onchange = (e) => { set(e.target.checked); render(); };
    };
    sec('DRIVING');
    seg('On-screen controls', ['Wheel + throttle', '1 stick'], S.style === 1 ? 0 : 1, (i) => setStyle(i === 0 ? 1 : 0));
    seg('Top speed', ['Fast', 'Slow'], S.slowTop ? 1 : 0, (i) => setSlowTop(i === 1));
    slider('Steering feel', 'Direct', 'Gentle in the middle', Math.round(S.feel * 100), 100, (v) => { save('feel', v / 100); drive.setFeel(v / 100); });
    slider('On-screen dead zone', '0 %', '25 %', Math.round(S.touchDead * 100), 25, (v) => save('touchDead', v / 100), '%');
    sec('GAME CONTROLLER');
    slider('Stick dead zone', '0 %', '30 %', Math.round(S.padDead * 100), 30, (v) => save('padDead', v / 100), '%');
    toggle('The left stick steers too', 'when the right stick is in the middle', S.leftSteers, (v) => save('leftSteers', v));
    toggle('Show the controller\'s buttons on the screen', 'little A / B / RT … badges on the buttons', S.overlay, (v) => save('overlay', v));
    const m = card(); m.innerHTML = '<button class="key" style="width:100%;height:48px">Controller buttons and remap  ›</button>';
    m.firstChild.onclick = () => { $('settings').close(); openController(); };
    sec('OTHER');
    const r = card(); r.innerHTML = '<button class="key" style="width:100%;height:48px;color:#FF8FA3">Reset everything to the defaults</button>';
    r.firstChild.onclick = () => message('Reset?', '<p>Every setting and every controller button back as it came.</p>', [['Reset', () => {
      try { Object.keys(localStorage).filter((k) => k.startsWith('rc:')).forEach((k) => localStorage.removeItem(k)); } catch (e) { /* */ }
      location.reload();
    }]]);
    $('settings').showModal();
  }

  // ------------------------------------------------------------------ the controller screen (the same drawing as the app)
  let padShift = false, highlight = -1;
  $('padShift').onclick = () => { padShift = !padShift; $('padShift').classList.toggle('on', padShift); drawPad(); };
  $('padRemap').onclick = () => chooseJob(-1);
  $('padReset').onclick = () => { map.reset(); store.set('map', map.toText()); drawPad(); render(); };
  $('controller').addEventListener('close', () => { remapJob = null; render(); });
  function openController() { $('controller').showModal(); requestAnimationFrame(() => { sizePad(); drawPad(); }); }
  function sizePad() { const c = $('padCanvas'); c.width = c.clientWidth * dpr(); c.height = c.clientHeight * dpr(); }
  addEventListener('resize', () => { if ($('controller').open) { sizePad(); drawPad(); } });

  function chooseJob(button) {
    $('jobsTitle').textContent = button < 0 ? 'Remap: choose a job' : `${ControllerMap.BUTTON_NAMES[button]}${padShift ? ' (with LB held)' : ''} does …`;
    const list = $('jobsList'); list.innerHTML = '';
    ControllerMap.JOBS.forEach((job, i) => {
      const b = document.createElement('button'); b.className = 'key'; b.textContent = ControllerMap.JOB_NAMES[i];
      b.onclick = () => {
        $('jobs').close();
        if (button < 0) { remapJob = job; heldShift = -1; } else { map.set(button, padShift, job); store.set('map', map.toText()); }
        drawPad(); render();
      };
      list.appendChild(b);
    });
    $('jobs').showModal();
  }
  function assign(b, layer) {
    map.set(b, layer, remapJob);
    store.set('map', map.toText());
    remapJob = null; heldShift = -1; highlight = b;
    setTimeout(() => { highlight = -1; drawPad(); }, 700);
    drawPad(); render();
  }

  const AT = [[712, 296], [772, 236], [652, 236], [712, 176], [262, 104], [738, 104], [260, 46], [740, 46], [290, 238], [610, 400],
    [568, 236], [432, 236], [390, 352], [436, 398], [390, 444], [344, 398]];
  const LEFT = [6, 4, 100, 8, 11, 12, 15, 13, 14], RIGHT = [7, 5, 3, 2, 1, 0, 10, 101, 9];
  let labelBoxes = [];
  let geo = { s: 1, ox: 0, oy: 0 };
  function drawPad() {
    const c = $('padCanvas'), g = c.getContext('2d'), d = dpr(), w = c.width / d, h = c.height / d;
    if (!w) return;
    const gp = pad.index >= 0;
    $('padTitle').textContent = gp ? '🎮 ' + shortName(pad.id) : 'No controller';
    $('padHint').textContent = remapJob ? `Press the button for "${ControllerMap.jobName(remapJob)}" now (hold LB first to put it on the LB layer).`
      : !gp ? 'Cyclone 2: plug in its 2.4 GHz dongle (Home light green) or a USB cable, then press any button so the browser sees it.'
        : 'Press anything: it lights up. Click a button to give it another job. (These buttons don\'t drive the car here.)';
    g.setTransform(d, 0, 0, d, 0, 0);
    g.clearRect(0, 0, w, h);
    const col = Math.min(w * 0.27, 250), gap = 12, dw = w - 2 * col - 2 * gap, dh = h - 24;
    const s = Math.min(dw / 1000, dh / 640), ox = col + gap + (dw - 1000 * s) / 2, oy = (dh - 640 * s) / 2;
    geo = { s, ox, oy };
    const X = (x) => ox + x * s, Y = (y) => oy + y * s;
    const down = (b) => pad.down[b] || b === highlight;
    const KEY = '#2C3877', KEYD = '#141B44';
    // body
    g.beginPath(); g.moveTo(X(230), Y(122)); g.lineTo(X(770), Y(122));
    g.bezierCurveTo(X(880), Y(122), X(930), Y(190), X(952), Y(300)); g.bezierCurveTo(X(978), Y(430), X(992), Y(565), X(900), Y(605));
    g.bezierCurveTo(X(828), Y(636), X(772), Y(565), X(722), Y(505)); g.bezierCurveTo(X(660), Y(470), X(340), Y(470), X(278), Y(505));
    g.bezierCurveTo(X(228), Y(565), X(172), Y(636), X(100), Y(605)); g.bezierCurveTo(X(8), Y(565), X(22), Y(430), X(48), Y(300));
    g.bezierCurveTo(X(70), Y(190), X(120), Y(122), X(230), Y(122)); g.closePath();
    const bodyG = g.createLinearGradient(0, Y(122), 0, Y(620));
    bodyG.addColorStop(0, '#2A3672'); bodyG.addColorStop(1, '#10173A');
    g.fillStyle = bodyG; g.fill(); g.strokeStyle = 'rgba(255,255,255,.18)'; g.lineWidth = 1.5; g.stroke();
    // triggers + bumpers
    [6, 7].forEach((b) => {
      const cx = AT[b][0]; rrect(g, X(cx - 70), Y(22), 140 * s, 50 * s, 14 * s); g.fillStyle = KEY; g.fill();
      const v = pad.trig[b - 6];
      if (v > 0.01) { g.save(); g.clip(); g.fillStyle = down(b) ? C.accent : 'rgba(255,122,26,.55)'; g.fillRect(X(cx - 70), Y(72) - 50 * s * v, 140 * s, 50 * s * v); g.restore(); }
    });
    [4, 5].forEach((b) => { rrect(g, X(AT[b][0] - 92), Y(90), 184 * s, 28 * s, 14 * s); g.fillStyle = down(b) ? C.accent : KEY; g.fill(); });
    // sticks
    const stick = (x, y, sx, sy, click) => {
      g.fillStyle = '#0A0F28'; g.beginPath(); g.arc(X(x), Y(y), 66 * s, 0, TAU); g.fill();
      const moved = Math.abs(sx) > 0.05 || Math.abs(sy) > 0.05, kx = x + sx * 26, ky = y + sy * 26;
      g.fillStyle = down(click) ? C.accent : moved ? '#4A5AA6' : KEY; g.beginPath(); g.arc(X(kx), Y(ky), 42 * s, 0, TAU); g.fill();
      g.strokeStyle = moved ? C.accent : 'rgba(255,255,255,.2)'; g.lineWidth = 2.5 * s; g.beginPath(); g.arc(X(kx), Y(ky), 28 * s, 0, TAU); g.stroke();
    };
    stick(290, 238, pad.lx, pad.ly, 8); stick(610, 400, pad.rx, pad.ry, 9);
    // D-pad
    g.fillStyle = KEYD; rrect(g, X(368), Y(334), 44 * s, 128 * s, 6 * s); g.fill(); rrect(g, X(326), Y(376), 128 * s, 44 * s, 6 * s); g.fill();
    [12, 13, 14, 15].forEach((b) => { if (down(b)) { g.fillStyle = C.accent; rrect(g, X(AT[b][0] - 22), Y(AT[b][1] - 22), 44 * s, 44 * s, 6 * s); g.fill(); } });
    // face buttons
    ['A', 'B', 'X', 'Y'].forEach((l, b) => {
      g.fillStyle = down(b) ? padColor(l) : KEYD; g.beginPath(); g.arc(X(AT[b][0]), Y(AT[b][1]), 27 * s, 0, TAU); g.fill();
      g.fillStyle = down(b) ? '#070B1A' : padColor(l); g.font = `900 ${26 * s}px ${FONT}`; g.textAlign = 'center'; g.textBaseline = 'alphabetic';
      g.fillText(l, X(AT[b][0]), Y(AT[b][1] + 9));
    });
    [11, 10].forEach((b) => { rrect(g, X(AT[b][0] - 22), Y(AT[b][1] - 12), 44 * s, 24 * s, 12 * s); g.fillStyle = down(b) ? C.accent : KEYD; g.fill(); });
    g.fillStyle = pad.home ? C.accent : KEYD; g.beginPath(); g.arc(X(500), Y(178), 22 * s, 0, TAU); g.fill();
    g.strokeStyle = C.accent; g.lineWidth = 2.5 * s; g.stroke();
    // labels
    labelBoxes = [];
    const size = Math.max(11, Math.min(14, h / 28));
    const top = Math.max(0, oy), bottom = Math.min(h - 24, oy + 640 * s);
    const column = (rows, x, leftSide) => {
      const rowH = Math.max(26, (bottom - top) / rows.length);
      let y = top + Math.max(0, (bottom - top - rowH * rows.length) / 2);
      for (const row of rows) {
        const cy = y + rowH / 2;
        let who, job, b = row, lit;
        if (row === 100) { who = 'Left stick'; job = 'Throttle ' + num(pad.t); b = -1; lit = pad.t !== 0; }
        else if (row === 101) { who = 'Right stick'; job = 'Steering ' + num(pad.s); b = -1; lit = pad.s !== 0; }
        else {
          who = ControllerMap.BUTTON_NAMES[row];
          const j = map.job(row, padShift || shiftHeld());
          job = j === '^' ? 'SHIFT (hold)' : ControllerMap.jobName(j);
          if (row === 6 || row === 7) job += '  ' + Math.round(pad.trig[row - 6] * 100) + '%';
          lit = down(row);
        }
        const bx = { x, y: cy - rowH * 0.42, w: col, h: rowH * 0.84, row };
        if (row < 16) labelBoxes.push(bx);
        rrect(g, bx.x, bx.y, bx.w, bx.h, 10); g.fillStyle = lit ? 'rgba(255,122,26,.22)' : 'rgba(255,255,255,.06)'; g.fill();
        g.textAlign = 'left'; g.textBaseline = 'alphabetic';
        g.font = `600 ${size * 0.82}px ${MONO}`; g.fillStyle = b >= 0 && b <= 3 ? padColor(ControllerMap.BUTTON_NAMES[b]) : C.label;
        g.fillText(who.toUpperCase(), x + 10, cy + size * 0.34);
        const nameW = g.measureText('D-PAD RIGHT  ').width;
        g.font = `700 ${size}px ${FONT}`; g.fillStyle = lit ? C.accent2 : C.text;
        g.fillText(ellipsize(g, job, col - nameW - 18), x + 10 + nameW, cy + size * 0.36);
        const at = row === 100 ? [290, 238] : row === 101 ? [610, 400] : AT[row];
        g.strokeStyle = lit ? C.accent : 'rgba(154,166,208,.3)'; g.lineWidth = 1;
        g.beginPath(); g.moveTo(leftSide ? x + col : x, cy); g.lineTo(X(at[0]), Y(at[1])); g.stroke();
        y += rowH;
      }
    };
    column(LEFT, 0, true); column(RIGHT, w - col, false);
    g.fillStyle = C.faint; g.font = `600 ${size * 0.8}px ${MONO}`; g.textAlign = 'center';
    g.fillText('HOME = THE COMPUTER\'S   ·   M1 / M2 (BACK) = COPY ANY BUTTON IN THE GAMESIR APP', w / 2, h - 6);
  }
  const num = (v) => (v >= 0 ? '+' : '') + v.toFixed(2);
  $('padCanvas').addEventListener('click', (e) => {
    const r = e.target.getBoundingClientRect(), x = e.clientX - r.left, y = e.clientY - r.top;
    const hit = labelBoxes.find((b) => x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h);
    if (hit) return chooseJob(hit.row);
    let best = -1, bestD = 60 * geo.s;
    AT.forEach((a, b) => { const dd = Math.hypot(geo.ox + a[0] * geo.s - x, geo.oy + a[1] * geo.s - y); if (dd < bestD) { bestD = dd; best = b; } });
    if (best >= 0) chooseJob(best);
  });

  // ------------------------------------------------------------------ the clocks: one for the car (50 times a second), one for the picture
  let last = now();
  setInterval(() => {
    const t = now();
    keyboardTick(t - last);
    last = t;
    padTick();
    drive.tick(t);
  }, 20);
  let lastFrame = now();
  function frame() {
    const t = now(), dt = Math.min(50, t - lastFrame);
    lastFrame = t;
    animateControls(dt);
    drawScene(dt);
    drawControls();
    if (page === 2) drawRadar();
    requestAnimationFrame(frame);
  }
  addEventListener('beforeunload', () => { if (connected) drive.halt(now()); });

  showPage(0);
  layout();
  requestAnimationFrame(frame);
  window.__robotCar = { drive, car, map, S, scene, ctl, onLine, layout, setStyle, doJob, get connected() { return connected; }, set connected(v) { connected = v; } };
})();
