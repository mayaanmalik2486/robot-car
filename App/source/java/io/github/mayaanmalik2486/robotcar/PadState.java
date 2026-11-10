package io.github.mayaanmalik2486.robotcar;

/**
 * Which controller buttons are down right now. Controllers report some buttons in two ways
 * at once (the D-pad as keys AND as an axis, the triggers as buttons AND as an axis); this
 * joins both so every press is counted exactly once.
 *
 * The triggers are analog: they count as pressed past 60 % and as let go under 30 %.
 *
 * No Android in here, so it can be tested on a computer (App\tests).
 */
final class PadState {

    /** Told about every real press and release. */
    interface Listener {
        void onButton(int button, boolean down);
    }

    static final float TRIGGER_ON = 0.60f;
    static final float TRIGGER_OFF = 0.30f;

    private final Listener listener;
    private final boolean[] byKey = new boolean[ControllerMap.BUTTONS];     // down, says a key event
    private final boolean[] byAxis = new boolean[ControllerMap.BUTTONS];    // down, says an axis
    private final float[] trigger = new float[2];                           // LT, RT: 0 ... 1

    PadState(Listener listener) {
        this.listener = listener;
    }

    boolean down(int button) {
        return byKey[button] || byAxis[button];
    }

    /** How far a trigger is pulled, 0 ... 1 (which: 0 = LT, 1 = RT). */
    float trigger(int which) {
        return Math.max(trigger[which], byKey[ControllerMap.LT + which] ? 1f : 0f);
    }

    /** A key event for a button. */
    void key(int button, boolean isDown) {
        set(byKey, button, isDown);
    }

    /** The D-pad as an axis: x -1 = left, 1 = right; y -1 = up, 1 = down. */
    void hat(float x, float y) {
        set(byAxis, ControllerMap.LEFT, x < -0.5f);
        set(byAxis, ControllerMap.RIGHT, x > 0.5f);
        set(byAxis, ControllerMap.UP, y < -0.5f);
        set(byAxis, ControllerMap.DOWN, y > 0.5f);
    }

    /** An analog trigger (which: 0 = LT, 1 = RT), 0 ... 1. */
    void trigger(int which, float value) {
        trigger[which] = Math.max(0f, Math.min(1f, value));
        int button = ControllerMap.LT + which;
        if (!byAxis[button] && value >= TRIGGER_ON)
            set(byAxis, button, true);
        else if (byAxis[button] && value <= TRIGGER_OFF)
            set(byAxis, button, false);
    }

    /** The controller is gone: every button counts as let go. */
    void releaseAll() {
        for (int b = 0; b < ControllerMap.BUTTONS; b++) {
            boolean was = down(b);
            byKey[b] = byAxis[b] = false;
            if (was)
                listener.onButton(b, false);
        }
        trigger[0] = trigger[1] = 0;
    }

    private void set(boolean[] source, int button, boolean isDown) {
        if (button < 0 || button >= ControllerMap.BUTTONS || source[button] == isDown)
            return;
        boolean was = down(button);
        source[button] = isDown;
        boolean now = down(button);
        if (was != now)
            listener.onButton(button, now);
    }
}
