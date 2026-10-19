package io.github.mayaanmalik2486.robotcar;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.provider.Settings;
import android.view.Gravity;
import android.view.HapticFeedbackConstants;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
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
 * The Robot Car app: a remote control for the car's HC-05 Bluetooth module.
 *
 * Every button sends one letter - the same letters the car's program understands
 * (see Robot_Car_Main.ino, "BLUETOOTH LETTERS"). The car answers with a status line,
 * "~MODE|1010": what it is doing, then 1/0 for muted, football mode, slow, low battery.
 */
public class MainActivity extends Activity {

    private static final UUID SERIAL_PORT = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private static final int ASK_PERMISSION = 1;
    private static final int ASK_BLUETOOTH_ON = 2;
    private static final String GITHUB = "https://github.com/mayaanmalik2486/robot-car";
    private static final String APP_LINK = GITHUB + "/raw/main/App/RobotCar.apk";

    // colours
    private static final int BG = Color.rgb(15, 23, 32);
    private static final int CARD = Color.rgb(27, 37, 48);
    private static final int GREY = Color.rgb(144, 164, 174);
    private static final int BLUE = Color.rgb(30, 136, 229);
    private static final int RED = Color.rgb(229, 57, 53);
    private static final int GREEN = Color.rgb(67, 160, 71);
    private static final int TEAL = Color.rgb(0, 137, 123);
    private static final int ORANGE = Color.rgb(245, 124, 0);
    private static final int PURPLE = Color.rgb(142, 36, 170);
    private static final int DIM = Color.rgb(69, 90, 100);

    private BluetoothAdapter adapter;
    private BluetoothSocket socket;
    private OutputStream out;
    private volatile boolean connected;
    private volatile boolean connecting;
    private volatile boolean closingOnPurpose;
    private final ExecutorService sender = Executors.newSingleThreadExecutor();
    private final Handler ui = new Handler(Looper.getMainLooper());
    private SharedPreferences prefs;

    private TextView connText;
    private TextView modeText;
    private TextView flagsText;
    private TextView alertText;
    private Button connectButton;
    private Button slowButton;
    private Button fullButton;
    private Button footballButton;
    private Button soundButton;

    // what the car told us
    private String mode = "";
    private boolean muted;
    private boolean football;
    private boolean slow;
    private boolean batteryLow;

    // ------------------------------------------------------------------ start / stop

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().setStatusBarColor(BG);
        prefs = getSharedPreferences("robotcar", MODE_PRIVATE);
        adapter = BluetoothAdapter.getDefaultAdapter();
        setContentView(buildScreen());
        showStatus();

        if (adapter == null) {
            setConn("This phone has no Bluetooth", RED);
            return;
        }

        // connect to the car we used last time, straight away
        String last = prefs.getString("device", null);
        if (last != null && hasPermission() && adapter.isEnabled())
            connect(last);
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
        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(BG);

        LinearLayout col = new LinearLayout(this);
        col.setOrientation(LinearLayout.VERTICAL);
        col.setPadding(dp(12), dp(12), dp(12), dp(28));
        scroll.addView(col);

        // title + connect button
        LinearLayout top = new LinearLayout(this);
        top.setGravity(Gravity.CENTER_VERTICAL);
        TextView title = text("🤖 Robot Car", 26, Color.WHITE, true);
        top.addView(title, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        connectButton = button("Connect", BLUE);
        connectButton.setOnClickListener(v -> onConnectPressed());
        top.addView(connectButton);
        col.addView(top);

        connText = text("Not connected - tap Connect", 14, GREY, false);
        connText.setPadding(dp(4), dp(2), dp(4), dp(8));
        col.addView(connText);

        // what the car is doing
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setBackground(round(CARD));
        card.setPadding(dp(12), dp(10), dp(12), dp(10));
        modeText = text("-", 30, Color.WHITE, true);
        modeText.setGravity(Gravity.CENTER);
        flagsText = text("", 14, GREY, false);
        flagsText.setGravity(Gravity.CENTER);
        card.addView(modeText);
        card.addView(flagsText);
        col.addView(card);

        alertText = text("", 16, Color.WHITE, true);
        alertText.setGravity(Gravity.CENTER);
        alertText.setBackground(round(RED));
        alertText.setPadding(dp(10), dp(10), dp(10), dp(10));
        alertText.setVisibility(View.GONE);
        alertText.setOnClickListener(v -> v.setVisibility(View.GONE));
        LinearLayout.LayoutParams alp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        alp.setMargins(0, dp(8), 0, 0);
        col.addView(alertText, alp);

        // driving: hold an arrow, let go = stop
        col.addView(header("Drive  (hold an arrow - let go to stop)"));
        col.addView(row(holdButton("↖", 'G'), holdButton("▲", 'F'), holdButton("↗", 'H')));
        Button stop = button("STOP", RED);
        stop.setMinHeight(dp(70));
        stop.setTextSize(20);
        stop.setOnClickListener(v -> { haptic(v); send('S'); });
        col.addView(row(holdButton("↺", 'L'), stop, holdButton("↻", 'R')));
        col.addView(row(new View(this), holdButton("▼", 'B'), new View(this)));

        slowButton = tapButton("🐢 Slow", 'Q', PURPLE);
        fullButton = tapButton("🐇 Full", 'W', PURPLE);
        col.addView(row(slowButton, fullButton, tapButton("🚀 Turbo", 'T', RED), tapButton("📯 Horn", 'Y', DIM)));

        // modes
        col.addView(header("Modes"));
        col.addView(row(tapButton("💃 Dance", '4', TEAL), tapButton("🔍 Search", 'I', TEAL), tapButton("😈 Chaos", '1', TEAL)));
        col.addView(row(tapButton("🍩 Donut", '3', TEAL), tapButton("↩ 180° turn", 'U', TEAL), tapButton("🛑 Brake", 'u', TEAL)));
        col.addView(row(tapButton("⏫ Auto forward", '2', TEAL), tapButton("⏬ Auto back", 'J', TEAL)));

        // play modes
        col.addView(header("Play"));
        col.addView(row(tapButton("🚶 Follow me", 'M', ORANGE), tapButton("🥅 Goalkeeper", 'K', ORANGE), tapButton("⚽ KICK!", 'k', RED)));
        col.addView(row(tapButton("🚨 Guard", 'A', ORANGE), tapButton("🎵 Music", 'N', ORANGE), tapButton("✋ Gestures", 'E', ORANGE)));
        col.addView(row(tapButton("🐶 Pet", 'P', ORANGE)));

        // switches (the car says if they're on)
        col.addView(header("Switches"));
        footballButton = tapButton("", 'V', DIM);
        soundButton = tapButton("", 'X', DIM);
        col.addView(row(footballButton, soundButton));

        // help
        col.addView(header("How it works"));
        TextView help = text(
            "• First time: pair the car in your phone's Bluetooth settings - it's called HC-05 (PIN 1234 or 0000). Then tap Connect.\n"
            + "• Dance: press on the FIRST BEAT of the song. Tap Dance again ON a beat if it drifts.\n"
            + "• Football: the distance sensor switches off, so it never stops for things. Search, chaos and the play modes still use it.\n"
            + "• Goalkeeper: put it in front of the goal, facing the player. KICK! shoots any time.\n"
            + "• Guard: walk away during the 5 beeps. Your phone buzzes if someone comes.\n"
            + "• Music: move your hand in front of the sensor (closer = higher).\n"
            + "• Gestures: tap = spin, two taps = wiggle, hold your hand close = it backs away, a bit further = it comes to you.\n"
            + "• Pet: it wanders, sits, gets bored and falls asleep - hold your hand in front of it to pet it.\n"
            + "• Any other button stops a mode. STOP stops everything.",
            14, GREY, false);
        help.setLineSpacing(dp(3), 1f);
        col.addView(help);

        Button share = button("📤 Share this app", BLUE);
        share.setOnClickListener(v -> shareApp());
        LinearLayout.LayoutParams slp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        slp.setMargins(dp(4), dp(16), dp(4), dp(4));
        col.addView(share, slp);

        TextView footer = text("Robot Car app 1.0  •  " + GITHUB.replace("https://", ""), 12, GREY, false);
        footer.setGravity(Gravity.CENTER);
        footer.setPadding(0, dp(8), 0, 0);
        footer.setOnClickListener(v -> openLink(GITHUB));
        col.addView(footer);

        return scroll;
    }

    // a button you hold: the letter when you press it, S (stop) when you let go
    private Button holdButton(String label, final char letter) {
        Button b = button(label, BLUE);
        b.setMinHeight(dp(70));
        b.setTextSize(26);
        b.setOnTouchListener((v, e) -> {
            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    v.setPressed(true);
                    haptic(v);
                    send(letter);
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    v.setPressed(false);
                    sendQuietly('S');
                    ui.postDelayed(() -> sendQuietly('S'), 60);     // twice, in case one gets lost
                    break;
            }
            return true;
        });
        return b;
    }

    // a button you tap: sends its letter once
    private Button tapButton(String label, final char letter, int color) {
        Button b = button(label, color);
        b.setMinHeight(dp(56));
        b.setOnClickListener(v -> { haptic(v); send(letter); });
        return b;
    }

    private Button button(String label, int color) {
        Button b = new Button(this);
        b.setText(label);
        b.setAllCaps(false);
        b.setTextColor(Color.WHITE);
        b.setTextSize(15);
        b.setTypeface(Typeface.DEFAULT_BOLD);
        b.setPadding(dp(6), dp(10), dp(6), dp(10));
        b.setStateListAnimator(null);
        paint(b, color);
        return b;
    }

    private void paint(Button b, int color) {
        StateListDrawable bg = new StateListDrawable();
        bg.addState(new int[]{android.R.attr.state_pressed}, round(darker(color)));
        bg.addState(new int[]{}, round(color));
        b.setBackground(bg);
    }

    private TextView text(String s, float size, int color, boolean bold) {
        TextView t = new TextView(this);
        t.setText(s);
        t.setTextSize(size);
        t.setTextColor(color);
        if (bold)
            t.setTypeface(Typeface.DEFAULT_BOLD);
        return t;
    }

    private TextView header(String s) {
        TextView t = text(s, 15, Color.WHITE, true);
        t.setPadding(dp(4), dp(18), dp(4), dp(4));
        return t;
    }

    private LinearLayout row(View... views) {
        LinearLayout r = new LinearLayout(this);
        r.setOrientation(LinearLayout.HORIZONTAL);
        for (View v : views) {
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
            lp.setMargins(dp(4), dp(4), dp(4), dp(4));
            r.addView(v, lp);
        }
        return r;
    }

    private GradientDrawable round(int color) {
        GradientDrawable g = new GradientDrawable();
        g.setColor(color);
        g.setCornerRadius(dp(16));
        return g;
    }

    private static int darker(int c) {
        return Color.rgb(Color.red(c) * 7 / 10, Color.green(c) * 7 / 10, Color.blue(c) * 7 / 10);
    }

    private int dp(float v) {
        return Math.round(v * getResources().getDisplayMetrics().density);
    }

    // shows what the car told us
    private void showStatus() {
        modeText.setText(connected ? (mode.isEmpty() ? "..." : mode) : "-");
        flagsText.setText(!connected ? "Connect to see what the car is doing"
            : (muted ? "🔇 Muted" : "🔊 Sound on")
            + "   " + (football ? "⚽ Sensor off" : "📡 Sensor on")
            + "   " + (slow ? "🐢 Slow" : "🐇 Full")
            + "   " + (batteryLow ? "🪫 Battery LOW" : "🔋 Battery OK"));
        footballButton.setText(football ? "⚽ Football: ON" : "⚽ Football: OFF");
        paint(footballButton, football ? GREEN : DIM);
        soundButton.setText(muted ? "🔇 Sound: OFF" : "🔊 Sound: ON");
        paint(soundButton, muted ? DIM : GREEN);
        paint(slowButton, connected && slow ? GREEN : PURPLE);
        paint(fullButton, connected && !slow ? GREEN : PURPLE);
    }

    private void setConn(String s, int color) {
        connText.setText(s);
        connText.setTextColor(color);
    }

    private void alert(String s, boolean urgent) {
        alertText.setText(s + "  (tap to hide)");
        alertText.setVisibility(View.VISIBLE);
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

    private void openLink(String url) {
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
        } catch (Exception e) {
            toast(url);
        }
    }

    private void shareApp() {
        Intent i = new Intent(Intent.ACTION_SEND);
        i.setType("text/plain");
        i.putExtra(Intent.EXTRA_TEXT, "The Robot Car app (Android): " + APP_LINK
            + "\nOpen the link, download, then tap the file to install. More: " + GITHUB);
        startActivity(Intent.createChooser(i, "Share the Robot Car app"));
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

    // the paired devices: pick the car (the HC-05 is listed first)
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
                .setMessage("First pair the car in your phone's Bluetooth settings: it's called HC-05 (PIN 1234 or 0000). Then come back and tap Connect.")
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
        setConn("Connecting...", Color.YELLOW);
        connectButton.setText("Cancel");

        new Thread(() -> {
            BluetoothSocket s = null;
            boolean wasConnected = false;
            try {
                final BluetoothDevice car = adapter.getRemoteDevice(address);
                try {
                    s = car.createRfcommSocketToServiceRecord(SERIAL_PORT);
                    socket = s;                         // (so Cancel can stop it)
                    s.connect();
                } catch (IOException first) {
                    closeQuietly(s);
                    if (closingOnPurpose)               // you pressed Cancel
                        throw first;
                    // some HC-05s only answer on channel 1
                    Method m = car.getClass().getMethod("createRfcommSocket", int.class);
                    s = (BluetoothSocket) m.invoke(car, 1);
                    socket = s;
                    s.connect();
                }

                socket = s;
                out = s.getOutputStream();
                connected = true;
                connecting = false;
                wasConnected = true;
                prefs.edit().putString("device", address).apply();
                final String name = nameOf(car);
                ui.post(() -> {
                    setConn("Connected to " + name, GREEN);
                    connectButton.setText("Disconnect");
                    mode = "";
                    showStatus();
                });
                sendQuietly('?');                       // ask the car what it is doing
                readLines(s.getInputStream());
            } catch (Exception e) {
                closeQuietly(s);
            }

            connected = false;
            connecting = false;
            final boolean lost = wasConnected && !closingOnPurpose;
            final boolean failed = !wasConnected && !closingOnPurpose;
            ui.post(() -> {
                connectButton.setText("Connect");
                if (lost)
                    setConn("Connection lost - tap Connect", RED);
                else if (failed)
                    setConn("Could not connect - is the car switched on?", RED);
                else
                    setConn("Not connected - tap Connect", GREY);
                showStatus();
            });
        }).start();
    }

    private void disconnect() {
        closingOnPurpose = true;
        closeQuietly(socket);
    }

    // reads the car's status lines until the connection ends
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
            toast("Not connected - tap Connect first");
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
