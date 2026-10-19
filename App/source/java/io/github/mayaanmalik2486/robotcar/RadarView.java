package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.SystemClock;
import android.view.View;

/**
 * The radar screen: what the car's sensor sees while it sweeps (radar mode, START / O).
 * The car sits at the bottom middle; each dot is something the sensor saw, fading as it gets old.
 */
class RadarView extends View {

    private static final int SPAN = 50;            // the car looks 50 degrees to each side

    private CarState state;
    private final Paint grid = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint fan = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint beam = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint dot = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint label = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF box = new RectF();

    RadarView(Context c) {
        super(c);
        grid.setStyle(Paint.Style.STROKE);
        grid.setStrokeWidth(Ui.dp(c, 1));
        grid.setColor(Ui.LCD_DIM);
        fan.setColor(Color.argb(40, 124, 230, 150));
        beam.setStrokeWidth(Ui.dp(c, 2));
        beam.setColor(Ui.LCD_TEXT);
        dot.setColor(Ui.LCD_TEXT);
        label.setColor(Ui.LCD_DIM);
        label.setTextSize(Ui.dp(c, 10));
        label.setTypeface(android.graphics.Typeface.MONOSPACE);
    }

    void setState(CarState state) {
        this.state = state;
        invalidate();
    }

    @Override
    protected void onDraw(Canvas c) {
        float pad = Ui.dp(getContext(), 8);
        float cx = getWidth() / 2f;
        float cy = getHeight() - pad;
        float r = Math.min(getWidth() / 2f / (float) Math.sin(Math.toRadians(70)), getHeight() - pad * 2.5f);

        // the rings at 50, 100 and 150 cm, and the part the sensor can see
        for (int ring = 1; ring <= 3; ring++) {
            float rr = r * ring / 3f;
            box.set(cx - rr, cy - rr, cx + rr, cy + rr);
            c.drawArc(box, -160, 140, false, grid);
            c.drawText(ring * 50 + "cm", cx + Ui.dp(getContext(), 3), cy - rr + Ui.dp(getContext(), 11), label);
        }
        box.set(cx - r, cy - r, cx + r, cy + r);
        c.drawArc(box, -90 - SPAN, SPAN * 2, true, fan);
        c.drawCircle(cx, cy, Ui.dp(getContext(), 5), dot);          // the car

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
            float d = r * state.radarCm[k] / CarState.RADAR_RANGE_CM;
            dot.setAlpha((int) (255 * Math.max(0.15, 1 - age / 6000.0)));
            c.drawCircle(cx + (float) (d * Math.sin(a)), cy - (float) (d * Math.cos(a)), Ui.dp(getContext(), 5), dot);
        }
        dot.setAlpha(255);
        if (live) {                                             // the sweep line
            double a = angle(state.radarLook);
            c.drawLine(cx, cy, cx + (float) (r * Math.sin(a)), cy - (float) (r * Math.cos(a)), beam);
            postInvalidateDelayed(100);                         // (keep fading the dots)
        }
    }

    // direction k (0 = 50 degrees right ... 10 = 50 degrees left) as an angle from straight ahead (+ = right)
    private static double angle(int k) {
        return Math.toRadians((5 - k) * 10.0);
    }
}
