package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.RelativeSizeSpan;

/** The app's colours and a few small drawing helpers. */
final class Ui {

    // a matte grey controller with a green LCD
    static final int BODY = Color.rgb(28, 31, 37);
    static final int PANEL = Color.rgb(38, 42, 50);
    static final int KEY = Color.rgb(62, 68, 80);
    static final int KEY_ON = Color.rgb(54, 128, 84);
    static final int LCD = Color.rgb(12, 24, 16);
    static final int LCD_EDGE = Color.rgb(60, 66, 76);
    static final int LCD_TEXT = Color.rgb(124, 230, 150);
    static final int LCD_DIM = Color.rgb(66, 124, 82);
    static final int LABEL = Color.rgb(150, 156, 168);

    static final int GREEN = Color.rgb(52, 150, 88);     // the four face buttons, like a real controller
    static final int RED = Color.rgb(196, 70, 70);
    static final int BLUE = Color.rgb(58, 112, 196);
    static final int YELLOW = Color.rgb(214, 168, 36);
    static final int STOP_RED = Color.rgb(178, 46, 46);

    static int dp(Context c, float v) {
        return Math.round(v * c.getResources().getDisplayMetrics().density);
    }

    static int darker(int c, float f) {
        return Color.rgb((int) (Color.red(c) * f), (int) (Color.green(c) * f), (int) (Color.blue(c) * f));
    }

    static int lighter(int c, float f) {
        return Color.rgb(Color.red(c) + (int) ((255 - Color.red(c)) * f), Color.green(c) + (int) ((255 - Color.green(c)) * f),
            Color.blue(c) + (int) ((255 - Color.blue(c)) * f));
    }

    static GradientDrawable rounded(int color, float radiusPx) {
        GradientDrawable g = new GradientDrawable();
        g.setColor(color);
        g.setCornerRadius(radiusPx);
        return g;
    }

    /** A big emoji with a small word under it. */
    static CharSequence emojiLabel(String emoji, String word) {
        if (word == null || word.isEmpty())
            return emoji;
        SpannableString s = new SpannableString(emoji + "\n" + word);
        s.setSpan(new RelativeSizeSpan(0.5f), emoji.length() + 1, s.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        return s;
    }

    private Ui() {
    }
}
