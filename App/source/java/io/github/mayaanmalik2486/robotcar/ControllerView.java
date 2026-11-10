package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;

import java.util.Locale;

/**
 * A drawing of the GameSir Cyclone 2, seen from the front, with every button's job written
 * beside it. Everything lights up live when you press it: the sticks move, the triggers fill.
 * Tap a button (or its label) to give it another job.
 *
 * The drawing is made in a 1000 x 640 box and scaled to fit; the labels sit in two columns.
 */
class ControllerView extends View {

    interface Listener {
        void onTapButton(int button);
    }

    // where each button is in the 1000 x 640 drawing
    private static final float[][] AT = new float[ControllerMap.BUTTONS][];
    private static final int LSTICK = 100, RSTICK = 101;     // (label rows for the sticks themselves)

    static {
        AT[ControllerMap.LT] = new float[]{260, 46};
        AT[ControllerMap.RT] = new float[]{740, 46};
        AT[ControllerMap.LB] = new float[]{262, 104};
        AT[ControllerMap.RB] = new float[]{738, 104};
        AT[ControllerMap.L3] = new float[]{290, 238};
        AT[ControllerMap.R3] = new float[]{610, 400};
        AT[ControllerMap.Y] = new float[]{712, 176};
        AT[ControllerMap.X] = new float[]{652, 236};
        AT[ControllerMap.B] = new float[]{772, 236};
        AT[ControllerMap.A] = new float[]{712, 296};
        AT[ControllerMap.SELECT] = new float[]{432, 236};
        AT[ControllerMap.START] = new float[]{568, 236};
        AT[ControllerMap.UP] = new float[]{390, 352};
        AT[ControllerMap.RIGHT] = new float[]{436, 398};
        AT[ControllerMap.DOWN] = new float[]{390, 444};
        AT[ControllerMap.LEFT] = new float[]{344, 398};
    }

    // the label rows: left column and right column, top to bottom
    private static final int[] LEFT = {ControllerMap.LT, ControllerMap.LB, LSTICK, ControllerMap.L3, ControllerMap.SELECT,
        ControllerMap.UP, ControllerMap.LEFT, ControllerMap.RIGHT, ControllerMap.DOWN};
    private static final int[] RIGHT = {ControllerMap.RT, ControllerMap.RB, ControllerMap.Y, ControllerMap.X, ControllerMap.B,
        ControllerMap.A, ControllerMap.START, RSTICK, ControllerMap.R3};

    private final ControllerInput pad;
    private final Listener listener;
    boolean showShift;                              // show the jobs of the SHIFT layer
    int highlight = -1;                             // a button to point at (remapping)
    private final Paint body = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint line = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint name = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path path = new Path();
    private final RectF r = new RectF();
    private float scale, ox, oy;
    private final RectF[] labelBox = new RectF[200];

    ControllerView(Context c, ControllerInput pad, Listener listener) {
        super(c);
        this.pad = pad;
        this.listener = listener;
        body.setColor(0xFF1C2028);
        line.setStyle(Paint.Style.STROKE);
        line.setStrokeCap(Paint.Cap.ROUND);
        text.setTypeface(Ui.MEDIUM);
        name.setTypeface(Ui.BOLD);
    }

    private float X(float x) {
        return ox + x * scale;
    }

    private float Y(float y) {
        return oy + y * scale;
    }

    @Override
    protected void onDraw(Canvas c) {
        float w = getWidth(), h = getHeight();
        float col = Math.min(w * 0.27f, Ui.px(getContext(), 250));
        float gap = Ui.px(getContext(), 12);
        float dw = w - 2 * col - 2 * gap, dh = h - Ui.px(getContext(), 24);
        scale = Math.min(dw / 1000f, dh / 640f);
        ox = col + gap + (dw - 1000 * scale) / 2;
        oy = (dh - 640 * scale) / 2;

        drawBody(c);
        drawParts(c);

        // the labels
        float size = Math.max(Ui.px(getContext(), 11), Math.min(Ui.px(getContext(), 14), h / 28f));
        text.setTextSize(size);
        name.setTextSize(size);
        float top = Math.max(0, oy), bottom = Math.min(h - Ui.px(getContext(), 24), oy + 640 * scale);
        drawColumn(c, LEFT, 0, col, top, bottom, true);
        drawColumn(c, RIGHT, w - col, col, top, bottom, false);

        text.setColor(Ui.FAINT);
        text.setTextAlign(Paint.Align.CENTER);
        c.drawText("HOME = Android    ·    SHARE = nothing    ·    M1 / M2 (back) = copy any button in the GameSir app",
            w / 2, h - Ui.px(getContext(), 6), text);
        text.setTextAlign(Paint.Align.LEFT);
    }

    private void drawBody(Canvas c) {
        path.reset();
        path.moveTo(X(230), Y(122));
        path.lineTo(X(770), Y(122));
        path.cubicTo(X(880), Y(122), X(930), Y(190), X(952), Y(300));
        path.cubicTo(X(978), Y(430), X(992), Y(565), X(900), Y(605));
        path.cubicTo(X(828), Y(636), X(772), Y(565), X(722), Y(505));
        path.cubicTo(X(660), Y(470), X(340), Y(470), X(278), Y(505));
        path.cubicTo(X(228), Y(565), X(172), Y(636), X(100), Y(605));
        path.cubicTo(X(8), Y(565), X(22), Y(430), X(48), Y(300));
        path.cubicTo(X(70), Y(190), X(120), Y(122), X(230), Y(122));
        path.close();
        c.drawPath(path, body);
        line.setColor(Ui.KEY_EDGE);
        line.setStrokeWidth(Ui.px(getContext(), 1.5f));
        c.drawPath(path, line);
    }

    private boolean down(int b) {
        return pad.buttons.down(b);
    }

    private int partColor(int b) {
        if (b == highlight)
            return Ui.ACCENT;
        return down(b) ? Ui.ACCENT : Ui.KEY;
    }

    private void drawParts(Canvas c) {
        float s = scale;
        // triggers: they fill up as you pull them
        for (int t = 0; t < 2; t++) {
            int b = ControllerMap.LT + t;
            float cx = AT[b][0];
            r.set(X(cx - 70), Y(22), X(cx + 70), Y(72));
            fill.setColor(Ui.KEY);
            c.drawRoundRect(r, 14 * s, 14 * s, fill);
            float v = pad.buttons.trigger(t);
            if (v > 0.01f) {
                fill.setColor(Ui.alpha(Ui.ACCENT, down(b) ? 1f : 0.55f));
                RectF f = new RectF(r.left, r.bottom - r.height() * v, r.right, r.bottom);
                c.drawRoundRect(f, 14 * s, 14 * s, fill);
            }
            outline(c, r, 14 * s, b);
        }
        // bumpers
        for (int b : new int[]{ControllerMap.LB, ControllerMap.RB}) {
            float cx = AT[b][0];
            r.set(X(cx - 92), Y(90), X(cx + 92), Y(118));
            fill.setColor(partColor(b));
            c.drawRoundRect(r, 14 * s, 14 * s, fill);
        }
        // sticks: the knob moves with the stick
        stick(c, 290, 238, pad.lx, pad.ly, ControllerMap.L3);
        stick(c, 610, 400, pad.rx, pad.ry, ControllerMap.R3);
        // D-pad
        float dx = 390, dy = 398, a = 22, len = 64;
        fill.setColor(Ui.KEY);
        r.set(X(dx - a), Y(dy - len), X(dx + a), Y(dy + len));
        c.drawRoundRect(r, 6 * s, 6 * s, fill);
        r.set(X(dx - len), Y(dy - a), X(dx + len), Y(dy + a));
        c.drawRoundRect(r, 6 * s, 6 * s, fill);
        int[] arms = {ControllerMap.UP, ControllerMap.RIGHT, ControllerMap.DOWN, ControllerMap.LEFT};
        for (int b : arms)
            if (down(b) || b == highlight) {
                fill.setColor(Ui.ACCENT);
                float x = AT[b][0], y = AT[b][1];
                r.set(X(x - a), Y(y - a), X(x + a), Y(y + a));
                c.drawRoundRect(r, 6 * s, 6 * s, fill);
            }
        // face buttons, with their own colours
        String[] letters = {"A", "B", "X", "Y"};
        for (int b = ControllerMap.A; b <= ControllerMap.Y; b++) {
            float x = AT[b][0], y = AT[b][1];
            fill.setColor(down(b) || b == highlight ? Ui.padColor(letters[b]) : Ui.KEY);
            c.drawCircle(X(x), Y(y), 27 * s, fill);
            name.setTextSize(26 * s);
            name.setTextAlign(Paint.Align.CENTER);
            name.setColor(down(b) || b == highlight ? Ui.BG : Ui.padColor(letters[b]));
            c.drawText(letters[b], X(x), Y(y + 9), name);
        }
        // the small middle buttons
        for (int b : new int[]{ControllerMap.SELECT, ControllerMap.START}) {
            r.set(X(AT[b][0] - 22), Y(AT[b][1] - 12), X(AT[b][0] + 22), Y(AT[b][1] + 12));
            fill.setColor(partColor(b));
            c.drawRoundRect(r, 12 * s, 12 * s, fill);
        }
        fill.setColor(Ui.KEY);
        c.drawCircle(X(500), Y(178), 22 * s, fill);                              // Home
        line.setColor(Ui.ACCENT);
        line.setStrokeWidth(2.5f * s);
        c.drawCircle(X(500), Y(178), 22 * s, line);
        r.set(X(486), Y(288), X(514), Y(306));                                    // Share
        fill.setColor(pad.other ? Ui.ACCENT : Ui.KEY);
        c.drawRoundRect(r, 5 * s, 5 * s, fill);
    }

    private void outline(Canvas c, RectF box, float rad, int b) {
        if (!down(b) && b != highlight)
            return;
        line.setColor(Ui.ACCENT);
        line.setStrokeWidth(Ui.px(getContext(), 2));
        c.drawRoundRect(box, rad, rad, line);
    }

    private void stick(Canvas c, float x, float y, float sx, float sy, int click) {
        fill.setColor(0xFF12151B);
        c.drawCircle(X(x), Y(y), 66 * scale, fill);
        float kx = x + sx * 26, ky = y + sy * 26;
        boolean moved = Math.abs(sx) > 0.05f || Math.abs(sy) > 0.05f;
        fill.setColor(down(click) || click == highlight ? Ui.ACCENT : moved ? Ui.lighter(Ui.KEY, 0.12f) : Ui.KEY);
        c.drawCircle(X(kx), Y(ky), 42 * scale, fill);
        line.setColor(moved ? Ui.ACCENT : Ui.KEY_EDGE);
        line.setStrokeWidth(2.5f * scale);
        c.drawCircle(X(kx), Y(ky), 28 * scale, line);
    }

    private void drawColumn(Canvas c, int[] rows, float x, float w, float top, float bottom, boolean left) {
        float rowH = Math.max(Ui.px(getContext(), 26), (bottom - top) / rows.length);
        float y = top + Math.max(0, ((bottom - top) - rowH * rows.length) / 2);
        for (int row : rows) {
            float cy = y + rowH / 2;
            String who, job;
            int b = row;
            if (row == LSTICK) {
                who = "Left stick";
                job = "Throttle " + num(-pad.throttle());
                b = -1;
            } else if (row == RSTICK) {
                who = "Right stick";
                job = "Steering " + num(pad.steer());
                b = -1;
            } else {
                who = ControllerMap.BUTTON_NAMES[row];
                char j = pad.map.job(row, showShift || pad.shiftHeld());
                job = j == ControllerMap.SHIFT ? "SHIFT (hold)" : ControllerMap.jobName(j);
                if (row == ControllerMap.LT || row == ControllerMap.RT)
                    job += "  " + Math.round(pad.buttons.trigger(row - ControllerMap.LT) * 100) + "%";
            }
            boolean lit = b >= 0 && (down(b) || b == highlight) || b < 0 && (row == LSTICK ? pad.throttle() != 0 : pad.steer() != 0);
            RectF box = new RectF(x, cy - rowH * 0.42f, x + w, cy + rowH * 0.42f);
            labelBox[row] = box;
            fill.setColor(lit ? Ui.alpha(Ui.ACCENT, 0.18f) : Ui.PANEL);
            c.drawRoundRect(box, Ui.px(getContext(), 8), Ui.px(getContext(), 8), fill);

            float pad8 = Ui.px(getContext(), 8);
            name.setTextAlign(Paint.Align.LEFT);
            name.setColor(b >= ControllerMap.A && b <= ControllerMap.Y ? Ui.padColor(ControllerMap.BUTTON_NAMES[b]) : Ui.LABEL);
            float nameW = name.measureText("D-pad right ");
            c.drawText(who, box.left + pad8, cy + text.getTextSize() * 0.36f, name);
            text.setColor(lit ? Ui.ACCENT : Ui.TEXT);
            String shown = job;
            float room = box.width() - nameW - 2 * pad8;
            while (shown.length() > 3 && text.measureText(shown) > room)
                shown = shown.substring(0, shown.length() - 2) + "…";
            c.drawText(shown, box.left + pad8 + nameW, cy + text.getTextSize() * 0.36f, text);

            // a thin line from the label to the button
            float[] at = row == LSTICK ? new float[]{290, 238} : row == RSTICK ? new float[]{610, 400} : AT[row];
            line.setColor(lit ? Ui.ACCENT : Ui.alpha(Ui.FAINT, 0.6f));
            line.setStrokeWidth(Ui.px(getContext(), 1));
            float lx = left ? box.right : box.left;
            c.drawLine(lx, cy, X(at[0]), Y(at[1]), line);
            y += rowH;
        }
    }

    private static String num(float v) {
        return String.format(Locale.US, "%+.2f", v);
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        if (e.getActionMasked() != MotionEvent.ACTION_UP)
            return true;
        float x = e.getX(), y = e.getY();
        for (int row = 0; row < ControllerMap.BUTTONS; row++)
            if (labelBox[row] != null && labelBox[row].contains(x, y)) {
                listener.onTapButton(row);
                return true;
            }
        int best = -1;
        float bestD = 60 * scale;
        for (int b = 0; b < ControllerMap.BUTTONS; b++) {
            float d = (float) Math.hypot(X(AT[b][0]) - x, Y(AT[b][1]) - y);
            if (d < bestD) {
                bestD = d;
                best = b;
            }
        }
        if (best >= 0)
            listener.onTapButton(best);
        return true;
    }
}
