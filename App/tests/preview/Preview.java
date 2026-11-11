package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.os.SystemClock;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.TextView;

import java.awt.Graphics2D;
import java.awt.image.BufferedImage;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

import javax.imageio.ImageIO;

/**
 * Draws the app's main screen to PNG pictures on the PC - with the app's REAL view code
 * (SceneView, WheelView, StickView, PadButton, Boards, RadarView, ControllerView, Layout),
 * on stand-in Android classes that draw with Java 2D (make-shim.js). So the look can be
 * checked without a phone, and every drawing path has really run once.
 *
 * It builds the screen the same way MainActivity.buildScreen does. Not pixel-perfect: the
 * fonts are the PC's, emoji have no colour, and nothing here can be tapped.
 * Run: powershell -ExecutionPolicy Bypass -File App\tests\preview\make-previews.ps1
 */
public final class Preview {

    static int checks, bad;

    static void check(String name, boolean ok, String detail) {
        checks++;
        if (!ok)
            bad++;
        System.out.println("  " + (ok ? "PASS" : "FAIL") + "  " + name + (ok ? "" : "  [" + detail + "]"));
    }

    public static void main(String[] args) throws Exception {
        File out = new File(args.length > 0 ? args[0] : ".");
        out.mkdirs();
        render(out, "phone-wheel-driving", 800, 400, Layout.TWO_STICKS, "drive");
        render(out, "small-phone-wheel", 640, 360, Layout.TWO_STICKS, "ready");
        render(out, "phone-one-stick-dance", 800, 400, Layout.ONE_STICK, "dance");
        render(out, "tablet-wheel-radar", 1280, 800, Layout.TWO_STICKS, "radar");
        render(out, "upright-blocked", 600, 900, Layout.TWO_STICKS, "blocked");
        render(out, "phone-not-connected", 800, 400, Layout.TWO_STICKS, "off");
        render(out, "phone-tilt", 760, 360, Layout.TILT, "ready");
        controller(out);
        controls();
        System.out.println(bad == 0 ? "\nALL " + checks + " PREVIEW CHECKS PASSED - pictures in " + out : "\n" + bad + " PREVIEW CHECK(S) FAILED");
        System.exit(bad == 0 ? 0 : 1);
    }

    static PadButton key(Context c, CharSequence text, float size) {
        PadButton b = new PadButton(c, text, PadButton.GLASS, false);
        b.setTextSize(size);
        return b;
    }

    static PadButton action(Context c, String name, int hue, float size, boolean dark) {
        PadButton b = key(c, name, size);
        b.setHue(hue);
        b.setDarkText(dark);
        b.setAutoRound(true);
        return b;
    }

    static View grid(Context c, Object[][] items, List<PadButton> all) {
        Boards.Grid g = new Boards.Grid(c);
        for (Object[] it : items) {
            PadButton b = key(c, ((String) it[1]).toUpperCase(), 13);
            b.setChip((String) it[0], (Integer) it[2]);
            all.add(b);
            g.addView(b);
        }
        return g;
    }

    static void render(File out, String name, int wDp, int hDp, final int style, String state) throws Exception {
        Context c = new Context();
        float d = c.getResources().getDisplayMetrics().density;
        int w = Math.round(wDp * d), h = Math.round(hDp * d);
        boolean on = !state.equals("off");
        CarState car = new CarState();
        long now = SystemClock.now;

        Boards.Screen board = new Boards.Screen(c, () -> style);
        PadButton connect = key(c, "", 13);
        connect.setHue(on ? PadButton.GLASS : Ui.HUE_ORANGE);
        connect.setText(on ? Ui.twoLines("● HC-05", "CONNECTED", Ui.ACCENT2) : Ui.twoLines("CONNECT", "TO THE CAR", 0xD9FFFFFF));
        connect.setOn(on);
        board.put(Layout.CONNECT, connect);
        TextView song = new TextView(c);
        song.setTextColor(Ui.LABEL);
        song.setTextSize(12);
        song.setTypeface(Ui.MONO);
        song.setGravity(android.view.Gravity.CENTER_VERTICAL | android.view.Gravity.LEFT);
        song.setText(on ? "♪ Robot Groove" : "");
        board.put(Layout.SONG, song);
        PadButton pad = key(c, "", 12);
        board.put(Layout.PAD, pad);
        PadButton speed = key(c, Ui.twoLines("FAST", "TOP SPEED"), 15);
        board.put(Layout.SPEED, speed);
        PadButton styleB = key(c, Ui.twoLines(style == Layout.TWO_STICKS ? "WHEEL" : style == Layout.TILT ? "TILT" : "1 STICK", "DRIVING"), 15);
        board.put(Layout.STYLE, styleB);
        PadButton gear = key(c, "", 14);
        gear.setIcon(PadButton.ICON_GEAR);
        board.put(Layout.SETTINGS, gear);
        PadButton help = key(c, "", 14);
        help.setIcon(PadButton.ICON_HELP);
        board.put(Layout.HELP, help);

        SceneView scene = new SceneView(c);
        scene.setCar(car);
        board.put(Layout.LCD, scene);

        int page = state.equals("radar") ? 2 : state.equals("dance") ? 1 : 0;
        String[] names = {"MOVES", "PLAY", "RADAR"}, tabKeys = {Layout.TAB0, Layout.TAB1, Layout.TAB2};
        for (int i = 0; i < 3; i++) {
            PadButton t = key(c, names[i], 15);
            t.setFlat(true);
            t.setLetterSpacing(0.12f);
            t.setTextColor(Ui.LABEL);
            t.setOn(i == page);
            board.put(tabKeys[i], t);
        }
        board.setTab(page);

        List<PadButton> modes = new ArrayList<>();
        FrameLayout stack = new FrameLayout(c);
        View[] pages = new View[3];
        pages[0] = grid(c, new Object[][]{{"🔍", "Search", 210}, {"😈", "Chaos", 275}, {"🍩", "Donut", 330}, {"↩", "180°", 186},
            {"⏫", "Auto", 145}, {"⏬", "Reverse", 38}, {"🅿", "Brake", 355}, {"📯", "Horn", 48}}, modes);
        pages[1] = grid(c, new Object[][]{{"🚶", "Follow me", 150}, {"🥅", "Goalkeeper", 215}, {"🚨", "Guard", 2},
            {"🎵", "Music", 265}, {"✋", "Gestures", 30}, {"🐶", "Pet", 320}}, modes);
        RadarView radar = new RadarView(c);
        radar.setState(car);
        PadButton radarB = key(c, state.equals("radar") ? "STOP RADAR" : "START RADAR", 14);
        radarB.setHue(Ui.HUE_TEAL);
        radarB.setDarkText(true);
        pages[2] = new Boards.RadarPage(c, radar, radarB);
        for (int i = 0; i < 3; i++) {
            stack.addView(pages[i]);
            pages[i].setVisibility(i == page ? View.VISIBLE : View.GONE);
        }
        board.put(Layout.PAGES, stack);

        final float[] got = new float[3];
        StickView stick = new StickView(c, new StickView.Listener() {
            @Override
            public void onStick(StickView s, float x, float y) {
                got[0] = x;
                got[1] = y;
            }

            @Override
            public void onHold(boolean holding) {
            }
        });
        stick.setKind(style == Layout.TWO_STICKS ? StickView.LEVER : StickView.STICK);
        stick.setTiltMode(style == Layout.TILT);
        board.put(Layout.STICK, stick);
        WheelView wheel = new WheelView(c, steer -> got[2] = steer);
        board.put(Layout.STICK2, wheel);

        PadButton stop = action(c, "STOP", Ui.HUE_STOP, 21, false);
        stop.setLetterSpacing(0.16f);
        board.put(Layout.STOP, stop);
        PadButton dance = action(c, "DANCE", Ui.HUE_PINK, 15, false);
        board.put(Layout.DANCE, dance);
        PadButton kick = action(c, "KICK", Ui.HUE_BLUE, 15, false);
        board.put(Layout.KICK, kick);
        board.put(Layout.HORN, action(c, "HORN", Ui.HUE_YELLOW, 15, true));
        board.put(Layout.TURBO, action(c, "TURBO", Ui.HUE_TEAL, 15, true));

        // ----- the state to show
        String[] flags = {"SOUND", "SENSOR", "FAST", "BATTERY"};
        boolean[] good = {true, true, true, true};
        float l = 0, r = 0;
        String mode = "";
        switch (state) {
            case "drive":
                l = 1f;
                r = 0.45f;
                mode = "DRIVE";
                scene.set("CONNECTED · STEERING", false, Ui.GOOD, false, "DRIVE 100%", flags, good);
                stick.show(0, -0.85f);
                wheel.show(0.45f);
                for (PadButton b : modes)
                    b.setBadge(null);
                dance.setBadge("A");
                kick.setBadge("X");
                stop.setBadge("B");
                modes.get(0).setBadge("▲");
                break;
            case "dance":
                mode = "DANCE";
                scene.set("CONNECTED · STEERING", false, Ui.GOOD, false, "DANCE 0:12", flags, good);
                dance.setOn(true);
                modes.get(8 + 3).setOn(true);
                kick.setPressed(true);
                break;
            case "radar":
                mode = "RADAR";
                car.read("~RADAR|0000", now);
                String[] dots = {"#R:a120", "#R:b96", "#R:c999", "#R:d60", "#R:e45", "#R:f42", "#R:g38", "#R:h70", "#R:i999", "#R:j110"};
                for (String line : dots)
                    car.read(line, now);
                scene.set("CONNECTED · STEERING", false, Ui.GOOD, false, "RADAR 38 cm", flags, good);
                break;
            case "blocked":
                mode = "BLOCKED";
                l = r = 0.7f;
                scene.set("CONNECTED · STEERING", false, Ui.GOOD, false, "BLOCKED", new String[]{"MUTED", "SENSOR", "SLOW", "LOW BATTERY"}, new boolean[]{false, true, true, false});
                stick.show(0, -0.6f);
                break;
            case "off":
                scene.set("NOT CONNECTED · TAP TO CONNECT", false, Ui.BAD, true, "ROBOT CAR", new String[0], new boolean[0]);
                break;
            default:
                mode = "READY";
                scene.set("CONNECTED · STEERING", false, Ui.GOOD, false, "READY", flags, good);
                break;
        }
        scene.drive(l, r, mode, on, on);

        board.measure(w, h);
        board.layout(0, 0, w, h);
        board.measure(w, h);                                   // (a second pass, like Android does: sizes are known now)
        board.layout(0, 0, w, h);
        BufferedImage img = new BufferedImage(w, h, BufferedImage.TYPE_INT_ARGB);
        // let the scene, the wheel and the lever run for 2 seconds of frames, then keep the last one
        for (int frame = 0; frame < 120; frame++) {
            SystemClock.now += 16;
            Graphics2D g = img.createGraphics();
            board.draw(new Canvas(g));
            g.dispose();
        }
        File f = new File(out, name + ".png");
        ImageIO.write(img, "png", f);
        check(name + " (" + wDp + "x" + hDp + " dp) drawn", f.length() > 20000, f.length() + " bytes");
    }

    // the controller screen's drawing, with a few buttons held
    static void controller(File out) throws Exception {
        Context c = new Context();
        ControllerInput pad = new ControllerInput(new ControllerInput.Listener() {
            @Override public void onPadJob(char job, int button) { }
            @Override public void onPadButton(int button, boolean down) { }
            @Override public void onPadDrive(float throttle, float steer) { }
            @Override public void onPadChanged() { }
            @Override public void onPadDevice(android.view.InputDevice d) { }
        });
        pad.enabled = false;
        pad.onKey(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_A));
        pad.onKey(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_UP));
        MotionEvent m = new MotionEvent(MotionEvent.ACTION_MOVE, 0, 0);
        m.axes[MotionEvent.AXIS_Y] = -0.8f;
        m.axes[MotionEvent.AXIS_Z] = 0.6f;
        m.axes[MotionEvent.AXIS_RTRIGGER] = 0.75f;
        pad.onMotion(m);
        check("controller: left stick up = throttle +0.77 after the dead zone, right stick = steering", Math.abs(pad.throttle() - 0.773f) < 0.01f && pad.steer() > 0.5f,
            pad.throttle() + " / " + pad.steer());
        check("controller: A, D-pad up and RT (75 %) are down", pad.buttons.down(ControllerMap.A) && pad.buttons.down(ControllerMap.UP) && pad.buttons.down(ControllerMap.RT), "");
        ControllerView v = new ControllerView(c, pad, button -> { });
        int w = 1600, h = 760;
        v.layout(0, 0, w, h);
        BufferedImage img = new BufferedImage(w, h, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = img.createGraphics();
        g.setColor(new java.awt.Color(Ui.BG));
        g.fillRect(0, 0, w, h);
        v.draw(new Canvas(g));
        g.dispose();
        File f = new File(out, "controller-screen.png");
        ImageIO.write(img, "png", f);
        check("controller-screen drawn", f.length() > 20000, f.length() + " bytes");
    }

    // the lever and the wheel, driven with pretend fingers
    static void controls() {
        Context c = new Context();
        final float[] got = new float[3];
        final List<Float> steers = new ArrayList<>();
        StickView lever = new StickView(c, new StickView.Listener() {
            @Override
            public void onStick(StickView s, float x, float y) {
                got[0] = x;
                got[1] = y;
            }

            @Override
            public void onHold(boolean holding) {
            }
        });
        lever.setKind(StickView.LEVER);
        lever.layout(0, 0, 336, 600);
        float cy = (600 - 44) / 2f;
        lever.onTouchEvent(new MotionEvent(MotionEvent.ACTION_DOWN, 168, cy));
        check("lever: touched in the middle = 0", got[1] == 0 && got[0] == 0, got[0] + "," + got[1]);
        lever.onTouchEvent(new MotionEvent(MotionEvent.ACTION_MOVE, 30, cy - 1000));
        check("lever: pushed far up = full forward (y = -1), never sideways", got[1] == -1 && got[0] == 0, got[0] + "," + got[1]);
        lever.onTouchEvent(new MotionEvent(MotionEvent.ACTION_MOVE, 168, cy + 60));
        check("lever: pulled down a little = a little backward", got[1] > 0.1f && got[1] < 0.5f, "" + got[1]);
        lever.onTouchEvent(new MotionEvent(MotionEvent.ACTION_UP, 168, cy + 60));
        check("lever: let go = 0 at once", got[1] == 0, "" + got[1]);

        WheelView wheel = new WheelView(c, s -> {
            got[2] = s;
            steers.add(s);
        });
        wheel.layout(0, 0, 600, 600);
        float wx = 300, wy = (600 - 44) / 2f, rad = 200;
        wheel.onTouchEvent(new MotionEvent(MotionEvent.ACTION_DOWN, wx, wy - rad));          // grab it at the top
        for (int deg = 2; deg <= 60; deg += 2)                                               // and turn it 60 degrees clockwise
            wheel.onTouchEvent(new MotionEvent(MotionEvent.ACTION_MOVE, wx + rad * (float) Math.sin(Math.toRadians(deg)), wy - rad * (float) Math.cos(Math.toRadians(deg))));
        check("wheel: turned 60 degrees right = steering +0.48 (60 of 126 degrees)", Math.abs(got[2] - 60 / 126.05f) < 0.01f, "" + got[2]);
        for (int deg = 60; deg <= 300; deg += 2)
            wheel.onTouchEvent(new MotionEvent(MotionEvent.ACTION_MOVE, wx + rad * (float) Math.sin(Math.toRadians(deg)), wy - rad * (float) Math.cos(Math.toRadians(deg))));
        check("wheel: turned on and on = it stops at full right (+1), it never wraps round", got[2] == 1f, "" + got[2]);
        for (int deg = 300; deg >= -200; deg -= 2)
            wheel.onTouchEvent(new MotionEvent(MotionEvent.ACTION_MOVE, wx + rad * (float) Math.sin(Math.toRadians(deg)), wy - rad * (float) Math.cos(Math.toRadians(deg))));
        check("wheel: turned all the way back the other way = full left (-1)", got[2] == -1f, "" + got[2]);
        boolean smooth = true;
        for (int i = 1; i < steers.size(); i++)
            if (Math.abs(steers.get(i) - steers.get(i - 1)) > 0.05f)
                smooth = false;
        check("wheel: it never jumps (every step under 5 %)", smooth, "");
        wheel.onTouchEvent(new MotionEvent(MotionEvent.ACTION_UP, wx, wy));
        check("wheel: let go = straight at once", got[2] == 0, "" + got[2]);
    }

    private Preview() {
    }
}
