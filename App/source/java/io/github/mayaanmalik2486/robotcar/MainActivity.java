package io.github.mayaanmalik2486.robotcar;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Typeface;
import android.hardware.input.InputManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.provider.Settings;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Robot Car - a game-pad style remote for the car (held sideways).
 *
 * Left: the thumbstick (or tilt driving). Right: STOP and the four face buttons. Middle: the
 * car's little screen, then three pages of buttons - Moves, Play and Radar. Top: connect,
 * the song, football and sound switches, a game controller if one is connected, and help.
 *
 * The parts: Car (the letters), CarLink (Bluetooth), CarState (what the car says),
 * StickView, TiltDriver, Gamepad, RadarView and PadButton (the buttons).
 */
public class MainActivity extends Activity implements CarLink.Listener, StickView.Listener,
        TiltDriver.Listener, Gamepad.Listener, InputManager.InputDeviceListener {

    private static final int ASK_PERMISSION = 1;
    private static final int ASK_BLUETOOTH_ON = 2;
    private static final String APP_LINK = "https://github.com/mayaanmalik2486/robot-car/raw/main/App/RobotCar.apk";

    private static final int PAGE_MOVES = 0, PAGE_PLAY = 1, PAGE_RADAR = 2;

    private BluetoothAdapter adapter;
    private CarLink link;
    private final CarState car = new CarState();
    private final Handler ui = new Handler(Looper.getMainLooper());
    private SharedPreferences prefs;
    private TiltDriver tilt;
    private final Gamepad pad = new Gamepad(this);
    private InputManager inputs;

    // the screen
    private StickView stick;
    private RadarView radar;
    private TextView lcdConn, lcdMode, lcdFlags;
    private PadButton connectButton, speedButton, tiltButton, footballButton, soundButton, radarButton;
    private TextView songChip, padChip;
    private final PadButton[] tabs = new PadButton[3];
    private final View[] pages = new View[3];
    private int page = PAGE_MOVES;
    private final Map<Character, PadButton> buttonFor = new HashMap<>();   // to flash what a controller pressed

    private char driving = Car.STOP;            // the drive letter being held right now
    private boolean tiltOn, tiltHolding;
    private long lastWarnAt;

    // ------------------------------------------------------------------ start / stop

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        prefs = getSharedPreferences("robotcar", MODE_PRIVATE);
        adapter = BluetoothAdapter.getDefaultAdapter();
        link = new CarLink(adapter, this);
        tilt = new TiltDriver(this, this);
        inputs = (InputManager) getSystemService(Context.INPUT_SERVICE);
        setContentView(buildScreen());
        showPage(PAGE_MOVES);
        showStatus();
        findGamepad();

        if (adapter == null) {
            lcdConn.setText("NO BLUETOOTH ON THIS PHONE");
            return;
        }
        String last = prefs.getString("device", null);
        if (last != null && hasPermission() && adapter.isEnabled())
            link.connect(last);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (inputs != null)
            inputs.registerInputDeviceListener(this, ui);
        findGamepad();
        if (tiltOn)
            tilt.start();
        ui.post(clock);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)                                // full screen, like a game
            getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
    }

    @Override
    protected void onPause() {
        super.onPause();
        drive(Car.STOP);                             // never leave the car driving when the app goes away
        tilt.stop();
        tiltHolding = false;
        if (inputs != null)
            inputs.unregisterInputDeviceListener(this);
        ui.removeCallbacks(clock);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        link.shutdown();
    }

    // the dance clock on the screen ticks while dancing
    private final Runnable clock = new Runnable() {
        @Override
        public void run() {
            if (car.mode.equals("DANCE"))
                showStatus();
            ui.postDelayed(this, 500);
        }
    };

    // ------------------------------------------------------------------ building the screen

    private View buildScreen() {
        LinearLayout root = column();
        root.setBackgroundColor(Ui.BODY);
        root.setPadding(dp(14), dp(8), dp(14), dp(10));
        root.addView(topBar(), new LinearLayout.LayoutParams(-1, dp(40)));

        LinearLayout body = new LinearLayout(this);
        LinearLayout.LayoutParams bp = new LinearLayout.LayoutParams(-1, 0, 1f);
        bp.topMargin = dp(8);
        root.addView(body, bp);

        body.addView(leftSide(), new LinearLayout.LayoutParams(0, -1, 0.29f));
        LinearLayout.LayoutParams mp = new LinearLayout.LayoutParams(0, -1, 0.42f);
        mp.leftMargin = mp.rightMargin = dp(12);
        body.addView(middle(), mp);
        body.addView(rightSide(), new LinearLayout.LayoutParams(0, -1, 0.29f));
        return root;
    }

    private View topBar() {
        LinearLayout bar = new LinearLayout(this);
        bar.setGravity(Gravity.CENTER_VERTICAL);

        connectButton = key("", Ui.KEY, 13);
        connectButton.setOnClickListener(v -> onConnectPressed());
        bar.addView(connectButton, new LinearLayout.LayoutParams(dp(132), -1));

        songChip = chip();
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(0, -2, 1f);
        sp.leftMargin = dp(10);
        bar.addView(songChip, sp);

        padChip = chip();
        padChip.setOnClickListener(v -> showControllerMap());
        bar.addView(padChip, new LinearLayout.LayoutParams(-2, -2));

        footballButton = key("", Ui.KEY, 13);
        footballButton.setOnClickListener(v -> send(Car.FOOTBALL));
        soundButton = key("", Ui.KEY, 13);
        soundButton.setOnClickListener(v -> send(Car.MUTE));
        PadButton help = key("❔", Ui.KEY, 15);
        help.setOnClickListener(v -> showHelp());
        for (View v : new View[]{footballButton, soundButton, help}) {
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(v == help ? dp(46) : dp(104), -1);
            p.leftMargin = dp(8);
            bar.addView(v, p);
        }
        return bar;
    }

    private View leftSide() {
        LinearLayout left = column();
        LinearLayout row = new LinearLayout(this);
        speedButton = key("", Ui.KEY, 13);
        speedButton.setOnClickListener(v -> send(Car.SPEED_TOGGLE));
        tiltButton = key("", Ui.KEY, 13);
        tiltButton.setOnClickListener(v -> setTilt(!tiltOn));
        row.addView(speedButton, new LinearLayout.LayoutParams(0, -1, 1f));
        LinearLayout.LayoutParams tp = new LinearLayout.LayoutParams(0, -1, 1f);
        tp.leftMargin = dp(8);
        row.addView(tiltButton, tp);
        left.addView(row, new LinearLayout.LayoutParams(-1, dp(46)));

        stick = new StickView(this, this);
        LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(-1, 0, 1f);
        sp.topMargin = dp(8);
        left.addView(stick, sp);
        return left;
    }

    private View middle() {
        LinearLayout mid = column();

        LinearLayout lcd = column();
        android.graphics.drawable.GradientDrawable screen = Ui.rounded(Ui.LCD, dp(10));
        screen.setStroke(dp(2), Ui.LCD_EDGE);
        lcd.setBackground(screen);
        lcd.setPadding(dp(10), dp(5), dp(10), dp(5));
        lcd.setOnClickListener(v -> onConnectPressed());
        lcdConn = lcdText(11, Ui.LCD_DIM);
        lcdMode = lcdText(21, Ui.LCD_TEXT);
        lcdFlags = lcdText(12, Ui.LCD_TEXT);
        lcd.addView(lcdConn);
        lcd.addView(lcdMode);
        lcd.addView(lcdFlags);
        mid.addView(lcd, new LinearLayout.LayoutParams(-1, -2));

        LinearLayout tabRow = new LinearLayout(this);
        String[] names = {"🕹 Moves", "🎮 Play", "📡 Radar"};
        for (int i = 0; i < 3; i++) {
            final int p = i;
            tabs[i] = key(names[i], Ui.KEY, 13);
            tabs[i].setOnClickListener(v -> showPage(p));
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(0, -1, 1f);
            if (i > 0)
                lp.leftMargin = dp(6);
            tabRow.addView(tabs[i], lp);
        }
        LinearLayout.LayoutParams tp = new LinearLayout.LayoutParams(-1, dp(38));
        tp.topMargin = dp(8);
        mid.addView(tabRow, tp);

        FrameLayout stack = new FrameLayout(this);
        pages[PAGE_MOVES] = grid(4, new Object[][]{
            {"🔍", "search", Car.SEARCH}, {"😈", "chaos", Car.CHAOS}, {"🍩", "donut", Car.DONUT},
            {"↩️", "180°", Car.TURN180}, {"⏫", "auto", Car.AUTO_FORWARD}, {"⏬", "reverse", Car.AUTO_BACKWARD},
            {"🅿️", "brake", Car.BRAKE}, {"📯", "horn", Car.HORN},
        });
        pages[PAGE_PLAY] = grid(3, new Object[][]{
            {"🚶", "follow me", Car.FOLLOW}, {"🥅", "goalkeeper", Car.KEEPER}, {"🚨", "guard", Car.GUARD},
            {"🎵", "music", Car.MUSIC}, {"✋", "gestures", Car.GESTURE}, {"🐶", "pet", Car.PET},
        });
        pages[PAGE_RADAR] = radarPage();
        for (View v : pages)
            stack.addView(v, new FrameLayout.LayoutParams(-1, -1));
        LinearLayout.LayoutParams gp = new LinearLayout.LayoutParams(-1, 0, 1f);
        gp.topMargin = dp(8);
        mid.addView(stack, gp);
        return mid;
    }

    private View radarPage() {
        LinearLayout page = column();
        radar = new RadarView(this);
        radar.setState(car);
        radar.setBackground(Ui.rounded(Ui.LCD, dp(10)));
        page.addView(radar, new LinearLayout.LayoutParams(-1, 0, 1f));
        radarButton = key("", Ui.KEY, 14);
        radarButton.setOnClickListener(v -> {
            if (car.mode.equals("RADAR"))
                stopEverything();
            else
                send(Car.RADAR);
        });
        LinearLayout.LayoutParams bp = new LinearLayout.LayoutParams(-1, dp(42));
        bp.topMargin = dp(6);
        page.addView(radarButton, bp);
        return page;
    }

    private View rightSide() {
        LinearLayout right = column();
        PadButton stop = key("🛑  STOP", Ui.STOP_RED, 15);
        stop.setOnClickListener(v -> stopEverything());
        buttonFor.put(Car.STOP, stop);
        right.addView(stop, new LinearLayout.LayoutParams(-1, dp(46)));

        LinearLayout face = column();
        face.setGravity(Gravity.CENTER);
        face.addView(faceRow(null, face("💃", Car.DANCE, Ui.YELLOW), null));
        face.addView(faceRow(face("⚽", Car.KICK, Ui.BLUE), null, face("📯", Car.HORN, Ui.RED)));
        face.addView(faceRow(null, face("🚀", Car.TURBO, Ui.GREEN), null));
        LinearLayout.LayoutParams fp = new LinearLayout.LayoutParams(-1, 0, 1f);
        fp.topMargin = dp(6);
        right.addView(face, fp);
        return right;
    }

    private View faceRow(View a, View b, View c) {
        LinearLayout r = new LinearLayout(this);
        r.setGravity(Gravity.CENTER);
        for (View v : new View[]{a, b, c}) {
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(dp(66), dp(70));
            p.setMargins(dp(4), 0, dp(4), 0);
            r.addView(v != null ? v : new View(this), p);
        }
        return r;
    }

    private PadButton face(String emoji, char letter, int color) {
        PadButton b = new PadButton(this, emoji, color, true);
        b.setOnClickListener(v -> send(letter));
        buttonFor.put(letter, b);                       // (the face buttons light up for a controller, not the page ones)
        return b;
    }

    // rows of equal buttons that fill the space
    private View grid(int columns, Object[][] items) {
        LinearLayout g = column();
        for (int i = 0; i < items.length; i += columns) {
            LinearLayout row = new LinearLayout(this);
            for (int j = 0; j < columns; j++) {
                LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(0, -1, 1f);
                p.setMargins(dp(3), dp(3), dp(3), dp(3));
                if (i + j >= items.length) {
                    row.addView(new View(this), p);
                    continue;
                }
                Object[] it = items[i + j];
                final char letter = (Character) it[2];
                final String name = (String) it[1];
                PadButton b = key(Ui.emojiLabel((String) it[0], name), Ui.KEY, 20);
                b.setOnClickListener(v -> send(letter));
                b.setOnLongClickListener(v -> {
                    toast(name);
                    return true;
                });
                if (!buttonFor.containsKey(letter))
                    buttonFor.put(letter, b);
                row.addView(b, p);
            }
            g.addView(row, new LinearLayout.LayoutParams(-1, 0, 1f));
        }
        return g;
    }

    private PadButton key(CharSequence text, int color, float size) {
        PadButton b = new PadButton(this, text, color, false);
        b.setTextSize(size);
        return b;
    }

    private TextView chip() {
        TextView t = new TextView(this);
        t.setTextColor(Ui.LABEL);
        t.setTextSize(13);
        t.setSingleLine(true);
        t.setEllipsize(TextUtils.TruncateAt.END);
        t.setTypeface(Typeface.DEFAULT_BOLD);
        return t;
    }

    private TextView lcdText(float size, int color) {
        TextView t = new TextView(this);
        t.setTextSize(size);
        t.setTextColor(color);
        t.setTypeface(Typeface.MONOSPACE);
        t.setSingleLine(true);
        t.setEllipsize(TextUtils.TruncateAt.END);
        return t;
    }

    private LinearLayout column() {
        LinearLayout l = new LinearLayout(this);
        l.setOrientation(LinearLayout.VERTICAL);
        return l;
    }

    private int dp(float v) {
        return Ui.dp(this, v);
    }

    private void showPage(int p) {
        page = p;
        for (int i = 0; i < 3; i++) {
            pages[i].setVisibility(i == p ? View.VISIBLE : View.GONE);
            tabs[i].setColor(i == p ? Ui.KEY_ON : Ui.KEY);
        }
    }

    // ------------------------------------------------------------------ showing what the car says

    private void showStatus() {
        boolean on = link.isConnected();
        if (!on) {
            lcdConn.setText(link.isConnecting() ? "CONNECTING..." : "○ NOT CONNECTED - TAP 🔗");
            lcdMode.setText("--");
            lcdFlags.setText("");
        } else {
            lcdConn.setText("● CONNECTED");
            lcdMode.setText(modeText());
            lcdFlags.setText((car.muted ? "🔇 " : "🔊 ") + (car.football ? "⚽ " : "📡 ")
                + (car.slow ? "🐢 " : "🐇 ") + (car.batteryLow ? "🪫 LOW BATTERY" : "🔋"));
        }
        connectButton.setText(on ? "⏏ disconnect" : link.isConnecting() ? "⏳ connecting" : "🔗 connect");
        connectButton.setColor(on ? Ui.KEY : Ui.KEY_ON);
        songChip.setText(on && !car.song.isEmpty() ? "🎵 " + car.song : "");
        speedButton.setText(car.slow ? "🐢 slow" : "🐇 fast");
        tiltButton.setText(tiltOn ? "📱 tilt ON" : "📱 tilt");
        tiltButton.setColor(tiltOn ? Ui.KEY_ON : Ui.KEY);
        footballButton.setText(car.football ? "⚽ sensor off" : "⚽ football");
        footballButton.setColor(car.football ? Ui.KEY_ON : Ui.KEY);
        soundButton.setText(car.muted ? "🔇 muted" : "🔊 sound");
        soundButton.setColor(car.muted ? Ui.KEY_ON : Ui.KEY);
        boolean scanning = car.mode.equals("RADAR");
        radarButton.setText(scanning ? "🛑 stop the radar" : "📡 start the radar");
        radarButton.setColor(scanning ? Ui.KEY_ON : Ui.KEY);
    }

    private String modeText() {
        if (car.mode.isEmpty())
            return "...";
        if (car.mode.equals("DANCE")) {
            long s = (SystemClock.uptimeMillis() - car.modeSince) / 1000;
            String clock = s / 60 + ":" + (s % 60 < 10 ? "0" : "") + s % 60;
            return "💃 " + clock + (car.song.isEmpty() ? "" : "  " + car.song);
        }
        if (car.mode.equals("RADAR")) {
            int near = car.radarNearest(SystemClock.uptimeMillis());
            return "📡 RADAR" + (near >= 0 ? "  near " + near + " cm" : "");
        }
        return car.mode;
    }

    // ------------------------------------------------------------------ the car's link

    @Override
    public void onLinkChanged() {
        if (link.isConnected()) {
            prefs.edit().putString("device", link.address()).apply();
            car.reset();
        }
        showStatus();
    }

    @Override
    public void onLinkBroken(boolean lost) {
        lcdConn.setText(lost ? "○ CONNECTION LOST - TAP 🔗" : "○ NO ANSWER - IS THE CAR ON?");
    }

    @Override
    public void onLine(String line) {
        String oldMode = car.mode;
        boolean wasLow = car.batteryLow;
        int kind = car.read(line, SystemClock.uptimeMillis());
        if (kind == CarState.RADAR) {
            radar.invalidate();
            if (page == PAGE_RADAR || car.mode.equals("RADAR"))
                showStatus();
            return;
        }
        if (kind == CarState.NOTHING)
            return;
        if (car.mode.equals("RADAR") && !oldMode.equals("RADAR"))
            showPage(PAGE_RADAR);                          // the radar was started (maybe from a controller): show it
        if (car.mode.equals("INTRUDER") && !oldMode.equals("INTRUDER"))
            alert("🚨 INTRUDER! Guard mode saw someone.", true);
        if (car.batteryLow && !wasLow)
            alert("🪫 The car's battery is nearly empty - charge it soon.", false);
        showStatus();
    }

    // ------------------------------------------------------------------ driving

    // send the drive letter only when it changes; S (twice) when let go
    private void drive(char c) {
        if (c == driving)
            return;
        driving = c;
        if (c == Car.STOP) {
            link.send(Car.STOP);
            ui.postDelayed(() -> link.send(Car.STOP), 60);
        } else {
            stick.performHapticFeedback(android.view.HapticFeedbackConstants.VIRTUAL_KEY);
            send(c);
        }
    }

    private void stopEverything() {
        driving = Car.STOP;
        send(Car.STOP);
    }

    @Override
    public void onStick(float x, float y) {
        drive(Car.direction(x, y));
    }

    private void setTilt(boolean on) {
        if (on && !tilt.available()) {
            toast("This phone has no tilt sensor");
            return;
        }
        tiltOn = on;
        tiltHolding = false;
        stick.setTiltMode(on);
        drive(Car.STOP);
        if (on) {
            tilt.start();
            toast("Hold the pad and tilt the phone to drive");
        } else {
            tilt.stop();
        }
        showStatus();
    }

    @Override
    public void onHold(boolean holding) {
        tiltHolding = holding;
        if (holding)
            tilt.center();                                   // the way you hold it now = straight
        else
            drive(Car.STOP);
    }

    @Override
    public void onTilt(float x, float y) {
        if (!tiltHolding)
            return;
        stick.show(x, y);
        drive(Car.direction(x, y));
    }

    // ------------------------------------------------------------------ game controller

    @Override
    public boolean onGenericMotionEvent(MotionEvent e) {
        return pad.onMotion(e) || super.onGenericMotionEvent(e);
    }

    @Override
    public boolean onKeyDown(int code, KeyEvent e) {
        return pad.onKeyDown(code, e) || super.onKeyDown(code, e);
    }

    @Override
    public boolean onKeyUp(int code, KeyEvent e) {
        return pad.onKeyUp(code, e) || super.onKeyUp(code, e);
    }

    @Override
    public void onPadAction(char letter) {
        PadButton b = letter == Car.SPEED_TOGGLE ? speedButton : letter == Car.FOOTBALL ? footballButton
            : letter == Car.MUTE ? soundButton : letter == Car.RADAR ? radarButton : buttonFor.get(letter);
        if (b != null && b.isShown())
            b.flash();                                        // show which button the controller pressed
        if (letter == Car.STOP)
            stopEverything();
        else
            send(letter);
    }

    @Override
    public void onPadDrive(char direction, float showX, float showY) {
        if (!tiltOn)
            stick.show(showX, showY);
        drive(direction);
    }

    private InputDevice gamepad() {
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice d = InputDevice.getDevice(id);
            if (Gamepad.isGamepad(d))
                return d;
        }
        return null;
    }

    private void findGamepad() {
        InputDevice d = gamepad();
        padChip.setText(d == null ? "" : "🎮 " + shortName(d.getName()) + "  ");
        padChip.setVisibility(d == null ? View.GONE : View.VISIBLE);
    }

    private static String shortName(String n) {
        if (n == null)
            return "controller";
        n = n.trim();
        return n.length() > 18 ? n.substring(0, 18) : n;
    }

    @Override
    public void onInputDeviceAdded(int id) {
        findGamepad();
        if (Gamepad.isGamepad(InputDevice.getDevice(id)))
            toast("🎮 Controller connected - tap 🎮 for its buttons");
    }

    @Override
    public void onInputDeviceRemoved(int id) {
        findGamepad();
        drive(Car.STOP);
    }

    @Override
    public void onInputDeviceChanged(int id) {
        findGamepad();
    }

    // ------------------------------------------------------------------ sending, alerts, help

    private void send(char c) {
        if (c == Car.SPEED_TOGGLE)
            c = car.slow ? Car.FULL : Car.SLOW;
        if (!link.isConnected()) {
            if (SystemClock.uptimeMillis() - lastWarnAt > 2000) {
                lastWarnAt = SystemClock.uptimeMillis();
                toast("Not connected - tap 🔗 connect first");
            }
            return;
        }
        link.send(c);
    }

    private void alert(String s, boolean urgent) {
        new AlertDialog.Builder(this).setMessage(s).setPositiveButton("OK", null).show();
        long[] pattern = urgent ? new long[]{0, 400, 150, 400, 150, 800} : new long[]{0, 300};
        buzz((Vibrator) getSystemService(VIBRATOR_SERVICE), pattern);
        InputDevice d = gamepad();
        if (d != null)
            buzz(d.getVibrator(), pattern);                   // the controller rumbles too
    }

    private static void buzz(Vibrator v, long[] pattern) {
        if (v == null || !v.hasVibrator())
            return;
        if (Build.VERSION.SDK_INT >= 26)
            v.vibrate(VibrationEffect.createWaveform(pattern, -1));
        else
            v.vibrate(pattern, -1);
    }

    private void toast(String s) {
        Toast.makeText(this, s, Toast.LENGTH_SHORT).show();
    }

    private void showHelp() {
        new AlertDialog.Builder(this)
            .setTitle("Robot Car")
            .setMessage(
                "🕹 Stick: drive. Up = forward, diagonals = curve, sides = spin, down = back. Let go = stop.\n"
                + "📱 tilt: hold the pad and tilt the phone like a steering wheel (tip it away = forward). Let go = stop.\n"
                + "🐇/🐢 fast / slow.   🛑 STOP: stops everything.\n\n"
                + "💃 dance - press on the first beat, again ON a beat to line it up. The top shows the song.\n"
                + "⚽ kick   📯 horn   🚀 turbo\n\n"
                + "Moves: 🔍 search  😈 chaos  🍩 donut  ↩️ 180°  ⏫/⏬ auto  🅿️ brake\n"
                + "Play: 🚶 follow me  🥅 goalkeeper  🚨 guard  🎵 music  ✋ gestures  🐶 pet\n"
                + "Radar: the car stands still and sweeps its sensor; dots = things it sees (up to 1.5 m).\n"
                + "⚽ football = distance sensor off.   🔊 sound on / off.   Hold a button to see its name.\n\n"
                + "First time: pair HC-05 in the phone's Bluetooth settings (PIN 1234 or 0000), then tap 🔗 connect.")
            .setPositiveButton("OK", null)
            .setNeutralButton("🎮 Controller", (d, w) -> showControllerMap())
            .setNegativeButton("Share app", (d, w) -> shareApp())
            .show();
    }

    private void showControllerMap() {
        new AlertDialog.Builder(this)
            .setTitle("🎮 Game controller (GameSir Cyclone 2 ...)")
            .setMessage(
                "Connect it to this phone (Bluetooth, or its USB dongle with a USB-C adapter). On the Cyclone 2 use the Android / Xbox mode.\n\n"
                + "DRIVING (like a racing game)\n"
                + "RT = gas   LT = reverse   left stick = steer\n"
                + "(or drive with the left stick alone)\n\n"
                + "BUTTONS\n"
                + "A turbo   B STOP   X kick   Y dance\n"
                + "RB fast / slow   L3 (click the stick) horn   R3 brake\n"
                + "D-pad: ↑ search  → chaos  ↓ donut  ← 180° turn\n"
                + "START radar   SELECT football (sensor off)\n\n"
                + "HOLD LB FOR THE PLAY MODES\n"
                + "LB+A follow me   LB+B guard   LB+X goalkeeper   LB+Y pet\n"
                + "LB+↑ music   LB+↓ gestures   LB+→ auto forward   LB+← auto backward\n"
                + "LB+SELECT sound on / off\n\n"
                + "The controller rumbles when guard mode sees an intruder or the battery is low. "
                + "The Cyclone 2's back buttons can copy any of these (set them in the GameSir app).")
            .setPositiveButton("OK", null)
            .show();
    }

    private void shareApp() {
        Intent i = new Intent(Intent.ACTION_SEND);
        i.setType("text/plain");
        i.putExtra(Intent.EXTRA_TEXT, "Robot Car app (Android): " + APP_LINK);
        startActivity(Intent.createChooser(i, "Share the Robot Car app"));
    }

    // ------------------------------------------------------------------ connecting

    private boolean hasPermission() {
        return Build.VERSION.SDK_INT < 31
            || checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED;
    }

    private void onConnectPressed() {
        if (adapter == null) {
            toast("This phone has no Bluetooth");
            return;
        }
        if (link.isConnected() || link.isConnecting()) {
            link.disconnect();
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
            .setItems(names, (d, which) -> link.connect(devices.get(which).getAddress()))
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
}
