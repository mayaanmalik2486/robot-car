package io.github.mayaanmalik2486.robotcar;

import android.app.Activity;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.view.Surface;

/**
 * Tilt driving: the phone's motion sensor works like a steering wheel. How you hold the phone
 * when you put your finger down is "straight"; tip the top away to go forward, toward you to
 * go back, and turn it like a wheel to steer.
 */
final class TiltDriver implements SensorEventListener {

    interface Listener {
        /** The tilt as a stick position (x, y from -1 to 1, y down = positive). */
        void onTilt(float x, float y);
    }

    private static final double FULL_TILT = Math.toRadians(25);   // tilting this far = the stick all the way

    private final Activity activity;
    private final Listener listener;
    private final SensorManager sensors;
    private final Sensor sensor;
    private boolean running;
    private boolean haveCenter;
    private double pitch0, roll0;               // the "straight" position
    private double pitch, roll;

    TiltDriver(Activity activity, Listener listener) {
        this.activity = activity;
        this.listener = listener;
        sensors = (SensorManager) activity.getSystemService(Context.SENSOR_SERVICE);
        Sensor s = sensors == null ? null : sensors.getDefaultSensor(Sensor.TYPE_GRAVITY);
        if (s == null && sensors != null)
            s = sensors.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        sensor = s;
    }

    boolean available() {
        return sensor != null;
    }

    void start() {
        if (running || sensor == null)
            return;
        running = true;
        haveCenter = false;
        sensors.registerListener(this, sensor, SensorManager.SENSOR_DELAY_GAME);
    }

    void stop() {
        if (!running)
            return;
        running = false;
        sensors.unregisterListener(this);
    }

    /** "The way the phone is held right now is straight." */
    void center() {
        pitch0 = pitch;
        roll0 = roll;
        haveCenter = true;
    }

    @Override
    public void onSensorChanged(SensorEvent e) {
        // turn the phone's own axes into the screen's (the app runs sideways)
        float x = e.values[0], y = e.values[1], z = e.values[2];
        float sx, sy;
        switch (activity.getWindowManager().getDefaultDisplay().getRotation()) {
            case Surface.ROTATION_90:  sx = -y; sy = x;  break;
            case Surface.ROTATION_180: sx = -x; sy = -y; break;
            case Surface.ROTATION_270: sx = y;  sy = -x; break;
            default:                   sx = x;  sy = y;  break;
        }
        double g = Math.sqrt(sx * sx + sy * sy + z * z);
        if (g < 1)
            return;
        pitch = Math.atan2(sy, z);                         // the top tipped away = smaller
        roll = Math.asin(Math.max(-1, Math.min(1, sx / g)));   // the right side down = smaller
        if (!haveCenter)
            center();
        float stickX = (float) (-(roll - roll0) / FULL_TILT);
        float stickY = (float) ((pitch - pitch0) / FULL_TILT);
        listener.onTilt(Math.max(-1, Math.min(1, stickX)), Math.max(-1, Math.min(1, stickY)));
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }
}
