package io.github.mayaanmalik2486.robotcar;

/**
 * The car's language (see Robot_Car_Main.ino, section 15): the letters it understands, the
 * steering codes that set the speed of each side, and the rule that turns a stick position
 * into a driving letter.
 */
final class Car {

    static final char FORWARD = 'F';
    static final char BACKWARD = 'B';
    static final char LEFT = 'L';               // spin on the spot
    static final char RIGHT = 'R';
    static final char CURVE_LEFT = 'G';         // forward while turning
    static final char CURVE_RIGHT = 'H';
    static final char STOP = 'S';

    static final char SLOW = 'Q';
    static final char FULL = 'W';
    static final char TURBO = 'T';
    static final char HORN = 'Y';
    static final char DANCE = '4';

    static final char SEARCH = 'I';
    static final char CHAOS = '1';
    static final char DONUT = '3';
    static final char TURN180 = 'U';
    static final char BRAKE = 'u';
    static final char AUTO_FORWARD = '2';
    static final char AUTO_BACKWARD = 'J';

    static final char FOLLOW = 'M';
    static final char KEEPER = 'K';
    static final char KICK = 'k';
    static final char GUARD = 'A';
    static final char MUSIC = 'N';
    static final char GESTURE = 'E';
    static final char PET = 'P';
    static final char RADAR = 'O';

    static final char MUTE = 'X';               // mute / unmute
    static final char FOOTBALL = 'V';           // distance sensor off / on
    static final char ASK = '?';                // "what are you doing?" - the car answers with its status + song

    /** Never sent: the app picks SLOW or FULL, whichever the car isn't on. */
    static final char SPEED_TOGGLE = '%';

    /** Steering: the fastest a side can go. Speeds run from -WHEEL_MAX (backward) to WHEEL_MAX. */
    static final int WHEEL_MAX = 31;

    /**
     * Steering code for one side of the car: a number above 127, so the car can tell it from
     * a letter. Bit 6 = the right side, the low 6 bits = the speed + 32 (32 = stop).
     */
    static int wheel(boolean rightSide, int speed) {
        speed = Math.max(-WHEEL_MAX, Math.min(WHEEL_MAX, speed));
        return 0x80 | (rightSide ? 0x40 : 0) | (speed + 32);
    }

    /** A letter that starts something that drives the wheels by itself (so the sticks must leave it alone). */
    static boolean takesTheWheels(char c) {
        return "I12J34UMKANEPOkuT".indexOf(c) >= 0;
    }

    /** The driving letter for a stick position (x, y from -1 to 1, y down = positive). */
    static char direction(float x, float y) {
        if (Math.hypot(x, y) < 0.35)
            return STOP;
        double a = Math.toDegrees(Math.atan2(-y, x));      // 0 = right, 90 = up
        if (a >= 67.5 && a < 112.5)
            return FORWARD;
        if (a >= 112.5 && a < 157.5)
            return CURVE_LEFT;
        if (a >= 22.5 && a < 67.5)
            return CURVE_RIGHT;
        if (a >= -22.5 && a < 22.5)
            return RIGHT;
        if (a >= 157.5 || a < -157.5)
            return LEFT;
        return BACKWARD;
    }

    private Car() {
    }
}
