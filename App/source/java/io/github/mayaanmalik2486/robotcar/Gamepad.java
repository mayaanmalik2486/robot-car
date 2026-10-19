package io.github.mayaanmalik2486.robotcar;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

/**
 * A game controller (GameSir Cyclone 2, Xbox, PlayStation, ...) connected to the phone.
 * It works like a racing game:
 * <pre>
 *   RT = gas (forward)   LT = reverse   left stick = steer (or drive with the stick alone)
 *   A turbo   B STOP   X kick   Y dance          RB fast / slow    L3 horn   R3 brake
 *   D-pad: up search, right chaos, down donut, left 180 turn
 *   START radar   SELECT football (sensor off)
 *   Hold LB for the play modes:
 *     LB+A follow  LB+B guard  LB+X goalkeeper  LB+Y pet
 *     LB+up music  LB+down gestures  LB+right auto forward  LB+left auto backward
 *     LB+SELECT sound on/off
 * </pre>
 */
final class Gamepad {

    interface Listener {
        /** A one-shot action: one of the Car letters (or Car.SPEED_TOGGLE). */
        void onPadAction(char letter);

        /** Driving: the letter to drive with now, and where to show the on-screen knob. */
        void onPadDrive(char direction, float showX, float showY);
    }

    private final Listener listener;
    private float stickX, stickY;                  // left stick
    private float ltAxis, rtAxis;                  // analog triggers
    private boolean ltKey, rtKey;                  // triggers that send buttons instead
    private boolean shift;                         // LB held
    private int hatX, hatY;                        // D-pad sent as an axis
    private boolean driving;

    Gamepad(Listener listener) {
        this.listener = listener;
    }

    static boolean isGamepad(InputDevice d) {
        if (d == null || d.isVirtual())
            return false;
        int s = d.getSources();
        return (s & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
            || (s & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }

    static boolean fromGamepad(KeyEvent e) {
        int s = e.getSource();
        return (s & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
            || (s & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
            || (s & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD && isGamepad(e.getDevice());
    }

    /** Sticks, triggers and an axis D-pad. Returns true if it was the controller's. */
    boolean onMotion(MotionEvent e) {
        if ((e.getSource() & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK || e.getAction() != MotionEvent.ACTION_MOVE)
            return false;
        stickX = deadZone(e.getAxisValue(MotionEvent.AXIS_X));
        stickY = deadZone(e.getAxisValue(MotionEvent.AXIS_Y));
        ltAxis = Math.max(e.getAxisValue(MotionEvent.AXIS_LTRIGGER), e.getAxisValue(MotionEvent.AXIS_BRAKE));
        rtAxis = Math.max(e.getAxisValue(MotionEvent.AXIS_RTRIGGER), e.getAxisValue(MotionEvent.AXIS_GAS));

        int hx = Math.round(e.getAxisValue(MotionEvent.AXIS_HAT_X));
        int hy = Math.round(e.getAxisValue(MotionEvent.AXIS_HAT_Y));
        if (hx != hatX) {
            hatX = hx;
            if (hx != 0)
                dpad(hx > 0 ? KeyEvent.KEYCODE_DPAD_RIGHT : KeyEvent.KEYCODE_DPAD_LEFT);
        }
        if (hy != hatY) {
            hatY = hy;
            if (hy != 0)
                dpad(hy > 0 ? KeyEvent.KEYCODE_DPAD_DOWN : KeyEvent.KEYCODE_DPAD_UP);
        }
        drive();
        return true;
    }

    /** Buttons. Returns true if it was the controller's. */
    boolean onKeyDown(int code, KeyEvent e) {
        if (!fromGamepad(e))
            return false;
        if (e.getRepeatCount() > 0)
            return true;
        switch (code) {
            case KeyEvent.KEYCODE_BUTTON_L1:     shift = true; return true;
            case KeyEvent.KEYCODE_BUTTON_L2:     ltKey = true; drive(); return true;
            case KeyEvent.KEYCODE_BUTTON_R2:     rtKey = true; drive(); return true;
            case KeyEvent.KEYCODE_DPAD_UP:
            case KeyEvent.KEYCODE_DPAD_DOWN:
            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_DPAD_RIGHT:    dpad(code); return true;
            case KeyEvent.KEYCODE_BUTTON_A:      act(shift ? Car.FOLLOW : Car.TURBO); return true;
            case KeyEvent.KEYCODE_BUTTON_B:      act(shift ? Car.GUARD : Car.STOP); return true;
            case KeyEvent.KEYCODE_BUTTON_X:      act(shift ? Car.KEEPER : Car.KICK); return true;
            case KeyEvent.KEYCODE_BUTTON_Y:      act(shift ? Car.PET : Car.DANCE); return true;
            case KeyEvent.KEYCODE_BUTTON_R1:     act(Car.SPEED_TOGGLE); return true;
            case KeyEvent.KEYCODE_BUTTON_THUMBL: act(Car.HORN); return true;
            case KeyEvent.KEYCODE_BUTTON_THUMBR: act(Car.BRAKE); return true;
            case KeyEvent.KEYCODE_BUTTON_START:  act(Car.RADAR); return true;
            case KeyEvent.KEYCODE_BUTTON_SELECT:
            case KeyEvent.KEYCODE_BACK:          act(shift ? Car.MUTE : Car.FOOTBALL); return true;
            default:                             return true;     // other buttons do nothing (and don't close the app)
        }
    }

    boolean onKeyUp(int code, KeyEvent e) {
        if (!fromGamepad(e))
            return false;
        switch (code) {
            case KeyEvent.KEYCODE_BUTTON_L1: shift = false; break;
            case KeyEvent.KEYCODE_BUTTON_L2: ltKey = false; drive(); break;
            case KeyEvent.KEYCODE_BUTTON_R2: rtKey = false; drive(); break;
        }
        return true;
    }

    private void dpad(int code) {
        switch (code) {
            case KeyEvent.KEYCODE_DPAD_UP:    act(shift ? Car.MUSIC : Car.SEARCH); break;
            case KeyEvent.KEYCODE_DPAD_RIGHT: act(shift ? Car.AUTO_FORWARD : Car.CHAOS); break;
            case KeyEvent.KEYCODE_DPAD_DOWN:  act(shift ? Car.GESTURE : Car.DONUT); break;
            case KeyEvent.KEYCODE_DPAD_LEFT:  act(shift ? Car.AUTO_BACKWARD : Car.TURN180); break;
        }
    }

    private void act(char c) {
        listener.onPadAction(c);
    }

    // RT/LT = gas and reverse, steered with the stick; no trigger = drive with the stick alone
    private void drive() {
        float gas = Math.max(rtAxis, rtKey ? 1 : 0) - Math.max(ltAxis, ltKey ? 1 : 0);
        char dir;
        float showY;
        if (Math.abs(gas) > 0.25f) {
            if (gas > 0)
                dir = stickX < -0.5f ? Car.CURVE_LEFT : stickX > 0.5f ? Car.CURVE_RIGHT : Car.FORWARD;
            else
                dir = Car.BACKWARD;
            showY = -gas;
        } else {
            dir = Car.direction(stickX, stickY);
            showY = stickY;
        }
        if (dir == Car.STOP && !driving)
            return;                                // (the stick resting: don't keep sending stop)
        driving = dir != Car.STOP;
        listener.onPadDrive(dir, stickX, showY);
    }

    private static float deadZone(float v) {
        return Math.abs(v) < 0.15f ? 0 : v;
    }
}
