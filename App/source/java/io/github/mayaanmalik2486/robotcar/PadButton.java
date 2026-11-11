package io.github.mayaanmalik2486.robotcar;

import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RadialGradient;
import android.graphics.RectF;
import android.graphics.Shader;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.HapticFeedbackConstants;
import android.view.MotionEvent;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.OvershootInterpolator;
import android.widget.TextView;

/**
 * A glossy 3D "bubble" key: a rounded face standing on a darker base, with a shine on top
 * and a soft glow of its own colour under it. Pressed, the face sinks onto the base, the key
 * squashes and springs back, a ripple spreads from the finger and the phone gives a click.
 *
 * It is a glass key (dark, see-through look) or a coloured bubble (a hue 0 ... 360). It can
 * carry a round picture bubble ("chip") with an emoji, a small badge in its corner - the
 * controller button that does the same job ("A", "RT", "LB+A", "▲") - or a simple icon.
 */
class PadButton extends TextView {

    static final int ICON_NONE = 0, ICON_GEAR = 1, ICON_HELP = 2, ICON_PAD = 3;
    static final int GLASS = -1;

    private int hue = GLASS;                   // -1 = a glass key, else its colour
    private boolean round;                     // a circle (when its box is square)
    private boolean autoRound;                 // become a circle whenever the box is square
    private boolean flat;                      // no key shape at all: just the words (the tabs)
    private boolean darkText;
    private boolean on;                        // switched on: a glowing ring
    private String chip;                       // the emoji in the round picture bubble
    private int chipHue;
    private String badge;
    private int icon;
    private float rippleX, rippleY, ripple = 1;   // 0 ... 1, 1 = no ripple
    private boolean chipBeside;
    private float chipSize, chipCx, chipCy;
    private final float lip;                   // how far the face stands up from the base
    private float wantedSp = 14;               // the letter size asked for (they shrink to fit a narrow key)
    private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint fit = new Paint();
    private final RectF r = new RectF();
    private final Path path = new Path();
    private Shader faceShader, glossShader, chipShader;
    private int shaderW, shaderH;

    PadButton(Context c, CharSequence label, int hue, boolean round) {
        super(c);
        this.hue = hue;
        this.round = round;
        lip = Ui.px(c, 4);
        setText(label);
        setTextColor(Ui.TEXT);
        setTypeface(Ui.BOLD);
        setTextSize(14);
        setLetterSpacing(0.04f);
        setGravity(Gravity.CENTER);
        setIncludeFontPadding(false);
        setMaxLines(2);
        setEllipsize(TextUtils.TruncateAt.END);
        setClickable(true);
        setHapticFeedbackEnabled(true);
        setBackground(null);
        text.setTypeface(Ui.BOLD);
        text.setTextAlign(Paint.Align.CENTER);
        applyPadding();
    }

    /** A coloured bubble (hue 0 ... 360), or GLASS. */
    void setHue(int hue) {
        if (this.hue == hue)
            return;
        this.hue = hue;
        faceShader = null;
        setTextColor(darkText && hue != GLASS ? 0xFF2A1B00 : Ui.TEXT);
        invalidate();
    }

    /** Dark words (on the yellow and the teal bubble). */
    void setDarkText(boolean dark) {
        darkText = dark;
        setTextColor(dark && hue != GLASS ? 0xFF2A1B00 : Ui.TEXT);
    }

    void setFlat(boolean flat) {
        this.flat = flat;
        invalidate();
    }

    /** Words are always in capitals (styled two-line labels are written in capitals already). */
    @Override
    public void setText(CharSequence text, BufferType type) {
        super.setText(text instanceof String ? ((String) text).toUpperCase() : text, type);
    }

    @Override
    public void setTextSize(float sp) {
        wantedSp = sp;
        super.setTextSize(sp);
        if (getWidth() > 0)
            applyPadding();
    }

    // letters as big as asked, but never wider than the key
    private void fitText(float room) {
        float d = getResources().getDisplayMetrics().density, full = wantedSp * d;
        fit.setTypeface(getPaint().getTypeface());
        fit.setTextSize(full);
        fit.setLetterSpacing(getPaint().getLetterSpacing());
        float widest = 0;
        for (String line : getText().toString().split("\n"))
            widest = Math.max(widest, fit.measureText(line));
        float size = widest > room && room > 0 ? Math.max(full * 0.55f, full * room / widest) : full;
        if (Math.abs(size - getTextSize()) > 0.5f)
            super.setTextSize(0, size);                 // (0 = in pixels)
    }

    /** Be a circle whenever the box is square (the four action bubbles beside one stick). */
    void setAutoRound(boolean auto) {
        autoRound = auto;
    }

    void setRound(boolean round) {
        if (this.round == round)
            return;
        this.round = round;
        faceShader = null;
        invalidate();
    }

    /** Switched on: a glowing ring in the accent colour (or the chip's colour). */
    void setOn(boolean on) {
        if (this.on == on)
            return;
        this.on = on;
        if (flat)
            setTextColor(on ? Ui.TEXT : Ui.LABEL);
        invalidate();
    }

    /** The round picture bubble: an emoji on a glossy ball of this hue. */
    void setChip(String emoji, int hue) {
        chip = emoji;
        chipHue = hue;
        chipShader = null;
        requestLayout();
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
        rippleX = getWidth() / 2f;
        rippleY = getHeight() / 2f;
        setPressed(true);
        postDelayed(() -> setPressed(false), 150);
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        if (e.getActionMasked() == MotionEvent.ACTION_DOWN) {
            rippleX = e.getX();
            rippleY = e.getY();
        }
        return super.onTouchEvent(e);
    }

    @Override
    public void setPressed(boolean pressed) {
        boolean was = isPressed();
        super.setPressed(pressed);
        if (pressed == was)
            return;
        if (pressed) {
            if (Ui.haptics)
                performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
            animate().scaleX(0.95f).scaleY(0.95f).setDuration(60).setInterpolator(new DecelerateInterpolator()).start();
            ValueAnimator a = ValueAnimator.ofFloat(0, 1);                  // the ripple
            a.setDuration(480);
            a.addUpdateListener(v -> {
                ripple = (float) v.getAnimatedValue();
                invalidate();
            });
            a.start();
        } else {
            animate().scaleX(1).scaleY(1).setDuration(320).setInterpolator(new OvershootInterpolator(3.5f)).start();   // it springs back
        }
        applyPadding();
        invalidate();
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
        setAlpha(enabled ? 1f : 0.4f);
    }

    private float sink() {
        return isPressed() && !flat ? lip - Ui.px(getContext(), 1) : 0;
    }

    @Override
    protected void onSizeChanged(int w, int h, int ow, int oh) {
        super.onSizeChanged(w, h, ow, oh);
        if (autoRound)
            setRound(Math.abs(w - h) < Ui.px(getContext(), 4));
        applyPadding();
    }

    // where the words go: under the picture bubble, or beside it on a low, wide key
    private void applyPadding() {
        int w = getWidth(), h = getHeight();
        int side = Ui.dp(getContext(), 4);
        int down = Math.round(sink()), bottom = Math.round(flat ? 0 : lip) - down;
        if (w > 0)
            fitText(w - 2 * side - (round ? w * 0.12f : 0));
        if (chip == null || w == 0) {
            setGravity(Gravity.CENTER);
            setPadding(side, down, side, bottom);
            return;
        }
        float faceH = h - lip;
        chipBeside = faceH < Ui.px(getContext(), 62) && w >= Ui.px(getContext(), 124);
        if (chipBeside) {
            chipSize = Math.min(faceH * 0.62f, Ui.px(getContext(), 40));
            fitText(w - 3 * side - chipSize - Ui.px(getContext(), 8));
            float textW = getPaint().measureText(getText().toString().toUpperCase());
            float total = chipSize + Ui.px(getContext(), 8) + textW;
            float left = Math.max(side, (w - total) / 2f);
            chipCx = left + chipSize / 2;
            chipCy = faceH / 2;
            setGravity(Gravity.CENTER_VERTICAL | Gravity.LEFT);
            setPadding(Math.round(left + chipSize + Ui.px(getContext(), 8)), down, side, bottom);
        } else {
            float textH = getTextSize() * 1.15f;
            chipSize = Math.max(Ui.px(getContext(), 16), Math.min(Ui.px(getContext(), 46), Math.min((faceH - textH) * 0.72f, w * 0.5f)));
            float gap = chipSize * 0.14f;
            float top = Math.max(Ui.px(getContext(), 3), (faceH - (chipSize + gap + textH)) / 2f);
            chipCx = w / 2f;
            chipCy = top + chipSize / 2;
            setGravity(Gravity.CENTER_HORIZONTAL | Gravity.TOP);
            setPadding(side / 2, Math.round(top + chipSize + gap) + down, side / 2, 0);
        }
    }

    @Override
    protected void onDraw(Canvas c) {
        float w = getWidth(), h = getHeight(), d = getResources().getDisplayMetrics().density;
        if (w == 0 || h == 0)
            return;
        float sink = sink();
        float faceH = h - lip;
        if (!flat)
            drawKey(c, w, h, faceH, sink, d);
        if (chip != null)
            drawChip(c, sink, d);
        if (icon != ICON_NONE)
            drawIcon(c, w / 2f, sink + faceH / 2f, Math.min(w, faceH) * 0.24f);
        super.onDraw(c);                              // the words
        if (badge != null && w > 40 * d) {
            text.setTextSize(9.5f * d);
            text.setTypeface(Ui.MONO);
            float tw = text.measureText(badge), bh = 15 * d, pad = 5 * d;
            // on a key with a picture it sits in the corner; on a plain key it rides on the top edge, clear of the words
            float x = round ? w / 2f - tw / 2 - pad : chip != null ? 5 * d : 9 * d, y = round || chip == null ? sink - 6 * d : sink + 5 * d;
            r.set(x, y, x + tw + 2 * pad, y + bh);
            p.setShader(null);
            p.setStyle(Paint.Style.FILL);
            p.setColor(0xF2060A1C);
            c.drawRoundRect(r, bh / 2, bh / 2, p);
            p.setStyle(Paint.Style.STROKE);
            p.setStrokeWidth(d);
            p.setColor(0x40FFFFFF);
            c.drawRoundRect(r, bh / 2, bh / 2, p);
            p.setStyle(Paint.Style.FILL);
            text.setColor(Ui.padColor(badge));
            c.drawText(badge, r.centerX(), r.bottom - bh * 0.28f, text);
        }
    }

    private void drawKey(Canvas c, float w, float h, float faceH, float sink, float d) {
        boolean glass = hue == GLASS;
        float rad = round ? Math.min(w, faceH) / 2f : Math.min(16 * d, faceH / 2f);
        p.setStyle(Paint.Style.FILL);
        p.setShader(null);
        // the glow under a coloured bubble, or round a key that is switched on
        int glow = on ? (chip != null ? Ui.hsl(chipHue, 1f, 0.6f) : Ui.ACCENT) : glass ? 0 : Ui.hsl(hue, 1f, 0.55f);
        if (glow != 0) {
            for (int i = 3; i >= 1; i--) {
                float g = i * 3.2f * d;
                p.setColor(Ui.alpha(glow, on ? 0.10f : 0.085f));
                r.set(-g * 0.4f, lip + (on ? 0 : 4 * d) - g * 0.5f, w + g * 0.4f, h + g * (on ? 0.6f : 1f));
                c.drawRoundRect(r, rad + g, rad + g, p);
            }
        }
        // the base the key stands on
        p.setColor(glass ? Ui.KEY_BASE : Ui.hsl(hue, 0.85f, 0.28f));
        r.set(0, lip, w, h);
        c.drawRoundRect(r, rad, rad, p);
        // the face
        if (faceShader == null || shaderW != getWidth() || shaderH != getHeight()) {
            shaderW = getWidth();
            shaderH = getHeight();
            faceShader = new LinearGradient(0, 0, 0, faceH, glass ? Ui.KEY_HI : Ui.hsl(hue, 1f, 0.66f), glass ? Ui.KEY : Ui.hsl(hue, 0.92f, 0.50f), Shader.TileMode.CLAMP);
            glossShader = new LinearGradient(0, 0, 0, faceH * 0.5f, glass ? 0x38FFFFFF : 0x70FFFFFF, 0x00FFFFFF, Shader.TileMode.CLAMP);
        }
        c.save();
        c.translate(0, sink);
        r.set(0, 0, w, faceH);
        p.setShader(faceShader);
        c.drawRoundRect(r, rad, rad, p);
        // the shine on its upper half
        p.setShader(glossShader);
        if (round) {
            r.set(w * 0.16f, faceH * 0.05f, w * 0.84f, faceH * 0.48f);
            c.drawOval(r, p);
        } else {
            path.reset();
            r.set(3 * d, 2 * d, w - 3 * d, faceH * 0.48f);
            float tr = Math.max(2, rad - 2 * d), br = Math.min(r.width() / 2, faceH * 0.2f);
            path.addRoundRect(r, new float[]{tr, tr, tr, tr, br, br, br, br}, Path.Direction.CW);
            c.drawPath(path, p);
        }
        p.setShader(null);
        // its edge
        p.setStyle(Paint.Style.STROKE);
        if (on) {
            p.setStrokeWidth(2 * d);
            p.setColor(chip != null ? Ui.hsl(chipHue, 1f, 0.64f) : Ui.ACCENT);
        } else {
            p.setStrokeWidth(d);
            p.setColor(glass ? 0x30FFFFFF : 0x55FFFFFF);
        }
        float in = p.getStrokeWidth() / 2;
        r.set(in, in, w - in, faceH - in);
        c.drawRoundRect(r, rad - in, rad - in, p);
        p.setStyle(Paint.Style.FILL);
        // the ripple from the finger
        if (ripple < 1) {
            c.save();
            path.reset();
            r.set(0, 0, w, faceH);
            path.addRoundRect(r, rad, rad, Path.Direction.CW);
            c.clipPath(path);
            p.setColor(Ui.alpha(0xFFFFFFFF, 0.34f * (1 - ripple)));
            c.drawCircle(rippleX, rippleY - sink, (float) Math.hypot(w, h) * ripple, p);
            c.restore();
        }
        c.restore();
    }

    private void drawChip(Canvas c, float sink, float d) {
        float cx = chipCx, cy = chipCy + sink, rad = chipSize / 2;
        p.setStyle(Paint.Style.FILL);
        p.setShader(null);
        p.setColor(Ui.alpha(Ui.hsl(chipHue, 1f, 0.55f), 0.30f));          // its glow
        c.drawCircle(cx, cy + rad * 0.28f, rad * 1.12f, p);
        if (chipShader == null)
            chipShader = new RadialGradient(-0.32f, -0.42f, 1.5f, new int[]{0xE6FFFFFF, Ui.hsl(chipHue, 1f, 0.70f), Ui.hsl(chipHue, 0.9f, 0.46f)},
                new float[]{0.06f, 0.30f, 1f}, Shader.TileMode.CLAMP);
        c.save();
        c.translate(cx, cy);
        c.scale(rad, rad);
        p.setShader(chipShader);
        c.drawCircle(0, 0, 1, p);
        p.setShader(null);
        c.restore();
        text.setTypeface(null);
        text.setColor(0xFFFFFFFF);
        text.setTextSize(chipSize * 0.52f);
        c.drawText(chip, cx, cy + chipSize * 0.19f, text);
    }

    private void drawIcon(Canvas c, float cx, float cy, float s) {
        p.setShader(null);
        p.setColor(on ? Ui.ACCENT2 : Ui.TEXT);
        p.setStrokeWidth(Math.max(2f, s * 0.2f));
        p.setStrokeCap(Paint.Cap.ROUND);
        p.setStrokeJoin(Paint.Join.ROUND);
        p.setStyle(Paint.Style.STROKE);
        switch (icon) {
            case ICON_GEAR:
                c.drawCircle(cx, cy, s * 0.36f, p);
                c.drawCircle(cx, cy, s * 0.78f, p);
                for (int i = 0; i < 8; i++) {
                    double a = Math.PI / 4 * i;
                    c.drawLine(cx + (float) Math.cos(a) * s * 0.8f, cy + (float) Math.sin(a) * s * 0.8f,
                        cx + (float) Math.cos(a) * s * 1.1f, cy + (float) Math.sin(a) * s * 1.1f, p);
                }
                break;
            case ICON_HELP:
                path.reset();
                path.moveTo(cx - s * 0.5f, cy - s * 0.45f);
                path.cubicTo(cx - s * 0.5f, cy - s * 1.1f, cx + s * 0.55f, cy - s * 1.1f, cx + s * 0.55f, cy - s * 0.4f);
                path.cubicTo(cx + s * 0.55f, cy + s * 0.05f, cx, cy, cx, cy + s * 0.35f);
                c.drawPath(path, p);
                p.setStyle(Paint.Style.FILL);
                c.drawCircle(cx, cy + s * 0.9f, p.getStrokeWidth() * 0.7f, p);
                break;
            case ICON_PAD:
                r.set(cx - s * 1.15f, cy - s * 0.55f, cx + s * 1.15f, cy + s * 0.6f);
                c.drawRoundRect(r, s * 0.55f, s * 0.55f, p);
                c.drawLine(cx - s * 0.75f, cy, cx - s * 0.35f, cy, p);
                c.drawLine(cx - s * 0.55f, cy - s * 0.2f, cx - s * 0.55f, cy + s * 0.2f, p);
                p.setStyle(Paint.Style.FILL);
                c.drawCircle(cx + s * 0.5f, cy - s * 0.12f, s * 0.12f, p);
                c.drawCircle(cx + s * 0.75f, cy + s * 0.13f, s * 0.12f, p);
                break;
            default:
                break;
        }
        p.setStyle(Paint.Style.FILL);
    }
}
