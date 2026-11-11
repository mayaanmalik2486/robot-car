'use strict';
// The live 3D picture of the car: a little 3D engine of our own (no library), the same one
// as the Android app's SceneView.java. The car is built from boxes and wheels, lit from above,
// and drawn back to front. It drives over a glowing grid: the wheels spin, the floor rushes
// by and turns when you steer, the car leans in bends, bounces when it dances, its sensor
// head sweeps in radar mode (with the dots it sees on the floor), and a red wall appears
// when it is blocked.
(function (root) {
  const TAU = Math.PI * 2;

  // ---------- building the car (units: about 10 cm) ----------
  function quad(a, b, c, d, color) { return { v: [a, b, c, d], c: color }; }

  // a box: centre, half sizes, colour (the top a little lighter, like a painted toy)
  function box(out, cx, cy, cz, hx, hy, hz, col, topCol) {
    const x0 = cx - hx, x1 = cx + hx, y0 = cy - hy, y1 = cy + hy, z0 = cz - hz, z1 = cz + hz;
    out.push(quad([x0, y1, z0], [x0, y1, z1], [x1, y1, z1], [x1, y1, z0], topCol || col));   // top
    out.push(quad([x0, y0, z1], [x0, y0, z0], [x1, y0, z0], [x1, y0, z1], col));             // bottom
    out.push(quad([x0, y0, z1], [x1, y0, z1], [x1, y1, z1], [x0, y1, z1], col));             // front (+z)
    out.push(quad([x1, y0, z0], [x0, y0, z0], [x0, y1, z0], [x1, y1, z0], col));             // back
    out.push(quad([x1, y0, z1], [x1, y0, z0], [x1, y1, z0], [x1, y1, z1], col));             // right (+x)
    out.push(quad([x0, y0, z0], [x0, y0, z1], [x0, y1, z1], [x0, y1, z0], col));             // left
  }

  // a wheel: a short cylinder lying on its side (axis = x), centred on 0,0,0. side: 1 = the outside faces +x
  function wheel(out, r, half, n, tyre, hubA, hubB) {
    for (let i = 0; i < n; i++) {
      const a0 = TAU * i / n, a1 = TAU * (i + 1) / n;
      const y0 = Math.cos(a0) * r, z0 = Math.sin(a0) * r, y1 = Math.cos(a1) * r, z1 = Math.sin(a1) * r;
      out.push(quad([-half, y0, z0], [half, y0, z0], [half, y1, z1], [-half, y1, z1], tyre));
      const h = r * 0.62, hy0 = Math.cos(a0) * h, hz0 = Math.sin(a0) * h, hy1 = Math.cos(a1) * h, hz1 = Math.sin(a1) * h;
      const hub = i % 2 ? hubA : hubB;                         // two tones, so you see it spin
      out.push({ v: [[half, 0, 0], [half, hy1, hz1], [half, hy0, hz0]], c: hub });
      out.push({ v: [[-half, 0, 0], [-half, hy0, hz0], [-half, hy1, hz1]], c: hub });
      out.push(quad([half, hy0, hz0], [half, hy1, hz1], [half, y1, z1], [half, y0, z0], tyre));      // the tyre's side wall
      out.push(quad([-half, hy1, hz1], [-half, hy0, hz0], [-half, y0, z0], [-half, y1, z1], tyre));
    }
  }

  // an "eye" of the distance sensor: a short cylinder looking forward (axis = z)
  function eye(out, cx, cy, cz, r, len, n, side, cap, pupil) {
    for (let i = 0; i < n; i++) {
      const a0 = TAU * i / n, a1 = TAU * (i + 1) / n;
      const x0 = cx + Math.cos(a0) * r, y0 = cy + Math.sin(a0) * r, x1 = cx + Math.cos(a1) * r, y1 = cy + Math.sin(a1) * r;
      out.push(quad([x0, y0, cz], [x1, y1, cz], [x1, y1, cz + len], [x0, y0, cz + len], side));
      out.push({ v: [[cx, cy, cz + len], [x0, y0, cz + len], [x1, y1, cz + len]], c: cap });
      const p = 0.55;
      out.push({ v: [[cx, cy, cz + len + 0.01], [cx + (x0 - cx) * p, cy + (y0 - cy) * p, cz + len + 0.01], [cx + (x1 - cx) * p, cy + (y1 - cy) * p, cz + len + 0.01]], c: pupil });
    }
  }

  const ORANGE = [255, 122, 26], ORANGE_TOP = [255, 150, 60], DARK = [34, 40, 58], DARKER = [22, 26, 40], BOARD = [26, 150, 160],
    BOARD_TOP = [40, 190, 196], TYRE = [24, 26, 34], HUB_A = [255, 205, 60], HUB_B = [230, 170, 30], SILVER = [200, 208, 224],
    PUPIL = [40, 46, 64], BLUE = [58, 120, 255], BLUE_TOP = [90, 150, 255], SCREEN = [10, 14, 26], GLOW = [80, 240, 225], RED = [255, 70, 100];

  function buildCar() {
    const body = [], head = [], wheelGeo = [];
    box(body, 0, 0.50, 0, 0.78, 0.07, 1.30, DARK, [48, 56, 80]);              // lower deck
    box(body, 0, 0.72, -0.05, 0.70, 0.15, 1.12, ORANGE, ORANGE_TOP);          // the orange body
    box(body, 0, 0.93, -0.55, 0.46, 0.07, 0.42, BOARD, BOARD_TOP);            // the Arduino
    box(body, 0, 1.00, -0.55, 0.30, 0.02, 0.26, DARKER);                      // (its chip)
    box(body, 0, 0.95, 0.30, 0.36, 0.09, 0.22, DARKER, [40, 46, 66]);         // the screen's block
    box(body, 0, 1.045, 0.30, 0.28, 0.006, 0.15, SCREEN);                     // the OLED
    box(body, -0.12, 1.055, 0.30, 0.06, 0.004, 0.08, GLOW);                   // its two eyes
    box(body, 0.12, 1.055, 0.30, 0.06, 0.004, 0.08, GLOW);
    box(body, 0, 0.60, -1.28, 0.50, 0.10, 0.06, RED, [255, 120, 140]);        // tail light
    box(body, 0, 0.93, 0.92, 0.16, 0.06, 0.14, BLUE, BLUE_TOP);               // the servo
    // the sensor head (turns on the servo)
    box(head, 0, 1.13, 0.98, 0.46, 0.14, 0.04, BLUE, BLUE_TOP);
    eye(head, -0.22, 1.13, 1.02, 0.115, 0.12, 10, SILVER, [228, 234, 246], PUPIL);
    eye(head, 0.22, 1.13, 1.02, 0.115, 0.12, 10, SILVER, [228, 234, 246], PUPIL);
    wheel(wheelGeo, 0.44, 0.16, 12, TYRE, HUB_A, HUB_B);
    return { body, head, wheelGeo, headPivot: [0, 0, 0.98], wheels: [[0.97, 0.44, 0.82, 0], [-0.97, 0.44, 0.82, 1], [0.97, 0.44, -0.82, 0], [-0.97, 0.44, -0.82, 1]] };   // (with z = forward, +x is the car's LEFT side)
  }

  // ---------- the scene ----------
  class Scene {
    constructor() {
      this.car = buildCar();
      this.az = 0.75;             // where the camera stands (an angle round the car)
      this.spin = [0, 0];         // how far the left / right wheels have turned
      this.x = 0; this.z = 0; this.heading = 0;
      this.v = 0; this.w = 0;     // speed and turning, smoothed
      this.roll = 0; this.pitch = 0; this.head = 0; this.bob = 0; this.idle = 9; this.t = 0; this.wall = 0;
      this.poly = [];
    }

    /**
     * Moves the scene on by dt seconds. left / right: the wheel speeds -1 ... 1; mode: what the car
     * says it is doing; look: where the sensor looks -1 (right) ... 1 (left).
     */
    update(dt, left, right, mode, look, headWanted) {
      dt = Math.min(dt, 0.05);
      this.t += dt;
      let l = left, r = right;
      const dance = mode === 'DANCE', radar = mode === 'RADAR';
      if (dance) { l = Math.sin(this.t * 6.3) * 0.5; r = -l; }                     // a little dance wiggle
      else if (mode === 'DONUT') { l = 0.9; r = -0.9; }
      else if (mode === 'TURBO!') { l = r = 1; }
      else if (l === 0 && r === 0) {
        if (/^(FORWARD|AUTO FWD|SEARCH|CHAOS!|FOLLOW|PET)$/.test(mode)) l = r = 0.6;
        else if (/^(BACKWARD|AUTO BCK)$/.test(mode)) l = r = -0.6;
        else if (/^(LEFT|TURN L|LOOK L)$/.test(mode)) { l = -0.5; r = 0.5; }
        else if (/^(RIGHT|TURN R|LOOK R|TURN 180)$/.test(mode)) { l = 0.5; r = -0.5; }
        else if (mode === 'CURVE L') { l = 0.2; r = 0.9; } else if (mode === 'CURVE R') { l = 0.9; r = 0.2; }
      }
      const k = 1 - Math.exp(-dt * 9);
      const v = (l + r) / 2 * 7.5, w = (l - r) * 1.7;
      const accel = (v - this.v);
      this.v += (v - this.v) * k;
      this.w += (w - this.w) * k;
      this.heading -= this.w * dt;                                               // (turning right = toward -x)
      this.x += Math.sin(this.heading) * this.v * dt;
      this.z += Math.cos(this.heading) * this.v * dt;
      this.spin[0] += l * 16 * dt;
      this.spin[1] += r * 16 * dt;
      this.roll += (this.w * this.v * 0.012 - this.roll) * k;
      this.pitch += (-accel * 0.035 - this.pitch) * k * 0.6;
      this.bob = dance ? Math.abs(Math.sin(this.t * 6.3)) * 0.16 : this.bob * (1 - k);
      this.head += ((radar ? look : (headWanted || 0)) * 0.87 - this.head) * k;
      this.wall += ((mode === 'BLOCKED' ? 1 : 0) - this.wall) * k;
      const moving = Math.abs(this.v) > 0.15 || Math.abs(this.w) > 0.1;
      this.idle = moving ? 0 : this.idle + dt;
      if (this.idle > 1.5) this.az += dt * 0.22;                                   // standing still: the camera strolls round
      else {                                                                       // driving: the camera swings behind the car
        const want = Math.PI + 0.55;
        let d = (want - this.az) % TAU;
        if (d > Math.PI) d -= TAU; if (d < -Math.PI) d += TAU;
        this.az += d * (1 - Math.exp(-dt * 2.2));
      }
    }

    /** Draws the scene into a w x h area. radar: [{a: -1..1 (left +), d: 0..1}] dots, or null. */
    draw(g, w, h, mode, radar, hue) {
      const el = 0.40, dist = 8.2, ty = 0.62;
      const ex = dist * Math.cos(el) * Math.sin(this.az), ey = ty + dist * Math.sin(el), ez = dist * Math.cos(el) * Math.cos(this.az);
      // the camera's own axes
      let fx = -ex, fy = ty - ey, fz = -ez;
      const fl = Math.hypot(fx, fy, fz); fx /= fl; fy /= fl; fz /= fl;
      let rx = -fz, rz = fx; const rl = Math.hypot(rx, rz); rx /= rl; rz /= rl;      // right = forward x up
      const ux = -rz * fy, uy = rz * fx - rx * fz, uz = rx * fy;                      // up = right x forward
      // (in a wide, low picture the car stands right of the middle: the words are on the left)
      const focal = Math.min(h * 3.3, w * 1.25), cx = w > h * 2.1 ? w * 0.63 : w / 2, cy = h * 0.55;
      const cam = (p) => {
        const dx = p[0] - ex, dy = p[1] - ey, dz = p[2] - ez;
        return [dx * rx + dz * rz, dx * ux + dy * uy + dz * uz, dx * fx + dy * fy + dz * fz];
      };
      const px = (c) => [cx + focal * c[0] / c[2], cy - focal * c[1] / c[2]];

      // ----- the floor grid (it moves under the car)
      const blocked = this.wall > 0.02;
      const gridCol = mode === 'DANCE' ? `hsla(${(this.t * 120) % 360},95%,65%,` : blocked ? 'rgba(255,90,120,' : `hsla(${hue},92%,62%,`;
      const ch = Math.cos(this.heading), sh = Math.sin(this.heading);
      const toCar = (wx, wz) => { const dx = wx - this.x, dz = wz - this.z; return [dx * ch - dz * sh, 0, dx * sh + dz * ch]; };
      const N = 9, bx = Math.floor(this.x), bz = Math.floor(this.z), SEG = 6;
      g.lineWidth = Math.max(1, h / 260);
      for (let pass = 0; pass < 2; pass++)
        for (let i = -N; i <= N + 1; i++)
          for (let s = 0; s < SEG; s++) {
            const t0 = -N + (2 * N + 1) * s / SEG, t1 = -N + (2 * N + 1) * (s + 1) / SEG;
            const a = pass ? toCar(bx + i, bz + t0) : toCar(bx + t0, bz + i), b = pass ? toCar(bx + i, bz + t1) : toCar(bx + t1, bz + i);
            const far = Math.hypot((a[0] + b[0]) / 2, (a[2] + b[2]) / 2);
            const alpha = Math.max(0, 1 - far / (N - 0.5)) * 0.55;
            if (alpha <= 0.01) continue;
            let ca = cam(a), cb = cam(b);
            if (ca[2] < 0.4 && cb[2] < 0.4) continue;
            if (ca[2] < 0.4) ca = clip(cb, ca); else if (cb[2] < 0.4) cb = clip(ca, cb);
            const pa = px(ca), pb = px(cb);
            g.strokeStyle = gridCol + alpha.toFixed(3) + ')';
            g.beginPath(); g.moveTo(pa[0], pa[1]); g.lineTo(pb[0], pb[1]); g.stroke();
          }

      // ----- the shadow under the car
      g.fillStyle = 'rgba(0,0,0,.45)';
      g.beginPath();
      for (let i = 0; i < 18; i++) { const a = TAU * i / 18, p = px(cam([Math.cos(a) * 1.25, 0, Math.sin(a) * 1.7])); if (i) g.lineTo(p[0], p[1]); else g.moveTo(p[0], p[1]); }
      g.fill();

      // ----- the radar: what the sensor sees, on the floor
      if (radar) {
        g.fillStyle = `hsla(${hue},92%,62%,.10)`;
        g.beginPath();
        let p = px(cam([0, 0.02, 1.0])); g.moveTo(p[0], p[1]);
        for (let i = 0; i <= 12; i++) { const a = (-50 + 100 * i / 12) * Math.PI / 180; p = px(cam([Math.sin(a) * 6.5, 0.02, 1.0 + Math.cos(a) * 6.5])); g.lineTo(p[0], p[1]); }
        g.fill();
        for (const d of radar) {
          const a = d.a * 50 * Math.PI / 180, rr = d.d * 6.5, c = cam([Math.sin(a) * rr, 0.25, 1.0 + Math.cos(a) * rr]);
          if (c[2] < 0.4) continue;
          const q = px(c), size = focal * 0.16 / c[2];
          g.fillStyle = `rgba(255,255,255,${(0.25 * d.f).toFixed(2)})`; g.beginPath(); g.arc(q[0], q[1], size * 1.9, 0, TAU); g.fill();
          g.fillStyle = `hsla(${hue},95%,70%,${d.f.toFixed(2)})`; g.beginPath(); g.arc(q[0], q[1], size, 0, TAU); g.fill();
        }
      }

      // ----- the car: every face turned, lit and sorted back to front
      const poly = this.poly; poly.length = 0;
      const cr = Math.cos(this.roll), sr = Math.sin(this.roll), cp = Math.cos(this.pitch), sp = Math.sin(this.pitch);
      const place = (p) => {                                   // the whole car: lean, tip, bounce
        const x = p[0] * cr - p[1] * sr, y0 = p[0] * sr + p[1] * cr;
        const y = y0 * cp - p[2] * sp, z = y0 * sp + p[2] * cp;
        return [x, y + this.bob, z];
      };
      const add = (faces, move) => {
        for (const f of faces) {
          const wv = f.v.map((p) => place(move ? move(p) : p));
          // the face's own direction (for the light)
          const ax = wv[1][0] - wv[0][0], ay = wv[1][1] - wv[0][1], az = wv[1][2] - wv[0][2];
          const bx2 = wv[2][0] - wv[0][0], by2 = wv[2][1] - wv[0][1], bz2 = wv[2][2] - wv[0][2];
          let nx = ay * bz2 - az * by2, ny = az * bx2 - ax * bz2, nz = ax * by2 - ay * bx2;
          const nl = Math.hypot(nx, ny, nz) || 1; nx /= nl; ny /= nl; nz /= nl;
          const cv = wv.map(cam);
          const pts = cv.map(px);
          let area = 0;
          for (let i = 0; i < pts.length; i++) { const q = pts[(i + 1) % pts.length]; area += pts[i][0] * q[1] - q[0] * pts[i][1]; }
          if (area >= 0) continue;                             // facing away from the camera
          let depth = 0; for (const c of cv) depth += c[2];
          const light = 0.52 + 0.48 * Math.max(0, nx * 0.35 + ny * 0.86 + nz * 0.36);
          poly.push({ pts, depth: depth / cv.length, c: f.c, light });
        }
      };
      add(this.car.body, null);
      const chd = Math.cos(this.head), shd = Math.sin(this.head), hp = this.car.headPivot;
      add(this.car.head, (p) => { const x = p[0] - hp[0], z = p[2] - hp[2]; return [hp[0] + x * chd + z * shd, p[1], hp[2] - x * shd + z * chd]; });
      for (const wh of this.car.wheels) {
        const a = this.spin[wh[3]], ca = Math.cos(a), sa = Math.sin(a);
        add(this.car.wheelGeo, (p) => [p[0] + wh[0], p[1] * ca - p[2] * sa + wh[1], p[1] * sa + p[2] * ca + wh[2]]);
      }
      poly.sort((a, b) => b.depth - a.depth);

      const wallC = cam([0, 0.9, 2.7]);
      const drawWall = () => {
        if (!blocked) return;
        const q = [[-1.9, 0, 2.7], [1.9, 0, 2.7], [1.9, 1.9, 2.7], [-1.9, 1.9, 2.7]].map((p) => px(cam(p)));
        g.fillStyle = `rgba(255,70,100,${(0.30 * this.wall).toFixed(2)})`; g.strokeStyle = `rgba(255,120,140,${(0.9 * this.wall).toFixed(2)})`;
        g.lineWidth = Math.max(1.5, h / 160);
        g.beginPath(); q.forEach((p, i) => (i ? g.lineTo(p[0], p[1]) : g.moveTo(p[0], p[1]))); g.closePath(); g.fill(); g.stroke();
      };
      const carDepth = cam([0, 0.7, 0])[2];
      if (wallC[2] > carDepth) drawWall();
      for (const f of poly) {
        const L = f.light;
        g.fillStyle = `rgb(${Math.round(f.c[0] * L)},${Math.round(f.c[1] * L)},${Math.round(f.c[2] * L)})`;
        g.beginPath(); g.moveTo(f.pts[0][0], f.pts[0][1]);
        for (let i = 1; i < f.pts.length; i++) g.lineTo(f.pts[i][0], f.pts[i][1]);
        g.closePath(); g.fill();
      }
      if (wallC[2] <= carDepth) drawWall();
    }
  }

  // cuts a line where it would go behind the camera
  function clip(inFront, behind) {
    const t = (inFront[2] - 0.4) / (inFront[2] - behind[2]);
    return [inFront[0] + (behind[0] - inFront[0]) * t, inFront[1] + (behind[1] - inFront[1]) * t, 0.4];
  }

  if (typeof module !== 'undefined' && module.exports) module.exports = { Scene }; else root.Scene = Scene;
})(typeof window !== 'undefined' ? window : globalThis);
