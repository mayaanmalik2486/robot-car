package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.StateListDrawable;
import android.view.Gravity;
import android.view.HapticFeedbackConstants;
import android.widget.TextView;

/**
 * A chunky button that looks like a real one: a raised top on a darker base. Pressed, the top
 * sinks down onto the base, the label goes down with it, the button shrinks a little and the
 * phone gives a small click.
 */
class PadButton extends TextView {

    private final boolean round;
    private final int lip;                     // how far the top stands up from the base
    private int color;

    PadButton(Context c, CharSequence label, int color, boolean round) {
        super(c);
        this.round = round;
        this.lip = Ui.dp(c, round ? 5 : 4);
        setText(label);
        setTextColor(Color.WHITE);
        setTypeface(Typeface.DEFAULT_BOLD);
        setTextSize(round ? 22 : 15);
        setGravity(Gravity.CENTER);
        setIncludeFontPadding(false);
        setClickable(true);
        setHapticFeedbackEnabled(true);
        setColor(color);
        showPressed(false);
    }

    int color() {
        return color;
    }

    void setColor(int color) {
        this.color = color;
        StateListDrawable bg = new StateListDrawable();
        bg.addState(new int[]{android.R.attr.state_pressed}, layers(true));
        bg.addState(new int[]{}, layers(false));
        setBackground(bg);
    }

    /** Shows the button pressed for a moment (when a game controller pressed it). */
    void flash() {
        setPressed(true);
        postDelayed(() -> setPressed(false), 140);
    }

    @Override
    public void setPressed(boolean pressed) {
        if (pressed && !isPressed())
            performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
        super.setPressed(pressed);
        showPressed(pressed);
    }

    private void showPressed(boolean pressed) {
        int down = pressed ? lip - Ui.dp(getContext(), 1) : 0;
        setPadding(Ui.dp(getContext(), 2), down, Ui.dp(getContext(), 2), lip - down);
        float scale = pressed ? 0.95f : 1f;
        animate().scaleX(scale).scaleY(scale).setDuration(70).start();
        setAlpha(isEnabled() ? 1f : 0.45f);
    }

    private Drawable layers(boolean pressed) {
        float r = round ? 1000 : Ui.dp(getContext(), 12);
        GradientDrawable base = shape(Ui.darker(color, 0.55f), r);
        GradientDrawable top = new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM,
            new int[]{Ui.lighter(color, pressed ? 0.04f : 0.14f), pressed ? Ui.darker(color, 0.9f) : color});
        top.setShape(round ? GradientDrawable.OVAL : GradientDrawable.RECTANGLE);
        if (!round)
            top.setCornerRadius(r);
        top.setStroke(Ui.dp(getContext(), 1), Ui.lighter(color, pressed ? 0.08f : 0.22f));
        LayerDrawable d = new LayerDrawable(new Drawable[]{base, top});
        int sink = pressed ? lip - Ui.dp(getContext(), 1) : 0;
        d.setLayerInset(1, 0, sink, 0, lip - sink);
        return d;
    }

    private GradientDrawable shape(int c, float r) {
        GradientDrawable g = new GradientDrawable();
        g.setShape(round ? GradientDrawable.OVAL : GradientDrawable.RECTANGLE);
        if (!round)
            g.setCornerRadius(r);
        g.setColor(c);
        return g;
    }
}
