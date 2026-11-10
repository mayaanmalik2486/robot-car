'use strict';
// The Robot Car web remote: the same screen, rules and controller map as the Android app,
// for Chrome or Edge on a computer (and Chrome on Android). It talks to the car's HC-05 with
// Web Serial; the GameSir Cyclone 2 (or any controller) comes in through the Gamepad API,
// and the keyboard works too.
/* global Car, DriveLogic, ControllerMap, CarState, Layout */
(() => {
  const $ = (id) => document.getElementById(id);
  const SPP = '00001101-0000-1000-8000-00805f9b34fb';
  const APP_LINK = 'https://github.com/mayaanmalik2486/robot-car/raw/main/App/RobotCar.apk';
  const now = () => performance.now();
  const store = {
    get(k, d) { try { const v = localStorage.getItem('rc:' + k); return v === null ? d : JSON.parse(v); } catch (e) { return d; } },
    set(k, v) { try { localStorage.setItem('rc:' + k, JSON.stringify(v)); } catch (e) { /* private window */ } },
  };
  const S = {
    style: store.get('style', 0), slowTop: store.get('slowTop', false), feel: store.get('feel', 0.6), touchDead: store.get('touchDead', 0.10),
    padDead: store.get('padDead', 0.12), leftSteers: store.get('leftSteers', false), overlay: store.get('overlay', true),
  };
  const save = (k, v) => { S[k] = v; store.set(k, v); };

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

  const drive = new DriveLogic((c) => {
    if (!connected) return;
    sendByte(c);
    if (c >= 0x80) {
      const w = `L${sign(drive.left())} R${sign(drive.right())}`;
      if (w !== lastWheels) { lastWheels = w; addLog('-> ' + w); }
      lastSent = w;
    } else { lastWheels = ''; addLog('-> ' + String.fromCharCode(c)); lastSent = String.fromCharCode(c); }
  });
  const sign = (v) => (v > 0 ? '+' : '') + v;
  drive.setFeel(S.feel);
  drive.setSlowTop(S.slowTop);

  async function connect(pick) {
    if (!('serial' in navigator)) {
      message('This browser can\'t talk to the car', '<p>Use <b>Chrome</b> or <b>Edge</b> on a computer (Windows, Mac, Linux or a Chromebook), '
        + 'or Chrome on an Android phone. Safari and Firefox can\'t do Bluetooth serial, and neither can an iPhone.</p>'
        + '<p>On a phone, the Android app is the best way: <a style="color:var(--accent)" href="' + APP_LINK + '">download it here</a>.</p>');
      return;
    }
    if (connected || connecting) return;
    connecting = true; render();
    try {
      let ports = await navigator.serial.getPorts();
      port = !pick && ports.length ? ports[0]
        : await navigator.serial.requestPort({ allowedBluetoothServiceClassIds: [SPP] });
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
  const code = (c) => c.charCodeAt(0);

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
    if (kind === 'radar') { drawRadar(); return; }
    if (kind === 'nothing') return;
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
    if (now() - warnedAt < 2000) return;
    warnedAt = now();
    toast('Not connected - click Connect first');
  }

  function doJob(job) {
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
  function setSource(src, t, s) {
    source[src] = [t, s];
    if (t !== 0 || s !== 0) active = src;
    else if (active === src) { active = -1; source.forEach((v, i) => { if (v[0] !== 0 || v[1] !== 0) active = i; }); }
    const v = active < 0 ? [0, 0] : source[active];
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
  document.addEventListener('visibilitychange', () => { if (document.hidden) { drive.halt(now()); for (let i = 0; i < 3; i++) source[i] = [0, 0]; active = -1; } });
  function keyboardTick(dt) {
    const wantT = (keys.has('w') || keys.has('ArrowUp') ? 1 : 0) - (keys.has('s') || keys.has('ArrowDown') ? 1 : 0);
    const wantS = (keys.has('d') || keys.has('ArrowRight') ? 1 : 0) - (keys.has('a') || keys.has('ArrowLeft') ? 1 : 0);
    const step = dt / 350;                                   // 0 -> full in 0.35 s
    const ramp = (v, w) => (w === 0 ? 0 : Math.abs(w - v) <= step ? w : v + Math.sign(w - v) * step);
    keyT = ramp(keyT, wantT); keyS = ramp(keyS, wantS);
    if (keyT !== source[SRC_KEYS][0] || keyS !== source[SRC_KEYS][1]) {
      if ((keyT || keyS) && !connected) notConnected();
      setSource(SRC_KEYS, keyT, keyS);
    }
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
  const shortName = (id) => (id || 'Controller').replace(/\s*\(.*$/, '').slice(0, 24) || 'Controller';
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
      if (!$('controller').open) {
        if ((t || s) && !connected) notConnected();
        setSource(SRC_PAD, t, s);
        showSticks(t, s);
      }
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
    gear: '<svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="3.2"/><path d="M12 2.5v2.6M12 18.9v2.6M2.5 12h2.6M18.9 12h2.6M5.3 5.3l1.8 1.8M16.9 16.9l1.8 1.8M5.3 18.7l1.8-1.8M16.9 7.1l1.8-1.8"/><circle cx="12" cy="12" r="6.6"/></svg>',
    help: '<svg class="icon" viewBox="0 0 24 24"><path d="M9 9a3 3 0 1 1 4.2 2.8c-.8.4-1.2 1-1.2 1.9V14"/><circle cx="12" cy="18" r=".6" fill="currentColor"/></svg>',
    pad: '<svg class="icon" viewBox="0 0 24 24"><rect x="2.5" y="7" width="19" height="11" rx="5.5"/><path d="M7 12.5h3M8.5 11v3"/><circle cx="15.5" cy="11.5" r=".8" fill="currentColor"/><circle cx="17.5" cy="13.5" r=".8" fill="currentColor"/></svg>',
  };
  function key(name, html, onClick, cls) {
    const b = document.createElement('button');
    b.className = 'key' + (cls ? ' ' + cls : '');
    b.innerHTML = html;
    b.addEventListener('click', onClick);
    screen.appendChild(b);
    el[name] = b;
    return b;
  }
  function addJob(job, b) { (jobButtons[job] = jobButtons[job] || []).push(b); }
  const two = (big, small) => `<span>${big}</span><small>${small}</small>`;

  key('connect', '', () => (connected ? confirmDisconnect() : connect(false)));
  el.song = Object.assign(document.createElement('div'), { id: 'song' });
  screen.appendChild(el.song);
  key('pad', '', () => openController());
  key('speed', '', () => doJob('%'));
  key('style', '', () => setStyle((S.style + 1) % 3));
  key('settings', icons.gear, () => openSettings()).title = 'Settings';
  key('help', icons.help, () => openHelp()).title = 'Help';

  el.lcd = document.createElement('canvas');
  el.lcd.addEventListener('click', () => { if (!connected && !connecting) connect(false); });
  el.lcd.addEventListener('contextmenu', (e) => { e.preventDefault(); message('Link log (-> sent, <- from the car)', '<pre>' + esc(log.join('\n') || 'Nothing yet.') + '</pre>'); });
  el.lcd.title = 'Right-click: the link log';
  screen.appendChild(el.lcd);

  ['MOVES', 'PLAY', 'RADAR'].forEach((n, i) => key('tab' + i, n, () => showPage(i)));
  el.pages = document.createElement('div');
  screen.appendChild(el.pages);
  const PAGES = [
    [['🔍', 'Search', 'I'], ['😈', 'Chaos', '1'], ['🍩', 'Donut', '3'], ['↩️', '180°', 'U'], ['⏫', 'Auto', '2'], ['⏬', 'Reverse', 'J'], ['🅿️', 'Brake', 'u'], ['📯', 'Horn', 'Y']],
    [['🚶', 'Follow me', 'M'], ['🥅', 'Goalkeeper', 'K'], ['🚨', 'Guard', 'A'], ['🎵', 'Music', 'N'], ['✋', 'Gestures', 'E'], ['🐶', 'Pet', 'P']],
  ];
  const pageEls = PAGES.map((items) => {
    const p = document.createElement('div');
    p.className = 'page';
    for (const [emoji, word, job] of items) {
      const b = document.createElement('button');
      b.className = 'key';
      b.innerHTML = `<span class="emoji">${emoji}</span><span>${word}</span>`;
      b.addEventListener('click', () => doJob(job));
      addJob(job, b);
      p.appendChild(b);
    }
    el.pages.appendChild(p);
    return p;
  });
  const radarPage = document.createElement('div');
  radarPage.className = 'page';
  el.radar = document.createElement('canvas');
  radarPage.appendChild(el.radar);
  el.radarB = document.createElement('button');
  el.radarB.className = 'key';
  el.radarB.addEventListener('click', () => doJob('O'));
  addJob('O', el.radarB);
  radarPage.appendChild(el.radarB);
  el.pages.appendChild(radarPage);
  pageEls.push(radarPage);

  el.stick = document.createElement('canvas'); screen.appendChild(el.stick);
  el.stick2 = document.createElement('canvas'); screen.appendChild(el.stick2);
  [['stop', 'STOP', 'S', 'stop'], ['dance', 'Dance', '4'], ['kick', 'Kick', 'k'], ['horn', 'Horn', 'Y'], ['turbo', 'Turbo', 'T']].forEach(([n, label, job, cls]) => {
    addJob(job, key(n, `<span>${label}</span>`, () => doJob(job), cls));
  });

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
    active = -1;
    save('style', s === 2 ? 0 : s);                            // (tilt is the phone app's: here it is 1 stick)
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

  function layout() {
    const W = window.innerWidth, H = window.innerHeight;
    const boxes = Layout.screen(W, H, S.style);
    for (const n of ['connect', 'song', 'pad', 'speed', 'style', 'settings', 'help', 'lcd', 'tab0', 'tab1', 'tab2', 'pages', 'stick', 'stick2', 'stop', 'dance', 'kick', 'horn', 'turbo'])
      place(el[n], boxes[n]);
    const P = boxes.pages;
    pageEls.forEach((p, i) => {
      Object.assign(p.style, { position: 'absolute', inset: '0' });
      if (i < 2) {
        const g = Layout.grid(p.children.length, P.w, P.h);
        [...p.children].forEach((b, k) => place(b, { x: g[4] + (k % g[0]) * (g[2] + 8), y: g[5] + Math.floor(k / g[0]) * (g[3] + 8), w: g[2], h: g[3] }));
      } else {
        const side = P.w > P.h * 2;
        const bw = side ? Layout.RADAR_BUTTON : P.w, bh = side ? P.h : 48;
        place(el.radar, { x: 0, y: 0, w: side ? P.w - bw - 8 : P.w, h: side ? P.h : P.h - bh - 8 });
        place(el.radarB, { x: P.w - bw, y: P.h - bh, w: bw, h: bh });
      }
    });
    el.pad.innerHTML = boxes.pad && boxes.pad.w > 100 ? '' : icons.pad;
    render();
    drawSticks();
    drawRadar();
  }
  addEventListener('resize', layout);

  // what the screen shows
  function render() {
    const steering = connected && drive.steering;
    el.connect.innerHTML = connected ? two('● HC-05', 'connected') : connecting ? two('Connecting', 'pick the car') : two('Connect', 'to the car');
    el.connect.classList.toggle('on', connected);
    el.song.textContent = connected && car.song ? '♪  ' + car.song : '';
    const gp = pad.index >= 0;
    if (el.pad.style.width && parseInt(el.pad.style.width, 10) > 100)
      el.pad.innerHTML = gp ? two(esc(shortName(pad.id)), 'controller') : two('No controller', 'plug one in');
    el.pad.classList.toggle('on', gp);
    el.speed.innerHTML = two(S.slowTop ? 'SLOW' : 'FAST', 'speed');
    el.speed.classList.toggle('on', S.slowTop);
    el.style.innerHTML = two(S.style === 1 ? '2 STICKS' : '1 STICK', 'drive');
    const scanning = car.mode === 'RADAR';
    el.radarB.innerHTML = scanning ? 'STOP RADAR' : 'START RADAR';
    el.radarB.classList.toggle('on', scanning);
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
    drawLcd(steering);
  }

  function runningJob() {
    const m = car.mode;
    if (/^LOOK|^TURN [LR]$|^SEARCH$|^BOXED IN$/.test(m)) return 'I';
    return { 'CHAOS!': '1', DONUT: '3', 'TURN 180': 'U', 'AUTO FWD': '2', 'AUTO BCK': 'J', BRAKING: 'u', DANCE: '4', 'TURBO!': 'T', FOLLOW: 'M', KEEPER: 'K', 'SAVE!': 'K',
      GUARD: 'A', ARMING: 'A', INTRUDER: 'A', MUSIC: 'N', GESTURE: 'E', PET: 'P', 'Zzz...': 'P', BORED: 'P', 'PURR <3': 'P' }[m] || '';
  }

  const padColor = (l) => ({ A: '#5BD46A', B: '#FF6464', X: '#52A6FF', Y: '#FFD23F' }[l] || '#8D96A7');

  function flashJob(job) {
    const list = job === '%' || job === 'Q' || job === 'W' ? [el.speed] : jobButtons[job] || [];
    for (const b of list) { b.classList.add('flash'); setTimeout(() => b.classList.remove('flash'), 140); }
  }

  // the green LCD
  function drawLcd(steering) {
    const c = el.lcd, g = c.getContext('2d'), d = dpr(), w = c.width / d, h = c.height / d;
    if (!w) return;
    g.setTransform(d, 0, 0, d, 0, 0);
    g.clearRect(0, 0, w, h);
    rrect(g, 0, 0, w, h, 12); g.fillStyle = '#07120B'; g.fill();
    g.fillStyle = 'rgba(0,0,0,.07)';
    for (let y = 2; y < h; y += 3) g.fillRect(0, y, w, 1);
    rrect(g, .75, .75, w - 1.5, h - 1.5, 12); g.strokeStyle = '#2A313B'; g.lineWidth = 1.5; g.stroke();
    let top, big, bottom = '', warn = false;
    if (!connected) { top = connecting ? 'CONNECTING...' : 'NOT CONNECTED - CLICK HERE TO CONNECT'; big = connecting ? '...' : '--'; }
    else {
      const old = !heardStatus && now() - connectedAt > 3000;
      top = '● CONNECTED' + (!heardStatus ? '' : drive.steering ? '  STEERING' : '  LETTERS (old car program)') + (lastSent ? '   sent ' + lastSent : '');
      if (old) { top = '● OLD CAR PROGRAM - upload Robot_Car_Main'; warn = true; } else if (heardStatus && !heardSong) top += '   (no radar: old program)';
      big = modeText();
      bottom = `${car.muted ? 'SOUND OFF' : 'SOUND ON'}   ${car.football ? 'SENSOR OFF' : 'SENSOR ON'}   ${(drive.steering || !heardStatus ? S.slowTop : car.slow) ? 'SLOW' : 'FAST'}   ${car.batteryLow ? '⚠ LOW BATTERY' : 'BATTERY OK'}`;
    }
    const pad = 12, bar = steering ? 8 : 0, x0 = pad + (steering ? bar + 8 : 0), tw = w - 2 * x0;
    const small = Math.min(h * 0.16, 13);
    let bigSize = Math.min(h * 0.38, 34);
    g.font = `700 ${bigSize}px ${getComputedStyle(document.body).getPropertyValue('--mono')}`;
    const fit = g.measureText(big).width;
    if (fit > tw) bigSize *= tw / fit;
    const mono = getComputedStyle(document.body).getPropertyValue('--mono');
    g.font = `700 ${small}px ${mono}`;
    g.fillStyle = warn ? '#FFC857' : '#3D8A55';
    g.fillText(ellipsize(g, top, tw), x0, pad * 0.6 + small);
    g.fillStyle = '#8CF5A8';
    g.fillText(ellipsize(g, bottom, tw), x0, h - pad * 0.7);
    g.font = `700 ${bigSize}px ${mono}`;
    g.fillText(big, x0, h / 2 + bigSize * 0.36);
    if (steering) { wheelBar(g, pad, drive.left(), h, pad); wheelBar(g, w - pad - 8, drive.right(), h, pad); }
  }
  function wheelBar(g, x, v, h, pad) {
    const top = pad * 0.7, bot = h - pad * 0.7, mid = (top + bot) / 2, len = (bot - top) / 2 * Math.abs(v) / 31;
    g.fillStyle = 'rgba(61,138,85,.35)'; rrect(g, x, top, 8, bot - top, 4); g.fill();
    g.fillStyle = '#8CF5A8';
    if (v) { rrect(g, x, v > 0 ? mid - len : mid, 8, len, 4); g.fill(); }
    g.fillRect(x - 2, mid - 1, 12, 2);
  }
  function ellipsize(g, s, w) { if (g.measureText(s).width <= w) return s; while (s.length > 1 && g.measureText(s + '…').width > w) s = s.slice(0, -1); return s + '…'; }
  function modeText() {
    if (!car.mode) return '...';
    if (car.mode === 'DANCE') { const s = Math.floor((now() - car.modeSince) / 1000); return `DANCE ${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}${car.song ? '  ' + car.song : ''}`; }
    if (car.mode === 'RADAR') { const n = car.radarNearest(now()); return 'RADAR' + (n >= 0 ? `  near ${n} cm` : ''); }
    if (car.mode === 'DRIVE' && drive.steering) return `DRIVE  ${Math.round(100 * Math.max(Math.abs(drive.left()), Math.abs(drive.right())) / 31)}%`;
    return car.mode;
  }
  function rrect(g, x, y, w, h, r) { g.beginPath(); g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r); g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath(); }

  // the radar
  function drawRadar() {
    const c = el.radar, g = c.getContext('2d'), d = dpr(), w = c.width / d, h = c.height / d;
    if (!w || page !== 2) return;
    g.setTransform(d, 0, 0, d, 0, 0);
    g.clearRect(0, 0, w, h);
    rrect(g, 0, 0, w, h, 12); g.fillStyle = '#07120B'; g.fill();
    const pad = 8, cx = w / 2, cy = h - pad, r = Math.min(w / 2 / Math.sin(70 * Math.PI / 180), h - pad * 2.5);
    g.strokeStyle = '#3D8A55'; g.lineWidth = 1; g.fillStyle = '#3D8A55'; g.font = `10px ${getComputedStyle(document.body).getPropertyValue('--mono')}`;
    for (let k = 1; k <= 3; k++) { g.beginPath(); g.arc(cx, cy, r * k / 3, -160 * Math.PI / 180, -20 * Math.PI / 180); g.stroke(); g.fillText(k * 50 + 'cm', cx + 3, cy - r * k / 3 + 11); }
    g.fillStyle = 'rgba(124,230,150,.16)'; g.beginPath(); g.moveTo(cx, cy); g.arc(cx, cy, r, -140 * Math.PI / 180, -40 * Math.PI / 180); g.fill();
    g.fillStyle = '#8CF5A8'; g.beginPath(); g.arc(cx, cy, 5, 0, 7); g.fill();
    const t = now();
    let live = false;
    for (let k = 0; k < 11; k++) {
      const age = t - car.radarAt[k];
      if (!car.radarAt[k] || age > 6000) continue;
      live = live || age < 1000;
      if (car.radarCm[k] >= 150) continue;
      const a = (5 - k) * 10 * Math.PI / 180, dd = r * car.radarCm[k] / 150;
      g.globalAlpha = Math.max(0.15, 1 - age / 6000);
      g.beginPath(); g.arc(cx + dd * Math.sin(a), cy - dd * Math.cos(a), 5, 0, 7); g.fill();
    }
    g.globalAlpha = 1;
    if (live) {
      const a = (5 - car.radarLook) * 10 * Math.PI / 180;
      g.strokeStyle = '#8CF5A8'; g.lineWidth = 2; g.beginPath(); g.moveTo(cx, cy); g.lineTo(cx + r * Math.sin(a), cy - r * Math.cos(a)); g.stroke();
    }
  }

  // the on-screen sticks (mouse or touch)
  const sticks = [
    { c: el.stick, x: 0, y: 0, id: null },
    { c: el.stick2, x: 0, y: 0, id: null },
  ];
  let touchT = 0, touchS = 0;
  const axesOf = (i) => (S.style !== 1 ? 'both' : i === 0 ? 'ud' : 'lr');
  sticks.forEach((st, i) => {
    const move = (e) => {
      const r = st.c.getBoundingClientRect(), geo = stickGeo(st.c, i);
      let x = (e.clientX - r.left - geo.cx) / (geo.r * 0.62), y = (e.clientY - r.top - geo.cy) / (geo.r * 0.62);
      if (axesOf(i) === 'ud') x = 0;
      if (axesOf(i) === 'lr') y = 0;
      const len = Math.hypot(x, y);
      if (len > 1) { x /= len; y /= len; }
      st.x = x; st.y = y;
      stickChanged(i);
    };
    st.c.addEventListener('pointerdown', (e) => { st.id = e.pointerId; st.c.setPointerCapture(e.pointerId); move(e); });
    st.c.addEventListener('pointermove', (e) => { if (st.id === e.pointerId) move(e); });
    const up = (e) => { if (st.id !== e.pointerId) return; st.id = null; st.x = st.y = 0; stickChanged(i); };
    st.c.addEventListener('pointerup', up);
    st.c.addEventListener('pointercancel', up);
  });
  function stickChanged(i) {
    const st = sticks[i];
    if (S.style === 1) {
      if (i === 0) touchT = -DriveLogic.deadZone1(st.y, S.touchDead); else touchS = DriveLogic.deadZone1(st.x, S.touchDead);
    } else {
      const k = DriveLogic.deadZone2(st.x, st.y, S.touchDead);
      touchT = Math.fround(-st.y * k); touchS = Math.fround(st.x * k);
    }
    if ((touchT || touchS) && !connected) notConnected();
    setSource(SRC_TOUCH, touchT, touchS);
    drawSticks();
  }
  function showSticks(t, s) {
    if (S.style === 1) { if (sticks[0].id === null) sticks[0].y = -t; if (sticks[1].id === null) sticks[1].x = s; }
    else if (sticks[0].id === null) { sticks[0].x = s; sticks[0].y = -t; }
    drawSticks();
  }
  function stickGeo(c, i) {
    const w = c.clientWidth, h = c.clientHeight - 18;
    const r = axesOf(i) === 'ud' ? Math.min(w * 0.5, h * 0.5) * 0.92 : Math.min(w, h) / 2 * 0.92;
    return { cx: w / 2, cy: h / 2, r };
  }
  function drawSticks() {
    sticks.forEach((st, i) => {
      const c = st.c, g = c.getContext('2d'), d = dpr(), w = c.width / d, h = c.height / d;
      if (!w || c.style.display === 'none') return;
      g.setTransform(d, 0, 0, d, 0, 0);
      g.clearRect(0, 0, w, h);
      const { cx, cy, r } = stickGeo(c, i), kr = r * 0.38, ax = axesOf(i);
      const activeNow = st.id !== null || st.x || st.y;
      g.fillStyle = '#161A21'; g.strokeStyle = activeNow ? '#FF7A1A' : '#353D4A'; g.lineWidth = 1.5;
      if (ax === 'both') {
        g.beginPath(); g.arc(cx, cy, r, 0, 7); g.fill(); g.stroke();
        g.strokeStyle = 'rgba(53,61,74,.6)'; g.lineWidth = 1; g.beginPath(); g.arc(cx, cy, r * 0.55, 0, 7); g.stroke();
      } else {
        const half = kr * 1.15;
        if (ax === 'ud') rrect(g, cx - half, cy - r, 2 * half, 2 * r, half); else rrect(g, cx - r, cy - half, 2 * r, 2 * half, half);
        g.fill(); g.stroke();
      }
      g.strokeStyle = '#586070'; g.lineWidth = 2; g.lineCap = 'round';
      const t = 5, m = r - 10, chev = (x1, y1, x2, y2, x3, y3) => { g.beginPath(); g.moveTo(x1, y1); g.lineTo(x2, y2); g.lineTo(x3, y3); g.stroke(); };
      if (ax !== 'lr') { chev(cx - t, cy - m + t, cx, cy - m, cx + t, cy - m + t); chev(cx - t, cy + m - t, cx, cy + m, cx + t, cy + m - t); }
      if (ax !== 'ud') { chev(cx - m + t, cy - t, cx - m, cy, cx - m + t, cy + t); chev(cx + m - t, cy - t, cx + m, cy, cx + m - t, cy + t); }
      g.fillStyle = '#8D96A7'; g.font = `600 11px ${getComputedStyle(document.body).getPropertyValue('--font')}`; g.textAlign = 'center';
      g.fillText(ax === 'ud' ? 'T H R O T T L E' : ax === 'lr' ? 'S T E E R' : 'D R I V E', cx, h - 3);
      const px = cx + st.x * (r - kr), py = cy + st.y * (r - kr);
      if (activeNow) { g.strokeStyle = 'rgba(255,122,26,.35)'; g.lineWidth = kr * 0.9; g.beginPath(); g.moveTo(cx, cy); g.lineTo(px, py); g.stroke(); }
      g.fillStyle = '#171A20'; g.beginPath(); g.arc(px, py + 3, kr, 0, 7); g.fill();
      g.fillStyle = activeNow ? '#353B45' : '#262C36'; g.beginPath(); g.arc(px, py, kr, 0, 7); g.fill();
      g.strokeStyle = activeNow ? '#FF7A1A' : '#353D4A'; g.lineWidth = 2; g.beginPath(); g.arc(px, py, kr * 0.62, 0, 7); g.stroke();
    });
  }

  // ------------------------------------------------------------------ dialogs
  document.querySelectorAll('[data-close]').forEach((b) => b.addEventListener('click', () => b.closest('dialog').close()));
  function esc(s) { return String(s).replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c])); }
  function message(title, html, buttons) {
    $('msgTitle').textContent = title;
    $('msgBody').innerHTML = html;
    if (buttons) {
      const row = document.createElement('div'); row.className = 'seg';
      for (const [label, fn] of buttons) { const b = document.createElement('button'); b.className = 'key'; b.textContent = label; b.onclick = () => { $('message').close(); fn(); }; row.appendChild(b); }
      $('msgBody').appendChild(row);
    }
    if (!$('message').open) $('message').showModal();
  }
  let toastTimer;
  function toast(s) { const t = $('toast'); t.textContent = s; t.classList.add('show'); clearTimeout(toastTimer); toastTimer = setTimeout(() => t.classList.remove('show'), 2200); }
  function confirmDisconnect() { message('Disconnect?', '<p>The car stops.</p>', [['Disconnect', () => disconnect(false)]]); }

  function openHelp() {
    const lines = [];
    for (let b = 0; b < 16; b++) { const j = map.job(b, false); if (j !== '-') lines.push(`${ControllerMap.BUTTON_NAMES[b]} = ${j === '^' ? 'SHIFT (hold)' : ControllerMap.jobName(j)}`); }
    const sh = ControllerMap.BUTTON_NAMES[map.buttonFor('^')] || 'SHIFT';
    for (let b = 0; b < 16; b++) { const j = map.job(b, true); if (j !== '-' && j !== '^') lines.push(`${sh} + ${ControllerMap.BUTTON_NAMES[b]} = ${ControllerMap.jobName(j)}`); }
    $('helpBody').innerHTML = `
      <p><b>Connect:</b> pair the car's <b>HC-05</b> with this computer first (Windows: Settings &gt; Bluetooth &amp; devices &gt; Add device, PIN 1234 or 0000).
      Then click <b>Connect</b> and pick it (it may be called "HC-05" or a COM port). Works in Chrome and Edge.</p>
      <p><b>Drive:</b> drag the stick with the mouse or a finger - a little = slowly, all the way = full speed. Or use the keyboard:
      <kbd>W</kbd><kbd>A</kbd><kbd>S</kbd><kbd>D</kbd> or the arrows, <kbd>Space</kbd> STOP, <kbd>Enter</kbd> dance, <kbd>H</kbd> horn, <kbd>K</kbd> kick,
      <kbd>T</kbd> turbo, <kbd>B</kbd> brake, <kbd>R</kbd> radar, <kbd>M</kbd> mute, <kbd>F</kbd> football, <kbd>Q</kbd> fast / slow.</p>
      <p><b>GameSir Cyclone 2 on a computer:</b> plug in its 2.4 GHz dongle (Home light <b>green</b> = XInput), or connect it by cable.
      Left stick = throttle, right stick = steering. Click the controller button (top) to see every button live.</p>
      <pre>${esc(lines.join('\n'))}</pre>
      <p style="margin-top:12px">With the newest car program the steering is smooth (each side of the car gets its own speed); older programs get the 6 driving letters.
      Right-click the green screen for the link log. On an Android phone, the app is better: <a style="color:var(--accent)" href="${APP_LINK}">download it</a>.</p>`;
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
    seg('On-screen controls', ['1 stick', '2 sticks'], S.style === 1 ? 1 : 0, (i) => setStyle(i));
    seg('Top speed', ['Fast', 'Slow'], S.slowTop ? 1 : 0, (i) => setSlowTop(i === 1));
    slider('Steering feel', 'Direct', 'Gentle in the middle', Math.round(S.feel * 100), 100, (v) => { save('feel', v / 100); drive.setFeel(v / 100); });
    slider('On-screen stick dead zone', '0 %', '25 %', Math.round(S.touchDead * 100), 25, (v) => save('touchDead', v / 100), '%');
    sec('GAME CONTROLLER');
    slider('Stick dead zone', '0 %', '30 %', Math.round(S.padDead * 100), 30, (v) => save('padDead', v / 100), '%');
    toggle('The left stick steers too', 'when the right stick is in the middle', S.leftSteers, (v) => save('leftSteers', v));
    toggle('Show the controller\'s buttons on the screen', 'little A / B / RT ... badges on the buttons', S.overlay, (v) => save('overlay', v));
    const m = card(); m.innerHTML = '<button class="key" style="width:100%;height:48px">Controller buttons and remap  ›</button>';
    m.firstChild.onclick = () => { $('settings').close(); openController(); };
    sec('OTHER');
    const r = card(); r.innerHTML = '<button class="key" style="width:100%;height:48px;color:var(--stop)">Reset everything to the defaults</button>';
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
    $('jobsTitle').textContent = button < 0 ? 'Remap: choose a job' : `${ControllerMap.BUTTON_NAMES[button]}${padShift ? ' (with LB held)' : ''} does ...`;
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
    $('padTitle').textContent = gp ? '🎮 ' + shortName(pad.id) + '  ·  connected' : 'No controller connected';
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
    // body
    g.beginPath(); g.moveTo(X(230), Y(122)); g.lineTo(X(770), Y(122));
    g.bezierCurveTo(X(880), Y(122), X(930), Y(190), X(952), Y(300)); g.bezierCurveTo(X(978), Y(430), X(992), Y(565), X(900), Y(605));
    g.bezierCurveTo(X(828), Y(636), X(772), Y(565), X(722), Y(505)); g.bezierCurveTo(X(660), Y(470), X(340), Y(470), X(278), Y(505));
    g.bezierCurveTo(X(228), Y(565), X(172), Y(636), X(100), Y(605)); g.bezierCurveTo(X(8), Y(565), X(22), Y(430), X(48), Y(300));
    g.bezierCurveTo(X(70), Y(190), X(120), Y(122), X(230), Y(122)); g.closePath();
    g.fillStyle = '#1C2028'; g.fill(); g.strokeStyle = '#353D4A'; g.lineWidth = 1.5; g.stroke();
    // triggers + bumpers
    [6, 7].forEach((b) => {
      const cx = AT[b][0]; rrect(g, X(cx - 70), Y(22), 140 * s, 50 * s, 14 * s); g.fillStyle = '#262C36'; g.fill();
      const v = pad.trig[b - 6];
      if (v > 0.01) { g.save(); g.clip(); g.fillStyle = down(b) ? '#FF7A1A' : 'rgba(255,122,26,.55)'; g.fillRect(X(cx - 70), Y(72) - 50 * s * v, 140 * s, 50 * s * v); g.restore(); }
    });
    [4, 5].forEach((b) => { rrect(g, X(AT[b][0] - 92), Y(90), 184 * s, 28 * s, 14 * s); g.fillStyle = down(b) ? '#FF7A1A' : '#262C36'; g.fill(); });
    // sticks
    const stick = (x, y, sx, sy, click) => {
      g.fillStyle = '#12151B'; g.beginPath(); g.arc(X(x), Y(y), 66 * s, 0, 7); g.fill();
      const moved = Math.abs(sx) > 0.05 || Math.abs(sy) > 0.05, kx = x + sx * 26, ky = y + sy * 26;
      g.fillStyle = down(click) ? '#FF7A1A' : moved ? '#353B45' : '#262C36'; g.beginPath(); g.arc(X(kx), Y(ky), 42 * s, 0, 7); g.fill();
      g.strokeStyle = moved ? '#FF7A1A' : '#353D4A'; g.lineWidth = 2.5 * s; g.beginPath(); g.arc(X(kx), Y(ky), 28 * s, 0, 7); g.stroke();
    };
    stick(290, 238, pad.lx, pad.ly, 8); stick(610, 400, pad.rx, pad.ry, 9);
    // D-pad
    g.fillStyle = '#262C36'; rrect(g, X(368), Y(334), 44 * s, 128 * s, 6 * s); g.fill(); rrect(g, X(326), Y(376), 128 * s, 44 * s, 6 * s); g.fill();
    [12, 13, 14, 15].forEach((b) => { if (down(b)) { g.fillStyle = '#FF7A1A'; rrect(g, X(AT[b][0] - 22), Y(AT[b][1] - 22), 44 * s, 44 * s, 6 * s); g.fill(); } });
    // face buttons
    ['A', 'B', 'X', 'Y'].forEach((l, b) => {
      g.fillStyle = down(b) ? padColor(l) : '#262C36'; g.beginPath(); g.arc(X(AT[b][0]), Y(AT[b][1]), 27 * s, 0, 7); g.fill();
      g.fillStyle = down(b) ? '#0D0F13' : padColor(l); g.font = `700 ${26 * s}px ${getComputedStyle(document.body).getPropertyValue('--font')}`; g.textAlign = 'center';
      g.fillText(l, X(AT[b][0]), Y(AT[b][1] + 9));
    });
    [11, 10].forEach((b) => { rrect(g, X(AT[b][0] - 22), Y(AT[b][1] - 12), 44 * s, 24 * s, 12 * s); g.fillStyle = down(b) ? '#FF7A1A' : '#262C36'; g.fill(); });
    g.fillStyle = pad.home ? '#FF7A1A' : '#262C36'; g.beginPath(); g.arc(X(500), Y(178), 22 * s, 0, 7); g.fill();
    g.strokeStyle = '#FF7A1A'; g.lineWidth = 2.5 * s; g.stroke();
    // labels
    labelBoxes = [];
    const size = Math.max(11, Math.min(14, h / 28));
    const top = Math.max(0, oy), bottom = Math.min(h - 24, oy + 640 * s);
    const font = getComputedStyle(document.body).getPropertyValue('--font');
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
        rrect(g, bx.x, bx.y, bx.w, bx.h, 8); g.fillStyle = lit ? 'rgba(255,122,26,.18)' : '#1B2028'; g.fill();
        g.textAlign = 'left';
        g.font = `700 ${size}px ${font}`; g.fillStyle = b >= 0 && b <= 3 ? padColor(ControllerMap.BUTTON_NAMES[b]) : '#8D96A7';
        g.fillText(who, x + 8, cy + size * 0.36);
        const nameW = g.measureText('D-pad right ').width;
        g.font = `500 ${size}px ${font}`; g.fillStyle = lit ? '#FF7A1A' : '#ECEFF4';
        g.fillText(ellipsize(g, job, col - nameW - 16), x + 8 + nameW, cy + size * 0.36);
        const at = row === 100 ? [290, 238] : row === 101 ? [610, 400] : AT[row];
        g.strokeStyle = lit ? '#FF7A1A' : 'rgba(88,96,112,.6)'; g.lineWidth = 1;
        g.beginPath(); g.moveTo(leftSide ? x + col : x, cy); g.lineTo(X(at[0]), Y(at[1])); g.stroke();
        y += rowH;
      }
    };
    column(LEFT, 0, true); column(RIGHT, w - col, false);
    g.fillStyle = '#586070'; g.font = `500 ${size}px ${font}`; g.textAlign = 'center';
    g.fillText('HOME = the computer\'s   ·   M1 / M2 (back) = copy any button in the GameSir app', w / 2, h - 6);
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

  // ------------------------------------------------------------------ the clock
  let last = now(), lastSlow = 0;
  setInterval(() => {
    const t = now();
    keyboardTick(t - last);
    last = t;
    padTick();
    drive.tick(t);
    if (t - lastSlow > 100) { lastSlow = t; drawLcd(connected && drive.steering); if (page === 2) drawRadar(); }
  }, 20);
  addEventListener('beforeunload', () => { if (connected) drive.halt(now()); });

  showPage(0);
  layout();
  window.__robotCar = { drive, car, map, S, onLine, layout, setStyle, get connected() { return connected; }, set connected(v) { connected = v; } };
})();
