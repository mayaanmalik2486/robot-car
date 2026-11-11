package io.github.mayaanmalik2486.robotcar;

import android.os.Build;
import android.os.CombinedVibration;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

/**
 * A game controller connected to the phone - made for the GameSir Cyclone 2, and it works
 * with Xbox, PlayStation and other controllers too.
 *
 * The Cyclone 2 can talk in four ways (the colour of its Home light), and Android sees each a
 * little differently. This class makes them all look the same:
 * <pre>
 *   yellow  Android (HID)   over Bluetooth. Right stick = Z / RZ, triggers = GAS / BRAKE or L/R TRIGGER
 *   green   XInput          the 2.4 GHz dongle in the phone (USB-C adapter). The View button arrives
 *                           as BACK (it must not close the app!), Home as HOME
 *   blue    DS4             like a PlayStation pad: triggers come as a button AND an axis
 *   red     Switch          the triggers are only on / off (buttons L2 / R2), no axis
 * </pre>
 * The D-pad arrives as keys on some, as a "hat" axis on others (and as both on a few): PadState
 * counts every press once. The sticks: left = throttle (up / down), right = steering
 * (left / right), with a round dead zone. Buttons go through the ControllerMap.
 * The back buttons M1 / M2 have no code of their own: in the GameSir app you set them to
 * copy another button, and then they work like that button.
 */
final class ControllerInput implements PadState.Listener {

    interface Listener {
        /** A button with a job was pressed (job = a Car letter or Car.SPEED_TOGGLE). */
        void onPadJob(char job, int button);

        /** The sticks: throttle -1 ... 1 (up = forward), steer -1 ... 1 (right). */
        void onPadDrive(float throttle, float steer);

        /** A button went down or up (always - also on the controller screen, where buttons have no jobs). */
        void onPadButton(int button, boolean down);

        /** Any change at all (for the live drawing). */
        void onPadChanged();

        /** A controller started talking (or a different one did). */
        void onPadDevice(InputDevice device);
    }

    final PadState buttons = new PadState(this);
    final ControllerMap map = new ControllerMap();
    private final Listener listener;
    float deadZone = 0.12f;                        // (the same as Settings.DEAD_ZONE)
    boolean leftSteers;
    boolean enabled = true;                        // false = buttons are only shown, not used (the controller screen)

    // raw stick values, -1 ... 1 (for the drawing), and what was last given to the driving
    float lx, ly, rx, ry;
    private float sentThrottle, sentSteer;
    private float zFirst = Float.NaN, rxFirst = Float.NaN;
    private boolean zMoved, rxMoved;
    int deviceId = -1;
    String name;
    String descriptor;
    boolean other;                                 // a button with no job of its own (Share, Home ...) is held

    ControllerInput(Listener listener) {
        this.listener = listener;
    }

    // ------------------------------------------------------------------ which events are the controller's

    static boolean isGamepad(InputDevice d) {
        if (d == null || d.isVirtual())
            return false;
        int s = d.getSources();
        return (s & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
            || (s & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }

    static boolean fromGamepad(KeyEvent e) {
        int s = e.getSource();
        if ((s & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
            || (s & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)
            return true;
        // a D-pad or BACK key from a device that is a controller (some send them as "keyboard")
        return ((s & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD || (s & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD)
            && isGamepad(e.getDevice()) && buttonFor(e.getKeyCode()) >= 0;
    }

    /** Android's key code -> our button (ControllerMap.A ...), or -1. */
    static int buttonFor(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A:      return ControllerMap.A;
            case KeyEvent.KEYCODE_BUTTON_B:      return ControllerMap.B;
            case KeyEvent.KEYCODE_BUTTON_X:      return ControllerMap.X;
            case KeyEvent.KEYCODE_BUTTON_Y:      return ControllerMap.Y;
            case KeyEvent.KEYCODE_BUTTON_L1:     return ControllerMap.LB;
            case KeyEvent.KEYCODE_BUTTON_R1:     return ControllerMap.RB;
            case KeyEvent.KEYCODE_BUTTON_L2:     return ControllerMap.LT;
            case KeyEvent.KEYCODE_BUTTON_R2:     return ControllerMap.RT;
            case KeyEvent.KEYCODE_BUTTON_THUMBL: return ControllerMap.L3;
            case KeyEvent.KEYCODE_BUTTON_THUMBR: return ControllerMap.R3;
            case KeyEvent.KEYCODE_BUTTON_START:
            case KeyEvent.KEYCODE_MENU:          return ControllerMap.START;      // (Menu: some Android modes)
            case KeyEvent.KEYCODE_BUTTON_SELECT:
            case KeyEvent.KEYCODE_BACK:          return ControllerMap.SELECT;     // (Back: the XInput dongle's View button)
            case KeyEvent.KEYCODE_DPAD_UP:       return ControllerMap.UP;
            case KeyEvent.KEYCODE_DPAD_RIGHT:    return ControllerMap.RIGHT;
            case KeyEvent.KEYCODE_DPAD_DOWN:     return ControllerMap.DOWN;
            case KeyEvent.KEYCODE_DPAD_LEFT:     return ControllerMap.LEFT;
            default:                             return -1;
        }
    }

    // ------------------------------------------------------------------ events

    /** A key. Returns true if it was the controller's (then nothing else may use it). */
    boolean onKey(KeyEvent e) {
        int code = e.getKeyCode();
        if (code == KeyEvent.KEYCODE_HOME || code == KeyEvent.KEYCODE_BUTTON_MODE
            || code == KeyEvent.KEYCODE_VOLUME_UP || code == KeyEvent.KEYCODE_VOLUME_DOWN)
            return false;                                   // Home and the volume belong to Android
        if (!fromGamepad(e))
            return false;
        noteDevice(e.getDevice());
        int b = buttonFor(code);
        boolean down = e.getAction() == KeyEvent.ACTION_DOWN;
        if (b < 0) {                                        // Share / Capture and other extras: shown, no job
            if (e.getAction() != KeyEvent.ACTION_MULTIPLE && other != down) {
                other = down;
                listener.onPadChanged();
            }
            return true;
        }
        if (down && e.getRepeatCount() > 0)
            return true;                                    // (held down: Android repeats it - ignore)
        if (down || e.getAction() == KeyEvent.ACTION_UP)
            buttons.key(b, down);
        return true;
    }

    /** Sticks, triggers and a D-pad "hat". Returns true if it was the controller's. */
    boolean onMotion(MotionEvent e) {
        if ((e.getSource() & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK || e.getActionMasked() != MotionEvent.ACTION_MOVE)
            return false;
        noteDevice(e.getDevice());
        for (int i = 0; i < e.getHistorySize(); i++)      // (Android packs fast moves together: read every one)
            read(e, i);
        read(e, -1);
        drive();
        listener.onPadChanged();
        return true;
    }

    private static float axis(MotionEvent e, int axis, int pos) {
        return pos < 0 ? e.getAxisValue(axis) : e.getHistoricalAxisValue(axis, pos);
    }

    private void read(MotionEvent e, int pos) {
        lx = axis(e, MotionEvent.AXIS_X, pos);
        ly = axis(e, MotionEvent.AXIS_Y, pos);

        // the right stick: Z / RZ on most, RX / RY on some - use whichever really moves
        float zx = axis(e, MotionEvent.AXIS_Z, pos), zy = axis(e, MotionEvent.AXIS_RZ, pos);
        float qx = axis(e, MotionEvent.AXIS_RX, pos), qy = axis(e, MotionEvent.AXIS_RY, pos);
        if (Float.isNaN(zFirst)) {
            zFirst = zx;
            rxFirst = qx;
        }
        zMoved |= Math.abs(zx - zFirst) > 0.4f || Math.abs(zy) > 0.4f;
        rxMoved |= Math.abs(qx - rxFirst) > 0.4f || Math.abs(qy) > 0.4f;
        boolean useRx = rxMoved && !zMoved;
        rx = useRx ? qx : zx;
        ry = useRx ? qy : zy;

        // triggers: two names for the same thing - take the bigger
        buttons.trigger(0, Math.max(axis(e, MotionEvent.AXIS_LTRIGGER, pos), axis(e, MotionEvent.AXIS_BRAKE, pos)));
        buttons.trigger(1, Math.max(axis(e, MotionEvent.AXIS_RTRIGGER, pos), axis(e, MotionEvent.AXIS_GAS, pos)));
        buttons.hat(axis(e, MotionEvent.AXIS_HAT_X, pos), axis(e, MotionEvent.AXIS_HAT_Y, pos));
    }

    /** The throttle after the dead zone (up = forward). */
    float throttle() {
        float k = DriveLogic.deadZone(lx, ly, deadZone);
        return -ly * k;
    }

    /** The steering after the dead zone (right = positive). */
    float steer() {
        float k = DriveLogic.deadZone(rx, ry, deadZone);
        float s = rx * k;
        if (s == 0 && leftSteers)
            s = lx * DriveLogic.deadZone(lx, ly, deadZone);
        return s;
    }

    private void drive() {
        float t = throttle(), s = steer();
        if (t == sentThrottle && s == sentSteer)
            return;                                         // (resting sticks: nothing new)
        sentThrottle = t;
        sentSteer = s;
        if (enabled)
            listener.onPadDrive(t, s);
    }

    private void noteDevice(InputDevice d) {
        if (d == null || d.getId() == deviceId)
            return;
        deviceId = d.getId();
        name = d.getName();
        descriptor = d.getDescriptor();
        zFirst = rxFirst = Float.NaN;
        zMoved = rxMoved = false;
        listener.onPadDevice(d);
    }

    /** Is a SHIFT button held right now? */
    boolean shiftHeld() {
        for (int b = 0; b < ControllerMap.BUTTONS; b++)
            if (buttons.down(b) && map.job(b, false) == ControllerMap.SHIFT)
                return true;
        return false;
    }

    @Override
    public void onButton(int button, boolean down) {
        listener.onPadButton(button, down);
        listener.onPadChanged();
        if (!down || !enabled)
            return;
        char job = map.job(button, shiftHeld());
        if (job != ControllerMap.SHIFT && job != ControllerMap.NONE)
            listener.onPadJob(job, button);
    }

    /** The controller was unplugged or switched off: everything let go, sticks centred. */
    void lost() {
        lx = ly = rx = ry = 0;
        other = false;
        buttons.releaseAll();
        deviceId = -1;
        drive();
        listener.onPadChanged();
    }

    // ------------------------------------------------------------------ rumble

    /** Makes the controller rumble (if it can - not every controller does over Bluetooth). */
    static void rumble(InputDevice d, long[] pattern) {
        if (d == null)
            return;
        try {
            if (Build.VERSION.SDK_INT >= 31) {
                VibratorManager vm = d.getVibratorManager();
                if (vm.getVibratorIds().length > 0) {
                    vm.vibrate(CombinedVibration.createParallel(VibrationEffect.createWaveform(pattern, -1)));
                    return;
                }
            }
            Vibrator v = d.getVibrator();
            if (v == null || !v.hasVibrator())
                return;
            if (Build.VERSION.SDK_INT >= 26)
                v.vibrate(VibrationEffect.createWaveform(pattern, -1));
            else
                v.vibrate(pattern, -1);
        } catch (RuntimeException e) {
            // this controller can't rumble
        }
    }
}
