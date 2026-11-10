package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.StateListDrawable;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.HapticFeedbackConstants;
import android.widget.TextView;

/**
 * A chunky key that looks like a real one: a raised top on a darker base. Pressed, the top
 * sinks onto the base, the button shrinks a little and the phone gives a small click.
 *
 * It can also show a small badge in its corner - the controller button that does the same
 * job ("A", "RT", "LB+A", "▲") - and draw a simple icon instead of text.
 */
class PadButton extends TextView {

    static final int ICON_NONE = 0, ICON_GEAR = 1, ICON_HELP = 2, ICON_PAD = 3, ICON_LINK = 4;

    private final boolean round;
    private final int lip;                     // how far the top stands up from the base
    private int color;
    private boolean on;                        // a switch that is on: an orange edge
    private String badge;
    private int icon;
    private int iconColor = Ui.TEXT;
    private final Paint badgePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint badgeBack = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint iconPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF r = new RectF();
    private final Path path = new Path();

    PadButton(Context c, CharSequence label, int color, boolean round) {
        super(c);
        this.round = round;
        this.lip = Ui.dp(c, round ? 4 : 3);
        setText(label);
        setTextColor(Ui.TEXT);
        setTypeface(Ui.BOLD);
        setTextSize(round ? 15 : 14);
        setGravity(Gravity.CENTER);
        setIncludeFontPadding(false);
        setMaxLines(2);
        setEllipsize(TextUtils.TruncateAt.END);
        setClickable(true);
        setHapticFeedbackEnabled(true);
        badgePaint.setTypeface(Ui.BOLD);
        badgePaint.setTextSize(Ui.px(c, 10));
        badgePaint.setTextAlign(Paint.Align.CENTER);
        badgeBack.setColor(0x66000000);
        iconPaint.setStyle(Paint.Style.STROKE);
        iconPaint.setStrokeCap(Paint.Cap.ROUND);
        iconPaint.setStrokeJoin(Paint.Join.ROUND);
        setColor(color);
        showPressed(false);
    }

    void setColor(int color) {
        this.color = color;
        StateListDrawable bg = new StateListDrawable();
        bg.addState(new int[]{android.R.attr.state_pressed}, layers(true));
        bg.addState(new int[]{}, layers(false));
        setBackground(bg);
    }

    /** A switch look: on = an orange edge and an orange label. */
    void setOn(boolean on) {
        if (this.on == on)
            return;
        this.on = on;
        setColor(color);
        setTextColor(on ? Ui.ACCENT : Ui.TEXT);
        iconColor = on ? Ui.ACCENT : Ui.TEXT;
        invalidate();
    }

    /** The controller button for the same job, shown in the corner (null = none). */
    void setBadge(String badge) {
        if (TextUtils.equals(this.badge, badge))
            return;
        this.badge = badge;
        invalidate();
    }

    void setIcon(int icon) {
        this.icon = icon;
        invalidate();
    }

    /** Shows the button pressed for a moment (when a game controller pressed it). */
    void flash() {
        setPressed(true);
        postDelayed(() -> setPressed(false), 140);
    }

    @Override
    public void setPressed(boolean pressed) {
        if (pressed && !isPressed() && Ui.haptics)
            performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
        super.setPressed(pressed);
        showPressed(pressed);
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
        setAlpha(enabled ? 1f : 0.4f);
    }

    private void showPressed(boolean pressed) {
        int down = pressed ? lip - Ui.dp(getContext(), 1) : 0;
        int side = Ui.dp(getContext(), 4);
        setPadding(side, down, side, lip - down);
        float scale = pressed ? 0.96f : 1f;
        animate().scaleX(scale).scaleY(scale).setDuration(70).start();
        invalidate();
    }

    @Override
    protected void onDraw(Canvas c) {
        super.onDraw(c);
        int sink = isPressed() ? lip - Ui.dp(getContext(), 1) : 0;
        int faceH = getHeight() - lip;
        if (icon != ICON_NONE)
            drawIcon(c, getWidth() / 2f, sink + faceH / 2f, Math.min(getWidth(), faceH) * 0.24f);
        if (badge != null && getWidth() > Ui.dp(getContext(), 40)) {
            float tw = badgePaint.measureText(badge);
            float bh = Ui.px(getContext(), 15);
            float pad = Ui.px(getContext(), 5);
            float x = round ? getWidth() / 2f - tw / 2 - pad : Ui.px(getContext(), 5);
            float y = round ? sink + Ui.px(getContext(), 4) : sink + Ui.px(getContext(), 5);
            r.set(x, y, x + tw + 2 * pad, y + bh);
            c.drawRoundRect(r, bh / 2, bh / 2, badgeBack);
            badgePaint.setColor(Ui.padColor(badge));
            c.drawText(badge, r.centerX(), r.bottom - bh * 0.27f, badgePaint);
        }
    }

    private void drawIcon(Canvas c, float cx, float cy, float s) {
        iconPaint.setColor(iconColor);
        iconPaint.setStrokeWidth(Math.max(2f, s * 0.22f));
        iconPaint.setStyle(Paint.Style.STROKE);
        switch (icon) {
            case ICON_GEAR:
                c.drawCircle(cx, cy, s * 0.42f, iconPaint);
                for (int i = 0; i < 8; i++) {
                    double a = Math.PI / 4 * i;
                    c.drawLine(cx + (float) Math.cos(a) * s * 0.72f, cy + (float) Math.sin(a) * s * 0.72f,
                        cx + (float) Math.cos(a) * s * 1.02f, cy + (float) Math.sin(a) * s * 1.02f, iconPaint);
                }
                break;
            case ICON_HELP:
                path.reset();
                path.moveTo(cx - s * 0.5f, cy - s * 0.45f);
                path.cubicTo(cx - s * 0.5f, cy - s * 1.1f, cx + s * 0.55f, cy - s * 1.1f, cx + s * 0.55f, cy - s * 0.4f);
                path.cubicTo(cx + s * 0.55f, cy + s * 0.05f, cx, cy, cx, cy + s * 0.35f);
                c.drawPath(path, iconPaint);
                iconPaint.setStyle(Paint.Style.FILL);
                c.drawCircle(cx, cy + s * 0.9f, iconPaint.getStrokeWidth() * 0.7f, iconPaint);
                break;
            case ICON_PAD:
                r.set(cx - s * 1.15f, cy - s * 0.55f, cx + s * 1.15f, cy + s * 0.6f);
                c.drawRoundRect(r, s * 0.55f, s * 0.55f, iconPaint);
                c.drawLine(cx - s * 0.75f, cy, cx - s * 0.35f, cy, iconPaint);
                c.drawLine(cx - s * 0.55f, cy - s * 0.2f, cx - s * 0.55f, cy + s * 0.2f, iconPaint);
                iconPaint.setStyle(Paint.Style.FILL);
                c.drawCircle(cx + s * 0.5f, cy - s * 0.12f, s * 0.12f, iconPaint);
                c.drawCircle(cx + s * 0.75f, cy + s * 0.13f, s * 0.12f, iconPaint);
                break;
            default:
                break;
        }
    }

    private Drawable layers(boolean pressed) {
        float rad = round ? 1000 : Ui.px(getContext(), 12);
        GradientDrawable base = shape(Ui.darker(color, 0.5f), rad);
        GradientDrawable top = new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM,
            new int[]{Ui.lighter(color, pressed ? 0.02f : 0.09f), pressed ? Ui.darker(color, 0.92f) : color});
        top.setShape(round ? GradientDrawable.OVAL : GradientDrawable.RECTANGLE);
        if (!round)
            top.setCornerRadius(rad);
        top.setStroke(Ui.dp(getContext(), on ? 2 : 1), on ? Ui.ACCENT : Ui.lighter(color, pressed ? 0.05f : 0.14f));
        LayerDrawable d = new LayerDrawable(new Drawable[]{base, top});
        int sink = pressed ? lip - Ui.dp(getContext(), 1) : 0;
        d.setLayerInset(1, 0, sink, 0, lip - sink);
        return d;
    }

    private GradientDrawable shape(int c, float rad) {
        GradientDrawable g = new GradientDrawable();
        g.setShape(round ? GradientDrawable.OVAL : GradientDrawable.RECTANGLE);
        if (!round)
            g.setCornerRadius(rad);
        g.setColor(c);
        return g;
    }
}
