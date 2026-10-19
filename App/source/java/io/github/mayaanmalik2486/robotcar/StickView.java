package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.MotionEvent;
import android.view.View;

/**
 * The on-screen thumbstick. In tilt mode it becomes a "hold to drive" pad: the knob then
 * shows how the phone is tilted, and the car only drives while a finger is on it.
 */
class StickView extends View {

    interface Listener {
        /** The stick moved (x, y from -1 to 1, y down = positive; 0, 0 = let go). */
        void onStick(float x, float y);

        /** Tilt mode: a finger went down (true) or was lifted (false). */
        void onHold(boolean holding);
    }

    private final Listener listener;
    private float kx, ky;                              // knob position, -1 .. 1
    private boolean tiltMode;
    private boolean holding;
    private final Paint base = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint ring = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint knob = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint knobEdge = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint mark = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint words = new Paint(Paint.ANTI_ALIAS_FLAG);

    StickView(Context c, Listener listener) {
        super(c);
        this.listener = listener;
        base.setColor(Ui.PANEL);
        ring.setStyle(Paint.Style.STROKE);
        ring.setStrokeWidth(Ui.dp(c, 3));
        knobEdge.setStyle(Paint.Style.STROKE);
        knobEdge.setStrokeWidth(Ui.dp(c, 4));
        knobEdge.setColor(Color.rgb(40, 44, 52));
        mark.setColor(Color.rgb(110, 118, 132));
        mark.setTextSize(Ui.dp(c, 14));
        mark.setTextAlign(Paint.Align.CENTER);
        words.setColor(Ui.LABEL);
        words.setTextSize(Ui.dp(c, 12));
        words.setTextAlign(Paint.Align.CENTER);
        words.setFakeBoldText(true);
    }

    void setTiltMode(boolean on) {
        tiltMode = on;
        holding = false;
        show(0, 0);
    }

    /** Moves the knob (to show what a game controller or the tilt is doing). */
    void show(float x, float y) {
        kx = Math.max(-1, Math.min(1, x));
        ky = Math.max(-1, Math.min(1, y));
        invalidate();
    }

    @Override
    protected void onDraw(Canvas c) {
        float cx = getWidth() / 2f, cy = getHeight() / 2f;
        float r = Math.min(cx, cy) * 0.94f;
        float kr = r * 0.40f;
        boolean live = !tiltMode || holding;
        ring.setColor(tiltMode && holding ? Ui.KEY_ON : Color.rgb(64, 70, 80));
        c.drawCircle(cx, cy, r, base);
        c.drawCircle(cx, cy, r, ring);
        float m = r - Ui.dp(getContext(), 12);
        c.drawText("▲", cx, cy - m + Ui.dp(getContext(), 5), mark);
        c.drawText("▼", cx, cy + m + Ui.dp(getContext(), 5), mark);
        c.drawText("◀", cx - m, cy + Ui.dp(getContext(), 5), mark);
        c.drawText("▶", cx + m, cy + Ui.dp(getContext(), 5), mark);
        if (tiltMode && !holding) {
            c.drawText("HOLD HERE", cx, cy - Ui.dp(getContext(), 4), words);
            c.drawText("+ TILT 📱", cx, cy + Ui.dp(getContext(), 14), words);
            return;
        }
        float px = cx + kx * (r - kr), py = cy + ky * (r - kr);
        knob.setColor(live && tiltMode ? Ui.KEY_ON : Color.rgb(84, 90, 104));
        c.drawCircle(px, py + Ui.dp(getContext(), 3), kr, knobEdge);             // its shadow edge
        c.drawCircle(px, py, kr, knob);
        c.drawCircle(px, py, kr * 0.55f, knobEdge);
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        int action = e.getActionMasked();
        if (tiltMode) {
            boolean down = action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_MOVE;
            if (down != holding) {
                holding = down;
                if (down)
                    performHapticFeedback(android.view.HapticFeedbackConstants.VIRTUAL_KEY);
                listener.onHold(down);
                invalidate();
            }
            return true;
        }
        float cx = getWidth() / 2f, cy = getHeight() / 2f;
        float r = Math.min(cx, cy) * 0.94f * 0.6f;
        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_MOVE: {
                float x = (e.getX() - cx) / r, y = (e.getY() - cy) / r;
                float len = (float) Math.hypot(x, y);
                if (len > 1) {
                    x /= len;
                    y /= len;
                }
                show(x, y);
                listener.onStick(x, y);
                return true;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                show(0, 0);
                listener.onStick(0, 0);
                return true;
        }
        return true;
    }
}
