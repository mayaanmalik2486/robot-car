package io.github.mayaanmalik2486.robotcar;

/**
 * Which controller button does what. Every button has two jobs: its normal one, and one
 * while the SHIFT button (LB) is held. The sticks are not in here - they always drive.
 *
 * A job is one of the car's letters (see Car), or NONE, SHIFT or Car.SPEED_TOGGLE.
 * The whole map is saved as a 32-letter text, one per controller.
 *
 * No Android in here, so it can be tested on a computer (App\tests).
 */
final class ControllerMap {

    // the buttons
    static final int A = 0, B = 1, X = 2, Y = 3, LB = 4, RB = 5, LT = 6, RT = 7, L3 = 8, R3 = 9,
        START = 10, SELECT = 11, UP = 12, RIGHT = 13, DOWN = 14, LEFT = 15;
    static final int BUTTONS = 16;

    static final String[] BUTTON_NAMES = {"A", "B", "X", "Y", "LB", "RB", "LT", "RT", "L3", "R3",
        "START", "SELECT", "D-pad up", "D-pad right", "D-pad down", "D-pad left"};

    /** Short names, for the little badges on the screen's buttons. */
    static final String[] SHORT_NAMES = {"A", "B", "X", "Y", "LB", "RB", "LT", "RT", "L3", "R3",
        "START", "SELECT", "▲", "▶", "▼", "◀"};

    static final char NONE = '-';
    static final char SHIFT = '^';

    /** Every job a button can have, with its name. */
    static final char[] JOBS = {Car.STOP, Car.DANCE, Car.KICK, Car.HORN, Car.TURBO, Car.BRAKE, Car.FOOTBALL, Car.MUTE,
        Car.SEARCH, Car.CHAOS, Car.DONUT, Car.TURN180, Car.AUTO_FORWARD, Car.AUTO_BACKWARD,
        Car.FOLLOW, Car.GUARD, Car.KEEPER, Car.PET, Car.MUSIC, Car.GESTURE, Car.RADAR,
        Car.SLOW, Car.FULL, Car.SPEED_TOGGLE, SHIFT, NONE};
    static final String[] JOB_NAMES = {"STOP", "Dance", "Kick", "Horn", "Turbo", "Smooth brake", "Football mode", "Mute",
        "Search", "Chaos", "Donut", "180 turn", "Auto forward", "Auto backward",
        "Follow me", "Guard", "Goalkeeper", "Pet", "Music", "Gestures", "Radar",
        "Slow", "Fast", "Slow / fast", "SHIFT (hold)", "Nothing"};

    //                                    A B X Y LB RB LT RT L3 R3 ST SE UP RI DO LE
    static final String DEFAULT_NORMAL = "4SkY^VuT2JOXI13U";
    static final String DEFAULT_SHIFT  = "MAKP^-------NWEQ";

    private final char[][] jobs = new char[2][BUTTONS];

    ControllerMap() {
        reset();
    }

    /** Back to the map the app comes with. */
    void reset() {
        DEFAULT_NORMAL.getChars(0, BUTTONS, jobs[0], 0);
        DEFAULT_SHIFT.getChars(0, BUTTONS, jobs[1], 0);
    }

    /** The job of a button (with or without SHIFT held). */
    char job(int button, boolean shift) {
        if (button < 0 || button >= BUTTONS)
            return NONE;
        if (jobs[0][button] == SHIFT)                // a SHIFT button is SHIFT on both layers
            return SHIFT;
        return jobs[shift ? 1 : 0][button];
    }

    /** Gives a button a new job. SHIFT always goes on both layers. */
    void set(int button, boolean shift, char job) {
        if (button < 0 || button >= BUTTONS || !isJob(job))
            return;
        if (job == SHIFT) {
            jobs[0][button] = jobs[1][button] = SHIFT;
            return;
        }
        if (jobs[0][button] == SHIFT) {              // it was a SHIFT button: free both layers
            jobs[0][button] = jobs[1][button] = NONE;
        }
        jobs[shift ? 1 : 0][button] = job;
    }

    /** The map as a text to save. */
    String toText() {
        return new String(jobs[0]) + new String(jobs[1]);
    }

    /** Loads a saved map. Anything odd in it (an old version, a damaged text) = the default map. */
    void fromText(String text) {
        reset();
        if (text == null || text.length() != BUTTONS * 2)
            return;
        for (int i = 0; i < text.length(); i++)
            if (!isJob(text.charAt(i)))
                return;
        text.getChars(0, BUTTONS, jobs[0], 0);
        text.getChars(BUTTONS, BUTTONS * 2, jobs[1], 0);
        for (int b = 0; b < BUTTONS; b++)
            if (jobs[0][b] == SHIFT || jobs[1][b] == SHIFT)
                jobs[0][b] = jobs[1][b] = SHIFT;
    }

    boolean isDefault() {
        return toText().equals(DEFAULT_NORMAL + DEFAULT_SHIFT);
    }

    /** The first button with this job on the normal layer, or -1. */
    int buttonFor(char job) {
        for (int b = 0; b < BUTTONS; b++)
            if (jobs[0][b] == job)
                return b;
        return -1;
    }

    /** The controller button for a job, as a short badge ("A", "RT", "LB+A", "▲"), or null. */
    String labelFor(char job) {
        int b = buttonFor(job);
        if (b >= 0)
            return SHORT_NAMES[b];
        int shift = buttonFor(SHIFT);
        for (int i = 0; i < BUTTONS; i++)
            if (job(i, true) == job && shift >= 0)
                return SHORT_NAMES[shift] + "+" + SHORT_NAMES[i];
        return null;
    }

    static boolean isJob(char c) {
        return jobIndex(c) >= 0;
    }

    static int jobIndex(char c) {
        for (int i = 0; i < JOBS.length; i++)
            if (JOBS[i] == c)
                return i;
        return -1;
    }

    static String jobName(char job) {
        int i = jobIndex(job);
        return i < 0 ? "?" : JOB_NAMES[i];
    }

    /** The whole map as a few lines of text, for the help. */
    String describe() {
        StringBuilder s = new StringBuilder();
        String shiftName = "SHIFT";
        for (int b = 0; b < BUTTONS; b++)
            if (jobs[0][b] == SHIFT) {
                shiftName = BUTTON_NAMES[b];
                break;
            }
        for (int b = 0; b < BUTTONS; b++) {
            char j = job(b, false);
            if (j != NONE)
                s.append(BUTTON_NAMES[b]).append(" = ").append(j == SHIFT ? "SHIFT (hold for the second jobs)" : jobName(j)).append('\n');
        }
        s.append('\n');
        for (int b = 0; b < BUTTONS; b++) {
            char j = job(b, true);
            if (j != NONE && j != SHIFT)
                s.append(shiftName).append(" + ").append(BUTTON_NAMES[b]).append(" = ").append(jobName(j)).append('\n');
        }
        return s.toString().trim();
    }
}
