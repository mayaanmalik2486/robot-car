package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.text.style.RelativeSizeSpan;

/**
 * The app's look: a dark graphite controller with one orange accent (the car's colour),
 * a green LCD for the car's screen, and red only for STOP.
 */
final class Ui {

    static final int BG = 0xFF0D0F13;          // the body
    static final int PANEL = 0xFF161A21;       // stick bases, cards
    static final int KEY = 0xFF262C36;         // a normal key
    static final int KEY_EDGE = 0xFF353D4A;
    static final int TEXT = 0xFFECEFF4;
    static final int LABEL = 0xFF8D96A7;
    static final int FAINT = 0xFF586070;
    static final int ACCENT = 0xFFFF7A1A;      // the one accent colour (the orange car)
    static final int ACCENT_KEY = 0xFF5A2E10;  // a key that is switched on
    static final int STOP = 0xFFD93C42;

    static final int LCD = 0xFF07120B;         // the car's green screen
    static final int LCD_EDGE = 0xFF2A313B;
    static final int LCD_TEXT = 0xFF8CF5A8;
    static final int LCD_DIM = 0xFF3D8A55;
    static final int LCD_WARN = 0xFFFFC857;

    // the controller's own button colours (only on the little letters, so you find them on the pad)
    static final int PAD_A = 0xFF5BD46A;
    static final int PAD_B = 0xFFFF6464;
    static final int PAD_X = 0xFF52A6FF;
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
        return Color.argb(Math.round(255 * a), Color.red(c), Color.green(c), Color.blue(c));
    }

    static GradientDrawable rounded(int color, float radiusPx) {
        GradientDrawable g = new GradientDrawable();
        g.setColor(color);
        g.setCornerRadius(radiusPx);
        return g;
    }

    static GradientDrawable outlined(int fill, int stroke, int strokePx, float radiusPx) {
        GradientDrawable g = rounded(fill, radiusPx);
        g.setStroke(strokePx, stroke);
        return g;
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
        if (small == null || small.isEmpty())
            return big;
        SpannableString s = new SpannableString(big + "\n" + small);
        s.setSpan(new RelativeSizeSpan(0.62f), big.length() + 1, s.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        s.setSpan(new ForegroundColorSpan(LABEL), big.length() + 1, s.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        return s;
    }

    private Ui() {
    }
}
