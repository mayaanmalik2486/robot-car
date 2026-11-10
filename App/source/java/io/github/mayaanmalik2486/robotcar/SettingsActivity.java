package io.github.mayaanmalik2486.robotcar;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.ColorStateList;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;

/**
 * The settings screen. Everything is saved the moment you change it.
 */
public class SettingsActivity extends Activity {

    private Settings s;
    private String padKey;
    private LinearLayout list;

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        getWindow().setBackgroundDrawable(new ColorDrawable(Ui.BG));
        s = new Settings(this);
        padKey = getIntent().getStringExtra("pad");
        build();
    }

    private void build() {
        int m = Ui.dp(this, 16);
        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(Ui.BG);
        LinearLayout outer = new LinearLayout(this);
        outer.setGravity(Gravity.CENTER_HORIZONTAL);
        list = new LinearLayout(this);
        list.setOrientation(LinearLayout.VERTICAL);
        list.setPadding(m, Ui.dp(this, 8), m, Ui.dp(this, 24));
        outer.addView(list, new LinearLayout.LayoutParams(Math.min(getResources().getDisplayMetrics().widthPixels, Ui.dp(this, 720)), -2));
        scroll.addView(outer);

        LinearLayout bar = new LinearLayout(this);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        PadButton back = new PadButton(this, "‹  Back", Ui.KEY, false);
        back.setOnClickListener(v -> finish());
        bar.addView(back, new LinearLayout.LayoutParams(Ui.dp(this, 96), Ui.dp(this, 44)));
        TextView title = text("Settings", 20, Ui.TEXT);
        title.setTypeface(Ui.BOLD);
        LinearLayout.LayoutParams tp = new LinearLayout.LayoutParams(-2, -2);
        tp.leftMargin = m;
        bar.addView(title, tp);
        list.addView(bar);

        section("DRIVING");
        boolean tilt = getIntent().getBooleanExtra("tilt", true);
        choice("On-screen controls", new String[]{"1 stick", "2 sticks", tilt ? "Tilt" : "Tilt (no sensor)"}, s.style(), i -> {
            if (i == Layout.TILT && !tilt)
                return false;
            s.setStyle(i);
            return true;
        });
        choice("Top speed", new String[]{"Fast", "Slow"}, s.slowTop() ? 1 : 0, i -> {
            s.setSlowTop(i == 1);
            return true;
        });
        slider("Steering feel", "Direct", "Gentle in the middle", Math.round(s.feel() * 100), 100, v -> s.setFeel(v / 100f), null);
        slider("On-screen stick dead zone", "0 %", "25 %", Math.round(s.touchDeadZone() * 100), 25, v -> s.setTouchDeadZone(v / 100f), "%");

        section("GAME CONTROLLER");
        String padName = getIntent().getStringExtra("padName");
        row(padName == null ? "No controller connected right now" : "🎮  " + padName, null);
        slider("Stick dead zone" + (padName == null ? "" : " (this controller)"), "0 %", "30 %",
            Math.round(s.padDeadZone(padKey) * 100), 30, v -> s.setPadDeadZone(padKey, v / 100f), "%");
        toggle("The left stick steers too", "when the right stick is in the middle", s.leftSteers(), s::setLeftSteers);
        toggle("Show the controller's buttons on the screen", "little A / B / RT ... badges on the buttons", s.overlay(), s::setOverlay);
        PadButton map = new PadButton(this, "Controller buttons and remap  ›", Ui.KEY, false);
        map.setOnClickListener(v -> startActivity(new Intent(this, ControllerActivity.class)));
        add(map, Ui.dp(this, 52));

        section("OTHER");
        toggle("Haptics", "a little click when you press a button", s.haptics(), on -> {
            s.setHaptics(on);
            Ui.haptics = on;
        });
        PadButton reset = new PadButton(this, "Reset everything to the defaults", Ui.KEY, false);
        reset.setTextColor(Ui.STOP);
        reset.setOnClickListener(v -> new AlertDialog.Builder(this)
            .setMessage("Put every setting and every controller button back as it came? (The car stays remembered.)")
            .setPositiveButton("Reset", (d, w) -> {
                s.reset();
                recreate();
            })
            .setNegativeButton("Cancel", null)
            .show());
        add(reset, Ui.dp(this, 52));
        String version = "?";
        try {
            version = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        } catch (PackageManager.NameNotFoundException e) {
            // (can't happen)
        }
        TextView about = text("Robot Car " + version + "  ·  github.com/mayaanmalik2486/robot-car", 12, Ui.FAINT);
        about.setGravity(Gravity.CENTER);
        about.setPadding(0, Ui.dp(this, 24), 0, 0);
        list.addView(about);
        setContentView(scroll);
    }

    // ------------------------------------------------------------------ the building blocks

    interface Pick {
        boolean pick(int i);
    }

    interface Change {
        void change(int v);
    }

    interface Flip {
        void flip(boolean on);
    }

    private TextView text(String t, float size, int color) {
        TextView v = new TextView(this);
        v.setText(t);
        v.setTextSize(size);
        v.setTextColor(color);
        v.setTypeface(Ui.MEDIUM);
        return v;
    }

    private void section(String name) {
        TextView t = text(name, 12, Ui.ACCENT);
        t.setTypeface(Ui.BOLD);
        t.setLetterSpacing(0.12f);
        t.setPadding(0, Ui.dp(this, 24), 0, Ui.dp(this, 8));
        list.addView(t);
    }

    private void add(View v, int h) {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(-1, h);
        p.topMargin = Ui.dp(this, 8);
        list.addView(v, p);
    }

    private LinearLayout card() {
        LinearLayout c = new LinearLayout(this);
        c.setOrientation(LinearLayout.VERTICAL);
        int p = Ui.dp(this, 16);
        c.setPadding(p, Ui.dp(this, 12), p, Ui.dp(this, 12));
        c.setBackground(Ui.rounded(Ui.PANEL, Ui.px(this, 12)));
        add(c, -2);
        return c;
    }

    private void row(String title, String sub) {
        LinearLayout c = card();
        c.addView(text(title, 15, Ui.TEXT));
        if (sub != null)
            c.addView(text(sub, 12, Ui.LABEL));
    }

    private void choice(String title, String[] options, int now, Pick pick) {
        LinearLayout c = card();
        c.addView(text(title, 15, Ui.TEXT));
        LinearLayout r = new LinearLayout(this);
        PadButton[] bs = new PadButton[options.length];
        for (int i = 0; i < options.length; i++) {
            final int k = i;
            bs[i] = new PadButton(this, options[i], Ui.KEY, false);
            bs[i].setOn(i == now);
            bs[i].setOnClickListener(v -> {
                if (!pick.pick(k))
                    return;
                for (int j = 0; j < bs.length; j++)
                    bs[j].setOn(j == k);
            });
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(0, Ui.dp(this, 48), 1f);
            if (i > 0)
                p.leftMargin = Ui.dp(this, 8);
            r.addView(bs[i], p);
        }
        LinearLayout.LayoutParams rp = new LinearLayout.LayoutParams(-1, -2);
        rp.topMargin = Ui.dp(this, 8);
        c.addView(r, rp);
    }

    private void slider(String title, String low, String high, int now, int max, Change change, String unit) {
        LinearLayout c = card();
        LinearLayout top = new LinearLayout(this);
        top.addView(text(title, 15, Ui.TEXT), new LinearLayout.LayoutParams(0, -2, 1f));
        TextView value = text(unit == null ? "" : now + " " + unit, 15, Ui.ACCENT);
        top.addView(value);
        c.addView(top);
        SeekBar bar = new SeekBar(this);
        bar.setMax(max);
        bar.setProgress(now);
        bar.setThumbTintList(ColorStateList.valueOf(Ui.ACCENT));
        bar.setProgressTintList(ColorStateList.valueOf(Ui.ACCENT));
        bar.setProgressBackgroundTintList(ColorStateList.valueOf(Ui.KEY_EDGE));
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar b, int v, boolean fromUser) {
                if (unit != null)
                    value.setText(v + " " + unit);
                if (fromUser)
                    change.change(v);
            }

            @Override
            public void onStartTrackingTouch(SeekBar b) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar b) {
            }
        });
        c.addView(bar, new LinearLayout.LayoutParams(-1, Ui.dp(this, 44)));
        LinearLayout ends = new LinearLayout(this);
        ends.addView(text(low, 12, Ui.LABEL), new LinearLayout.LayoutParams(0, -2, 1f));
        ends.addView(text(high, 12, Ui.LABEL));
        c.addView(ends);
    }

    private void toggle(String title, String sub, boolean now, Flip flip) {
        LinearLayout c = card();
        c.setOrientation(LinearLayout.HORIZONTAL);
        c.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout words = new LinearLayout(this);
        words.setOrientation(LinearLayout.VERTICAL);
        words.addView(text(title, 15, Ui.TEXT));
        words.addView(text(sub, 12, Ui.LABEL));
        c.addView(words, new LinearLayout.LayoutParams(0, -2, 1f));
        Switch sw = new Switch(this);
        sw.setChecked(now);
        int[][] states = {{android.R.attr.state_checked}, {}};
        if (Build.VERSION.SDK_INT >= 23) {          // (Android 5 has no switch colours: it keeps its own)
            sw.setThumbTintList(new ColorStateList(states, new int[]{Ui.ACCENT, Ui.LABEL}));
            sw.setTrackTintList(new ColorStateList(states, new int[]{Ui.alpha(Ui.ACCENT, 0.5f), Ui.KEY_EDGE}));
        }
        sw.setOnCheckedChangeListener((b, on) -> flip.flip(on));
        sw.setMinimumHeight(Ui.dp(this, 48));
        c.addView(sw);
        c.setOnClickListener(v -> sw.toggle());
    }
}
