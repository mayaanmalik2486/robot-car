package io.github.mayaanmalik2486.robotcar;

/**
 * The driving brain: turns "how far is the throttle pushed, how far is the steering turned"
 * into what is sent to the car - and makes sure the car is never flooded.
 *
 * It talks to the car in one of two ways:
 * <pre>
 *   STEERING (car program with "#V:2"): the speed of each side of the car, -31 ... 31.
 *            Smooth, like a real RC car. Sent when it changes (at most every 60 ms), again
 *            every 0.2 s while driving (the car stops by itself if we go quiet), and
 *            "both sides 0" + S when you let go.
 *   LETTERS  (older car programs): F B L R G H, with Q / W for slow / fast when the stick
 *            crosses 75 %. Only sent on a change, at most 18 a second, S twice when you let go.
 * </pre>
 * When the car starts something of its own (a dance, search, turbo ...), the sticks are
 * "parked": nothing more is sent until you centre them or move them clearly - so a thumb
 * resting on a stick can't cancel the mode by accident.
 *
 * No Android in here, so it can be tested on a computer (App\tests).
 */
final class DriveLogic {

    /** Where the codes go (one letter or steering code at a time). */
    interface Out {
        void send(int code);
    }

    static final int WEAKEST = 11;              // the slowest speed that still turns the motors (PWM 95)
    static final int SLOW_TOP = 18;             // top speed when "slow" is on (PWM 151, the car's own slow speed)
    static final float SPIN_GAIN = 0.85f;       // spinning on the spot: 85 % (like the car's L / R)
    static final float LOCK_GAIN = 1.857f;      // full steering at full throttle: the inner side runs backward at 30 %
    static final long WHEELS_GAP_MS = 60;       // steering: at most one change every 60 ms
    static final long LETTER_GAP_MS = 55;       // letters: at most 18 a second
    static final long BEAT_MS = 200;            // steering: say it again this often while driving
    static final long STOP_AGAIN_MS = 60;       // the second S
    static final long MODE_DEAF_MS = 1500;      // the car ignores S this long after a mode letter
    static final long TURBO_MS = 1600;          // the sticks are parked this long for a turbo burst
    static final float FAST_ABOVE = 0.75f;      // letters: stick past this = fast (W) ...
    static final float SLOW_BELOW = 0.65f;      // ... back under this = slow (Q)
    static final float RETAKE = 0.35f;          // parked sticks: moved this far = "I'm driving again"

    private static final int UNKNOWN = 99;

    private final Out out;
    private boolean steering;                   // the car knows the steering codes
    private boolean slowTop;                    // the "slow" switch is on
    private float feel = 0.6f;                  // steering feel: 0 = direct ... 1 = very gentle around the middle
    private boolean oneStick;                   // throttle and steering come from one stick (not two)

    private float throttle, steer;              // what the driver wants right now
    private int sentLeft, sentRight;            // steering: the speeds the car has
    private char sentLetter = Car.STOP;         // letters: the letter the car has (0 = don't know)
    private int sentSlow = UNKNOWN;             // letters: 1 = the car is on slow, 0 = fast
    private boolean fastZone;                   // letters: the stick is in the fast part
    private long lastSendAt = -100000;
    private long lastBeatAt = -100000;
    private long lastModeAt = -100000;          // when the last mode letter went out
    private final long[] stopAt = new long[2];  // extra S letters still to send (0 = none)

    private boolean parked;                     // the car is doing its own thing: leave it alone
    private long parkedUntil;                   // 0 = until the sticks are centred or moved
    private float parkThrottle, parkSteer;      // where the sticks were when parked

    DriveLogic(Out out) {
        this.out = out;
    }

    // ------------------------------------------------------------------ settings

    /** Does the car know the steering codes? (false = drive with letters) */
    void setSteering(boolean on) {
        if (on == steering)
            return;
        steering = on;
        forget();
    }

    boolean steering() {
        return steering;
    }

    /** The slow / fast switch. */
    void setSlowTop(boolean slow) {
        slowTop = slow;
    }

    /** Do throttle and steering come from ONE stick? (Letters mode: then "how far it is pushed" sets slow / fast.) */
    void setOneStick(boolean one) {
        oneStick = one;
    }

    void setFeel(float feel) {
        this.feel = Math.max(0, Math.min(1, feel));
    }

    /** The car said which speed it is on (letters mode: so Q / W are only sent on a change). */
    void carSpeedIs(boolean slow) {
        sentSlow = slow ? 1 : 0;
    }

    /** A new connection: we know nothing about the car yet. */
    void reset() {
        throttle = steer = 0;
        sentLeft = sentRight = 0;
        sentLetter = Car.STOP;
        sentSlow = UNKNOWN;
        fastZone = false;
        parked = false;
        stopAt[0] = stopAt[1] = 0;
        lastSendAt = lastBeatAt = lastModeAt = -100000;
    }

    // ------------------------------------------------------------------ what the screen can show

    /** Are we driving the car right now (so it must be stopped if the app goes away)? */
    boolean driving() {
        return steering ? sentLeft != 0 || sentRight != 0 : sentLetter != Car.STOP && sentLetter != 0;
    }

    /** The speed of each side as it was last sent, -31 ... 31 (steering mode). */
    int left() {
        return sentLeft == UNKNOWN ? 0 : sentLeft;
    }

    int right() {
        return sentRight == UNKNOWN ? 0 : sentRight;
    }

    /** The driving letter that was last sent (letters mode). */
    char letter() {
        return sentLetter == 0 ? Car.STOP : sentLetter;
    }

    boolean parked() {
        return parked;
    }

    // ------------------------------------------------------------------ driving

    /**
     * Where the driver holds the controls: throttle 1 = full forward, -1 = full backward;
     * steer 1 = full right, -1 = full left. (Dead zones are taken off before this.)
     */
    void input(float throttle, float steer, long now) {
        this.throttle = clamp(throttle);
        this.steer = clamp(steer);
        pump(now);
    }

    /** Call this about 25 times a second: it sends what is due (the second S, the 0.2 s beat). */
    void tick(long now) {
        pump(now);
    }

    /**
     * The app just sent this letter itself (a button). Mode letters park the sticks;
     * Q / W are remembered.
     */
    void actionSent(char letter, long now) {
        if (letter == Car.SLOW || letter == Car.FULL) {
            sentSlow = letter == Car.SLOW ? 1 : 0;
            return;
        }
        if (!Car.takesTheWheels(letter))
            return;
        stopAt[0] = stopAt[1] = 0;                  // (an S now would only be "the button was let go")
        if (letter != Car.BRAKE && letter != Car.KICK)
            lastModeAt = now;
        park(letter == Car.TURBO ? now + TURBO_MS : 0);
    }

    /**
     * The STOP button: stops everything, whatever the car is doing. The car ignores an S for
     * 1.5 s after a mode letter (it thinks a button was let go), so: with steering, "both
     * sides 0" comes first (that always stops); with letters, one more S goes out when the
     * car listens again.
     */
    void stopAll(long now) {
        if (steering) {
            out.send(Car.wheel(false, 0));
            out.send(Car.wheel(true, 0));
        }
        out.send(Car.STOP);
        lastSendAt = now;
        stopAt[0] = now + STOP_AGAIN_MS;
        stopAt[1] = !steering && now - lastModeAt < MODE_DEAF_MS ? lastModeAt + MODE_DEAF_MS + 50 : 0;
        markStopped();
        park(0);
    }

    /**
     * The app is going away (paused, controller unplugged, screen changed): if we were
     * driving, stop right now - there may be no later.
     */
    void halt(long now) {
        boolean was = driving();
        throttle = steer = 0;
        stopAt[0] = stopAt[1] = 0;
        parked = false;
        if (!was) {
            markStopped();
            return;
        }
        if (steering) {
            out.send(Car.wheel(false, 0));
            out.send(Car.wheel(true, 0));
        }
        out.send(Car.STOP);
        out.send(Car.STOP);
        lastSendAt = now;
        markStopped();
    }

    // ------------------------------------------------------------------ the inside

    private void park(long until) {
        parked = true;
        parkedUntil = until;
        parkThrottle = throttle;
        parkSteer = steer;
    }

    private void markStopped() {
        sentLeft = sentRight = 0;
        sentLetter = Car.STOP;
    }

    private void forget() {
        sentLeft = sentRight = UNKNOWN;
        sentLetter = 0;
    }

    private void pump(long now) {
        for (int i = 0; i < stopAt.length; i++) {
            if (stopAt[i] != 0 && now >= stopAt[i]) {
                stopAt[i] = 0;
                out.send(Car.STOP);
                lastSendAt = now;
            }
        }

        boolean centred = throttle == 0 && steer == 0;
        if (parked) {
            if (centred) {                          // let go: the car carries on with its mode
                parked = false;
                markStopped();
                return;
            }
            if (parkedUntil != 0 && now >= parkedUntil) {
                parked = false;                     // the turbo burst is over: back to the sticks
                forget();
            } else if (Math.abs(throttle - parkThrottle) >= RETAKE || Math.abs(steer - parkSteer) >= RETAKE) {
                parked = false;                     // a clear move: the driver takes over
                forget();
            } else {
                return;
            }
        }

        if (steering)
            pumpWheels(now);
        else
            pumpLetters(now);
    }

    private final int[] want = new int[2];

    private void pumpWheels(long now) {
        mix(throttle, steer, slowTop, feel, want);
        int l = want[0], r = want[1];

        if (l == 0 && r == 0) {
            if (sentLeft != 0 || sentRight != 0) {  // let go: stop at once (never held back)
                out.send(Car.wheel(false, 0));
                out.send(Car.wheel(true, 0));
                sentLeft = sentRight = 0;
                lastSendAt = now;
                stopAt[0] = now + STOP_AGAIN_MS;    // then S: the car goes back to READY
            }
            return;
        }

        stopAt[0] = stopAt[1] = 0;                  // driving again: no S
        if (now - lastSendAt < WHEELS_GAP_MS)
            return;
        boolean beat = now - lastBeatAt >= BEAT_MS;
        if (!beat && l == sentLeft && r == sentRight)
            return;
        if (beat || l != sentLeft)
            out.send(Car.wheel(false, l));
        if (beat || r != sentRight)
            out.send(Car.wheel(true, r));
        if (beat)
            lastBeatAt = now;
        sentLeft = l;
        sentRight = r;
        lastSendAt = now;
    }

    private void pumpLetters(long now) {
        char wanted = letterFor(steer, -throttle, sentLetter);

        if (wanted == Car.STOP) {
            fastZone = false;
            if (sentLetter != Car.STOP) {           // let go: S now, and once more in 60 ms
                out.send(Car.STOP);
                sentLetter = Car.STOP;
                lastSendAt = now;
                stopAt[0] = now + STOP_AGAIN_MS;
            }
            return;
        }

        stopAt[0] = stopAt[1] = 0;
        if (now - lastSendAt < LETTER_GAP_MS)
            return;

        // how hard the stick is pushed: one stick = how far from the middle; two sticks = the
        // throttle (or the steering, when spinning on the spot)
        float push = oneStick ? Math.min(1f, (float) Math.hypot(throttle, steer))
            : wanted == Car.LEFT || wanted == Car.RIGHT ? Math.abs(steer) : Math.abs(throttle);
        fastZone = push > FAST_ABOVE || fastZone && push > SLOW_BELOW;
        int slow = slowTop || !fastZone ? 1 : 0;
        if (slow != sentSlow) {                     // the speed first, so the car starts at the right one
            out.send(slow == 1 ? Car.SLOW : Car.FULL);
            sentSlow = slow;
            lastSendAt = now;
            return;
        }
        if (wanted != sentLetter) {
            out.send(wanted);
            sentLetter = wanted;
            lastSendAt = now;
        }
    }

    // ------------------------------------------------------------------ the maths (static, so the tests can try them)

    /**
     * The driving letter for a stick position, staying on the letter we already have until
     * the stick is clearly in another direction (6 degrees past the border) - so a stick held
     * on a border doesn't flicker between two letters.
     */
    static char letterFor(float x, float y, char current) {
        char plain = Car.direction(x, y);
        if (current == 0 || current == Car.STOP || plain == current)
            return plain;
        if (plain == Car.STOP)
            return Car.direction(x * 1.17f, y * 1.17f) == Car.STOP ? Car.STOP : current;
        double turn = Math.toRadians(6);
        for (int sign = -1; sign <= 1; sign += 2) {
            double c = Math.cos(sign * turn), s = Math.sin(sign * turn);
            if (Car.direction((float) (x * c - y * s), (float) (x * s + y * c)) == current)
                return current;
        }
        return plain;
    }

    /**
     * Throttle + steering -> the speed of each side (-31 ... 31) into out[0] (left), out[1] (right).
     * No throttle: spin on the spot. Full throttle + full steering: a tight bend with the inner
     * side running backward at 30 % (like the car's own curve). In between it blends smoothly.
     */
    static void mix(float throttle, float steer, boolean slowTop, float feel, int[] out) {
        float t = clamp(throttle);
        float s = clamp(steer);
        s = (1 - feel) * s + feel * s * s * s;      // gentle around the middle, full at the end
        float gain = SPIN_GAIN + (LOCK_GAIN - SPIN_GAIN) * Math.abs(t);
        float l = t + s * gain;
        float r = t - s * gain;
        float big = Math.max(Math.abs(l), Math.abs(r));
        if (big < 0.02f) {
            out[0] = out[1] = 0;
            return;
        }
        float power = Math.min(1f, big);            // how hard the faster side works, 0 ... 1
        int top = slowTop ? SLOW_TOP : Car.WHEEL_MAX;
        float level = WEAKEST + (top - WEAKEST) * power;   // ... as a motor speed that really turns the wheels
        out[0] = Math.round(l / big * level);
        out[1] = Math.round(r / big * level);
    }

    /** A round dead zone for a stick: inside it = 0, outside it grows from 0 to 1 again. Returns the factor to multiply x and y with. */
    static float deadZone(float x, float y, float zone) {
        float len = (float) Math.hypot(x, y);
        if (len <= zone || len == 0)
            return 0;
        float scaled = Math.min(1f, (len - zone) / (1 - zone));
        return scaled / len;
    }

    /** A dead zone for one axis (a trigger, a slider). */
    static float deadZone(float v, float zone) {
        float a = Math.abs(v);
        if (a <= zone)
            return 0;
        return Math.signum(v) * Math.min(1f, (a - zone) / (1 - zone));
    }

    private static float clamp(float v) {
        return Math.max(-1f, Math.min(1f, v));
    }
}
