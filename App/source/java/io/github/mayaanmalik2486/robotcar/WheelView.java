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
 * The steering wheel (the right-hand control of "wheel" driving). Grab it anywhere and turn
 * it, like a real one: up to 126 degrees each way = full steering. Let go and it springs
 * back to straight (and the car goes straight at once). A dial round it shows how far it is
 * turned. It is drawn with shading - a rim, three spokes, two orange grips, a hub - and the
 * shine stays where the light is while the wheel turns under it.
 */
class WheelView extends View {

    interface Listener {
        /** The wheel was turned: -1 = full left ... 1 = full right (0 = let go). */
        void onWheel(float steer);
    }

    static final float MAX = 2.2f;                     // how far it turns each way (radians)

    private final Listener listener;
    private float angle, target;                       // drawn / wanted
    private boolean touching;
    private int pointer = -1;
    private float lastTouch;
    private long lastFrame;
    private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint words = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF r = new RectF();

    WheelView(Context c, Listener listener) {
        super(c);
        this.listener = listener;
        words.setTextAlign(Paint.Align.CENTER);
    }

    /** Turns the wheel to show what a game controller's stick is doing (-1 ... 1). */
    void show(float steer) {
        if (touching)
            return;
        target = Math.max(-1, Math.min(1, steer)) * MAX;
        invalidate();
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int rr, int b) {
        super.onLayout(changed, l, t, rr, b);
        if (Build.VERSION.SDK_INT >= 29)               // the phone's "back" swipe must not steal the thumb
            setSystemGestureExclusionRects(Collections.singletonList(new Rect(0, 0, rr - l, b - t)));
    }

    private float labelH() {
        return Ui.px(getContext(), 22);
    }

    private float cy() {
        return (getHeight() - labelH()) / 2f;
    }

    private float radius() {
        return Math.min(getWidth(), getHeight() - labelH()) / 2f * 0.86f;
    }

    @Override
    protected void onDraw(Canvas c) {
        long now = SystemClock.uptimeMillis();
        float dt = lastFrame == 0 ? 16 : Math.min(50, now - lastFrame);
        lastFrame = now;
        if (!touching)
            angle += (target - angle) * (1 - (float) Math.exp(-dt / 60f));          // it springs back / follows the controller
        boolean settling = !touching && Math.abs(target - angle) > 0.003f;

        float d = getResources().getDisplayMetrics().density;
        float cx = getWidth() / 2f, cy = cy(), rad = radius();
        boolean active = touching || Math.abs(angle) > 0.03f;
        float deg = (float) Math.toDegrees(angle);

        // the dial behind the wheel: how far it is turned
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeCap(Paint.Cap.ROUND);
        float dialW = Math.max(4 * d, rad * 0.05f);
        p.setStrokeWidth(dialW);
        p.setColor(0x17FFFFFF);
        r.set(cx - rad * 1.12f, cy - rad * 1.12f, cx + rad * 1.12f, cy + rad * 1.12f);
        float maxDeg = (float) Math.toDegrees(MAX);
        c.drawArc(r, -90 - maxDeg, maxDeg * 2, false, p);
        if (active) {
            p.setStrokeWidth(dialW * 2.6f);                                 // (its glow)
            p.setColor(Ui.alpha(Ui.ACCENT, 0.18f));
            c.drawArc(r, -90 + Math.min(0, deg), Math.abs(deg), false, p);
            p.setStrokeWidth(dialW);
            p.setColor(Ui.ACCENT2);
            c.drawArc(r, -90 + Math.min(0, deg), Math.abs(deg), false, p);
        }
        p.setStyle(Paint.Style.FILL);
        p.setColor(0x99FFFFFF);
        c.drawCircle(cx, cy - rad * 1.12f, dialW * 0.7f, p);
        // its shadow on the dashboard
        p.setShader(new RadialGradient(cx, cy + rad * 0.12f, rad * 1.15f, new int[]{0x8C000000, 0x8C000000, 0x00000000}, new float[]{0, 0.45f, 1}, Shader.TileMode.CLAMP));
        c.drawCircle(cx, cy + rad * 0.12f, rad * 1.15f, p);
        p.setShader(null);

        c.save();
        c.translate(cx, cy);
        c.rotate(deg);
        // the spokes
        for (int i = 0; i < 3; i++) {
            c.save();
            c.rotate(i == 0 ? 0 : i == 1 ? 180 : 90);
            p.setShader(new LinearGradient(0, -rad * 0.1f, 0, rad * 0.1f, 0xFF5262B0, 0xFF1E2757, Shader.TileMode.CLAMP));
            r.set(rad * 0.2f, -rad * 0.085f, rad * 0.86f, rad * 0.085f);
            c.drawRoundRect(r, rad * 0.06f, rad * 0.06f, p);
            c.restore();
        }
        p.setShader(null);
        // the rim: a thick ring, lighter on top
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeCap(Paint.Cap.BUTT);
        p.setStrokeWidth(rad * 0.26f);
        p.setColor(0xFF0A0F28);
        c.drawCircle(0, rad * 0.03f, rad * 0.87f, p);
        p.setStrokeWidth(rad * 0.22f);
        p.setShader(new LinearGradient(0, -rad, 0, rad, new int[]{0xFF5A6BC0, 0xFF2B3672, 0xFF131A3E}, new float[]{0, 0.45f, 1}, Shader.TileMode.CLAMP));
        c.drawCircle(0, 0, rad * 0.87f, p);
        // the two grips, and the mark on top
        r.set(-rad * 0.87f, -rad * 0.87f, rad * 0.87f, rad * 0.87f);
        if (active) {
            p.setShader(null);
            p.setStrokeWidth(rad * 0.34f);
            p.setColor(Ui.alpha(Ui.ACCENT, 0.20f));
            c.drawArc(r, 180 - 24, 48, false, p);
            c.drawArc(r, -24, 48, false, p);
        }
        p.setStrokeWidth(rad * 0.225f);
        p.setShader(new LinearGradient(0, -rad, 0, rad, 0xFFFFB56A, 0xFFF25A0A, Shader.TileMode.CLAMP));
        c.drawArc(r, 180 - 24, 48, false, p);
        c.drawArc(r, -24, 48, false, p);
        p.setShader(null);
        p.setColor(Ui.TEXT);
        c.drawArc(r, -90 - 3, 6, false, p);
        // the hub
        p.setStyle(Paint.Style.FILL);
        p.setColor(0xFF0A0F28);
        c.drawCircle(0, rad * 0.025f, rad * 0.3f, p);
        p.setShader(new RadialGradient(-rad * 0.08f, -rad * 0.1f, rad * 0.34f, 0xFF6E80DA, 0xFF1A2250, Shader.TileMode.CLAMP));
        c.drawCircle(0, 0, rad * 0.28f, p);
        p.setShader(null);
        p.setColor(Ui.ACCENT);
        c.drawCircle(0, 0, rad * 0.13f, p);
        words.setTypeface(Ui.BOLD);
        words.setFakeBoldText(true);
        words.setColor(0xFFFFFFFF);
        words.setTextSize(rad * 0.15f);
        c.drawText("RC", 0, rad * 0.055f, words);
        words.setFakeBoldText(false);
        c.restore();

        // the shine (it stays where the light is, while the wheel turns under it)
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeCap(Paint.Cap.ROUND);
        p.setStrokeWidth(rad * 0.035f);
        p.setColor(0x4DFFFFFF);
        r.set(cx - rad * 0.94f, cy - rad * 0.94f, cx + rad * 0.94f, cy + rad * 0.94f);
        c.drawArc(r, -155, 50, false, p);
        p.setStyle(Paint.Style.FILL);

        // its name, and how far it is turned
        words.setTypeface(Ui.MONO);
        words.setTextSize(10.5f * d);
        float y = getHeight() - 4 * d;
        if (Math.abs(angle) > 0.03f) {
            String value = String.format(Locale.US, "%+d°", Math.round(deg));
            float nw = words.measureText("STEER  "), vw = words.measureText(value);
            words.setColor(Ui.LABEL);
            c.drawText("STEER", cx - vw / 2 - words.measureText("  ") / 2, y, words);
            words.setColor(Ui.ACCENT2);
            c.drawText(value, cx + nw / 2, y, words);
        } else {
            words.setColor(Ui.LABEL);
            c.drawText("STEER", cx, y, words);
        }
        if (settling)
            postInvalidateOnAnimation();
        else
            lastFrame = 0;
    }

    private float touchAngle(MotionEvent e, int i) {
        return (float) Math.atan2(e.getY(i) - cy(), e.getX(i) - getWidth() / 2f);
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        switch (e.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                pointer = e.getPointerId(0);
                touching = true;
                lastTouch = touchAngle(e, 0);
                if (Ui.haptics)
                    performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
                getParent().requestDisallowInterceptTouchEvent(true);
                invalidate();
                return true;
            case MotionEvent.ACTION_MOVE: {
                int i = e.findPointerIndex(pointer);
                if (i < 0)
                    return true;
                float a = touchAngle(e, i);
                float d = a - lastTouch;                // how far the finger went round since last time
                if (d > Math.PI)
                    d -= (float) (Math.PI * 2);
                if (d < -Math.PI)
                    d += (float) (Math.PI * 2);
                lastTouch = a;
                float before = angle;
                angle = Math.max(-MAX, Math.min(MAX, angle + d));
                if (Ui.haptics && Math.abs(angle) == MAX && Math.abs(before) < MAX)
                    performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);     // a bump at the end stop
                target = angle;
                invalidate();
                listener.onWheel(angle / MAX);
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
                target = 0;                            // (it springs back by itself; the car goes straight at once)
                invalidate();
                listener.onWheel(0);
                return true;
            default:
                return true;
        }
    }
}
