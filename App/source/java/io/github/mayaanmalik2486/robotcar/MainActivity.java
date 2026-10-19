package io.github.mayaanmalik2486.robotcar;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.provider.Settings;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.RelativeSizeSpan;
import android.view.Gravity;
import android.view.HapticFeedbackConstants;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Robot Car - a game-pad style remote for the car's HC-05 Bluetooth module.
 *
 * Left: a thumbstick (drive). Right: four face buttons. Middle: a little screen that
 * shows what the car is doing, and the mode buttons. A real game controller (for example
 * a GameSir) connected to the phone works too.
 *
 * Every button sends one letter - the letters the car's program understands. The car answers
 * with "~MODE|1010": what it is doing, then 1/0 for muted, football, slow, low battery.
 */
public class MainActivity extends Activity {

    private static final UUID SERIAL_PORT = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private static final int ASK_PERMISSION = 1;
    private static final int ASK_BLUETOOTH_ON = 2;
    private static final String GITHUB = "https://github.com/mayaanmalik2486/robot-car";
    private static final String APP_LINK = GITHUB + "/raw/main/App/RobotCar.apk";

    // colours: a matte grey controller with a green LCD
    private static final int BODY = Color.rgb(30, 33, 39);
    private static final int PANEL = Color.rgb(40, 44, 52);
    private static final int KEY = Color.rgb(52, 57, 66);
    private static final int KEY_ON = Color.rgb(70, 120, 85);
    private static final int LCD = Color.rgb(14, 26, 18);
    private static final int LCD_TEXT = Color.rgb(124, 230, 150);
    private static final int LCD_DIM = Color.rgb(70, 130, 85);
    private static final int BTN_A = Color.rgb(60, 141, 90);
    private static final int BTN_B = Color.rgb(181, 72, 72);
    private static final int BTN_X = Color.rgb(63, 111, 181);
    private static final int BTN_Y = Color.rgb(201, 162, 39);
    private static final int STOP_RED = Color.rgb(170, 50, 50);

    private BluetoothAdapter adapter;
    private BluetoothSocket socket;
    private OutputStream out;
    private volatile boolean connected;
    private volatile boolean connecting;
    private volatile boolean closingOnPurpose;
    private final ExecutorService sender = Executors.newSingleThreadExecutor();
    private final Handler ui = new Handler(Looper.getMainLooper());
    private SharedPreferences prefs;

    private Stick stick;
    private TextView lcdConn;
    private TextView lcdMode;
    private TextView lcdFlags;
    private Button connectButton;
    private Button speedButton;
    private Button footballButton;
    private Button soundButton;

    // what the car told us
    private String mode = "";
    private boolean muted;
    private boolean football;
    private boolean slow;
    private boolean batteryLow;

    private char driving = 'S';            // the drive letter being held right now
    private long lastWarnAt;
    private boolean leftTriggerDown;
    private boolean rightTriggerDown;

    // ------------------------------------------------------------------ start / stop

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        prefs = getSharedPreferences("robotcar", MODE_PRIVATE);
        adapter = BluetoothAdapter.getDefaultAdapter();
        setContentView(buildScreen());
        showStatus();

        if (adapter == null) {
            lcdConn.setText("NO BLUETOOTH ON THIS PHONE");
            return;
        }
        String last = prefs.getString("device", null);
        if (last != null && hasPermission() && adapter.isEnabled())
            connect(last);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)                          // full screen, like a game
            getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
    }

    @Override
    protected void onPause() {
        super.onPause();
        drive('S');                            // never leave the car driving when the app goes away
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        closingOnPurpose = true;
        closeQuietly(socket);
        sender.shutdownNow();
    }

    // ------------------------------------------------------------------ the screen

    private View buildScreen() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.HORIZONTAL);
        root.setBackgroundColor(BODY);
        root.setPadding(dp(14), dp(10), dp(14), dp(10));

        // LEFT: speed shoulder button + thumbstick
        LinearLayout left = new LinearLayout(this);
        left.setOrientation(LinearLayout.VERTICAL);
        speedButton = keyButton("", 0);
        speedButton.setOnClickListener(v -> { haptic(v); send(slow ? 'W' : 'Q'); });
        left.addView(speedButton, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(44)));
        stick = new Stick(this);
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f);
        sp.topMargin = dp(8);
        left.addView(stick, sp);
        root.addView(left, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 0.30f));

        // MIDDLE: screen + mode buttons
        LinearLayout mid = new LinearLayout(this);
        mid.setOrientation(LinearLayout.VERTICAL);
        mid.setPadding(dp(12), 0, dp(12), 0);

        LinearLayout bar = new LinearLayout(this);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        TextView name = new TextView(this);
        name.setText("ROBOT CAR");
        name.setTextColor(Color.rgb(150, 156, 168));
        name.setTypeface(Typeface.create(Typeface.MONOSPACE, Typeface.BOLD));
        name.setTextSize(13);
        name.setLetterSpacing(0.15f);
        bar.addView(name, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        connectButton = keyButton("🔗", 0);
        connectButton.setOnClickListener(v -> onConnectPressed());
        bar.addView(connectButton, new LinearLayout.LayoutParams(dp(52), dp(36)));
        Button help = keyButton("❔", 0);
        help.setOnClickListener(v -> showHelp());
        LinearLayout.LayoutParams hp = new LinearLayout.LayoutParams(dp(44), dp(36));
        hp.leftMargin = dp(6);
        bar.addView(help, hp);
        mid.addView(bar);

        LinearLayout lcd = new LinearLayout(this);
        lcd.setOrientation(LinearLayout.VERTICAL);
        lcd.setBackground(rounded(LCD, 10, Color.rgb(60, 66, 76)));
        lcd.setPadding(dp(10), dp(6), dp(10), dp(6));
        lcd.setOnClickListener(v -> onConnectPressed());
        lcdConn = lcdText(11, LCD_DIM);
        lcdMode = lcdText(24, LCD_TEXT);
        lcdFlags = lcdText(13, LCD_TEXT);
        lcd.addView(lcdConn);
        lcd.addView(lcdMode);
        lcd.addView(lcdFlags);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(6);
        mid.addView(lcd, lp);

        GridLayout grid = new GridLayout(this);
        grid.setColumnCount(5);
        String[][] modes = {
            {"🔍", "search", "I"}, {"😈", "chaos", "1"}, {"🍩", "donut", "3"},
            {"↩️", "180°", "U"}, {"🅿️", "brake", "u"},
            {"⏫", "auto", "2"}, {"⏬", "reverse", "J"}, {"🚶", "follow", "M"},
            {"🥅", "keeper", "K"}, {"🚨", "guard", "A"},
            {"🎵", "music", "N"}, {"✋", "gesture", "E"}, {"🐶", "pet", "P"},
        };
        for (String[] m : modes) {
            Button b = keyButton(emojiLabel(m[0], m[1]), 0);
            final char letter = m[2].charAt(0);
            final String label = m[1];
            b.setOnClickListener(v -> { haptic(v); send(letter); });
            b.setOnLongClickListener(v -> { toast(label); return true; });
            grid.addView(b, cell());
        }
        footballButton = keyButton("", 0);
        footballButton.setOnClickListener(v -> { haptic(v); send('V'); });
        grid.addView(footballButton, cell());
        soundButton = keyButton("", 0);
        soundButton.setOnClickListener(v -> { haptic(v); send('X'); });
        grid.addView(soundButton, cell());

        ScrollView scroll = new ScrollView(this);
        scroll.addView(grid);
        LinearLayout.LayoutParams gp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f);
        gp.topMargin = dp(6);
        mid.addView(scroll, gp);
        root.addView(mid, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 0.40f));

        // RIGHT: STOP shoulder button + face buttons
        LinearLayout right = new LinearLayout(this);
        right.setOrientation(LinearLayout.VERTICAL);
        Button stop = keyButton("🛑  STOP", STOP_RED);
        stop.setOnClickListener(v -> { haptic(v); drive('S'); send('S'); });
        right.addView(stop, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(44)));

        LinearLayout face = new LinearLayout(this);
        face.setOrientation(LinearLayout.VERTICAL);
        face.setGravity(Gravity.CENTER);
        face.addView(faceRow(null, faceButton("💃", "dance", '4', BTN_Y), null));
        face.addView(faceRow(faceButton("⚽", "kick", 'k', BTN_X), null, faceButton("📯", "horn", 'Y', BTN_B)));
        face.addView(faceRow(null, faceButton("🚀", "turbo", 'T', BTN_A), null));
        LinearLayout.LayoutParams fp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f);
        fp.topMargin = dp(8);
        right.addView(face, fp);
        root.addView(right, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 0.30f));

        return root;
    }

    private LinearLayout faceRow(View a, View b, View c) {
        LinearLayout r = new LinearLayout(this);
        r.setGravity(Gravity.CENTER);
        for (View v : new View[]{a, b, c}) {
            View cellView = v != null ? v : new View(this);
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(dp(68), dp(68));
            p.setMargins(dp(4), dp(1), dp(4), dp(1));
            r.addView(cellView, p);
        }
        return r;
    }

    private Button faceButton(String emoji, String label, final char letter, int color) {
        Button b = new Button(this);
        b.setText(emojiLabel(emoji, label));
        b.setTextColor(Color.WHITE);
        b.setTextSize(22);
        b.setAllCaps(false);
        b.setPadding(0, 0, 0, 0);
        b.setStateListAnimator(null);
        StateListDrawable bg = new StateListDrawable();
        bg.addState(new int[]{android.R.attr.state_pressed}, oval(darker(color)));
        bg.addState(new int[]{}, oval(color));
        b.setBackground(bg);
        b.setOnClickListener(v -> { haptic(v); send(letter); });
        return b;
    }

    private Button keyButton(CharSequence text, int color) {
        Button b = new Button(this);
        b.setText(text);
        b.setTextColor(Color.WHITE);
        b.setTextSize(16);
        b.setAllCaps(false);
        b.setTypeface(Typeface.DEFAULT_BOLD);
        b.setPadding(dp(2), dp(2), dp(2), dp(2));
        b.setStateListAnimator(null);
        paintKey(b, color == 0 ? KEY : color);
        return b;
    }

    private void paintKey(Button b, int color) {
        StateListDrawable bg = new StateListDrawable();
        bg.addState(new int[]{android.R.attr.state_pressed}, rounded(darker(color), 10, 0));
        bg.addState(new int[]{}, rounded(color, 10, 0));
        b.setBackground(bg);
    }

    private GridLayout.LayoutParams cell() {
        GridLayout.LayoutParams p = new GridLayout.LayoutParams(GridLayout.spec(GridLayout.UNDEFINED), GridLayout.spec(GridLayout.UNDEFINED, 1f));
        p.width = 0;
        p.height = dp(54);
        p.setMargins(dp(3), dp(3), dp(3), dp(3));
        return p;
    }

    // a big emoji with a small word under it
    private CharSequence emojiLabel(String emoji, String word) {
        SpannableString s = new SpannableString(emoji + "\n" + word);
        s.setSpan(new RelativeSizeSpan(0.5f), emoji.length() + 1, s.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        return s;
    }

    private TextView lcdText(float size, int color) {
        TextView t = new TextView(this);
        t.setTextSize(size);
        t.setTextColor(color);
        t.setTypeface(Typeface.MONOSPACE);
        t.setSingleLine(true);
        return t;
    }

    private GradientDrawable rounded(int color, float radiusDp, int stroke) {
        GradientDrawable g = new GradientDrawable();
        g.setColor(color);
        g.setCornerRadius(dp(radiusDp));
        if (stroke != 0)
            g.setStroke(dp(2), stroke);
        return g;
    }

    private GradientDrawable oval(int color) {
        GradientDrawable g = new GradientDrawable();
        g.setShape(GradientDrawable.OVAL);
        g.setColor(color);
        g.setStroke(dp(3), darker(color));
        return g;
    }

    private static int darker(int c) {
        return Color.rgb(Color.red(c) * 7 / 10, Color.green(c) * 7 / 10, Color.blue(c) * 7 / 10);
    }

    private int dp(float v) {
        return Math.round(v * getResources().getDisplayMetrics().density);
    }

    // the little screen + the switch buttons show what the car told us
    private void showStatus() {
        if (!connected) {
            lcdConn.setText(connecting ? "CONNECTING..." : "○ NOT CONNECTED - TAP 🔗");
            lcdMode.setText("--");
            lcdFlags.setText("");
        } else {
            lcdConn.setText("● CONNECTED");
            lcdMode.setText(mode.isEmpty() ? "..." : mode);
            lcdFlags.setText((muted ? "🔇 " : "🔊 ") + (football ? "⚽ " : "📡 ")
                + (slow ? "🐢 " : "🐇 ") + (batteryLow ? "🪫 LOW BATTERY" : "🔋"));
        }
        speedButton.setText(slow ? "🐢  slow" : "🐇  fast");
        footballButton.setText(emojiLabel("⚽", football ? "sensor off" : "football"));
        paintKey(footballButton, football ? KEY_ON : KEY);
        soundButton.setText(emojiLabel(muted ? "🔇" : "🔊", muted ? "muted" : "sound"));
        paintKey(soundButton, muted ? KEY_ON : KEY);
        connectButton.setText(connected || connecting ? "⏏" : "🔗");
    }

    private void showHelp() {
        new AlertDialog.Builder(this)
            .setTitle("Robot Car")
            .setMessage(
                "🕹 Stick: drive. Up = forward, diagonals = curve, sides = spin, down = back. Let go = stop.\n"
                + "🐇/🐢 top left: fast / slow.   🛑 top right: stop everything.\n\n"
                + "💃 dance - press on the first beat, again ON a beat to line it up\n"
                + "⚽ kick   📯 horn   🚀 turbo\n\n"
                + "🔍 search  😈 chaos  🍩 donut  ↩️ 180° turn  🅿️ brake  ⏫/⏬ auto\n"
                + "🚶 follow me  🥅 goalkeeper  🚨 guard  🎵 music (wave your hand)  ✋ gestures  🐶 pet\n"
                + "⚽ football = distance sensor off.  🔊 sound on/off.\n"
                + "Hold a button to see its name.\n\n"
                + "🎮 GAME CONTROLLER (pair it with this phone first):\n"
                + "left stick / D-pad = drive   A = turbo   B = horn   X = kick   Y = dance\n"
                + "LB = fast/slow   RB = stop   LT = brake   RT = turbo\n"
                + "START = search   SELECT = football   left stick click = sound\n\n"
                + "First time: pair HC-05 in the phone's Bluetooth settings (PIN 1234 or 0000), then tap 🔗.")
            .setPositiveButton("OK", null)
            .setNeutralButton("Share app", (d, w) -> shareApp())
            .show();
    }

    private void alert(String s, boolean urgent) {
        new AlertDialog.Builder(this).setMessage(s).setPositiveButton("OK", null).show();
        buzz(urgent ? new long[]{0, 400, 150, 400, 150, 800} : new long[]{0, 300});
    }

    private void buzz(long[] pattern) {
        Vibrator v = (Vibrator) getSystemService(VIBRATOR_SERVICE);
        if (v == null)
            return;
        if (Build.VERSION.SDK_INT >= 26)
            v.vibrate(VibrationEffect.createWaveform(pattern, -1));
        else
            v.vibrate(pattern, -1);
    }

    private void haptic(View v) {
        v.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
    }

    private void toast(String s) {
        Toast.makeText(this, s, Toast.LENGTH_SHORT).show();
    }

    private void shareApp() {
        Intent i = new Intent(Intent.ACTION_SEND);
        i.setType("text/plain");
        i.putExtra(Intent.EXTRA_TEXT, "Robot Car app (Android): " + APP_LINK);
        startActivity(Intent.createChooser(i, "Share the Robot Car app"));
    }

    // ------------------------------------------------------------------ driving

    // which drive letter a stick position means (x, y from -1 to 1, y down = positive)
    static char direction(float x, float y) {
        if (Math.hypot(x, y) < 0.35)
            return 'S';
        double a = Math.toDegrees(Math.atan2(-y, x));      // 0 = right, 90 = up
        if (a >= 67.5 && a < 112.5)
            return 'F';
        if (a >= 112.5 && a < 157.5)
            return 'G';                                    // forward-left: curve left
        if (a >= 22.5 && a < 67.5)
            return 'H';                                    // forward-right: curve right
        if (a >= -22.5 && a < 22.5)
            return 'R';
        if (a >= 157.5 || a < -157.5)
            return 'L';
        return 'B';
    }

    // send the drive letter only when it changes; S (twice) when the stick is let go
    private void drive(char c) {
        if (c == driving)
            return;
        driving = c;
        if (c == 'S') {
            sendQuietly('S');
            ui.postDelayed(() -> sendQuietly('S'), 60);
        } else {
            if (stick != null)
                stick.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
            send(c);
        }
    }

    // the on-screen thumbstick
    private class Stick extends View {
        private float kx, ky;                              // knob position, -1 .. 1
        private final Paint base = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint ring = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint knob = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint knobEdge = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint mark = new Paint(Paint.ANTI_ALIAS_FLAG);

        Stick(Context c) {
            super(c);
            base.setColor(PANEL);
            ring.setColor(Color.rgb(64, 70, 80));
            ring.setStyle(Paint.Style.STROKE);
            ring.setStrokeWidth(dp(3));
            knob.setColor(Color.rgb(78, 84, 96));
            knobEdge.setColor(Color.rgb(44, 48, 56));
            knobEdge.setStyle(Paint.Style.STROKE);
            knobEdge.setStrokeWidth(dp(4));
            mark.setColor(Color.rgb(110, 118, 132));
            mark.setTextSize(dp(14));
            mark.setTextAlign(Paint.Align.CENTER);
        }

        void show(float x, float y) {
            kx = x;
            ky = y;
            invalidate();
        }

        @Override
        protected void onDraw(Canvas c) {
            float cx = getWidth() / 2f, cy = getHeight() / 2f;
            float r = Math.min(cx, cy) * 0.94f;
            float kr = r * 0.40f;
            c.drawCircle(cx, cy, r, base);
            c.drawCircle(cx, cy, r, ring);
            float m = r - dp(12);
            c.drawText("▲", cx, cy - m + dp(5), mark);
            c.drawText("▼", cx, cy + m + dp(5), mark);
            c.drawText("◀", cx - m, cy + dp(5), mark);
            c.drawText("▶", cx + m, cy + dp(5), mark);
            float px = cx + kx * (r - kr), py = cy + ky * (r - kr);
            c.drawCircle(px, py, kr, knob);
            c.drawCircle(px, py, kr, knobEdge);
            c.drawCircle(px, py, kr * 0.55f, knobEdge);
        }

        @Override
        public boolean onTouchEvent(MotionEvent e) {
            float cx = getWidth() / 2f, cy = getHeight() / 2f;
            float r = Math.min(cx, cy) * 0.94f * 0.6f;
            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_MOVE: {
                    float x = (e.getX() - cx) / r, y = (e.getY() - cy) / r;
                    float len = (float) Math.hypot(x, y);
                    if (len > 1) {
                        x /= len;
                        y /= len;
                    }
                    show(x, y);
                    drive(direction(x, y));
                    return true;
                }
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    show(0, 0);
                    drive('S');
                    return true;
            }
            return true;
        }
    }

    // ------------------------------------------------------------------ a real game controller

    private static boolean fromGamepad(InputDevice d, int source) {
        return (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
            || (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
            || (source & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD && d != null && !d.isVirtual();
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent e) {
        if ((e.getSource() & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK || e.getAction() != MotionEvent.ACTION_MOVE)
            return super.onGenericMotionEvent(e);

        float x = e.getAxisValue(MotionEvent.AXIS_X), y = e.getAxisValue(MotionEvent.AXIS_Y);
        float hx = e.getAxisValue(MotionEvent.AXIS_HAT_X), hy = e.getAxisValue(MotionEvent.AXIS_HAT_Y);
        if (Math.abs(hx) > 0.5f || Math.abs(hy) > 0.5f) {    // the D-pad
            x = hx;
            y = hy;
        }
        stick.show(Math.max(-1, Math.min(1, x)), Math.max(-1, Math.min(1, y)));
        drive(direction(x, y));

        float lt = Math.max(e.getAxisValue(MotionEvent.AXIS_LTRIGGER), e.getAxisValue(MotionEvent.AXIS_BRAKE));
        float rt = Math.max(e.getAxisValue(MotionEvent.AXIS_RTRIGGER), e.getAxisValue(MotionEvent.AXIS_GAS));
        if (lt > 0.6f && !leftTriggerDown)
            send('u');                                     // LT = brake
        if (rt > 0.6f && !rightTriggerDown)
            send('T');                                     // RT = turbo
        leftTriggerDown = lt > 0.6f;
        rightTriggerDown = rt > 0.6f;
        return true;
    }

    private int dpadX, dpadY;                              // D-pad buttons (controllers that send them as keys)

    @Override
    public boolean onKeyDown(int code, KeyEvent e) {
        if (!fromGamepad(e.getDevice(), e.getSource()))
            return super.onKeyDown(code, e);
        if (e.getRepeatCount() > 0)
            return true;
        switch (code) {
            case KeyEvent.KEYCODE_DPAD_UP:    dpadY = -1; break;
            case KeyEvent.KEYCODE_DPAD_DOWN:  dpadY = 1;  break;
            case KeyEvent.KEYCODE_DPAD_LEFT:  dpadX = -1; break;
            case KeyEvent.KEYCODE_DPAD_RIGHT: dpadX = 1;  break;
            case KeyEvent.KEYCODE_BUTTON_A:      send('T'); return true;               // turbo
            case KeyEvent.KEYCODE_BUTTON_B:      send('Y'); return true;               // horn
            case KeyEvent.KEYCODE_BUTTON_X:      send('k'); return true;               // kick
            case KeyEvent.KEYCODE_BUTTON_Y:      send('4'); return true;               // dance
            case KeyEvent.KEYCODE_BUTTON_L1:     send(slow ? 'W' : 'Q'); return true;  // fast / slow
            case KeyEvent.KEYCODE_BUTTON_R1:     drive('S'); send('S'); return true;   // stop
            case KeyEvent.KEYCODE_BUTTON_START:  send('I'); return true;               // search
            case KeyEvent.KEYCODE_BUTTON_SELECT: send('V'); return true;               // football
            case KeyEvent.KEYCODE_BUTTON_THUMBL: send('X'); return true;               // sound
            default: return true;
        }
        stick.show(dpadX, dpadY);
        drive(direction(dpadX, dpadY));
        return true;
    }

    @Override
    public boolean onKeyUp(int code, KeyEvent e) {
        if (!fromGamepad(e.getDevice(), e.getSource()))
            return super.onKeyUp(code, e);
        switch (code) {
            case KeyEvent.KEYCODE_DPAD_UP:
            case KeyEvent.KEYCODE_DPAD_DOWN:  dpadY = 0; break;
            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_DPAD_RIGHT: dpadX = 0; break;
            default: return true;
        }
        stick.show(dpadX, dpadY);
        drive(direction(dpadX, dpadY));
        return true;
    }

    // ------------------------------------------------------------------ Bluetooth

    private boolean hasPermission() {
        return Build.VERSION.SDK_INT < 31
            || checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED;
    }

    private void onConnectPressed() {
        if (adapter == null) {
            toast("This phone has no Bluetooth");
            return;
        }
        if (connected || connecting) {
            disconnect();
            return;
        }
        if (!hasPermission()) {
            requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT}, ASK_PERMISSION);
            return;
        }
        if (!adapter.isEnabled()) {
            try {
                startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), ASK_BLUETOOTH_ON);
            } catch (Exception e) {
                toast("Please switch Bluetooth on");
            }
            return;
        }
        chooseCar();
    }

    @Override
    public void onRequestPermissionsResult(int code, String[] permissions, int[] results) {
        if (code != ASK_PERMISSION)
            return;
        if (results.length > 0 && results[0] == PackageManager.PERMISSION_GRANTED)
            onConnectPressed();
        else
            toast("The app needs the Bluetooth permission to talk to the car");
    }

    @Override
    protected void onActivityResult(int code, int result, Intent data) {
        if (code == ASK_BLUETOOTH_ON && adapter != null && adapter.isEnabled())
            chooseCar();
    }

    private void chooseCar() {
        final List<BluetoothDevice> devices;
        try {
            devices = new ArrayList<>(adapter.getBondedDevices());
        } catch (SecurityException e) {
            toast("The app needs the Bluetooth permission");
            return;
        }
        if (devices.isEmpty()) {
            new AlertDialog.Builder(this)
                .setTitle("No paired devices")
                .setMessage("Pair the car first in your phone's Bluetooth settings: it's called HC-05 (PIN 1234 or 0000).")
                .setPositiveButton("Bluetooth settings", (d, w) -> openBluetoothSettings())
                .setNegativeButton("Cancel", null)
                .show();
            return;
        }
        Collections.sort(devices, (a, b) -> Boolean.compare(!nameOf(a).toUpperCase().contains("HC"), !nameOf(b).toUpperCase().contains("HC")));
        String[] names = new String[devices.size()];
        for (int i = 0; i < names.length; i++)
            names[i] = nameOf(devices.get(i)) + "   (" + devices.get(i).getAddress() + ")";
        new AlertDialog.Builder(this)
            .setTitle("Choose your car (HC-05)")
            .setItems(names, (d, which) -> connect(devices.get(which).getAddress()))
            .setNeutralButton("Pair a new one", (d, w) -> openBluetoothSettings())
            .show();
    }

    private void openBluetoothSettings() {
        try {
            startActivity(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS));
        } catch (Exception e) {
            toast("Open Bluetooth in your phone's settings");
        }
    }

    private String nameOf(BluetoothDevice d) {
        try {
            String n = d.getName();
            return n == null ? "(no name)" : n;
        } catch (SecurityException e) {
            return "(no name)";
        }
    }

    private void connect(final String address) {
        connecting = true;
        closingOnPurpose = false;
        showStatus();

        new Thread(() -> {
            BluetoothSocket s = null;
            boolean wasConnected = false;
            try {
                final BluetoothDevice car = adapter.getRemoteDevice(address);
                try {
                    s = car.createRfcommSocketToServiceRecord(SERIAL_PORT);
                    socket = s;                             // (so the eject button can stop it)
                    s.connect();
                } catch (IOException first) {
                    closeQuietly(s);
                    if (closingOnPurpose)
                        throw first;
                    Method m = car.getClass().getMethod("createRfcommSocket", int.class);   // some HC-05s only answer on channel 1
                    s = (BluetoothSocket) m.invoke(car, 1);
                    socket = s;
                    s.connect();
                }
                out = s.getOutputStream();
                connected = true;
                connecting = false;
                wasConnected = true;
                prefs.edit().putString("device", address).apply();
                ui.post(() -> {
                    mode = "";
                    showStatus();
                });
                sendQuietly('?');                           // ask the car what it is doing
                readLines(s.getInputStream());
            } catch (Exception e) {
                closeQuietly(s);
            }

            connected = false;
            connecting = false;
            final boolean lost = wasConnected && !closingOnPurpose;
            final boolean failed = !wasConnected && !closingOnPurpose;
            ui.post(() -> {
                showStatus();
                if (lost)
                    lcdConn.setText("○ CONNECTION LOST - TAP 🔗");
                else if (failed)
                    lcdConn.setText("○ NO ANSWER - IS THE CAR ON?");
            });
        }).start();
    }

    private void disconnect() {
        closingOnPurpose = true;
        closeQuietly(socket);
    }

    private void readLines(InputStream in) throws IOException {
        StringBuilder line = new StringBuilder();
        byte[] buf = new byte[64];
        while (true) {
            int n = in.read(buf);
            if (n < 0)
                throw new IOException("closed");
            for (int i = 0; i < n; i++) {
                char c = (char) (buf[i] & 0xFF);
                if (c == '\n') {
                    final String l = line.toString().trim();
                    line.setLength(0);
                    ui.post(() -> onLine(l));
                } else if (line.length() < 64) {
                    line.append(c);
                }
            }
        }
    }

    // "~MODE|1010": what the car is doing + muted, football, slow, low battery
    private void onLine(String l) {
        if (!l.startsWith("~"))
            return;
        int bar = l.indexOf('|');
        if (bar < 0 || l.length() < bar + 5)
            return;
        String newMode = l.substring(1, bar).trim();
        boolean wasLow = batteryLow;
        muted = l.charAt(bar + 1) == '1';
        football = l.charAt(bar + 2) == '1';
        slow = l.charAt(bar + 3) == '1';
        batteryLow = l.charAt(bar + 4) == '1';
        if (newMode.equals("INTRUDER") && !mode.equals("INTRUDER"))
            alert("🚨 INTRUDER! Guard mode saw someone.", true);
        if (batteryLow && !wasLow)
            alert("🪫 The car's battery is nearly empty - charge it soon.", false);
        mode = newMode;
        showStatus();
    }

    private void send(char c) {
        if (!connected) {
            if (System.currentTimeMillis() - lastWarnAt > 2000) {
                lastWarnAt = System.currentTimeMillis();
                toast("Not connected - tap 🔗 first");
            }
            return;
        }
        sendQuietly(c);
    }

    private void sendQuietly(final char c) {
        if (!connected)
            return;
        try {
            sender.execute(() -> {
                try {
                    OutputStream o = out;
                    if (o != null) {
                        o.write(c);
                        o.flush();
                    }
                } catch (IOException e) {
                    // the reading thread notices the lost connection
                }
            });
        } catch (Exception e) {
            // the app is closing
        }
    }

    private static void closeQuietly(BluetoothSocket s) {
        if (s == null)
            return;
        try {
            s.close();
        } catch (IOException e) {
            // already closed
        }
    }
}
