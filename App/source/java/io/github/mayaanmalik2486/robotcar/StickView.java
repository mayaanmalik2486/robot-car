package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Build;
import android.view.HapticFeedbackConstants;
import android.view.MotionEvent;
import android.view.View;

import java.util.Collections;

/**
 * An on-screen thumbstick. It can move every way (one-stick driving), only up and down (the
 * throttle of two-stick driving) or only left and right (the steering). In tilt mode it
 * becomes a "hold to drive" pad: the knob then shows how the phone is tilted, and the car only
 * drives while a finger is on it.
 */
class StickView extends View {

    static final int BOTH = 0, UP_DOWN = 1, LEFT_RIGHT = 2;

    interface Listener {
        /** The stick moved (x, y from -1 to 1, y down = positive; 0, 0 = let go). */
        void onStick(StickView stick, float x, float y);

        /** Tilt mode: a finger went down (true) or was lifted (false). */
        void onHold(boolean holding);
    }

    private final Listener listener;
    private int axes = BOTH;
    private String title = "";
    private float kx, ky;                              // knob position, -1 .. 1
    private boolean tiltMode, holding, touching;
    private int pointer = -1;
    private final Paint base = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint ring = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint knob = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint knobTop = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint mark = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint words = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint trail = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF box = new RectF();

    StickView(Context c, Listener listener) {
        super(c);
        this.listener = listener;
        base.setColor(Ui.PANEL);
        ring.setStyle(Paint.Style.STROKE);
        ring.setStrokeWidth(Ui.px(c, 1.5f));
        knob.setColor(Ui.KEY);
        knobTop.setStyle(Paint.Style.STROKE);
        knobTop.setStrokeWidth(Ui.px(c, 2));
        mark.setColor(Ui.FAINT);
        mark.setStrokeWidth(Ui.px(c, 2));
        mark.setStrokeCap(Paint.Cap.ROUND);
        words.setColor(Ui.LABEL);
        words.setTextSize(Ui.px(c, 11));
        words.setTextAlign(Paint.Align.CENTER);
        words.setTypeface(Ui.BOLD);
        words.setLetterSpacing(0.12f);
        trail.setColor(Ui.alpha(Ui.ACCENT, 0.35f));
        trail.setStrokeCap(Paint.Cap.ROUND);
    }

    void setAxes(int axes, String title) {
        this.axes = axes;
        this.title = title;
        show(0, 0);
    }

    void setTiltMode(boolean on) {
        tiltMode = on;
        holding = false;
        show(0, 0);
    }

    /** Moves the knob (to show what a game controller or the tilt is doing). */
    void show(float x, float y) {
        if (touching && !tiltMode)
            return;                                   // (a finger on it wins)
        place(x, y);
    }

    private void place(float x, float y) {
        kx = axes == UP_DOWN ? 0 : Math.max(-1, Math.min(1, x));
        ky = axes == LEFT_RIGHT ? 0 : Math.max(-1, Math.min(1, y));
        invalidate();
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        // the phone's "back" swipe from the screen edge must not steal the thumb
        if (Build.VERSION.SDK_INT >= 29)
            setSystemGestureExclusionRects(Collections.singletonList(new Rect(0, 0, r - l, b - t)));
    }

    private float radius() {
        float w = getWidth(), h = getHeight() - Ui.px(getContext(), 18);
        if (axes == UP_DOWN)
            return Math.min(w * 0.5f, h * 0.5f) * 0.92f;
        return Math.min(w, h) / 2f * 0.92f;
    }

    private float centerY() {
        return (getHeight() - Ui.px(getContext(), 18)) / 2f;
    }

    @Override
    protected void onDraw(Canvas c) {
        float cx = getWidth() / 2f, cy = centerY();
        float r = radius();
        float kr = r * 0.38f;
        boolean active = tiltMode ? holding : touching || kx != 0 || ky != 0;
        ring.setColor(active ? Ui.ACCENT : Ui.KEY_EDGE);

        // the base: a round pad, or a slot for a one-way stick
        if (axes == BOTH) {
            c.drawCircle(cx, cy, r, base);
            c.drawCircle(cx, cy, r, ring);
            c.drawCircle(cx, cy, r * 0.55f, ringFaint());
        } else {
            float half = kr * 1.15f;
            if (axes == UP_DOWN)
                box.set(cx - half, cy - r, cx + half, cy + r);
            else
                box.set(cx - r, cy - half, cx + r, cy + half);
            c.drawRoundRect(box, half, half, base);
            c.drawRoundRect(box, half, half, ring);
        }
        float tick = Ui.px(getContext(), 5), m = r - Ui.px(getContext(), 10);
        if (axes != LEFT_RIGHT) {
            c.drawLine(cx - tick, cy - m + tick, cx, cy - m, mark);
            c.drawLine(cx + tick, cy - m + tick, cx, cy - m, mark);
            c.drawLine(cx - tick, cy + m - tick, cx, cy + m, mark);
            c.drawLine(cx + tick, cy + m - tick, cx, cy + m, mark);
        }
        if (axes != UP_DOWN) {
            c.drawLine(cx - m + tick, cy - tick, cx - m, cy, mark);
            c.drawLine(cx - m + tick, cy + tick, cx - m, cy, mark);
            c.drawLine(cx + m - tick, cy - tick, cx + m, cy, mark);
            c.drawLine(cx + m - tick, cy + tick, cx + m, cy, mark);
        }
        c.drawText(tiltMode ? (holding ? "TILT TO DRIVE" : "HOLD + TILT") : title, cx, getHeight() - Ui.px(getContext(), 3), words);

        if (tiltMode && !holding) {
            knob.setColor(Ui.KEY);
            c.drawCircle(cx, cy, kr, knob);
            return;
        }
        float px = cx + kx * (r - kr), py = cy + ky * (r - kr);
        if (active) {
            trail.setStrokeWidth(kr * 0.9f);
            c.drawLine(cx, cy, px, py, trail);
        }
        knob.setColor(Ui.darker(Ui.KEY, 0.6f));
        c.drawCircle(px, py + Ui.px(getContext(), 3), kr, knob);
        knob.setColor(active ? Ui.lighter(Ui.KEY, 0.1f) : Ui.KEY);
        c.drawCircle(px, py, kr, knob);
        knobTop.setColor(active ? Ui.ACCENT : Ui.KEY_EDGE);
        c.drawCircle(px, py, kr * 0.62f, knobTop);
    }

    private Paint faint;

    private Paint ringFaint() {
        if (faint == null) {
            faint = new Paint(Paint.ANTI_ALIAS_FLAG);
            faint.setStyle(Paint.Style.STROKE);
            faint.setStrokeWidth(Ui.px(getContext(), 1));
            faint.setColor(Ui.alpha(Ui.KEY_EDGE, 0.6f));
        }
        return faint;
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
                invalidate();
            }
            return true;
        }
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                pointer = e.getPointerId(0);
                touching = true;
                getParent().requestDisallowInterceptTouchEvent(true);
                // fall through
            case MotionEvent.ACTION_MOVE: {
                int i = e.findPointerIndex(pointer);
                if (i < 0)
                    return true;
                float r = radius() * 0.62f;
                float x = (e.getX(i) - getWidth() / 2f) / r, y = (e.getY(i) - centerY()) / r;
                if (axes == UP_DOWN)
                    x = 0;
                if (axes == LEFT_RIGHT)
                    y = 0;
                float len = (float) Math.hypot(x, y);
                if (len > 1) {
                    x /= len;
                    y /= len;
                }
                place(x, y);
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
                place(0, 0);
                listener.onStick(this, 0, 0);
                return true;
        }
        return true;
    }
}
