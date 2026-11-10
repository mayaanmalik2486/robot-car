package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.View;

/**
 * The car's little green screen: the connection on top, what the car is doing in big
 * letters, its switches underneath. With steering, two bars at the sides show how fast each
 * side of the car is going (up = forward, down = backward).
 */
class LcdView extends View {

    private String top = "", big = "", bottom = "";
    private boolean warn;                      // the top line in yellow (a problem)
    private int left, right;                   // wheel speeds -31 ... 31
    private boolean bars;
    private final Paint back = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint edge = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint bar = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint scan = new Paint();
    private final RectF r = new RectF();

    LcdView(Context c) {
        super(c);
        back.setColor(Ui.LCD);
        edge.setStyle(Paint.Style.STROKE);
        edge.setStrokeWidth(Ui.px(c, 1.5f));
        edge.setColor(Ui.LCD_EDGE);
        text.setTypeface(Ui.MONO);
        scan.setColor(0x10000000);
        setClickable(true);
    }

    void set(String top, boolean warn, String big, String bottom) {
        if (top.equals(this.top) && warn == this.warn && big.equals(this.big) && bottom.equals(this.bottom))
            return;
        this.top = top;
        this.warn = warn;
        this.big = big;
        this.bottom = bottom;
        invalidate();
    }

    /** The wheel bars (show = false hides them). */
    void wheels(boolean show, int left, int right) {
        if (show == bars && left == this.left && right == this.right)
            return;
        bars = show;
        this.left = left;
        this.right = right;
        invalidate();
    }

    @Override
    protected void onDraw(Canvas c) {
        float w = getWidth(), h = getHeight();
        float rad = Ui.px(getContext(), 12);
        r.set(0, 0, w, h);
        c.drawRoundRect(r, rad, rad, back);
        for (float y = 2; y < h; y += Ui.px(getContext(), 3))          // faint scan lines, like a real LCD
            c.drawRect(0, y, w, y + 1, scan);
        r.inset(edge.getStrokeWidth() / 2, edge.getStrokeWidth() / 2);
        c.drawRoundRect(r, rad, rad, edge);

        float pad = Ui.px(getContext(), 12);
        float barW = bars ? Ui.px(getContext(), 8) : 0;
        float x0 = pad + (bars ? barW + Ui.px(getContext(), 8) : 0);
        float x1 = w - x0;
        float tw = x1 - x0;

        float small = Math.min(h * 0.16f, Ui.px(getContext(), 13));
        float bigSize = Math.min(h * 0.38f, Ui.px(getContext(), 34));
        text.setTextSize(bigSize);
        float fit = text.measureText(big);
        if (fit > tw)
            bigSize *= tw / fit;

        text.setTextSize(small);
        text.setColor(warn ? Ui.LCD_WARN : Ui.LCD_DIM);
        c.drawText(ellipsize(top, tw), x0, pad * 0.6f + small, text);
        text.setColor(Ui.LCD_TEXT);
        c.drawText(ellipsize(bottom, tw), x0, h - pad * 0.7f, text);
        text.setTextSize(bigSize);
        text.setColor(Ui.LCD_TEXT);
        c.drawText(big, x0, h / 2f + bigSize * 0.36f, text);

        if (bars) {
            drawBar(c, pad, left, h, pad);
            drawBar(c, w - pad - barW, right, h, pad);
        }
    }

    private void drawBar(Canvas c, float x, int speed, float h, float pad) {
        float bw = Ui.px(getContext(), 8);
        float top = pad * 0.7f, bot = h - pad * 0.7f, mid = (top + bot) / 2;
        bar.setColor(Ui.alpha(Ui.LCD_DIM, 0.35f));
        r.set(x, top, x + bw, bot);
        c.drawRoundRect(r, bw / 2, bw / 2, bar);
        bar.setColor(Ui.LCD_TEXT);
        float len = (bot - top) / 2 * Math.abs(speed) / Car.WHEEL_MAX;
        if (speed > 0)
            r.set(x, mid - len, x + bw, mid);
        else
            r.set(x, mid, x + bw, mid + len);
        if (speed != 0)
            c.drawRoundRect(r, bw / 2, bw / 2, bar);
        c.drawRect(x - 2, mid - 1, x + bw + 2, mid + 1, bar);
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
