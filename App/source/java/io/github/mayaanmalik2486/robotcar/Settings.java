package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.content.SharedPreferences;

/**
 * Everything the app remembers (saved on the phone). Controller settings - the button map
 * and the stick dead zone - are kept for each controller separately (by its Android
 * "descriptor"), so two controllers can be set up differently.
 */
final class Settings {

    static final float DEAD_ZONE = 0.12f;       // the Cyclone 2's Hall-effect sticks barely drift: 12 % is plenty
    static final float FEEL = 0.6f;

    private final SharedPreferences p;

    Settings(Context c) {
        p = c.getSharedPreferences("robotcar", Context.MODE_PRIVATE);
    }

    /** Layout.ONE_STICK, TWO_STICKS or TILT. */
    int style() {
        return p.getInt("style", Layout.ONE_STICK);
    }

    void setStyle(int s) {
        p.edit().putInt("style", s).apply();
    }

    float touchDeadZone() {
        return p.getFloat("touchDead", 0.10f);
    }

    void setTouchDeadZone(float v) {
        p.edit().putFloat("touchDead", v).apply();
    }

    /** Steering feel: 0 = direct ... 1 = very gentle around the middle. */
    float feel() {
        return p.getFloat("feel", FEEL);
    }

    void setFeel(float v) {
        p.edit().putFloat("feel", v).apply();
    }

    boolean haptics() {
        return p.getBoolean("haptics", true);
    }

    void setHaptics(boolean on) {
        p.edit().putBoolean("haptics", on).apply();
    }

    /** Show the controller's button names on the screen's buttons. */
    boolean overlay() {
        return p.getBoolean("overlay", true);
    }

    void setOverlay(boolean on) {
        p.edit().putBoolean("overlay", on).apply();
    }

    /** The top speed switch: slow (true) or fast. */
    boolean slowTop() {
        return p.getBoolean("slowTop", false);
    }

    void setSlowTop(boolean on) {
        p.edit().putBoolean("slowTop", on).apply();
    }

    /** The controller's left stick may steer too (when the right stick is in the middle). */
    boolean leftSteers() {
        return p.getBoolean("leftSteers", false);
    }

    void setLeftSteers(boolean on) {
        p.edit().putBoolean("leftSteers", on).apply();
    }

    /** The Bluetooth address of the last car. */
    String device() {
        return p.getString("device", null);
    }

    void setDevice(String address) {
        p.edit().putString("device", address).apply();
    }

    // ------------------------------------------------------------------ per controller

    private static String key(String what, String controller) {
        return what + ":" + (controller == null ? "default" : controller);
    }

    void loadMap(ControllerMap map, String controller) {
        map.fromText(p.getString(key("map", controller), p.getString(key("map", null), null)));
    }

    void saveMap(ControllerMap map, String controller) {
        p.edit().putString(key("map", controller), map.toText()).putString(key("map", null), map.toText()).apply();
    }

    float padDeadZone(String controller) {
        return p.getFloat(key("dead", controller), p.getFloat(key("dead", null), DEAD_ZONE));
    }

    void setPadDeadZone(String controller, float v) {
        p.edit().putFloat(key("dead", controller), v).putFloat(key("dead", null), v).apply();
    }

    /** Everything back as it came (the last car is kept, so it still connects by itself). */
    void reset() {
        String car = device();
        p.edit().clear().apply();
        if (car != null)
            setDevice(car);
    }
}
