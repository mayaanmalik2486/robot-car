package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RadialGradient;
import android.graphics.RectF;
import android.graphics.Shader;
import android.os.SystemClock;
import android.view.View;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * The live 3D picture of the car, with the car's "screen" written over it.
 *
 * It is a little 3D engine of our own (the same one as the web page's scene.js): the car is
 * built from boxes and wheels, lit from above and drawn back to front. It drives over a
 * glowing grid: the wheels spin, the floor rushes by and turns when you steer, the car leans
 * in bends, bounces when it dances, its sensor head sweeps in radar mode (with the dots it
 * sees on the floor), and a red wall appears when it is blocked.
 *
 * Over it: the connection (top left), what the car is doing in big letters (bottom left),
 * its switches under that, and two bars for how hard each side of the car works (right).
 */
class SceneView extends View {

    private static final float TAU = (float) (Math.PI * 2);

    // ---------- the model: every face is 3 or 4 corners (x, y, z each) and a colour ----------
    private static final class Mesh {
        final List<float[]> faces = new ArrayList<>();
        final List<Integer> colors = new ArrayList<>();

        void face(int color, float... xyz) {
            faces.add(xyz);
            colors.add(color);
        }

        // a box: centre, half sizes, colour (the top a little lighter, like a painted toy)
        void box(float cx, float cy, float cz, float hx, float hy, float hz, int col, int top) {
            float x0 = cx - hx, x1 = cx + hx, y0 = cy - hy, y1 = cy + hy, z0 = cz - hz, z1 = cz + hz;
            face(top, x0, y1, z0, x0, y1, z1, x1, y1, z1, x1, y1, z0);
            face(col, x0, y0, z1, x0, y0, z0, x1, y0, z0, x1, y0, z1);
            face(col, x0, y0, z1, x1, y0, z1, x1, y1, z1, x0, y1, z1);
            face(col, x1, y0, z0, x0, y0, z0, x0, y1, z0, x1, y1, z0);
            face(col, x1, y0, z1, x1, y0, z0, x1, y1, z0, x1, y1, z1);
            face(col, x0, y0, z0, x0, y0, z1, x0, y1, z1, x0, y1, z0);
        }

        // a wheel: a short cylinder lying on its side (axis = x), centred on 0,0,0
        void wheel(float r, float half, int n, int tyre, int hubA, int hubB) {
            for (int i = 0; i < n; i++) {
                double a0 = TAU * i / n, a1 = TAU * (i + 1) / n;
                float y0 = (float) Math.cos(a0) * r, z0 = (float) Math.sin(a0) * r, y1 = (float) Math.cos(a1) * r, z1 = (float) Math.sin(a1) * r;
                face(tyre, -half, y0, z0, half, y0, z0, half, y1, z1, -half, y1, z1);
                float h = 0.62f, hy0 = y0 * h, hz0 = z0 * h, hy1 = y1 * h, hz1 = z1 * h;
                int hub = i % 2 == 1 ? hubA : hubB;                    // two tones, so you see it spin
                face(hub, half, 0, 0, half, hy1, hz1, half, hy0, hz0);
                face(hub, -half, 0, 0, -half, hy0, hz0, -half, hy1, hz1);
                face(tyre, half, hy0, hz0, half, hy1, hz1, half, y1, z1, half, y0, z0);
                face(tyre, -half, hy1, hz1, -half, hy0, hz0, -half, y0, z0, -half, y1, z1);
            }
        }

        // an "eye" of the distance sensor: a short cylinder looking forward (axis = z)
        void eye(float cx, float cy, float cz, float r, float len, int n, int side, int cap, int pupil) {
            for (int i = 0; i < n; i++) {
                double a0 = TAU * i / n, a1 = TAU * (i + 1) / n;
                float x0 = cx + (float) Math.cos(a0) * r, y0 = cy + (float) Math.sin(a0) * r, x1 = cx + (float) Math.cos(a1) * r, y1 = cy + (float) Math.sin(a1) * r;
                face(side, x0, y0, cz, x1, y1, cz, x1, y1, cz + len, x0, y0, cz + len);
                face(cap, cx, cy, cz + len, x0, y0, cz + len, x1, y1, cz + len);
                float q = 0.55f, zz = cz + len + 0.01f;
                face(pupil, cx, cy, zz, cx + (x0 - cx) * q, cy + (y0 - cy) * q, zz, cx + (x1 - cx) * q, cy + (y1 - cy) * q, zz);
            }
        }
    }

    private static final int ORANGE = 0xFFFF7A1A, ORANGE_TOP = 0xFFFF963C, DARK = 0xFF22283A, DARK_TOP = 0xFF303850, DARKER = 0xFF161A28,
        BOARD = 0xFF1A96A0, BOARD_TOP = 0xFF28BEC4, TYRE = 0xFF181A22, HUB_A = 0xFFFFCD3C, HUB_B = 0xFFE6AA1E, SILVER = 0xFFC8D0E0,
        SILVER_CAP = 0xFFE4EAF6, PUPIL = 0xFF282E40, BLUE = 0xFF3A78FF, BLUE_TOP = 0xFF5A96FF, SCREEN = 0xFF0A0E1A, GLOW = 0xFF50F0E1,
        RED = 0xFFFF4664, RED_TOP = 0xFFFF788C;

    private final Mesh body = new Mesh(), head = new Mesh(), wheel = new Mesh();
    // the four wheels: x, y, z, which side spins it (0 = left, 1 = right). (With z = forward, +x is the car's LEFT side.)
    private static final float[][] WHEELS = {{0.97f, 0.44f, 0.82f, 0}, {-0.97f, 0.44f, 0.82f, 1}, {0.97f, 0.44f, -0.82f, 0}, {-0.97f, 0.44f, -0.82f, 1}};
    private static final float HEAD_Z = 0.98f;

    // ---------- the moving scene ----------
    private float az = 0.75f;                   // where the camera stands (an angle round the car)
    private final float[] spin = new float[2];  // how far the left / right wheels have turned
    private float x, z, heading, v, w, roll, pitch, headAngle, bob, idle = 9, time, wall;
    private long lastFrame;

    // ---------- what it is told ----------
    private float left, right;                  // wheel speeds -1 ... 1
    private String mode = "";
    private CarState car;
    private boolean live;                       // connected: the radar dots are real
    private String top = "", big = "", bigOld = "";
    private long bigAt;
    private boolean warn;
    private int dot = Ui.BAD;                   // the little light before the top line
    private boolean blink;
    private String[] flags = new String[0];
    private boolean[] flagGood = new boolean[0];
    private boolean bars;
    private float headWanted;                   // where the app pointed the sensor head: -1 (right) ... 1 (left)

    // ---------- drawing ----------
    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint line = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path path = new Path();
    private final Path clip = new Path();
    private final RectF r = new RectF();
    private Shader sky, glowWarm, glowRed;
    private int skyW, skyH;
    private float ex, ey, ez, rx, rz, ux, uy, uz, fx, fy, fz, focal, pcx, pcy;   // the camera
    private final float[] cam = new float[3];
    private float[] sx = new float[1600], sy = new float[1600], light = new float[420], depth = new float[420];
    private int[] start = new int[420], count = new int[420], color = new int[420];
    private long[] order = new long[420];
    private int nFaces, nPts;
    private final float[] tmp = new float[12];
    private final float[] hsv = new float[3];

    SceneView(Context c) {
        super(c);
        body.box(0, 0.50f, 0, 0.78f, 0.07f, 1.30f, DARK, DARK_TOP);                 // lower deck
        body.box(0, 0.72f, -0.05f, 0.70f, 0.15f, 1.12f, ORANGE, ORANGE_TOP);        // the orange body
        body.box(0, 0.93f, -0.55f, 0.46f, 0.07f, 0.42f, BOARD, BOARD_TOP);          // the Arduino
        body.box(0, 1.00f, -0.55f, 0.30f, 0.02f, 0.26f, DARKER, DARKER);            // (its chip)
        body.box(0, 0.95f, 0.30f, 0.36f, 0.09f, 0.22f, DARKER, 0xFF282E42);         // the screen's block
        body.box(0, 1.045f, 0.30f, 0.28f, 0.006f, 0.15f, SCREEN, SCREEN);           // the OLED
        body.box(-0.12f, 1.055f, 0.30f, 0.06f, 0.004f, 0.08f, GLOW, GLOW);          // its two eyes
        body.box(0.12f, 1.055f, 0.30f, 0.06f, 0.004f, 0.08f, GLOW, GLOW);
        body.box(0, 0.60f, -1.28f, 0.50f, 0.10f, 0.06f, RED, RED_TOP);              // tail light
        body.box(0, 0.93f, 0.92f, 0.16f, 0.06f, 0.14f, BLUE, BLUE_TOP);             // the servo
        head.box(0, 1.13f, 0.98f, 0.46f, 0.14f, 0.04f, BLUE, BLUE_TOP);             // the sensor head (turns on the servo)
        head.eye(-0.22f, 1.13f, 1.02f, 0.115f, 0.12f, 10, SILVER, SILVER_CAP, PUPIL);
        head.eye(0.22f, 1.13f, 1.02f, 0.115f, 0.12f, 10, SILVER, SILVER_CAP, PUPIL);
        wheel.wheel(0.44f, 0.16f, 12, TYRE, HUB_A, HUB_B);
        line.setStyle(Paint.Style.STROKE);
        line.setStrokeCap(Paint.Cap.ROUND);
        setClickable(true);
    }

    // ------------------------------------------------------------------ what the app tells it

    void setCar(CarState car) {
        this.car = car;
    }

    /** The words: the top line (with its little light), the big line, and the switches (name + is it good). */
    void set(String top, boolean warn, int dot, boolean blink, String big, String[] flags, boolean[] flagGood) {
        this.top = top;
        this.warn = warn;
        this.dot = dot;
        this.blink = blink;
        if (!big.equals(this.big)) {
            bigOld = this.big;
            this.big = big;
            bigAt = SystemClock.uptimeMillis();
        }
        this.flags = flags;
        this.flagGood = flagGood;
    }

    /** What the car does: the speed of each side -1 ... 1, the car's mode, and whether that is real. */
    void drive(float left, float right, String mode, boolean live, boolean bars) {
        this.left = left;
        this.right = right;
        this.mode = mode == null ? "" : mode;
        this.live = live;
        this.bars = bars;
    }

    /** Where the app pointed the sensor head, in degrees: -50 (right) ... 50 (left). */
    void head(float degrees) {
        headWanted = Math.max(-1, Math.min(1, degrees / 50f));
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        lastFrame = 0;
    }

    // ------------------------------------------------------------------ moving

    private void update(float dt) {
        time += dt;
        float l = left, rr = right;
        boolean dance = mode.equals("DANCE"), radar = mode.equals("RADAR");
        if (dance) {
            l = (float) Math.sin(time * 6.3f) * 0.5f;                       // a little dance wiggle
            rr = -l;
        } else if (mode.equals("DONUT")) {
            l = 0.9f;
            rr = -0.9f;
        } else if (mode.equals("TURBO!")) {
            l = rr = 1;
        } else if (l == 0 && rr == 0) {
            switch (mode) {
                case "FORWARD": case "AUTO FWD": case "SEARCH": case "CHAOS!": case "FOLLOW": case "PET":
                    l = rr = 0.6f;
                    break;
                case "BACKWARD": case "AUTO BCK":
                    l = rr = -0.6f;
                    break;
                case "LEFT": case "TURN L": case "LOOK L":
                    l = -0.5f;
                    rr = 0.5f;
                    break;
                case "RIGHT": case "TURN R": case "LOOK R": case "TURN 180":
                    l = 0.5f;
                    rr = -0.5f;
                    break;
                case "CURVE L":
                    l = 0.2f;
                    rr = 0.9f;
                    break;
                case "CURVE R":
                    l = 0.9f;
                    rr = 0.2f;
                    break;
                default:
                    break;
            }
        }
        float k = 1 - (float) Math.exp(-dt * 9);
        float wantV = (l + rr) / 2 * 7.5f, wantW = (l - rr) * 1.7f;
        float accel = wantV - v;
        v += (wantV - v) * k;
        w += (wantW - w) * k;
        heading -= w * dt;                                                  // (turning right = toward -x)
        x += (float) Math.sin(heading) * v * dt;
        z += (float) Math.cos(heading) * v * dt;
        spin[0] += l * 16 * dt;
        spin[1] += rr * 16 * dt;
        roll += (w * v * 0.012f - roll) * k;
        pitch += (-accel * 0.035f - pitch) * k * 0.6f;
        bob = dance ? (float) Math.abs(Math.sin(time * 6.3f)) * 0.16f : bob * (1 - k);
        float look = radar && car != null ? (car.radarLook - 5) / 5f : headWanted;
        headAngle += (look * 0.87f - headAngle) * k;
        wall += ((mode.equals("BLOCKED") ? 1 : 0) - wall) * k;
        boolean moving = Math.abs(v) > 0.15f || Math.abs(w) > 0.1f;
        idle = moving ? 0 : idle + dt;
        if (idle > 1.5f) {
            az += dt * 0.22f;                                               // standing still: the camera strolls round
        } else {                                                            // driving: the camera swings behind the car
            float d = ((float) Math.PI + 0.55f - az) % TAU;
            if (d > Math.PI)
                d -= TAU;
            if (d < -Math.PI)
                d += TAU;
            az += d * (1 - (float) Math.exp(-dt * 2.2f));
        }
    }

    // ------------------------------------------------------------------ the camera

    private void toCam(float px, float py, float pz) {
        float dx = px - ex, dy = py - ey, dz = pz - ez;
        cam[0] = dx * rx + dz * rz;
        cam[1] = dx * ux + dy * uy + dz * uz;
        cam[2] = dx * fx + dy * fy + dz * fz;
    }

    private float screenX() {
        return pcx + focal * cam[0] / cam[2];
    }

    private float screenY() {
        return pcy - focal * cam[1] / cam[2];
    }

    // ------------------------------------------------------------------ drawing

    @Override
    protected void onDraw(Canvas c) {
        long now = SystemClock.uptimeMillis();
        float dt = lastFrame == 0 ? 0.016f : Math.min(0.05f, (now - lastFrame) / 1000f);
        lastFrame = now;
        update(dt);

        float wd = getWidth(), ht = getHeight(), d = getResources().getDisplayMetrics().density;
        if (wd == 0 || ht == 0)
            return;
        float corner = 22 * d;
        clip.reset();
        r.set(0, 0, wd, ht);
        clip.addRoundRect(r, corner, corner, Path.Direction.CW);
        c.save();
        c.clipPath(clip);

        // the sky: night blue, with a warm glow where the floor ends
        if (sky == null || skyW != getWidth() || skyH != getHeight()) {
            skyW = getWidth();
            skyH = getHeight();
            sky = new LinearGradient(0, 0, 0, ht, new int[]{0xFF0A1030, 0xFF141F52, 0xFF2A2F6E, 0xFF070B1A}, new float[]{0, 0.5f, 0.56f, 1}, Shader.TileMode.CLAMP);
            glowWarm = new RadialGradient(wd / 2, ht * 0.5f, wd * 0.6f, 0x42FF7A1A, 0x00FF7A1A, Shader.TileMode.CLAMP);
            glowRed = new RadialGradient(wd / 2, ht * 0.5f, wd * 0.6f, 0x4DFF3C64, 0x00FF3C64, Shader.TileMode.CLAMP);
        }
        fill.setStyle(Paint.Style.FILL);
        fill.setShader(sky);
        c.drawRect(0, 0, wd, ht, fill);
        fill.setShader(mode.equals("BLOCKED") ? glowRed : glowWarm);
        c.drawRect(0, 0, wd, ht, fill);
        fill.setShader(null);

        // the camera
        float el = 0.40f, dist = 8.2f, ty = 0.62f;
        ex = dist * (float) (Math.cos(el) * Math.sin(az));
        ey = ty + dist * (float) Math.sin(el);
        ez = dist * (float) (Math.cos(el) * Math.cos(az));
        fx = -ex;
        fy = ty - ey;
        fz = -ez;
        float fl = (float) Math.sqrt(fx * fx + fy * fy + fz * fz);
        fx /= fl;
        fy /= fl;
        fz /= fl;
        rx = -fz;
        rz = fx;
        float rl = (float) Math.hypot(rx, rz);
        rx /= rl;
        rz /= rl;
        ux = -rz * fy;
        uy = rz * fx - rx * fz;
        uz = rx * fy;
        // (in a wide, low picture the car stands right of the middle: the words are on the left)
        focal = Math.min(ht * 3.3f, wd * 1.25f);
        pcx = wd > ht * 2.1f ? wd * 0.63f : wd / 2;
        pcy = ht * 0.55f;

        drawGrid(c, ht);
        drawShadow(c);
        if (mode.equals("RADAR"))
            drawRadar(c, now);

        // the car: every face turned, lit and sorted back to front
        nFaces = 0;
        nPts = 0;
        addMesh(body, 0, 0, 0, 0, 0);
        addMesh(head, 1, headAngle, 0, 0, 0);
        for (float[] wh : WHEELS)
            addMesh(wheel, 2, spin[(int) wh[3]], wh[0], wh[1], wh[2]);
        for (int i = 0; i < nFaces; i++)
            order[i] = ((long) Float.floatToIntBits(depth[i]) << 32) | i;
        Arrays.sort(order, 0, nFaces);

        toCam(0, 0.9f, 2.7f);
        float wallDepth = cam[2];
        toCam(0, 0.7f, 0);
        float carDepth = cam[2];
        if (wallDepth > carDepth)
            drawWall(c, d);
        for (int k = nFaces - 1; k >= 0; k--) {
            int i = (int) order[k];
            float L = light[i];
            int col = color[i];
            fill.setColor(Color.rgb((int) (Color.red(col) * L), (int) (Color.green(col) * L), (int) (Color.blue(col) * L)));
            path.reset();
            int s = start[i];
            path.moveTo(sx[s], sy[s]);
            for (int j = 1; j < count[i]; j++)
                path.lineTo(sx[s + j], sy[s + j]);
            path.close();
            c.drawPath(path, fill);
        }
        if (wallDepth <= carDepth)
            drawWall(c, d);

        drawWords(c, wd, ht, d, now);
        c.restore();
        // a fine edge round the picture
        line.setStrokeWidth(d);
        line.setColor(0x26FFFFFF);
        r.set(d / 2, d / 2, wd - d / 2, ht - d / 2);
        c.drawRoundRect(r, corner, corner, line);
        postInvalidateOnAnimation();
    }

    // kind: 0 = body, 1 = sensor head (a = its turn), 2 = a wheel (a = its spin, at ox, oy, oz)
    private void addMesh(Mesh m, int kind, float a, float ox, float oy, float oz) {
        float ca = (float) Math.cos(a), sa = (float) Math.sin(a);
        float cr = (float) Math.cos(roll), sr = (float) Math.sin(roll), cp = (float) Math.cos(pitch), sp = (float) Math.sin(pitch);
        for (int f = 0; f < m.faces.size(); f++) {
            float[] v3 = m.faces.get(f);
            int n = v3.length / 3;
            for (int i = 0; i < n; i++) {
                float px = v3[i * 3], py = v3[i * 3 + 1], pz = v3[i * 3 + 2];
                if (kind == 1) {                                            // the head turns on its post
                    float qx = px, qz = pz - HEAD_Z;
                    px = qx * ca + qz * sa;
                    pz = HEAD_Z - qx * sa + qz * ca;
                } else if (kind == 2) {                                     // a wheel spins on its axle
                    float qy = py, qz = pz;
                    px += ox;
                    py = qy * ca - qz * sa + oy;
                    pz = qy * sa + qz * ca + oz;
                }
                // the whole car: lean, tip, bounce
                float wx = px * cr - py * sr, y0 = px * sr + py * cr;
                float wy = y0 * cp - pz * sp + bob, wz = y0 * sp + pz * cp;
                tmp[i * 3] = wx;
                tmp[i * 3 + 1] = wy;
                tmp[i * 3 + 2] = wz;
            }
            // the face's own direction (for the light)
            float ax = tmp[3] - tmp[0], ay = tmp[4] - tmp[1], azz = tmp[5] - tmp[2];
            float bx = tmp[6] - tmp[0], by = tmp[7] - tmp[1], bz = tmp[8] - tmp[2];
            float nx = ay * bz - azz * by, ny = azz * bx - ax * bz, nz = ax * by - ay * bx;
            float nl = (float) Math.sqrt(nx * nx + ny * ny + nz * nz);
            if (nl == 0)
                nl = 1;
            if (nPts + n > sx.length || nFaces >= start.length)
                grow();
            float dsum = 0;
            for (int i = 0; i < n; i++) {
                toCam(tmp[i * 3], tmp[i * 3 + 1], tmp[i * 3 + 2]);
                sx[nPts + i] = screenX();
                sy[nPts + i] = screenY();
                dsum += cam[2];
            }
            float area = 0;
            for (int i = 0; i < n; i++) {
                int j = (i + 1) % n;
                area += sx[nPts + i] * sy[nPts + j] - sx[nPts + j] * sy[nPts + i];
            }
            if (area >= 0)
                continue;                                                   // facing away from the camera
            start[nFaces] = nPts;
            count[nFaces] = n;
            depth[nFaces] = dsum / n;
            color[nFaces] = m.colors.get(f);
            light[nFaces] = 0.52f + 0.48f * Math.max(0, (nx * 0.35f + ny * 0.86f + nz * 0.36f) / nl);
            nFaces++;
            nPts += n;
        }
    }

    private void grow() {
        sx = Arrays.copyOf(sx, sx.length * 2);
        sy = Arrays.copyOf(sy, sy.length * 2);
        int n = start.length * 2;
        start = Arrays.copyOf(start, n);
        count = Arrays.copyOf(count, n);
        color = Arrays.copyOf(color, n);
        light = Arrays.copyOf(light, n);
        depth = Arrays.copyOf(depth, n);
        order = Arrays.copyOf(order, n);
    }

    // the floor grid (it moves under the car)
    private void drawGrid(Canvas c, float ht) {
        int base;
        if (mode.equals("DANCE")) {
            hsv[0] = (time * 120) % 360;
            hsv[1] = 0.6f;
            hsv[2] = 1f;
            base = Color.HSVToColor(hsv);
        } else {
            base = wall > 0.02f ? 0xFFFF5A78 : Ui.CYAN;
        }
        float ch = (float) Math.cos(heading), sh = (float) Math.sin(heading);
        int n = 9, seg = 6;
        float bx = (float) Math.floor(x), bz = (float) Math.floor(z);
        line.setStrokeWidth(Math.max(1, ht / 260));
        for (int pass = 0; pass < 2; pass++)
            for (int i = -n; i <= n + 1; i++)
                for (int s = 0; s < seg; s++) {
                    float t0 = -n + (2 * n + 1) * s / (float) seg, t1 = -n + (2 * n + 1) * (s + 1) / (float) seg;
                    float ax = pass == 1 ? bx + i : bx + t0, az0 = pass == 1 ? bz + t0 : bz + i;
                    float bx1 = pass == 1 ? bx + i : bx + t1, bz1 = pass == 1 ? bz + t1 : bz + i;
                    // into the car's own frame
                    float dx = ax - x, dz = az0 - z, pax = dx * ch - dz * sh, paz = dx * sh + dz * ch;
                    dx = bx1 - x;
                    dz = bz1 - z;
                    float pbx = dx * ch - dz * sh, pbz = dx * sh + dz * ch;
                    float far = (float) Math.hypot((pax + pbx) / 2, (paz + pbz) / 2);
                    float alpha = Math.max(0, 1 - far / (n - 0.5f)) * 0.55f;
                    if (alpha <= 0.01f)
                        continue;
                    toCam(pax, 0, paz);
                    float a0 = cam[0], a1 = cam[1], a2 = cam[2];
                    toCam(pbx, 0, pbz);
                    float b0 = cam[0], b1 = cam[1], b2 = cam[2];
                    if (a2 < 0.4f && b2 < 0.4f)
                        continue;
                    if (a2 < 0.4f) {                                        // cut the line where it goes behind the camera
                        float t = (b2 - 0.4f) / (b2 - a2);
                        a0 = b0 + (a0 - b0) * t;
                        a1 = b1 + (a1 - b1) * t;
                        a2 = 0.4f;
                    } else if (b2 < 0.4f) {
                        float t = (a2 - 0.4f) / (a2 - b2);
                        b0 = a0 + (b0 - a0) * t;
                        b1 = a1 + (b1 - a1) * t;
                        b2 = 0.4f;
                    }
                    line.setColor(Ui.alpha(base, alpha));
                    c.drawLine(pcx + focal * a0 / a2, pcy - focal * a1 / a2, pcx + focal * b0 / b2, pcy - focal * b1 / b2, line);
                }
    }

    private void drawShadow(Canvas c) {
        path.reset();
        for (int i = 0; i < 18; i++) {
            double a = TAU * i / 18;
            toCam((float) Math.cos(a) * 1.25f, 0, (float) Math.sin(a) * 1.7f);
            if (i == 0)
                path.moveTo(screenX(), screenY());
            else
                path.lineTo(screenX(), screenY());
        }
        path.close();
        fill.setColor(0x73000000);
        c.drawPath(path, fill);
    }

    // the radar: what the sensor sees, on the floor
    private void drawRadar(Canvas c, long now) {
        path.reset();
        toCam(0, 0.02f, 1.0f);
        path.moveTo(screenX(), screenY());
        for (int i = 0; i <= 12; i++) {
            double a = Math.toRadians(-50 + 100 * i / 12.0);
            toCam((float) Math.sin(a) * 6.5f, 0.02f, 1.0f + (float) Math.cos(a) * 6.5f);
            path.lineTo(screenX(), screenY());
        }
        path.close();
        fill.setColor(Ui.alpha(Ui.CYAN, 0.10f));
        c.drawPath(path, fill);
        if (car == null || !live)
            return;
        for (int k = 0; k < CarState.RADAR_STEPS; k++) {
            long age = now - car.radarAt[k];
            if (car.radarAt[k] == 0 || age > 6000 || car.radarCm[k] >= CarState.RADAR_RANGE_CM)
                continue;
            double a = Math.toRadians((k - 5) * 10.0);                      // (+ = the car's left = +x)
            float rr = 6.5f * car.radarCm[k] / CarState.RADAR_RANGE_CM;
            toCam((float) Math.sin(a) * rr, 0.25f, 1.0f + (float) Math.cos(a) * rr);
            if (cam[2] < 0.4f)
                continue;
            float fade = Math.max(0.15f, 1 - age / 6000f), size = focal * 0.16f / cam[2];
            fill.setColor(Ui.alpha(0xFFFFFFFF, 0.25f * fade));
            c.drawCircle(screenX(), screenY(), size * 1.9f, fill);
            fill.setColor(Ui.alpha(0xFF9FF5EA, fade));
            c.drawCircle(screenX(), screenY(), size, fill);
        }
    }

    private void drawWall(Canvas c, float d) {
        if (wall <= 0.02f)
            return;
        float[][] q = {{-1.9f, 0}, {1.9f, 0}, {1.9f, 1.9f}, {-1.9f, 1.9f}};
        path.reset();
        for (int i = 0; i < 4; i++) {
            toCam(q[i][0], q[i][1], 2.7f);
            if (i == 0)
                path.moveTo(screenX(), screenY());
            else
                path.lineTo(screenX(), screenY());
        }
        path.close();
        fill.setColor(Ui.alpha(0xFFFF4664, 0.30f * wall));
        c.drawPath(path, fill);
        line.setStrokeWidth(2 * d);
        line.setColor(Ui.alpha(0xFFFF788C, 0.9f * wall));
        c.drawPath(path, line);
    }

    // ------------------------------------------------------------------ the words over the picture

    private void drawWords(Canvas c, float wd, float ht, float d, long now) {
        float pad = Math.max(10 * d, Math.min(18 * d, ht * 0.12f));
        float small = Math.max(9.5f * d, Math.min(12 * d, ht * 0.085f));
        // the status chip (top left)
        text.setTypeface(Ui.MONO);
        text.setTextSize(small);
        text.setTextAlign(Paint.Align.LEFT);
        text.setLetterSpacing(0);
        String chip = ellipsize(top, wd - pad * 2 - small * 3);
        float cw = text.measureText(chip) + small * 2.6f, chh = small * 2.1f;
        r.set(pad, pad, pad + cw, pad + chh);
        fill.setColor(0x8C040714);
        c.drawRoundRect(r, chh / 2, chh / 2, fill);
        float pulse = blink ? 0.55f + 0.45f * (float) Math.sin(now / 260.0) : 1;
        fill.setColor(Ui.alpha(warn ? Ui.WARN : dot, pulse));
        c.drawCircle(pad + small * 1.05f, pad + chh / 2, small * 0.38f, fill);
        text.setColor(warn ? Ui.WARN : Ui.TEXT);
        c.drawText(chip, pad + small * 1.9f, pad + chh / 2 + small * 0.36f, text);

        // the headline (bottom left): slides in when it changes
        float bigSize = Math.max(22 * d, Math.min(Math.min(ht * 0.30f, wd * 0.11f), 84 * d));
        float flagH = flags.length > 0 && ht > 110 * d ? small * 2.0f + 8 * d : 0;
        float baseY = ht - pad - flagH;
        float k = Math.min(1, (now - bigAt) / 260f), ease = 1 - (1 - k) * (1 - k) * (1 - k);
        float maxW = wd - pad * 2 - (bars ? 26 * d : 0);
        drawBig(c, bigOld, pad, baseY - bigSize * 0.5f * ease, bigSize, maxW, 1 - ease, d);
        drawBig(c, big, pad, baseY + bigSize * 0.5f * (1 - ease), bigSize, maxW, ease, d);

        // the little switches under it
        if (flagH > 0) {
            text.setTypeface(Ui.MONO);
            text.setTextSize(small);
            float xx = pad, fy0 = ht - pad - small * 2.0f;
            for (int i = 0; i < flags.length; i++) {
                float fw = text.measureText(flags[i]) + small * 2.3f;
                if (xx + fw > wd - pad - (bars ? 26 * d : 0))
                    break;
                r.set(xx, fy0, xx + fw, fy0 + small * 2.0f);
                fill.setColor(0x80040714);
                c.drawRoundRect(r, small, small, fill);
                fill.setColor(flagGood[i] ? Ui.CYAN : Ui.BAD);
                c.drawCircle(xx + small * 0.95f, fy0 + small, small * 0.3f, fill);
                text.setColor(flagGood[i] ? Ui.LABEL : 0xFFFFB3C0);
                c.drawText(flags[i], xx + small * 1.6f, fy0 + small * 1.36f, text);
                xx += fw + 6 * d;
            }
        }

        // how hard each side of the car works (right edge)
        if (bars && ht > 84 * d) {
            float bw = 7 * d, bh = Math.min(ht - pad * 2 - chh - 12 * d, 150 * d), by = ht - pad - bh, bx = wd - pad - bw * 2 - 5 * d;
            for (int i = 0; i < 2; i++) {
                float val = i == 0 ? left : right, xx = bx + i * (bw + 5 * d);
                r.set(xx, by, xx + bw, by + bh);
                fill.setColor(0x8C040714);
                c.drawRoundRect(r, bw / 2, bw / 2, fill);
                float len = bh / 2 * Math.min(1, Math.abs(val));
                if (len > 1) {
                    fill.setColor(val > 0 ? Ui.ACCENT2 : 0xFF8E9BFF);
                    if (val > 0)
                        r.set(xx, by + bh / 2 - len, xx + bw, by + bh / 2);
                    else
                        r.set(xx, by + bh / 2, xx + bw, by + bh / 2 + len);
                    c.drawRoundRect(r, bw / 2, bw / 2, fill);
                }
                fill.setColor(0x80FFFFFF);
                c.drawRect(xx - 2 * d, by + bh / 2 - d / 2, xx + bw + 2 * d, by + bh / 2 + d / 2, fill);
            }
        }
    }

    // the big line: white, with the number at its end ("82%", "0:12", "35 cm") in amber
    private void drawBig(Canvas c, String s, float x, float y, float size, float maxW, float alpha, float d) {
        if (s == null || s.isEmpty() || alpha <= 0.01f)
            return;
        text.setTypeface(Ui.BOLD);
        text.setTextAlign(Paint.Align.LEFT);
        text.setTextSize(size);
        text.setFakeBoldText(true);
        float total = text.measureText(s);
        if (total > maxW)
            text.setTextSize(size * maxW / total);
        int cut = s.length();
        for (int i = 0; i < s.length(); i++)
            if (s.charAt(i) == ' ' && i + 1 < s.length() && (Character.isDigit(s.charAt(i + 1)) || s.charAt(i + 1) == '+' || s.charAt(i + 1) == '-')) {
                cut = i;
                break;
            }
        String a = s.substring(0, cut), b = s.substring(cut);
        text.setColor(Ui.alpha(0xFF000000, 0.35f * alpha));
        c.drawText(s, x + d, y + 2 * d, text);
        text.setColor(Ui.alpha(Ui.TEXT, alpha));
        c.drawText(a, x, y, text);
        text.setColor(Ui.alpha(Ui.ACCENT2, alpha));
        c.drawText(b, x + text.measureText(a), y, text);
        text.setFakeBoldText(false);
    }

    private String ellipsize(String s, float width) {
        if (text.measureText(s) <= width)
            return s;
        int n = s.length();
        while (n > 1 && text.measureText(s, 0, n) + text.measureText("…") > width)
            n--;
        return s.substring(0, n) + "…";
    }
}
