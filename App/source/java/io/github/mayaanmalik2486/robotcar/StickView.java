package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.os.Build;
import android.os.SystemClock;
import android.view.HapticFeedbackConstants;
import android.view.MotionEvent;
import android.view.View;

import java.util.Collections;
import java.util.Locale;

/**
 * The left-hand control. It is one of:
 * <pre>
 *   LEVER  the throttle of "wheel" driving: a handle in a slot. Push it up = forward, pull it
 *          down = backward; let go and it springs back to the middle.
 *   STICK  a thumbstick that goes every way (one-stick driving).
 *   TILT   a "hold to drive" pad: the knob shows how the phone is tilted, and the car only
 *          drives while a finger is on it.
 * </pre>
 * Everything is drawn here, with shading so it looks like a real 3D part.
 */
class StickView extends View {

    static final int STICK = 0, LEVER = 1;

    interface Listener {
        /** The control moved (x, y from -1 to 1, y down = positive; 0, 0 = let go). */
        void onStick(StickView stick, float x, float y);

        /** Tilt mode: a finger went down (true) or was lifted (false). */
        void onHold(boolean holding);
    }

    private final Listener listener;
    private int kind = STICK;
    private float kx, ky;                              // where the knob is drawn, -1 .. 1
    private float tx, ty;                              // where it should be (it glides there)
    private boolean tiltMode, holding, touching;
    private int pointer = -1;
    private long lastFrame;
    private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint words = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF r = new RectF();

    StickView(Context c, Listener listener) {
        super(c);
        this.listener = listener;
        words.setTextSize(Ui.px(c, 10.5f));
        words.setTextAlign(Paint.Align.CENTER);
        words.setTypeface(Ui.MONO);
    }

    void setKind(int kind) {
        this.kind = kind;
        kx = ky = tx = ty = 0;
        invalidate();
    }

    void setTiltMode(boolean on) {
        tiltMode = on;
        holding = false;
        kx = ky = tx = ty = 0;
        invalidate();
    }

    /** Moves the knob (to show what a game controller or the tilt is doing). */
    void show(float x, float y) {
        if (touching && !tiltMode)
            return;                                   // (a finger on it wins)
        tx = kind == LEVER ? 0 : Math.max(-1, Math.min(1, x));
        ty = Math.max(-1, Math.min(1, y));
        invalidate();
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        // the phone's "back" swipe from the screen edge must not steal the thumb
        if (Build.VERSION.SDK_INT >= 29)
            setSystemGestureExclusionRects(Collections.singletonList(new Rect(0, 0, r - l, b - t)));
    }

    private float labelH() {
        return Ui.px(getContext(), 22);
    }

    private float centerY() {
        return (getHeight() - labelH()) / 2f;
    }

    private float radius() {
        return Math.min(getWidth(), getHeight() - labelH()) / 2f * 0.9f;
    }

    // the lever: how tall its slot is, how big its handle, how far it travels
    private float knobH() {
        return Math.max(Ui.px(getContext(), 40), Math.min(Ui.px(getContext(), 58), (getHeight() - labelH()) * 0.16f));
    }

    private float trackH() {
        return Math.min(getHeight() - labelH() - Ui.px(getContext(), 12), Ui.px(getContext(), 420));
    }

    private float travel() {
        return (trackH() - knobH()) / 2f - Ui.px(getContext(), 4);
    }

    @Override
    protected void onDraw(Canvas c) {
        // the knob glides to where it should be (and springs back when let go)
        long now = SystemClock.uptimeMillis();
        float dt = lastFrame == 0 ? 16 : Math.min(50, now - lastFrame);
        lastFrame = now;
        float k = touching ? 1 : 1 - (float) Math.exp(-dt / 60f);
        kx += (tx - kx) * k;
        ky += (ty - ky) * k;
        boolean settling = Math.abs(tx - kx) > 0.002f || Math.abs(ty - ky) > 0.002f;
        if (kind == LEVER && !tiltMode)
            drawLever(c);
        else
            drawStick(c);
        if (settling)
            postInvalidateOnAnimation();
        else
            lastFrame = 0;
    }

    private void drawLever(Canvas c) {
        float d = getResources().getDisplayMetrics().density;
        float w = getWidth(), cx = w / 2f, cy = centerY();
        float trackH = trackH(), trackW = Math.max(40 * d, Math.min(62 * d, w * 0.3f)), knobH = knobH(), knobW = Math.max(84 * d, Math.min(132 * d, w * 0.72f));
        float top = cy - trackH / 2, travel = travel();
        boolean active = touching || Math.abs(ky) > 0.02f;
        p.setStyle(Paint.Style.FILL);
        // the slot
        r.set(cx - trackW / 2, top, cx + trackW / 2, top + trackH);
        p.setShader(new LinearGradient(r.left, 0, r.right, 0, new int[]{0xD902040E, 0xBF0A1028, 0xD902040E}, null, Shader.TileMode.CLAMP));
        c.drawRoundRect(r, trackW / 2, trackW / 2, p);
        p.setShader(null);
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeWidth(d);
        p.setColor(0x1AFFFFFF);
        c.drawRoundRect(r, trackW / 2, trackW / 2, p);
        // the marks beside it
        p.setStrokeCap(Paint.Cap.ROUND);
        for (int i = -5; i <= 5; i++) {
            float y = cy + i * travel / 5;
            boolean big = i % 5 == 0;
            p.setColor(i == 0 ? 0x8CFFFFFF : 0x2EFFFFFF);
            p.setStrokeWidth(big ? 2 * d : d);
            float a = trackW / 2 + 7 * d, b = a + (big ? 12 : 6) * d;
            c.drawLine(cx - b, y, cx - a, y, p);
            c.drawLine(cx + a, y, cx + b, y, p);
        }
        p.setStyle(Paint.Style.FILL);
        words.setTextSize(9.5f * d);
        words.setColor(Ui.FAINT);
        if (trackH > 150 * d) {
            c.drawText("GAS", cx, top + 16 * d, words);
            c.drawText("REV", cx, top + trackH - 9 * d, words);
        }
        // the glowing bar from the middle to the handle
        float kyPx = cy + ky * travel;
        if (Math.abs(ky) > 0.01f) {
            boolean up = ky < 0;
            int col = up ? Ui.ACCENT : 0xFF7C8CFF;
            float bw = trackW * 0.42f;
            p.setColor(Ui.alpha(col, 0.16f));                               // (a soft glow round it)
            r.set(cx - bw, Math.min(cy, kyPx), cx + bw, Math.max(cy, kyPx));
            c.drawRoundRect(r, bw, bw, p);
            p.setShader(new LinearGradient(0, cy, 0, kyPx, Ui.alpha(col, 0.25f), up ? Ui.ACCENT2 : 0xFF8E9BFF, Shader.TileMode.CLAMP));
            r.set(cx - bw / 2, Math.min(cy, kyPx), cx + bw / 2, Math.max(cy, kyPx));
            c.drawRoundRect(r, bw / 2, bw / 2, p);
            p.setShader(null);
        }
        // the handle: a glossy 3D grip
        float rad = knobH / 2;
        p.setColor(0x73000000);
        r.set(cx - knobW / 2 + 2 * d, kyPx - rad + 8 * d, cx + knobW / 2 - 2 * d, kyPx + rad + 8 * d);
        c.drawRoundRect(r, rad, rad, p);
        p.setColor(active ? 0xFFB4480A : 0xFF1A2248);
        r.set(cx - knobW / 2, kyPx - rad + 5 * d, cx + knobW / 2, kyPx + rad + 5 * d);
        c.drawRoundRect(r, rad, rad, p);
        p.setShader(new LinearGradient(0, kyPx - rad, 0, kyPx + rad, active ? 0xFFFFB56A : 0xFF4A5AA6, active ? 0xFFFF6A13 : 0xFF2A3572, Shader.TileMode.CLAMP));
        r.set(cx - knobW / 2, kyPx - rad, cx + knobW / 2, kyPx + rad);
        c.drawRoundRect(r, rad, rad, p);
        p.setShader(null);
        p.setColor(0x47FFFFFF);
        r.set(cx - knobW / 2 + 5 * d, kyPx - rad + 3 * d, cx + knobW / 2 - 5 * d, kyPx - rad + 3 * d + knobH * 0.42f);
        c.drawRoundRect(r, knobH * 0.3f, knobH * 0.3f, p);
        p.setColor(0x47000000);
        p.setStrokeWidth(2 * d);
        p.setStyle(Paint.Style.STROKE);
        for (int i = -1; i <= 1; i++)
            c.drawLine(cx + i * 10 * d, kyPx + 2 * d, cx + i * 10 * d, kyPx + knobH * 0.28f, p);
        p.setStyle(Paint.Style.FILL);
        label(c, "THROTTLE", Math.abs(ky) > 0.02f ? String.format(Locale.US, "%+d%%", Math.round(-ky * 100)) : "");
    }

    private void drawStick(Canvas c) {
        float d = getResources().getDisplayMetrics().density;
        float cx = getWidth() / 2f, cy = centerY(), rr = radius(), kr = rr * 0.4f;
        boolean active = tiltMode ? holding : touching || Math.abs(kx) > 0.02f || Math.abs(ky) > 0.02f;
        p.setStyle(Paint.Style.FILL);
        if (active) {                                                       // a soft glow round the pad
            p.setColor(Ui.alpha(Ui.ACCENT, 0.10f));
            c.drawCircle(cx, cy, rr + 6 * d, p);
        }
        p.setShader(new RadialGradient(cx, cy - rr * 0.3f, rr * 1.3f, 0x8C283678, 0xBF040714, Shader.TileMode.CLAMP));
        c.drawCircle(cx, cy, rr, p);
        p.setShader(null);
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeWidth(active ? 2 * d : d);
        p.setColor(active ? Ui.ACCENT : 0x24FFFFFF);
        c.drawCircle(cx, cy, rr, p);
        p.setStrokeWidth(d);
        p.setColor(0x14FFFFFF);
        c.drawCircle(cx, cy, rr * 0.58f, p);
        // little arrows at the four sides
        p.setColor(0x59FFFFFF);
        p.setStrokeWidth(2 * d);
        p.setStrokeCap(Paint.Cap.ROUND);
        float t = 5 * d, m = rr - 12 * d;
        chevron(c, cx - t, cy - m + t, cx, cy - m, cx + t, cy - m + t);
        chevron(c, cx - t, cy + m - t, cx, cy + m, cx + t, cy + m - t);
        chevron(c, cx - m + t, cy - t, cx - m, cy, cx - m + t, cy + t);
        chevron(c, cx + m - t, cy - t, cx + m, cy, cx + m - t, cy + t);
        float px = cx + kx * (rr - kr), py = cy + ky * (rr - kr);
        if (tiltMode && !holding) {
            px = cx;
            py = cy;
        }
        if (active) {
            p.setColor(0x59FF7A1A);
            p.setStrokeWidth(kr * 0.9f);
            c.drawLine(cx, cy, px, py, p);
        }
        // the knob: a glossy ball
        p.setStyle(Paint.Style.FILL);
        p.setColor(0x80000000);
        c.drawCircle(px, py + 7 * d, kr, p);
        p.setShader(new RadialGradient(px - kr * 0.35f, py - kr * 0.4f, kr * 1.5f,
            active ? new int[]{0xFFFFE0B8, 0xFFFF9A3D, 0xFFE85A08} : new int[]{0xFF93A2EE, 0xFF4A5AA6, 0xFF222C62}, new float[]{0.03f, 0.3f, 1f}, Shader.TileMode.CLAMP));
        c.drawCircle(px, py, kr, p);
        p.setShader(null);
        p.setColor(0x38FFFFFF);
        r.set(px - kr * 0.62f, py - kr * 0.79f, px + kr * 0.62f, py - kr * 0.11f);
        c.drawOval(r, p);
        label(c, tiltMode ? (holding ? "TILT TO DRIVE" : "HOLD + TILT") : "DRIVE", "");
    }

    private void chevron(Canvas c, float x1, float y1, float x2, float y2, float x3, float y3) {
        c.drawLine(x1, y1, x2, y2, p);
        c.drawLine(x2, y2, x3, y3, p);
    }

    private void label(Canvas c, String name, String value) {
        float d = getResources().getDisplayMetrics().density;
        words.setTextSize(10.5f * d);
        float y = getHeight() - 4 * d, cx = getWidth() / 2f;
        if (value.isEmpty()) {
            words.setColor(Ui.LABEL);
            c.drawText(name, cx, y, words);
            return;
        }
        float nw = words.measureText(name + "  "), vw = words.measureText(value);
        words.setColor(Ui.LABEL);
        c.drawText(name, cx - vw / 2 - words.measureText("  ") / 2, y, words);
        words.setColor(Ui.ACCENT2);
        c.drawText(value, cx + nw / 2, y, words);
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        int action = e.getActionMasked();
        if (tiltMode) {
            boolean down = action != MotionEvent.ACTION_UP && action != MotionEvent.ACTION_CANCEL;
            if (down != holding) {
                holding = down;
                if (down && Ui.haptics)
                    performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
                listener.onHold(down);
                if (!down)
                    tx = ty = 0;
                invalidate();
            }
            return true;
        }
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                pointer = e.getPointerId(0);
                touching = true;
                if (Ui.haptics)
                    performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
                getParent().requestDisallowInterceptTouchEvent(true);
                // fall through
            case MotionEvent.ACTION_MOVE: {
                int i = e.findPointerIndex(pointer);
                if (i < 0)
                    return true;
                float x, y;
                if (kind == LEVER) {
                    x = 0;
                    y = Math.max(-1, Math.min(1, (e.getY(i) - centerY()) / travel()));
                } else {
                    float rr = radius() * 0.62f;
                    x = (e.getX(i) - getWidth() / 2f) / rr;
                    y = (e.getY(i) - centerY()) / rr;
                    float len = (float) Math.hypot(x, y);
                    if (len > 1) {
                        x /= len;
                        y /= len;
                    }
                }
                tx = kx = x;
                ty = ky = y;
                invalidate();
                listener.onStick(this, x, y);
                return true;
            }
            case MotionEvent.ACTION_POINTER_UP:
                if (e.getPointerId(e.getActionIndex()) != pointer)
                    return true;
                // fall through
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                touching = false;
                pointer = -1;
                tx = ty = 0;                           // (the knob springs back by itself; the car stops at once)
                invalidate();
                listener.onStick(this, 0, 0);
                return true;
        }
        return true;
    }
}
