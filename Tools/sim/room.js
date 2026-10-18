'use strict';
// A virtual room for SEARCH tests: the car drives around from its own motor
// signals (differential drive), and the HC-SR04 answers with the distance to
// the nearest wall inside a 20-degree cone in the direction the servo points.

function makeRoom(opts = {}) {
  const VMAX = opts.vmax || 50;            // cm/s at full PWM
  const TRACK = opts.track || 15;          // cm between left and right wheels
  const TURN_EFF = opts.turnEff || 0.5;    // real 4WD cars skid: they turn slower than the wheel speeds say
  const R = opts.radius || 9;              // car "size" for crash detection (cm)
  const walls = opts.walls;                // [[x1,y1,x2,y2], ...]
  const st = { x: opts.x, y: opts.y, th: opts.th * Math.PI / 180, lastT: null, crashes: 0, crashing: false,
    crashTimes: [], path: [], odo: 0, maxStill: 0, stillSince: 0, odoLog: [] };

  const segDist = (px, py, w) => {        // distance from point to wall segment
    const [x1, y1, x2, y2] = w, dx = x2 - x1, dy = y2 - y1;
    const u = Math.max(0, Math.min(1, ((px - x1) * dx + (py - y1) * dy) / (dx * dx + dy * dy)));
    return Math.hypot(px - (x1 + u * dx), py - (y1 + u * dy));
  };
  const nearestWall = (px, py) => Math.min(...walls.map((w) => segDist(px, py, w)));

  const ray = (ox, oy, a) => {             // distance along a ray to the first wall
    let best = Infinity;
    const dx = Math.cos(a), dy = Math.sin(a);
    for (const [x1, y1, x2, y2] of walls) {
      const ex = x2 - x1, ey = y2 - y1, den = dx * ey - dy * ex;
      if (Math.abs(den) < 1e-9) continue;
      const t = ((x1 - ox) * ey - (y1 - oy) * ex) / den;
      const u = ((x1 - ox) * dy - (y1 - oy) * dx) / den;
      if (t > 0 && u >= 0 && u <= 1) best = Math.min(best, t);
    }
    return best;
  };

  const wheel = (s) => { const m = s.match(/^([FB-])(\d+)$/); if (!m || m[1] === '-') return 0; return (m[1] === 'F' ? 1 : -1) * (+m[2] / 255) * VMAX; };

  function onSample(t, motorStr) {
    if (st.lastT === null) { st.lastT = t; return; }
    const dt = t - st.lastT; st.lastT = t;
    const [l, r] = motorStr.split(' ');
    const vl = wheel(l.slice(1)), vr = wheel(r.slice(1));
    const v = (vl + vr) / 2, w = (vr - vl) / TRACK * TURN_EFF;
    const nx = st.x + v * Math.cos(st.th) * dt, ny = st.y + v * Math.sin(st.th) * dt;
    st.th += w * dt;
    if (nearestWall(nx, ny) >= R) { st.odo += Math.hypot(nx - st.x, ny - st.y); st.x = nx; st.y = ny; }
    else if (!st.crashing) { st.crashing = true; st.crashes++; st.crashTimes.push(+t.toFixed(2)); }
    if (nearestWall(st.x, st.y) >= R + 1) st.crashing = false;
    if (Math.abs(v) > 1) { st.maxStill = Math.max(st.maxStill, t - st.stillSince); st.stillSince = t; }
    if (st.path.length === 0 || t - st.path[st.path.length - 1][0] >= 0.25) st.path.push([+t.toFixed(2), +st.x.toFixed(1), +st.y.toFixed(1), Math.round(st.th * 180 / Math.PI)]);
    if (st.odoLog.length === 0 || t * 1000 - st.odoLog[st.odoLog.length - 1][0] >= 20) st.odoLog.push([t * 1000, st.odo]);
  }

  // distance driven (cm) up to time tMs
  function odoAt(tMs) {
    let best = 0;
    for (const [t, o] of st.odoLog) { if (t > tMs) break; best = o; }
    return best;
  }

  function distance(t, servoAngle) {
    const sx = st.x + 6 * Math.cos(st.th), sy = st.y + 6 * Math.sin(st.th);   // sensor sits at the front
    const look = (servoAngle - 90) * (opts.mirror ? -1 : 1);                    // mirror: servo mounted the other way round
    const a = st.th + look * Math.PI / 180;                                       // servo 140 = looks 50 deg left
    let d = Infinity;
    for (let k = -10; k <= 10; k += 2.5) d = Math.min(d, ray(sx, sy, a + k * Math.PI / 180));
    return d > 400 ? null : Math.round(d * 10) / 10;
  }

  return { onSample, distance, odoAt, state: st };
}

module.exports = { makeRoom };
