package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.text.style.RelativeSizeSpan;
import android.text.style.TypefaceSpan;

/**
 * The app's look: a deep night-blue sky with slow bubbles, glossy 3D "bubble" buttons in
 * bright colours, glass keys, an orange accent (the car's colour) with a cyan glow for the
 * live things, and big condensed letters.
 */
final class Ui {

    static final int BG = 0xFF070B1A;          // the night sky
    static final int BG2 = 0xFF0E1638;
    static final int PANEL = 0xFF141C42;       // cards
    static final int KEY = 0xFF222C63;         // a glass key
    static final int KEY_HI = 0xFF34418A;
    static final int KEY_EDGE = 0xFF4A59A8;
    static final int KEY_BASE = 0xFF050817;    // what a key stands on
    static final int TEXT = 0xFFF4F6FF;
    static final int LABEL = 0xFF9AA6D0;
    static final int FAINT = 0xFF5B678F;
    static final int ACCENT = 0xFFFF7A1A;      // the orange car
    static final int ACCENT2 = 0xFFFFB52E;
    static final int CYAN = 0xFF2EE6D6;        // live things: the grid, the radar
    static final int STOP = 0xFFFF3B5C;
    static final int GOOD = 0xFF3BE38B;
    static final int BAD = 0xFFFF6B81;
    static final int WARN = 0xFFFFC857;

    // hues (0 ... 360) of the coloured bubbles
    static final int HUE_ORANGE = 24, HUE_STOP = 350, HUE_PINK = 325, HUE_BLUE = 215, HUE_YELLOW = 46, HUE_TEAL = 172;

    // the controller's own button colours (only on the little letters, so you find them on the pad)
    static final int PAD_A = 0xFF5BE37D;
    static final int PAD_B = 0xFFFF6B81;
    static final int PAD_X = 0xFF5AA8FF;
    static final int PAD_Y = 0xFFFFD23F;

    static final Typeface BOLD = Typeface.create("sans-serif-condensed", Typeface.BOLD);
    static final Typeface MEDIUM = Typeface.create("sans-serif-medium", Typeface.NORMAL);
    static final Typeface MONO = Typeface.create(Typeface.MONOSPACE, Typeface.BOLD);

    /** The haptics setting (every button click reads it). */
    static boolean haptics = true;

    static int dp(Context c, float v) {
        return Math.round(v * c.getResources().getDisplayMetrics().density);
    }

    static float px(Context c, float dp) {
        return dp * c.getResources().getDisplayMetrics().density;
    }

    static int darker(int c, float f) {
        return Color.argb(Color.alpha(c), (int) (Color.red(c) * f), (int) (Color.green(c) * f), (int) (Color.blue(c) * f));
    }

    static int lighter(int c, float f) {
        return Color.argb(Color.alpha(c), Color.red(c) + (int) ((255 - Color.red(c)) * f), Color.green(c) + (int) ((255 - Color.green(c)) * f),
            Color.blue(c) + (int) ((255 - Color.blue(c)) * f));
    }

    static int alpha(int c, float a) {
        return Color.argb(Math.round(255 * Math.max(0, Math.min(1, a))), Color.red(c), Color.green(c), Color.blue(c));
    }

    /** A colour from its hue (0 ... 360), how strong (0 ... 1) and how light (0 ... 1) it is. */
    static int hsl(float h, float s, float l) {
        float c = (1 - Math.abs(2 * l - 1)) * s;
        float hp = ((h % 360) + 360) % 360 / 60f;
        float x = c * (1 - Math.abs(hp % 2 - 1));
        float r = 0, g = 0, b = 0;
        switch ((int) hp) {
            case 0:  r = c; g = x; break;
            case 1:  r = x; g = c; break;
            case 2:  g = c; b = x; break;
            case 3:  g = x; b = c; break;
            case 4:  r = x; b = c; break;
            default: r = c; b = x; break;
        }
        float m = l - c / 2;
        return Color.rgb(Math.round((r + m) * 255), Math.round((g + m) * 255), Math.round((b + m) * 255));
    }

    static GradientDrawable rounded(int color, float radiusPx) {
        GradientDrawable g = new GradientDrawable();
        g.setColor(color);
        g.setCornerRadius(radiusPx);
        return g;
    }

    /** The night-sky background of a whole screen. */
    static GradientDrawable sky() {
        return new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM, new int[]{BG, BG2});
    }

    /** The colour of a controller button's letter (A green, B red, X blue, Y yellow, others grey). */
    static int padColor(String label) {
        if (label == null)
            return LABEL;
        switch (label) {
            case "A": return PAD_A;
            case "B": return PAD_B;
            case "X": return PAD_X;
            case "Y": return PAD_Y;
            default:  return LABEL;
        }
    }

    /** A word in big letters with a small line under it (in a dimmer colour). */
    static CharSequence twoLines(String big, String small) {
        return twoLines(big, small, LABEL);
    }

    static CharSequence twoLines(String big, String small, int smallColor) {
        if (small == null || small.isEmpty())
            return big;
        SpannableString s = new SpannableString(big + "\n" + small);
        s.setSpan(new RelativeSizeSpan(0.6f), big.length() + 1, s.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        s.setSpan(new ForegroundColorSpan(smallColor), big.length() + 1, s.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        s.setSpan(new TypefaceSpan("monospace"), big.length() + 1, s.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        return s;
    }

    private Ui() {
    }
}
