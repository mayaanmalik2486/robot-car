package io.github.mayaanmalik2486.robotcar;

import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;

/**
 * Tests for the parts of the app that need no phone: the car's language, what the car says,
 * the driving brain, the controller map and the button state.
 * Run: powershell -ExecutionPolicy Bypass -File App\tests\run-tests.ps1
 */
public final class LogicTests {

    static int passed, failed;

    static void check(String name, boolean ok, String detail) {
        if (ok) {
            passed++;
            System.out.println("  PASS  " + name);
        } else {
            failed++;
            System.out.println("  FAIL  " + name + "  [" + detail + "]");
        }
    }

    static void check(String name, boolean ok) {
        check(name, ok, "");
    }

    /** Remembers everything the driving brain sends, with the time. */
    static final class Wire implements DriveLogic.Out {
        final List<Integer> codes = new ArrayList<>();
        final List<Long> times = new ArrayList<>();
        long now;

        @Override
        public void send(int code) {
            codes.add(code);
            times.add(now);
        }

        /** Everything sent so far as readable text: letters as they are, steering codes as L+31 / R-9. */
        String text() {
            return text(0);
        }

        String text(int from) {
            StringBuilder s = new StringBuilder();
            for (int i = from; i < codes.size(); i++) {
                int c = codes.get(i);
                if (s.length() > 0)
                    s.append(' ');
                if (c >= 0x80) {
                    int v = (c & 0x3F) - 32;
                    s.append((c & 0x40) != 0 ? 'R' : 'L').append(v >= 0 ? "+" : "").append(v);
                } else {
                    s.append((char) c);
                }
            }
            return s.toString();
        }

        int count() {
            return codes.size();
        }
    }

    /** Runs the brain's clock from 'from' to 'to' (ms), ticking every 20 ms like the app does. */
    static void run(DriveLogic d, Wire w, long from, long to) {
        for (long t = from; t <= to; t += 20) {
            w.now = t;
            d.tick(t);
        }
    }

    static void input(DriveLogic d, Wire w, float throttle, float steer, long now) {
        w.now = now;
        d.input(throttle, steer, now);
    }

    public static void main(String[] args) throws Exception {
        String repo = args.length > 0 ? args[0] : ".";
        car();
        carState(new File(repo, "Tools/build/app-session-lines.txt"));
        mixing();
        steering();
        parking();
        letters();
        stopping();
        controllerMap();
        padState();
        replay(new File(repo, "Tools/build/app-replay.json"));
        System.out.println();
        System.out.println(failed == 0 ? "ALL " + passed + " CHECKS PASSED" : failed + " CHECK(S) FAILED, " + passed + " passed");
        System.exit(failed == 0 ? 0 : 1);
    }

    // ------------------------------------------------------------------ the car's language

    static void car() {
        System.out.println("\n=== Car: letters and steering codes");
        check("stick up = F", Car.direction(0, -1) == 'F');
        check("stick down = B", Car.direction(0, 1) == 'B');
        check("stick left = L, right = R", Car.direction(-1, 0) == 'L' && Car.direction(1, 0) == 'R');
        check("up-left = G, up-right = H", Car.direction(-0.7f, -0.7f) == 'G' && Car.direction(0.7f, -0.7f) == 'H');
        check("down-left and down-right = B (no backward curves)", Car.direction(-0.7f, 0.7f) == 'B' && Car.direction(0.7f, 0.7f) == 'B');
        check("stick near the middle = S", Car.direction(0.2f, -0.2f) == 'S' && Car.direction(0, 0) == 'S');
        check("just past the middle (0.36) = F", Car.direction(0, -0.36f) == 'F');
        check("steering code: left stop = 160, right stop = 224", Car.wheel(false, 0) == 160 && Car.wheel(true, 0) == 224,
            Car.wheel(false, 0) + " " + Car.wheel(true, 0));
        check("steering code: left full forward = 191, right full forward = 255", Car.wheel(false, 31) == 191 && Car.wheel(true, 31) == 255);
        check("steering code: left full backward = 129, right full backward = 193", Car.wheel(false, -31) == 129 && Car.wheel(true, -31) == 193);
        check("steering codes never collide with a letter (always above 127) and never send speed -32",
            Car.wheel(false, -99) == 129 && Car.wheel(true, 99) == 255 && Car.wheel(false, -31) > 127);
        check("mode letters take the wheels; horn, mute, football, speed do not",
            Car.takesTheWheels('4') && Car.takesTheWheels('I') && Car.takesTheWheels('T') && Car.takesTheWheels('O')
                && !Car.takesTheWheels('Y') && !Car.takesTheWheels('X') && !Car.takesTheWheels('V') && !Car.takesTheWheels('Q') && !Car.takesTheWheels('?'));
    }

    // ------------------------------------------------------------------ what the car says

    static void carState(File lines) throws Exception {
        System.out.println("\n=== CarState: reading the car's lines");
        CarState s = new CarState();
        check("status line", s.read("~FORWARD|0010", 100) == CarState.STATUS && s.mode.equals("FORWARD") && s.slow && !s.muted && !s.football && !s.batteryLow);
        check("all four flags", s.read("~READY|1101", 200) == CarState.STATUS && s.muted && s.football && !s.slow && s.batteryLow);
        check("the mode clock restarts when the mode changes", s.read("~DANCE|0000", 900) == CarState.STATUS && s.modeSince == 900);
        check("... and not when it stays", s.read("~DANCE|0100", 1500) == CarState.STATUS && s.modeSince == 900);
        check("a mode with a space or symbols", s.read("~PURR <3|0000", 1) == CarState.STATUS && s.mode.equals("PURR <3"));
        check("song line", s.read("#S:Robot Groove", 1) == CarState.SONG && s.song.equals("Robot Groove"));
        check("no steering until the car says so", !s.steers);
        check("#V:2 = the car knows steering", s.read("#V:2", 1) == CarState.VERSION && s.steers);
        check("radar line: f = straight ahead", s.read("#R:f42", 5000) == CarState.RADAR && s.radarCm[5] == 42 && s.radarLook == 5);
        check("radar line: nothing seen (999) is not 'near'", s.read("#R:a999", 5100) == CarState.RADAR && s.radarNearest(5200) == 42);
        check("old radar dots are forgotten after 4 s", s.radarNearest(9500) == -1);
        check("broken lines are ignored", s.read("~READY", 1) == CarState.NOTHING && s.read("~READY|01", 1) == CarState.NOTHING
            && s.read("#R:z5", 1) == CarState.NOTHING && s.read("#R:fxx", 1) == CarState.NOTHING && s.read("hello", 1) == CarState.NOTHING
            && s.read("", 1) == CarState.NOTHING && s.read("#V:x", 1) == CarState.NOTHING);
        s.reset();
        check("a new connection forgets everything", s.mode.isEmpty() && s.song.isEmpty() && !s.steers && s.radarNearest(5200) == -1);

        if (!lines.exists()) {
            check("the simulated car's real lines (run Tools\\tools\\test-all.ps1 -SkipSongs first)", false, "missing " + lines);
            return;
        }
        // every line the real car program sent in the simulator's app session
        List<String> real = Files.readAllLines(lines.toPath(), StandardCharsets.UTF_8);
        int status = 0, radar = 0, song = 0, version = 0, unknown = 0;
        List<String> modes = new ArrayList<>();
        long now = 0;
        for (String line : real) {
            if (line.trim().isEmpty())
                continue;
            int kind = s.read(line.trim(), now += 100);
            if (kind == CarState.STATUS) {
                status++;
                if (!modes.contains(s.mode))
                    modes.add(s.mode);
            } else if (kind == CarState.RADAR)
                radar++;
            else if (kind == CarState.SONG)
                song++;
            else if (kind == CarState.VERSION)
                version++;
            else
                unknown++;
        }
        check("the simulated car's session: every line understood", unknown == 0 && status >= 8 && radar >= 5 && song == 1 && version == 1,
            status + " status, " + radar + " radar, " + song + " song, " + version + " version, " + unknown + " unknown");
        check("... with the modes READY, SEARCH, FOLLOW, RADAR and PET", modes.contains("READY") && modes.contains("SEARCH")
            && modes.contains("FOLLOW") && modes.contains("RADAR") && modes.contains("PET"), modes.toString());
        check("... the car knows steering, and its song is Robot Groove", s.steers && s.song.equals("Robot Groove"));
    }

    // ------------------------------------------------------------------ throttle + steering -> two sides

    static String mixed(float t, float s, boolean slow, float feel) {
        int[] o = new int[2];
        DriveLogic.mix(t, s, slow, feel, o);
        return o[0] + "," + o[1];
    }

    static void mixing() {
        System.out.println("\n=== Mixing: throttle + steering -> the speed of each side");
        check("nothing held = both sides 0", mixed(0, 0, false, 0.6f).equals("0,0"));
        check("full throttle = both sides 31", mixed(1, 0, false, 0.6f).equals("31,31"), mixed(1, 0, false, 0.6f));
        check("full backward = both sides -31", mixed(-1, 0, false, 0.6f).equals("-31,-31"));
        check("a touch of throttle already turns the motors (the weakest speed is 11, not 1)", mixed(0.05f, 0, false, 0.6f).equals("12,12"), mixed(0.05f, 0, false, 0.6f));
        check("half throttle = about half-way between weakest and full", mixed(0.5f, 0, false, 0.6f).equals("21,21"), mixed(0.5f, 0, false, 0.6f));
        check("full throttle + full right: left full, right backward at 30 %", mixed(1, 1, false, 0.6f).equals("31,-9"), mixed(1, 1, false, 0.6f));
        check("full throttle + full left: mirrored", mixed(1, -1, false, 0.6f).equals("-9,31"), mixed(1, -1, false, 0.6f));
        check("no throttle + full right: spin on the spot at 85 %", mixed(0, 1, false, 0.6f).equals("28,-28"), mixed(0, 1, false, 0.6f));
        check("no throttle + full left: spin the other way", mixed(0, -1, false, 0.6f).equals("-28,28"));
        int[] gentle = new int[2], hard = new int[2];
        DriveLogic.mix(1, 0.3f, false, 0.6f, gentle);
        DriveLogic.mix(1, 0.7f, false, 0.6f, hard);
        check("a little steering = a wide bend (inner side still forward), more steering = tighter",
            gentle[0] == 31 && gentle[1] > 15 && hard[0] == 31 && hard[1] < gentle[1] && hard[1] >= -9, gentle[1] + " then " + hard[1]);
        boolean smooth = true;
        int last = 31;
        int[] o = new int[2];
        for (int i = 0; i <= 100; i++) {
            DriveLogic.mix(1, i / 100f, false, 0.6f, o);
            if (o[0] != 31 || o[1] > last || last - o[1] > 3)
                smooth = false;
            last = o[1];
        }
        check("steering from 0 to full is smooth: the inner side slows in small steps, never jumps", smooth);
        check("backward + steering right: the car still turns clockwise (right side faster backward)", mixed(-1, 1, false, 0.6f).equals("9,-31"), mixed(-1, 1, false, 0.6f));
        check("slow switch: full throttle = 18 (the car's slow speed)", mixed(1, 0, true, 0.6f).equals("18,18"), mixed(1, 0, true, 0.6f));
        check("steering feel 0 = direct: half steering turns harder than with feel 1",
            Integer.parseInt(mixed(1, 0.5f, false, 0f).split(",")[1]) < Integer.parseInt(mixed(1, 0.5f, false, 1f).split(",")[1]));
        boolean inRange = true;
        for (float t = -1; t <= 1.001f; t += 0.1f)
            for (float st = -1; st <= 1.001f; st += 0.1f) {
                DriveLogic.mix(t, st, false, 0.6f, o);
                if (Math.abs(o[0]) > 31 || Math.abs(o[1]) > 31)
                    inRange = false;
            }
        check("no stick position ever asks for more than 31", inRange);

        check("round dead zone: inside it = 0", DriveLogic.deadZone(0.1f, 0.05f, 0.14f) == 0);
        check("round dead zone: just outside starts from 0 (no jump)", DriveLogic.deadZone(0.15f, 0, 0.14f) * 0.15f < 0.02f);
        check("round dead zone: fully pushed = 1", Math.abs(DriveLogic.deadZone(1f, 0, 0.14f) - 1f) < 0.001f);
        check("round dead zone: a stick that goes past 1 is held at 1", Math.abs(DriveLogic.deadZone(0.9f, 0.9f, 0.14f) * (float) Math.hypot(0.9, 0.9) - 1f) < 0.001f);
        check("one-axis dead zone", DriveLogic.deadZone(0.1f, 0.14f) == 0 && DriveLogic.deadZone(-1f, 0.14f) == -1f && DriveLogic.deadZone(0.57f, 0.14f) == 0.5f);
    }

    // ------------------------------------------------------------------ steering (the new car program)

    static void steering() {
        System.out.println("\n=== Driving with steering codes");
        Wire w = new Wire();
        DriveLogic d = new DriveLogic(w);
        d.setSteering(true);
        d.reset();

        run(d, w, 0, 3000);
        check("resting sticks send nothing at all", w.count() == 0, w.text());

        input(d, w, 1, 0, 3000);
        check("full throttle: both sides full, at once", w.text().equals("L+31 R+31"), w.text());
        run(d, w, 3020, 3180);
        check("nothing more while the stick is held still (first 0.18 s)", w.count() == 2, w.text());
        run(d, w, 3200, 4000);
        check("the 0.2 s beat: both sides again, 5 times a second", w.count() == 2 + 5 * 2 && w.text(2).equals("L+31 R+31 L+31 R+31 L+31 R+31 L+31 R+31 L+31 R+31"), w.text(2));

        int before = w.count();
        input(d, w, 1, 1, 4070);
        check("steering right: only the side that changed is sent", w.text(before).equals("R-9"), w.text(before));

        before = w.count();
        input(d, w, 0, 0, 4100);
        check("let go: both sides 0 at once", w.text(before).equals("L+0 R+0"), w.text(before));
        run(d, w, 4120, 4140);
        check("no S yet after 40 ms", w.count() == before + 2);
        run(d, w, 4160, 6000);
        check("then one S (60 ms later), then silence", w.text(before).equals("L+0 R+0 S") && w.times.get(w.count() - 1) == 4160, w.text(before));
        check("the brain knows it is not driving any more", !d.driving());

        // a thumb sweeping the stick about for 2 seconds
        before = w.count();
        for (long t = 7000; t < 9000; t += 4) {
            double a = (t - 7000) / 150.0;
            input(d, w, (float) (0.6 + 0.4 * Math.sin(a)), (float) Math.cos(a * 1.7), t);
        }
        int sent = w.count() - before;
        check("a wildly moving stick: never more than 2 codes per 60 ms (" + sent + " codes in 2 s)", sent <= 2 * (2000 / 60 + 2) && sent > 20, "" + sent);
        boolean gaps = true;
        for (int i = before + 1; i < w.count(); i++) {
            long gap = w.times.get(i) - w.times.get(i - 1);
            if (gap != 0 && gap < 60)
                gaps = false;
        }
        check("... and never two changes closer than 60 ms", gaps);

        before = w.count();
        input(d, w, 0, 0, 9000);
        run(d, w, 9020, 9040);
        input(d, w, 0.5f, 0, 9050);
        run(d, w, 9060, 9500);
        check("let go and push again within 60 ms: the S is dropped (it would stop the new drive)", !w.text(before).contains("S"), w.text(before));

        input(d, w, 0, 0, 9600);
        run(d, w, 9620, 9800);
        before = w.count();
        d.setSlowTop(true);
        input(d, w, 1, 0, 10000);
        check("slow switch on: full throttle sends 18", w.text(before).equals("L+18 R+18"), w.text(before));
        d.setSlowTop(false);
        input(d, w, 0, 0, 10100);
        run(d, w, 10120, 10400);
    }

    // ------------------------------------------------------------------ parked sticks (the car is doing its own thing)

    static void parking() {
        System.out.println("\n=== Parked sticks: a mode keeps running");
        Wire w = new Wire();
        DriveLogic d = new DriveLogic(w);
        d.setSteering(true);
        d.reset();

        input(d, w, 0.8f, 0, 0);
        run(d, w, 20, 400);
        int before = w.count();
        w.now = 400;
        d.actionSent(Car.SEARCH, 400);                // the driver pressed "search" with a thumb still on the stick
        run(d, w, 420, 3000);
        check("after a mode letter the held stick sends nothing (no beat that would cancel the mode)", w.count() == before, w.text(before));
        input(d, w, 0.7f, 0.1f, 3000);
        run(d, w, 3020, 3400);
        check("a small wobble of the thumb changes nothing", w.count() == before, w.text(before));
        input(d, w, 0, 0, 3400);
        run(d, w, 3420, 4000);
        check("letting go sends nothing either: search keeps running", w.count() == before, w.text(before));
        input(d, w, 0.6f, 0, 4000);
        check("a fresh push takes the car back", w.text(before).startsWith("L+") && w.count() == before + 2, w.text(before));

        // moved clearly while parked
        input(d, w, 0, 0, 4100);
        run(d, w, 4120, 4400);
        input(d, w, 1, 0, 4500);
        run(d, w, 4520, 4700);
        d.actionSent(Car.DONUT, 4700);
        before = w.count();
        input(d, w, 0.9f, 0.1f, 4800);
        check("parked again by a new mode letter", w.count() == before && d.parked());
        input(d, w, 0.2f, 0, 4900);
        check("a clear move (more than 0.35) takes the car back, without centring first", w.count() == before + 2 && !d.parked(), w.text(before));
        input(d, w, 0, 0, 5000);
        run(d, w, 5020, 5400);

        // turbo: parked for 1.6 s, then the sticks drive again
        input(d, w, 1, 0, 6000);
        run(d, w, 6020, 6300);
        w.now = 6300;
        d.actionSent(Car.TURBO, 6300);
        before = w.count();
        run(d, w, 6320, 7880);
        check("turbo: nothing is sent during the 1.5 s burst", w.count() == before, w.text(before));
        run(d, w, 7900, 8000);
        check("... and after it the held stick drives on by itself", w.text(before).startsWith("L+31 R+31"), w.text(before));

        // horn, mute, football: never park
        before = w.count();
        d.actionSent(Car.HORN, 8000);
        d.actionSent(Car.MUTE, 8000);
        d.actionSent(Car.FOOTBALL, 8000);
        run(d, w, 8020, 8400);
        check("horn, mute and football don't park the sticks (the beat goes on)", w.count() > before && !d.parked());
    }

    // ------------------------------------------------------------------ letters (older car programs)

    static void letters() {
        System.out.println("\n=== Driving with letters (older car programs)");
        Wire w = new Wire();
        DriveLogic d = new DriveLogic(w);
        d.reset();

        run(d, w, 0, 3000);
        check("resting sticks send nothing at all", w.count() == 0, w.text());

        input(d, w, 0.5f, 0, 3000);
        run(d, w, 3020, 3200);
        check("half throttle: slow first, then forward (Q F)", w.text().equals("Q F"), w.text());
        run(d, w, 3220, 5000);
        check("held still for 2 s: nothing more (the car ignores a repeated letter)", w.count() == 2, w.text());

        input(d, w, 0.9f, 0, 5000);
        run(d, w, 5020, 5200);
        check("past 75 %: W, once", w.text(2).equals("W"), w.text(2));
        input(d, w, 0.7f, 0, 5200);
        run(d, w, 5220, 5400);
        check("back to 70 %: still fast (no flicker around 75 %)", w.count() == 3, w.text(3));
        input(d, w, 0.5f, 0, 5400);
        run(d, w, 5420, 5600);
        check("back to 50 %: Q, once", w.text(3).equals("Q"), w.text(3));

        int before = w.count();
        input(d, w, 0.5f, 0.6f, 5600);
        run(d, w, 5620, 5800);
        check("throttle forward + steering right: curve right (H) - and half throttle stays slow however far you steer", w.text(before).equals("H"), w.text(before));
        before = w.count();
        input(d, w, 0.5f, -0.6f, 5800);
        run(d, w, 5820, 6000);
        check("... + steering left: curve left (G)", w.text(before).equals("G"), w.text(before));
        before = w.count();
        input(d, w, 0, -0.6f, 6000);
        run(d, w, 6020, 6200);
        check("no throttle + steering left: spin left (L)", w.text(before).equals("L"), w.text(before));
        before = w.count();
        input(d, w, 0, 0.6f, 6200);
        run(d, w, 6220, 6400);
        check("no throttle + steering right: spin right (R)", w.text(before).equals("R"), w.text(before));
        before = w.count();
        input(d, w, -0.6f, 0.5f, 6400);
        run(d, w, 6420, 6600);
        check("throttle backward + steering: plain backward (B) - the car has no backward curves", w.text(before).equals("B"), w.text(before));

        before = w.count();
        input(d, w, 0, 0, 6600);
        check("let go: S at once", w.text(before).equals("S"), w.text(before));
        run(d, w, 6620, 6640);
        check("no second S yet after 40 ms", w.count() == before + 1);
        run(d, w, 6660, 9000);
        check("the second S 60 ms later, then silence", w.text(before).equals("S S") && w.times.get(w.count() - 1) == 6660, w.text(before));

        // rate limit
        before = w.count();
        for (long t = 10000; t < 12000; t += 4) {
            double a = (t - 10000) / 90.0;
            input(d, w, (float) Math.sin(a), (float) Math.cos(a * 1.3), t);
        }
        int sent = w.count() - before;
        boolean gaps = true;
        for (int i = before + 1; i < w.count(); i++)
            if (w.codes.get(i) != 'S' && w.times.get(i) - w.times.get(i - 1) < 55)
                gaps = false;
        check("a wildly moving stick: at most 18 letters a second (" + sent + " in 2 s), never closer than 55 ms", sent <= 40 && gaps, "" + sent);
        input(d, w, 0, 0, 12000);
        run(d, w, 12020, 12400);

        // a stick held exactly on the border between two letters
        before = w.count();
        float bx = (float) Math.cos(Math.toRadians(67.5)), by = (float) Math.sin(Math.toRadians(67.5));
        for (long t = 13000; t < 15000; t += 20)
            input(d, w, by + (t % 40 == 0 ? 0.01f : -0.01f), bx + (t % 40 == 0 ? -0.01f : 0.01f), t);
        check("a stick trembling on the border between F and H: one letter, no flicker", w.count() - before <= 2, w.text(before));
        input(d, w, 0, 0, 15000);
        run(d, w, 15020, 15400);

        // one stick (the on-screen thumbstick): how far it is pushed sets slow / fast
        before = w.count();
        d.setOneStick(true);
        d.carSpeedIs(true);
        input(d, w, 0.7f, 0.7f, 15500);
        run(d, w, 15520, 15700);
        check("one stick pushed fully up-right: fast, curve right (W H)", w.text(before).equals("W H"), w.text(before));
        d.setOneStick(false);
        input(d, w, 0, 0, 15700);
        run(d, w, 15720, 15900);
        d.carSpeedIs(false);

        // the car says its speed
        before = w.count();
        d.carSpeedIs(true);
        input(d, w, 0.5f, 0, 16000);
        run(d, w, 16020, 16200);
        check("the car said it is on slow: no Q is sent, just F", w.text(before).equals("F"), w.text(before));
        input(d, w, 0, 0, 16200);
        run(d, w, 16220, 16500);

        // the slow switch keeps it slow even at full stick
        before = w.count();
        d.setSlowTop(true);
        input(d, w, 1, 0, 17000);
        run(d, w, 17020, 17300);
        check("slow switch on: a full stick never sends W", w.text(before).equals("F"), w.text(before));
        d.setSlowTop(false);
        input(d, w, 0, 0, 17300);
        run(d, w, 17320, 17600);

        // parked by a mode letter
        input(d, w, 0.5f, 0, 18000);
        run(d, w, 18020, 18200);
        before = w.count();
        d.actionSent(Car.DANCE, 18200);
        input(d, w, 0, 0, 18400);
        run(d, w, 18420, 19000);
        check("dance pressed while driving, then the stick let go: no S (the dance goes on)", w.count() == before, w.text(before));
        input(d, w, 0.5f, 0, 19000);
        run(d, w, 19020, 19200);
        check("a fresh push after that sends F again (the car forgot it)", w.text(before).equals("F"), w.text(before));
        input(d, w, 0, 0, 19200);
        run(d, w, 19220, 19500);
    }

    // ------------------------------------------------------------------ STOP and going away

    static void stopping() {
        System.out.println("\n=== STOP, and the app going away");
        Wire w = new Wire();
        DriveLogic d = new DriveLogic(w);
        d.setSteering(true);
        d.reset();

        w.now = 1000;
        d.actionSent(Car.DONUT, 1000);
        w.now = 1500;
        d.stopAll(1500);
        run(d, w, 1520, 4000);
        check("STOP with steering: both sides 0 first (that stops even a mode the car would protect), then S S",
            w.text().equals("L+0 R+0 S S"), w.text());

        input(d, w, 1, 0, 5000);
        run(d, w, 5020, 5100);
        int before = w.count();
        w.now = 5100;
        d.stopAll(5100);
        run(d, w, 5120, 6000);
        check("STOP while the stick is still held: stays stopped (the beat does not start again)", w.text(before).equals("L+0 R+0 S S"), w.text(before));
        input(d, w, 0, 0, 6000);
        input(d, w, 1, 0, 6100);
        check("... until the stick is let go and pushed again", w.text(before).endsWith("L+31 R+31"), w.text(before));

        before = w.count();
        w.now = 6200;
        d.halt(6200);
        check("the app goes away while driving: both sides 0 + S S, all at once", w.text(before).equals("L+0 R+0 S S") && !d.driving(), w.text(before));
        before = w.count();
        d.halt(6300);
        run(d, w, 6320, 7000);
        check("the app goes away while NOT driving: nothing is sent (a running mode is left alone)", w.count() == before, w.text(before));

        // letters
        Wire w2 = new Wire();
        DriveLogic d2 = new DriveLogic(w2);
        d2.reset();
        w2.now = 1000;
        d2.actionSent(Car.DONUT, 1000);
        w2.now = 1500;
        d2.stopAll(1500);
        run(d2, w2, 1520, 4000);
        check("STOP with letters, 0.5 s after a mode letter: S S now, and a third S when the car listens again (1.55 s after the mode letter)",
            w2.text().equals("S S S") && w2.times.get(2) >= 2550 && w2.times.get(2) <= 2570, w2.text() + " " + w2.times);
        int b2 = w2.count();
        w2.now = 6000;
        d2.stopAll(6000);
        run(d2, w2, 6020, 9000);
        check("STOP with letters, long after a mode letter: just S S", w2.text(b2).equals("S S"), w2.text(b2));
        input(d2, w2, 0.5f, 0, 10000);
        run(d2, w2, 10020, 10200);
        b2 = w2.count();
        w2.now = 10200;
        d2.halt(10200);
        check("the app goes away while driving with letters: S S at once", w2.text(b2).equals("S S") && !d2.driving(), w2.text(b2));

        // switching from letters to steering in the middle (the car's "#V:2" arrives late)
        Wire w3 = new Wire();
        DriveLogic d3 = new DriveLogic(w3);
        d3.reset();
        input(d3, w3, 1, 0, 0);
        run(d3, w3, 20, 200);
        d3.setSteering(true);
        run(d3, w3, 220, 400);
        check("the car says '#V:2' while the stick is held: letters first, then steering codes take over",
            w3.text().startsWith("W F L+31 R+31"), w3.text());
    }

    // ------------------------------------------------------------------ the controller map

    static void controllerMap() {
        System.out.println("\n=== ControllerMap: which button does what");
        ControllerMap m = new ControllerMap();
        String[][] table = {
            {"A", "4"}, {"B", "S"}, {"X", "k"}, {"Y", "Y"}, {"LB", "^"}, {"RB", "V"}, {"LT", "u"}, {"RT", "T"}, {"L3", "2"}, {"R3", "J"},
            {"START", "O"}, {"SELECT", "X"}, {"D-pad up", "I"}, {"D-pad right", "1"}, {"D-pad down", "3"}, {"D-pad left", "U"}};
        boolean ok = true;
        for (int b = 0; b < ControllerMap.BUTTONS; b++)
            if (!ControllerMap.BUTTON_NAMES[b].equals(table[b][0]) || m.job(b, false) != table[b][1].charAt(0))
                ok = false;
        check("the default map: A dance, B STOP, X kick, Y horn, LB shift, RB football, LT brake, RT turbo, L3 / R3 auto, START radar, SELECT mute, D-pad search / chaos / donut / 180", ok, m.toText());
        check("with LB held: A follow, B guard, X goalkeeper, Y pet", m.job(ControllerMap.A, true) == 'M' && m.job(ControllerMap.B, true) == 'A'
            && m.job(ControllerMap.X, true) == 'K' && m.job(ControllerMap.Y, true) == 'P');
        check("with LB held: D-pad up music, down gestures, left slow, right fast", m.job(ControllerMap.UP, true) == 'N' && m.job(ControllerMap.DOWN, true) == 'E'
            && m.job(ControllerMap.LEFT, true) == 'Q' && m.job(ControllerMap.RIGHT, true) == 'W');
        check("with LB held, the other buttons do nothing (no surprise turbo)", m.job(ControllerMap.RT, true) == '-' && m.job(ControllerMap.START, true) == '-'
            && m.job(ControllerMap.L3, true) == '-');
        check("LB is SHIFT on both layers", m.job(ControllerMap.LB, false) == '^' && m.job(ControllerMap.LB, true) == '^');
        check("every default job is a real job", defaultsAreJobs());

        m.set(ControllerMap.A, false, Car.HORN);
        check("remap: A = horn", m.job(ControllerMap.A, false) == 'Y' && m.job(ControllerMap.A, true) == 'M' && !m.isDefault());
        String saved = m.toText();
        ControllerMap n = new ControllerMap();
        n.fromText(saved);
        check("saved and loaded again: the same map", n.toText().equals(saved) && n.job(ControllerMap.A, false) == 'Y' && saved.length() == 32);
        n.fromText("garbage");
        check("a damaged saved map = the default map", n.isDefault());
        n.fromText(saved.replace('Y', '#'));
        check("a saved map with an unknown job = the default map", n.isDefault());
        n.fromText(null);
        check("no saved map = the default map", n.isDefault());

        m.set(ControllerMap.RB, false, ControllerMap.SHIFT);
        check("remap: RB = SHIFT (on both layers)", m.job(ControllerMap.RB, false) == '^' && m.job(ControllerMap.RB, true) == '^');
        m.set(ControllerMap.RB, true, Car.PET);
        check("giving a SHIFT button a job frees both its layers", m.job(ControllerMap.RB, true) == 'P' && m.job(ControllerMap.RB, false) == '-');
        m.set(ControllerMap.A, false, '#');
        check("an unknown job is refused", m.job(ControllerMap.A, false) == 'Y');
        m.reset();
        check("reset = the default map again", m.isDefault());
        check("find the button for a job (dance = A, STOP = B)", m.buttonFor('4') == ControllerMap.A && m.buttonFor('S') == ControllerMap.B && m.buttonFor('#') == -1);
        String help = m.describe();
        check("the map as text for the help", help.contains("A = Dance") && help.contains("RT = Turbo") && help.contains("LB + A = Follow me")
            && help.contains("LB + D-pad left = Slow") && help.contains("LB = SHIFT"), help);
    }

    static boolean defaultsAreJobs() {
        for (char c : (ControllerMap.DEFAULT_NORMAL + ControllerMap.DEFAULT_SHIFT).toCharArray())
            if (!ControllerMap.isJob(c))
                return false;
        return ControllerMap.DEFAULT_NORMAL.length() == 16 && ControllerMap.DEFAULT_SHIFT.length() == 16
            && ControllerMap.JOBS.length == ControllerMap.JOB_NAMES.length;
    }

    // ------------------------------------------------------------------ button state

    static void padState() {
        System.out.println("\n=== PadState: every press counted once");
        final List<String> events = new ArrayList<>();
        PadState p = new PadState((button, down) -> events.add(ControllerMap.BUTTON_NAMES[button] + (down ? " down" : " up")));

        p.hat(0, -1);
        p.key(ControllerMap.UP, true);
        p.key(ControllerMap.UP, true);
        check("D-pad up sent as an axis AND as a key: one press", events.toString().equals("[D-pad up down]"), events.toString());
        p.hat(0, 0);
        check("... still down while the key is held", events.size() == 1 && p.down(ControllerMap.UP));
        p.key(ControllerMap.UP, false);
        check("... one release", events.toString().equals("[D-pad up down, D-pad up up]"), events.toString());

        events.clear();
        p.hat(1, 1);
        check("a diagonal on the D-pad = two buttons", events.toString().equals("[D-pad right down, D-pad down down]"), events.toString());
        p.hat(0, 0);
        events.clear();

        p.trigger(1, 0.3f);
        p.trigger(1, 0.59f);
        check("trigger at 59 %: not pressed yet", events.isEmpty());
        p.trigger(1, 0.61f);
        check("trigger past 60 %: pressed", events.toString().equals("[RT down]"), events.toString());
        p.trigger(1, 0.45f);
        p.trigger(1, 0.7f);
        p.trigger(1, 0.31f);
        check("wobbling between 31 % and 70 %: still one press", events.size() == 1, events.toString());
        p.trigger(1, 0.29f);
        check("under 30 %: released", events.toString().equals("[RT down, RT up]"), events.toString());
        p.trigger(1, 0.9f);
        check("... and it can fire again", events.size() == 3 && Math.abs(p.trigger(1) - 0.9f) < 0.001f);
        p.trigger(1, 0);
        events.clear();

        p.key(ControllerMap.LT, true);
        p.trigger(0, 1f);
        check("a trigger sent as a button AND as an axis: one press", events.toString().equals("[LT down]") && p.trigger(0) == 1f, events.toString());
        p.key(ControllerMap.LT, false);
        p.trigger(0, 0f);
        check("... one release", events.toString().equals("[LT down, LT up]"), events.toString());

        events.clear();
        p.key(ControllerMap.A, true);
        p.key(ControllerMap.LB, true);
        p.releaseAll();
        check("controller unplugged: every held button is let go", events.toString().equals("[A down, LB down, A up, LB up]") && !p.down(ControllerMap.A), events.toString());
    }

    // ------------------------------------------------------------------ a whole drive, for the simulated car

    /**
     * Drives a little session with the real driving brain and writes down every code with its
     * time. The car simulator then plays exactly these codes to the real car program
     * (scenario "app-replay" in Tools\sim\scenarios.js) and checks the wheels.
     */
    static void replay(File file) throws Exception {
        System.out.println("\n=== A whole drive, written down for the simulated car");
        Wire w = new Wire();
        DriveLogic d = new DriveLogic(w);
        d.setSteering(true);
        d.reset();
        long t0 = 2000;                                // (the car has started by then)
        float[][] drive = {
            // from ms, to ms, throttle, steer
            {0, 1000, 1f, 0f},                         // straight on
            {1000, 2000, 1f, 1f},                      // tight bend to the right
            {2000, 3000, 0f, -1f},                     // spin left on the spot
            {3000, 4000, -1f, 0f},                     // backward
            {4000, 5000, 0f, 0f},                      // let go
            {5000, 6000, 0.5f, 0f},                    // half throttle
            {6000, 7000, 0f, 0f},                      // let go
        };
        for (long t = 0; t < 7000; t += 20) {
            for (float[] step : drive)
                if (t >= step[0] && t < step[1]) {
                    w.now = t0 + t;
                    d.input(step[2], step[3], t0 + t);
                }
        }
        // a mode, then STOP half a second later (the car ignores a plain S that soon)
        w.now = t0 + 7500;
        w.send(Car.DONUT);
        d.actionSent(Car.DONUT, t0 + 7500);
        w.now = t0 + 8000;
        d.stopAll(t0 + 8000);
        run(d, w, t0 + 8020, t0 + 9000);

        StringBuilder json = new StringBuilder("[");
        for (int i = 0; i < w.count(); i++)
            json.append(i == 0 ? "" : ",").append("[").append(w.times.get(i)).append(",").append(w.codes.get(i)).append("]");
        json.append("]");
        if (file.getParentFile().isDirectory()) {
            try (PrintWriter out = new PrintWriter(file, "UTF-8")) {
                out.println(json);
            }
        }
        check("the drive was written down (" + w.count() + " codes in 9 s)", w.count() > 40 && w.count() < 200, "" + w.count());
        check("... and it ends with STOP: both sides 0, S, S", w.text(w.count() - 4).equals("L+0 R+0 S S"), w.text(w.count() - 4));
    }

    private LogicTests() {
    }
}
