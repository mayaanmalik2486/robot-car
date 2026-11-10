package io.github.mayaanmalik2486.robotcar;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.graphics.drawable.ColorDrawable;
import android.hardware.input.InputManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.RelativeSizeSpan;
import android.view.Gravity;
import android.view.HapticFeedbackConstants;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Robot Car - a game-controller style remote for the car (held sideways).
 *
 * The parts: Car (the car's language), CarLink (Bluetooth), CarState (what the car says),
 * DriveLogic (sticks -> what is sent, never flooding the car), ControllerInput + ControllerMap
 * (the GameSir Cyclone 2 or any other game controller), StickView, TiltDriver, LcdView,
 * RadarView, PadButton, Layout + Boards (where everything sits), Settings.
 * Two more screens: SettingsActivity and ControllerActivity.
 */
public class MainActivity extends Activity implements CarLink.Listener, StickView.Listener, TiltDriver.Listener,
        ControllerInput.Listener, InputManager.InputDeviceListener, DriveLogic.Out, Boards.Screen.Style {

    static final String APP_LINK = "https://github.com/mayaanmalik2486/robot-car/raw/main/App/RobotCar.apk";
    private static final int ASK_PERMISSION = 1;
    private static final int ASK_BLUETOOTH_ON = 2;
    private static final int PAGE_MOVES = 0, PAGE_PLAY = 1, PAGE_RADAR = 2;
    private static final int SRC_TOUCH = 0, SRC_TILT = 1, SRC_PAD = 2;
    private static final long[] RUMBLE_INTRUDER = {0, 400, 150, 400, 150, 800};
    private static final long[] RUMBLE_BATTERY = {0, 300, 200, 300};
    private static final long[] RUMBLE_TICK = {0, 60};
    private static final long[] RUMBLE_TURBO = {0, 120};

    private BluetoothAdapter adapter;
    private CarLink link;
    private final CarState car = new CarState();
    private final Handler ui = new Handler(Looper.getMainLooper());
    private Settings settings;
    private DriveLogic drive;
    private ControllerInput pad;
    private TiltDriver tilt;
    private InputManager inputs;

    // the screen
    private Boards.Screen board;
    private PadButton connectB, padB, speedB, styleB, stopB, radarB;
    private TextView songChip;
    private LcdView lcd;
    private StickView stick, stick2;
    private RadarView radar;
    private final PadButton[] tabs = new PadButton[3];
    private final View[] pages = new View[3];
    private int page = PAGE_MOVES;
    private final Map<Character, List<PadButton>> buttonsFor = new HashMap<>();   // a job -> its buttons on the screen

    // driving
    private int style;
    private final float[][] source = new float[3][2];      // touch, tilt, controller: {throttle, steer}
    private int active = -1;                                 // which of them drives now
    private float touchThrottle, touchSteer;
    private boolean tiltHolding;

    // the car
    private boolean heardStatus, heardSong;
    private long connectedAt, lastWarnAt, backAt;
    private String lastSent = "";
    private String lastWheels = "";
    private boolean btOff;
    private AlertDialog problem;
    private final ArrayDeque<String> linkLog = new ArrayDeque<>();

    // ------------------------------------------------------------------ start / stop

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().setBackgroundDrawable(new ColorDrawable(Ui.BG));
        settings = new Settings(this);
        adapter = BluetoothAdapter.getDefaultAdapter();
        link = new CarLink(adapter, this);
        drive = new DriveLogic(this);
        pad = new ControllerInput(this);
        tilt = new TiltDriver(this, this);
        inputs = (InputManager) getSystemService(Context.INPUT_SERVICE);
        style = settings.style();
        if (style == Layout.TILT && !tilt.available())
            style = Layout.ONE_STICK;
        setContentView(buildScreen());
        applySettings();
        showPage(PAGE_MOVES);
        findController(false);
        registerReceiver(bluetoothState, new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED));
        showStatus();

        if (adapter == null)
            return;
        btOff = !adapter.isEnabled();
        String last = settings.device();
        if (last != null && hasPermission() && adapter.isEnabled())
            link.connect(last);
        showStatus();
    }

    @Override
    protected void onResume() {
        super.onResume();
        applySettings();
        if (inputs != null)
            inputs.registerInputDeviceListener(this, ui);
        findController(false);
        if (style == Layout.TILT)
            tilt.start();
        ui.removeCallbacks(tick);
        ui.post(tick);
        showStatus();
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
        drive.halt(SystemClock.uptimeMillis());      // never leave the car driving when the app goes away
        clearSources();
        tilt.stop();
        tiltHolding = false;
        if (inputs != null)
            inputs.unregisterInputDeviceListener(this);
        ui.removeCallbacks(tick);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            unregisterReceiver(bluetoothState);
        } catch (IllegalArgumentException e) {
            // (never registered)
        }
        link.shutdown();
    }

    @Override
    public void onBackPressed() {
        long now = SystemClock.uptimeMillis();
        if (now - backAt < 2000) {
            super.onBackPressed();
            return;
        }
        backAt = now;
        toast("Press back again to close the app (the car stops)");
    }

    /** Settings may have changed (back from the settings screen). */
    private void applySettings() {
        Ui.haptics = settings.haptics();
        drive.setFeel(settings.feel());
        drive.setSlowTop(settings.slowTop());
        pad.leftSteers = settings.leftSteers();
        settings.loadMap(pad.map, pad.descriptor);
        pad.deadZone = settings.padDeadZone(pad.descriptor);
        int s = settings.style();
        if (s == Layout.TILT && !tilt.available())
            s = Layout.ONE_STICK;
        if (s != style)
            setStyle(s, false);
        showBadges();
    }

    // 50 times a second: the driving brain sends what is due (the 0.2 s beat, the second S)
    private final Runnable tick = new Runnable() {
        private long lastClock;

        @Override
        public void run() {
            long now = SystemClock.uptimeMillis();
            drive.tick(now);
            lcd.wheels(link.isConnected() && drive.steering(), drive.left(), drive.right());
            if (now - lastClock >= 500) {
                lastClock = now;
                if (car.mode.equals("DANCE") || car.mode.equals("RADAR"))
                    showStatus();
            }
            ui.postDelayed(this, 20);
        }
    };

    // ------------------------------------------------------------------ building the screen

    @Override
    public int style() {
        return style;
    }

    private View buildScreen() {
        board = new Boards.Screen(this, this);
        board.setBackgroundColor(Ui.BG);

        connectB = key("", 13);
        connectB.setOnClickListener(v -> onConnectPressed());
        board.put(Layout.CONNECT, connectB);

        songChip = new TextView(this);
        songChip.setTextColor(Ui.LABEL);
        songChip.setTextSize(13);
        songChip.setTypeface(Ui.MEDIUM);
        songChip.setSingleLine(true);
        songChip.setEllipsize(TextUtils.TruncateAt.END);
        songChip.setGravity(Gravity.CENTER_VERTICAL);
        board.put(Layout.SONG, songChip);

        padB = key("", 12);
        padB.setOnClickListener(v -> openController());
        board.put(Layout.PAD, padB);

        speedB = key("", 14);
        speedB.setOnClickListener(v -> doJob(Car.SPEED_TOGGLE));
        board.put(Layout.SPEED, speedB);
        styleB = key("", 13);
        styleB.setOnClickListener(v -> setStyle(nextStyle(), true));
        board.put(Layout.STYLE, styleB);

        PadButton gear = key("", 14);
        gear.setIcon(PadButton.ICON_GEAR);
        gear.setContentDescription("Settings");
        gear.setOnClickListener(v -> openSettings());
        board.put(Layout.SETTINGS, gear);
        PadButton help = key("", 14);
        help.setIcon(PadButton.ICON_HELP);
        help.setContentDescription("Help");
        help.setOnClickListener(v -> showHelp());
        board.put(Layout.HELP, help);

        lcd = new LcdView(this);
        lcd.setOnClickListener(v -> {                   // tap: connect (it never disconnects - that is the top-left button)
            if (!link.isConnected() && !link.isConnecting())
                onConnectPressed();
        });
        lcd.setOnLongClickListener(v -> {               // hold: the link log
            showLinkLog();
            return true;
        });
        board.put(Layout.LCD, lcd);

        String[] names = {"MOVES", "PLAY", "RADAR"};
        String[] tabKeys = {Layout.TAB0, Layout.TAB1, Layout.TAB2};
        for (int i = 0; i < 3; i++) {
            final int p = i;
            tabs[i] = key(names[i], 14);
            tabs[i].setOnClickListener(v -> showPage(p));
            board.put(tabKeys[i], tabs[i]);
        }

        FrameLayout stack = new FrameLayout(this);
        stack.setClipChildren(false);
        pages[PAGE_MOVES] = grid(new Object[][]{
            {"🔍", "Search", Car.SEARCH}, {"😈", "Chaos", Car.CHAOS}, {"🍩", "Donut", Car.DONUT}, {"↩️", "180°", Car.TURN180},
            {"⏫", "Auto", Car.AUTO_FORWARD}, {"⏬", "Reverse", Car.AUTO_BACKWARD}, {"🅿️", "Brake", Car.BRAKE}, {"📯", "Horn", Car.HORN},
        });
        pages[PAGE_PLAY] = grid(new Object[][]{
            {"🚶", "Follow me", Car.FOLLOW}, {"🥅", "Goalkeeper", Car.KEEPER}, {"🚨", "Guard", Car.GUARD},
            {"🎵", "Music", Car.MUSIC}, {"✋", "Gestures", Car.GESTURE}, {"🐶", "Pet", Car.PET},
        });
        radar = new RadarView(this);
        radar.setState(car);
        radarB = key("", 14);
        radarB.setOnClickListener(v -> doJob(Car.RADAR));
        addJob(Car.RADAR, radarB);
        pages[PAGE_RADAR] = new Boards.RadarPage(this, radar, radarB);
        for (View v : pages)
            stack.addView(v, new FrameLayout.LayoutParams(-1, -1));
        board.put(Layout.PAGES, stack);

        stick = new StickView(this, this);
        board.put(Layout.STICK, stick);
        stick2 = new StickView(this, this);
        stick2.setAxes(StickView.LEFT_RIGHT, "STEER");
        board.put(Layout.STICK2, stick2);

        stopB = action("STOP", Car.STOP, Ui.STOP, 17);
        board.put(Layout.STOP, stopB);
        board.put(Layout.DANCE, action("Dance", Car.DANCE, Ui.KEY, 14));
        board.put(Layout.KICK, action("Kick", Car.KICK, Ui.KEY, 14));
        board.put(Layout.HORN, action("Horn", Car.HORN, Ui.KEY, 14));
        board.put(Layout.TURBO, action("Turbo", Car.TURBO, Ui.KEY, 14));

        setStyle(style, false);
        return board;
    }

    private PadButton action(String name, char job, int color, float size) {
        PadButton b = key(name, size);
        b.setColor(color);
        b.setOnClickListener(v -> doJob(job));
        addJob(job, b);
        return b;
    }

    private View grid(Object[][] items) {
        Boards.Grid g = new Boards.Grid(this);
        for (Object[] it : items) {
            final char job = (Character) it[2];
            String emoji = (String) it[0], word = (String) it[1];
            SpannableString s = new SpannableString(emoji + "\n" + word);
            s.setSpan(new RelativeSizeSpan(1.25f), 0, emoji.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            PadButton b = key(s, 14);
            b.setOnClickListener(v -> doJob(job));
            addJob(job, b);
            g.addView(b);
        }
        return g;
    }

    private void addJob(char job, PadButton b) {
        List<PadButton> l = buttonsFor.get(job);
        if (l == null)
            buttonsFor.put(job, l = new ArrayList<>());
        l.add(b);
    }

    private PadButton key(CharSequence text, float size) {
        PadButton b = new PadButton(this, text, Ui.KEY, false);
        b.setTextSize(size);
        return b;
    }

    private void showPage(int p) {
        page = p;
        for (int i = 0; i < 3; i++) {
            pages[i].setVisibility(i == p ? View.VISIBLE : View.GONE);
            tabs[i].setOn(i == p);
        }
        if (p == PAGE_RADAR)
            radar.invalidate();
    }

    private int nextStyle() {
        int s = (style + 1) % 3;
        if (s == Layout.TILT && !tilt.available())
            s = Layout.ONE_STICK;
        return s;
    }

    private void setStyle(int s, boolean save) {
        drive.halt(SystemClock.uptimeMillis());
        clearSources();
        style = s;
        if (save) {
            settings.setStyle(s);
            if (s == Layout.TILT)
                toast("Tilt: hold the round pad and tilt the phone like a steering wheel");
        }
        stick.setTiltMode(s == Layout.TILT);
        stick.setAxes(s == Layout.TWO_STICKS ? StickView.UP_DOWN : StickView.BOTH, s == Layout.TWO_STICKS ? "THROTTLE" : "DRIVE");
        if (s == Layout.TILT)
            tilt.start();
        else
            tilt.stop();
        tiltHolding = false;
        if (board != null)
            board.requestLayout();
        showStatus();
    }

    /** The little controller badges on the buttons (when a controller is there and the setting is on). */
    private void showBadges() {
        boolean show = settings.overlay() && pad.deviceId >= 0;
        for (Map.Entry<Character, List<PadButton>> e : buttonsFor.entrySet()) {
            String label = show ? pad.map.labelFor(e.getKey()) : null;
            for (PadButton b : e.getValue())
                b.setBadge(label);
        }
        String speed = show ? pad.map.labelFor(Car.SPEED_TOGGLE) : null;
        if (speed == null && show && pad.map.labelFor(Car.SLOW) != null)
            speed = pad.map.labelFor(Car.SLOW).replace("◀", "◀▶");
        speedB.setBadge(speed);
    }

    // ------------------------------------------------------------------ showing what the car says

    private void showStatus() {
        boolean on = link.isConnected();
        String top, big, bottom = "";
        boolean warn = false;
        if (adapter == null) {
            top = "THIS PHONE HAS NO BLUETOOTH";
            warn = true;
            big = "--";
        } else if (btOff) {
            top = "BLUETOOTH IS OFF - TAP TO SWITCH IT ON";
            warn = true;
            big = "--";
        } else if (!on) {
            top = link.isConnecting() ? "CONNECTING..." : "NOT CONNECTED - TAP HERE TO CONNECT";
            big = link.isConnecting() ? "..." : "--";
        } else {
            boolean old = !heardStatus && SystemClock.uptimeMillis() - connectedAt > 3000;
            String how = !heardStatus ? "" : drive.steering() ? "  STEERING" : "  LETTERS (old car program)";
            top = "● CONNECTED" + how + (lastSent.isEmpty() ? "" : "   sent " + lastSent);
            if (old) {
                top = "● OLD CAR PROGRAM - upload Robot_Car_Main from Documents\\Arduino\\robot-car";
                warn = true;
            } else if (heardStatus && !heardSong) {
                top += "   (no radar: old program)";
            }
            big = modeText();
            bottom = (car.muted ? "SOUND OFF" : "SOUND ON") + "   " + (car.football ? "SENSOR OFF" : "SENSOR ON") + "   "
                + (drive.steering() || !heardStatus ? (settings.slowTop() ? "SLOW" : "FAST") : car.slow ? "SLOW" : "FAST") + "   "
                + (car.batteryLow ? "⚠ LOW BATTERY" : "BATTERY OK");
        }
        lcd.set(top, warn, big, bottom);

        String name = link.isConnected() || link.isConnecting() ? carName() : null;
        connectB.setText(on ? Ui.twoLines("● " + name, "connected") : link.isConnecting() ? Ui.twoLines("Connecting", name)
            : Ui.twoLines("Connect", "to the car"));
        connectB.setOn(on);
        songChip.setText(on && !car.song.isEmpty() ? "♪  " + car.song : "");
        boolean slow = settings.slowTop();
        speedB.setText(Ui.twoLines(slow ? "SLOW" : "FAST", "speed"));
        speedB.setOn(slow);
        styleB.setText(Ui.twoLines(style == Layout.TWO_STICKS ? "2 STICKS" : style == Layout.TILT ? "TILT" : "1 STICK", "drive"));
        boolean scanning = car.mode.equals("RADAR");
        radarB.setText(scanning ? "STOP RADAR" : "START RADAR");
        radarB.setOn(scanning);
        char running = runningJob();
        for (Map.Entry<Character, List<PadButton>> e : buttonsFor.entrySet())
            for (PadButton b : e.getValue())
                if (b != stopB && b != radarB)
                    b.setOn(on && e.getKey() == running);
    }

    // the screen button for what the car is doing now (so it lights up)
    private char runningJob() {
        String m = car.mode;
        if (m.startsWith("LOOK") || m.startsWith("TURN ") && !m.equals("TURN 180") || m.equals("SEARCH") || m.equals("BOXED IN"))
            return Car.SEARCH;
        switch (m) {
            case "CHAOS!":   return Car.CHAOS;
            case "DONUT":    return Car.DONUT;
            case "TURN 180": return Car.TURN180;
            case "AUTO FWD": return Car.AUTO_FORWARD;
            case "AUTO BCK": return Car.AUTO_BACKWARD;
            case "BRAKING":  return Car.BRAKE;
            case "DANCE":    return Car.DANCE;
            case "TURBO!":   return Car.TURBO;
            case "FOLLOW":   return Car.FOLLOW;
            case "KEEPER":
            case "SAVE!":    return Car.KEEPER;
            case "GUARD":
            case "ARMING":
            case "INTRUDER": return Car.GUARD;
            case "MUSIC":    return Car.MUSIC;
            case "GESTURE":  return Car.GESTURE;
            case "PET":
            case "Zzz...":
            case "BORED":
            case "PURR <3":  return Car.PET;
            default:         return 0;
        }
    }

    private String modeText() {
        if (car.mode.isEmpty())
            return "...";
        if (car.mode.equals("DANCE")) {
            long s = (SystemClock.uptimeMillis() - car.modeSince) / 1000;
            return "DANCE " + s / 60 + ":" + (s % 60 < 10 ? "0" : "") + s % 60 + (car.song.isEmpty() ? "" : "  " + car.song);
        }
        if (car.mode.equals("RADAR")) {
            int near = car.radarNearest(SystemClock.uptimeMillis());
            return "RADAR" + (near >= 0 ? "  near " + near + " cm" : "");
        }
        if (car.mode.equals("DRIVE") && drive.steering())
            return "DRIVE  " + Math.round(100f * Math.max(Math.abs(drive.left()), Math.abs(drive.right())) / Car.WHEEL_MAX) + "%";
        return car.mode;
    }

    private String carName() {
        try {
            BluetoothDevice d = adapter.getRemoteDevice(link.address());
            String n = d.getName();
            return n == null ? "the car" : n;
        } catch (RuntimeException e) {
            return "the car";
        }
    }

    // ------------------------------------------------------------------ the car's link

    @Override
    public void onLinkChanged() {
        if (link.isConnected()) {
            settings.setDevice(link.address());
            car.reset();
            drive.reset();
            drive.setSteering(false);
            heardStatus = heardSong = false;
            lastSent = "";
            connectedAt = SystemClock.uptimeMillis();
            log("(connected)");
            ui.removeCallbacks(checkCarProgram);
            ui.postDelayed(checkCarProgram, 3000);
            ui.postDelayed(() -> {                         // the car's speed = the speed switch
                if (link.isConnected())
                    sendLetter(settings.slowTop() ? Car.SLOW : Car.FULL);
            }, 400);
            if (problem != null && problem.isShowing())
                problem.dismiss();
        } else if (!link.isConnecting()) {
            drive.reset();
            clearSources();
        }
        showStatus();
    }

    // 3 s after connecting: has the car answered '?' ? Older car programs never answer
    private final Runnable checkCarProgram = () -> {
        if (!link.isConnected())
            return;
        showStatus();
        if (!heardStatus)
            dialog("Old car program", "The car is connected, but it doesn't answer the app - so it is running an older program.\n\n"
                + "Driving works (with letters), but smooth steering, the play modes, the radar and the speed button need the newest one.\n\n"
                + "Upload Robot_Car_Main (or a song from the Dances folder) from Documents\\Arduino\\robot-car - "
                + "NOT the older Robot_Dances folder or the v7.1 sketch.");
    };

    @Override
    public void onLinkBroken(boolean lost) {
        showStatus();
        if (isFinishing())
            return;
        if (problem != null && problem.isShowing())
            problem.dismiss();
        problem = new AlertDialog.Builder(this)
            .setTitle(lost ? "Connection lost" : "The car didn't answer")
            .setMessage(lost ? "The Bluetooth link to the car broke - maybe it is out of range, or its battery is empty. "
                + "The car stops by itself when the app goes quiet."
                : "Is the car switched on, and close by? (Its HC-05 blinks fast when it is waiting for a phone.)")
            .setPositiveButton("Reconnect", (d, w) -> {
                String last = settings.device();
                if (last != null && adapter != null && adapter.isEnabled() && hasPermission())
                    link.connect(last);
                else
                    onConnectPressed();
            })
            .setNegativeButton("Not now", null)
            .show();
    }

    @Override
    public void onLine(String line) {
        String oldMode = car.mode;
        boolean wasLow = car.batteryLow;
        int kind = car.read(line, SystemClock.uptimeMillis());
        if (kind != CarState.RADAR)
            log("<- " + line);
        if (kind == CarState.RADAR) {
            radar.invalidate();
            return;
        }
        if (kind == CarState.NOTHING)
            return;
        if (kind == CarState.STATUS) {
            heardStatus = true;
            drive.carSpeedIs(car.slow);
        } else if (kind == CarState.SONG) {
            heardSong = true;
        } else if (kind == CarState.VERSION) {
            drive.setSteering(car.steers);
        }
        if (car.mode.equals("RADAR") && !oldMode.equals("RADAR"))
            showPage(PAGE_RADAR);                          // the radar was started (maybe from the controller): show it
        if (car.mode.equals("INTRUDER") && !oldMode.equals("INTRUDER"))
            alert("🚨 INTRUDER! Guard mode saw someone.", RUMBLE_INTRUDER);
        if (car.batteryLow && !wasLow)
            alert("🪫 The car's battery is nearly empty - charge it soon.", RUMBLE_BATTERY);
        if (car.mode.equals("BLOCKED") && !oldMode.equals("BLOCKED")) {
            rumble(RUMBLE_TICK);                           // a little tick: something is in the way
            if (Ui.haptics)
                lcd.performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
        }
        showStatus();
    }

    // the link log: every letter sent (->) and every line from the car (<-), newest last
    private void log(String s) {
        linkLog.addLast(s);
        while (linkLog.size() > 60)
            linkLog.removeFirst();
    }

    private void showLinkLog() {
        StringBuilder b = new StringBuilder();
        for (String s : linkLog)
            b.append(s).append('\n');
        dialog("Link log  (-> sent, <- from the car)", b.length() == 0 ? "Nothing yet." : b.toString());
    }

    // ------------------------------------------------------------------ sending

    /** The driving brain sends a letter or a steering code. */
    @Override
    public void send(int code) {
        if (!link.isConnected())
            return;
        link.send((char) code);
        if (code >= 0x80) {
            String w = "L" + sign(drive.left()) + " R" + sign(drive.right());
            if (!w.equals(lastWheels)) {                   // (the 0.2 s beat is not logged: it would fill the log)
                lastWheels = w;
                log("-> " + w);
            }
            lastSent = w;
        } else {
            lastWheels = "";
            log("-> " + (char) code);
            lastSent = String.valueOf((char) code);
        }
    }

    private static String sign(int v) {
        return (v > 0 ? "+" : "") + v;
    }

    /** A button's letter (a mode, horn, mute ...). */
    private void sendLetter(char c) {
        if (!link.isConnected()) {
            notConnected();
            return;
        }
        link.send(c);
        log("-> " + c);
        lastSent = String.valueOf(c);
        lastWheels = "";
        drive.actionSent(c, SystemClock.uptimeMillis());
        if (c == Car.TURBO)
            rumble(RUMBLE_TURBO);
        showStatus();
    }

    private void notConnected() {
        if (SystemClock.uptimeMillis() - lastWarnAt > 2000) {
            lastWarnAt = SystemClock.uptimeMillis();
            toast("Not connected - tap Connect first");
        }
    }

    /** What a button (on the screen or the controller) does. */
    private void doJob(char job) {
        long now = SystemClock.uptimeMillis();
        switch (job) {
            case Car.STOP:
                if (!link.isConnected()) {
                    notConnected();
                    return;
                }
                drive.stopAll(now);
                break;
            case Car.SPEED_TOGGLE:
                setSlowTop(!settings.slowTop());
                break;
            case Car.SLOW:
                setSlowTop(true);
                break;
            case Car.FULL:
                setSlowTop(false);
                break;
            case Car.RADAR:
                if (car.mode.equals("RADAR")) {
                    doJob(Car.STOP);
                    return;
                }
                if (link.isConnected() && heardStatus && !heardSong)
                    toast("The radar needs the newest car program - upload Robot_Car_Main again");
                sendLetter(Car.RADAR);
                showPage(PAGE_RADAR);
                break;
            default:
                sendLetter(job);
                break;
        }
        showStatus();
    }

    private void setSlowTop(boolean slow) {
        settings.setSlowTop(slow);
        drive.setSlowTop(slow);
        if (link.isConnected())
            sendLetter(slow ? Car.SLOW : Car.FULL);    // the car's own modes (auto, chaos, donut ...) use it too
        showStatus();
    }

    // ------------------------------------------------------------------ driving

    // the touch stick, the tilt and the controller: the last one that moved drives
    private void setSource(int src, float throttle, float steer) {
        source[src][0] = throttle;
        source[src][1] = steer;
        if (throttle != 0 || steer != 0)
            active = src;
        else if (active == src) {
            active = -1;
            for (int i = 0; i < source.length; i++)
                if (source[i][0] != 0 || source[i][1] != 0)
                    active = i;
        }
        float t = active < 0 ? 0 : source[active][0], s = active < 0 ? 0 : source[active][1];
        drive.setOneStick(active == SRC_TILT || active == SRC_TOUCH && style != Layout.TWO_STICKS);
        drive.input(t, s, SystemClock.uptimeMillis());
    }

    private void clearSources() {
        for (float[] s : source)
            s[0] = s[1] = 0;
        active = -1;
        touchThrottle = touchSteer = 0;
        drive.input(0, 0, SystemClock.uptimeMillis());
    }

    @Override
    public void onStick(StickView v, float x, float y) {
        float dz = settings.touchDeadZone();
        if (style == Layout.TWO_STICKS) {
            if (v == stick)
                touchThrottle = -DriveLogic.deadZone(y, dz);
            else
                touchSteer = DriveLogic.deadZone(x, dz);
        } else {
            float k = DriveLogic.deadZone(x, y, dz);
            touchThrottle = -y * k;
            touchSteer = x * k;
        }
        if (!link.isConnected() && (touchThrottle != 0 || touchSteer != 0))
            notConnected();
        setSource(SRC_TOUCH, touchThrottle, touchSteer);
    }

    @Override
    public void onHold(boolean holding) {
        tiltHolding = holding;
        if (holding)
            tilt.center();                                   // the way you hold it now = straight
        else
            setSource(SRC_TILT, 0, 0);
    }

    @Override
    public void onTilt(float x, float y) {
        if (!tiltHolding)
            return;
        stick.show(x, y);
        float k = DriveLogic.deadZone(x, y, 0.1f);
        setSource(SRC_TILT, -y * k, x * k);
    }

    // ------------------------------------------------------------------ the game controller

    @Override
    public boolean dispatchKeyEvent(KeyEvent e) {
        // the controller first: a focused button must never eat the D-pad, and B / View never closes the app
        if (pad.onKey(e))
            return true;
        return super.dispatchKeyEvent(e);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent e) {
        if (pad.onMotion(e))
            return true;
        return super.dispatchGenericMotionEvent(e);
    }

    @Override
    public void onPadJob(char job, int button) {
        List<PadButton> l = buttonsFor.get(job);
        if (job == Car.SPEED_TOGGLE || job == Car.SLOW || job == Car.FULL)
            speedB.flash();
        else if (l != null)
            for (PadButton b : l)
                if (b.isShown())
                    b.flash();                             // show which button the controller pressed
        doJob(job);
    }

    @Override
    public void onPadButton(int button, boolean down) {
    }

    @Override
    public void onPadDrive(float throttle, float steer) {
        if (style == Layout.TWO_STICKS) {
            stick.show(0, -throttle);
            stick2.show(steer, 0);
        } else if (style == Layout.ONE_STICK) {
            stick.show(steer, -throttle);
        }
        if (!link.isConnected() && (throttle != 0 || steer != 0))
            notConnected();
        setSource(SRC_PAD, throttle, steer);
    }

    @Override
    public void onPadChanged() {
    }

    @Override
    public void onPadDevice(InputDevice d) {
        settings.loadMap(pad.map, pad.descriptor);        // this controller's own map and dead zone
        pad.deadZone = settings.padDeadZone(pad.descriptor);
        findController(false);
    }

    private InputDevice controller() {
        if (pad.deviceId >= 0) {
            InputDevice d = InputDevice.getDevice(pad.deviceId);
            if (d != null)
                return d;
        }
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice d = InputDevice.getDevice(id);
            if (ControllerInput.isGamepad(d))
                return d;
        }
        return null;
    }

    private void findController(boolean announce) {
        InputDevice d = controller();
        if (d != null && pad.deviceId < 0) {
            pad.deviceId = d.getId();
            pad.name = d.getName();
            pad.descriptor = d.getDescriptor();
            settings.loadMap(pad.map, pad.descriptor);
            pad.deadZone = settings.padDeadZone(pad.descriptor);
        }
        if (d == null)
            pad.deviceId = -1;
        boolean roomy = padB.getWidth() == 0 || padB.getWidth() > Ui.dp(this, 100);
        padB.setIcon(roomy ? PadButton.ICON_NONE : PadButton.ICON_PAD);
        padB.setText(!roomy ? "" : d == null ? Ui.twoLines("No controller", "tap to connect one") : Ui.twoLines(shortName(d.getName()), "controller"));
        padB.setOn(d != null);
        if (announce && d != null)
            toast("🎮 " + shortName(d.getName()) + " connected - tap its name (top) to see every button");
        showBadges();
    }

    private static String shortName(String n) {
        if (n == null)
            return "Controller";
        n = n.trim();
        return n.length() > 20 ? n.substring(0, 20) : n;
    }

    @Override
    public void onInputDeviceAdded(int id) {
        if (ControllerInput.isGamepad(InputDevice.getDevice(id)))
            findController(true);
    }

    @Override
    public void onInputDeviceRemoved(int id) {
        if (id == pad.deviceId) {
            pad.lost();                                    // its sticks count as let go: the car stops
            setSource(SRC_PAD, 0, 0);
            drive.halt(SystemClock.uptimeMillis());
            toast("Controller disconnected - the car stopped");
        }
        findController(false);
    }

    @Override
    public void onInputDeviceChanged(int id) {
        findController(false);
    }

    private void rumble(long[] pattern) {
        ControllerInput.rumble(controller(), pattern);
    }

    // ------------------------------------------------------------------ alerts, help, other screens

    private void alert(String s, long[] pattern) {
        dialog(null, s);
        Vibrator v = (Vibrator) getSystemService(VIBRATOR_SERVICE);
        if (v != null && v.hasVibrator()) {
            if (Build.VERSION.SDK_INT >= 26)
                v.vibrate(VibrationEffect.createWaveform(pattern, -1));
            else
                v.vibrate(pattern, -1);
        }
        rumble(pattern);                                   // the controller rumbles too
    }

    private void dialog(String title, String message) {
        if (isFinishing())
            return;
        AlertDialog.Builder b = new AlertDialog.Builder(this).setMessage(message).setPositiveButton("OK", null);
        if (title != null)
            b.setTitle(title);
        b.show();
    }

    private void toast(String s) {
        Toast.makeText(this, s, Toast.LENGTH_SHORT).show();
    }

    private void showHelp() {
        String map = pad.map.describe();
        new AlertDialog.Builder(this)
            .setTitle("Robot Car 4.0")
            .setMessage(
                "DRIVING\n"
                + "1 STICK: push where you want to go - a little = slowly, all the way = full speed. Let go = stop.\n"
                + "2 STICKS: left = throttle (up / down), right = steering (left / right), like an RC car.\n"
                + "TILT: hold the round pad and tilt the phone like a steering wheel. Let go = stop.\n"
                + "FAST / SLOW: the top speed. STOP stops everything, also a running mode.\n\n"
                + "With the newest car program the steering is smooth (both sides of the car get their own speed); "
                + "older programs get the 6 driving letters.\n\n"
                + "Dance: press on the first beat, again ON a beat to line it up. Kick, horn, turbo (1.5 s full power).\n"
                + "MOVES: search, chaos, donut, 180°, auto, reverse, brake, horn.\n"
                + "PLAY: follow me, goalkeeper, guard, music, gestures, pet.\n"
                + "RADAR: the car stands still and sweeps its sensor; dots = what it sees (up to 1.5 m).\n"
                + "Hold the green screen: the link log.\n\n"
                + "GAMESIR CYCLONE 2\n"
                + "Phone by Bluetooth: hold HOME + A for 2 s until the light flashes YELLOW (Android mode), then pick "
                + "\"GameSir-Cyclone 2\" in the phone's Bluetooth list. Or plug its 2.4 GHz dongle into the phone with a "
                + "USB-C adapter (light GREEN). Left stick = throttle, right stick = steering.\n\n"
                + map + "\n\n"
                + "The back buttons M1 / M2: set them in the GameSir app to copy any button above.\n"
                + "Change any button: Settings > Controller buttons.\n\n"
                + "FIRST TIME: pair HC-05 in the phone's Bluetooth settings (PIN 1234 or 0000), then tap Connect.")
            .setPositiveButton("OK", null)
            .setNeutralButton("Controller", (d, w) -> openController())
            .setNegativeButton("Share app", (d, w) -> shareApp())
            .show();
    }

    private void openSettings() {
        Intent i = new Intent(this, SettingsActivity.class);
        i.putExtra("pad", pad.descriptor);
        i.putExtra("padName", pad.deviceId >= 0 ? pad.name : null);
        i.putExtra("tilt", tilt.available());
        startActivity(i);
    }

    private void openController() {
        startActivity(new Intent(this, ControllerActivity.class));
    }

    private void shareApp() {
        Intent i = new Intent(Intent.ACTION_SEND);
        i.setType("text/plain");
        i.putExtra(Intent.EXTRA_TEXT, "Robot Car app (Android): " + APP_LINK);
        startActivity(Intent.createChooser(i, "Share the Robot Car app"));
    }

    // ------------------------------------------------------------------ Bluetooth on / off

    private final BroadcastReceiver bluetoothState = new BroadcastReceiver() {
        @Override
        public void onReceive(Context c, Intent i) {
            int s = i.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
            if (s == BluetoothAdapter.STATE_OFF || s == BluetoothAdapter.STATE_TURNING_OFF) {
                btOff = true;
                drive.halt(SystemClock.uptimeMillis());
            } else if (s == BluetoothAdapter.STATE_ON) {
                btOff = false;
                String last = settings.device();
                if (last != null && hasPermission() && !link.isConnected() && !link.isConnecting())
                    link.connect(last);
            }
            showStatus();
        }
    };

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
            new AlertDialog.Builder(this)
                .setMessage(link.isConnected() ? "Disconnect from the car? It stops." : "Stop connecting?")
                .setPositiveButton("Disconnect", (d, w) -> {
                    drive.halt(SystemClock.uptimeMillis());
                    link.disconnect();
                })
                .setNegativeButton("Cancel", null)
                .show();
            return;
        }
        if (!hasPermission()) {
            requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT}, ASK_PERMISSION);
            return;
        }
        if (!adapter.isEnabled()) {
            try {
                startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), ASK_BLUETOOTH_ON);
            } catch (RuntimeException e) {
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
            dialog("Bluetooth permission", "The app needs the \"Nearby devices\" permission to talk to the car. "
                + "If you said no, allow it in the phone's Settings > Apps > Robot Car > Permissions.");
    }

    @Override
    protected void onActivityResult(int code, int result, Intent data) {
        if (code == ASK_BLUETOOTH_ON && adapter != null && adapter.isEnabled()) {
            btOff = false;
            chooseCar();
        }
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
            startActivity(new Intent(android.provider.Settings.ACTION_BLUETOOTH_SETTINGS));
        } catch (RuntimeException e) {
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
