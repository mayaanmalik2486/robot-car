package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RadialGradient;
import android.graphics.RectF;
import android.graphics.Shader;
import android.os.SystemClock;
import android.view.View;

/**
 * The radar page: what the car's sensor sees while it sweeps (radar mode, START / O), seen
 * from above. The car sits at the bottom middle; each glowing dot is something the sensor
 * saw, fading as it gets old. (The 3D picture above shows the same dots on its floor.)
 */
class RadarView extends View {

    private static final int SPAN = 50;            // the car looks 50 degrees to each side

    private CarState state;
    private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint label = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF box = new RectF();

    RadarView(Context c) {
        super(c);
        label.setTextSize(Ui.px(c, 10));
        label.setTypeface(Ui.MONO);
    }

    void setState(CarState state) {
        this.state = state;
        invalidate();
    }

    @Override
    protected void onDraw(Canvas c) {
        float d = getResources().getDisplayMetrics().density;
        float w = getWidth(), h = getHeight();
        float pad = 10 * d;
        float cx = w / 2f;
        float cy = h - pad;
        float r = Math.min(w / 2f / (float) Math.sin(Math.toRadians(70)), h - pad * 2.5f);

        // the glass panel
        p.setShader(null);
        p.setStyle(Paint.Style.FILL);
        p.setColor(0x99040714);
        box.set(0, 0, w, h);
        c.drawRoundRect(box, 18 * d, 18 * d, p);
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeWidth(d);
        p.setColor(0x1AFFFFFF);
        box.inset(d / 2, d / 2);
        c.drawRoundRect(box, 18 * d, 18 * d, p);

        // the rings at 50, 100 and 150 cm, and the part the sensor can see
        label.setTextAlign(Paint.Align.LEFT);
        label.setColor(Ui.alpha(Ui.CYAN, 0.7f));
        p.setColor(Ui.alpha(Ui.CYAN, 0.35f));
        for (int ring = 1; ring <= 3; ring++) {
            float rr = r * ring / 3f;
            box.set(cx - rr, cy - rr, cx + rr, cy + rr);
            c.drawArc(box, -160, 140, false, p);
            c.drawText(ring * 50 + " cm", cx + 4 * d, cy - rr + 12 * d, label);
        }
        p.setStyle(Paint.Style.FILL);
        p.setShader(new RadialGradient(cx, cy, r, Ui.alpha(Ui.CYAN, 0.30f), Ui.alpha(Ui.CYAN, 0.04f), Shader.TileMode.CLAMP));
        box.set(cx - r, cy - r, cx + r, cy + r);
        c.drawArc(box, -90 - SPAN, SPAN * 2, true, p);
        p.setShader(null);
        p.setColor(Ui.alpha(Ui.ACCENT, 0.25f));                           // the car, with a little glow
        c.drawCircle(cx, cy, 11 * d, p);
        p.setColor(Ui.ACCENT);
        c.drawCircle(cx, cy, 6 * d, p);

        if (state == null)
            return;
        long now = SystemClock.uptimeMillis();
        boolean live = false;
        for (int k = 0; k < CarState.RADAR_STEPS; k++) {
            long age = now - state.radarAt[k];
            if (state.radarAt[k] == 0 || age > 6000)
                continue;
            live |= age < 1000;
            if (state.radarCm[k] >= CarState.RADAR_RANGE_CM)
                continue;
            double a = angle(k);
            float dist = r * state.radarCm[k] / CarState.RADAR_RANGE_CM;
            float fade = (float) Math.max(0.15, 1 - age / 6000.0);
            float x = cx + (float) (dist * Math.sin(a)), y = cy - (float) (dist * Math.cos(a));
            p.setColor(Ui.alpha(Ui.CYAN, 0.25f * fade));
            c.drawCircle(x, y, 11 * d, p);
            p.setColor(Ui.alpha(0xFF9FF5EA, fade));
            c.drawCircle(x, y, 5.5f * d, p);
        }
        if (live) {                                             // the sweep line
            double a = angle(state.radarLook);
            p.setStrokeWidth(2 * d);
            p.setColor(Ui.CYAN);
            c.drawLine(cx, cy, cx + (float) (r * Math.sin(a)), cy - (float) (r * Math.cos(a)), p);
            postInvalidateDelayed(100);                         // (keep fading the dots)
        } else if (!state.mode.equals("RADAR")) {
            label.setTextAlign(Paint.Align.CENTER);
            label.setColor(Ui.LABEL);
            c.drawText("START THE RADAR: THE CAR LOOKS AROUND", cx, pad + 14 * d, label);
        }
    }

    // direction k (0 = 50 degrees right ... 10 = 50 degrees left) as an angle from straight ahead (+ = right)
    private static double angle(int k) {
        return Math.toRadians((5 - k) * 10.0);
    }
}
