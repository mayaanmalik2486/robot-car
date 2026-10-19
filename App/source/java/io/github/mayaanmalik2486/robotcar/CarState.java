package io.github.mayaanmalik2486.robotcar;

import java.util.Arrays;

/**
 * What the car has told us. It sends three kinds of lines:
 * <pre>
 *   ~MODE|1010   what it is doing, then 1/0 for: muted, football, slow, low battery
 *   #S:name      the song this car dances to (an answer to '?')
 *   #R:f42       radar: direction a..k (a = 50 degrees right, f = ahead, k = 50 degrees left), cm (999 = nothing)
 * </pre>
 */
final class CarState {

    static final int NOTHING = 0;
    static final int STATUS = 1;
    static final int SONG = 2;
    static final int RADAR = 3;

    static final int RADAR_STEPS = 11;
    static final int RADAR_RANGE_CM = 150;

    String mode = "";
    boolean muted;
    boolean football;
    boolean slow;
    boolean batteryLow;
    String song = "";
    long modeSince;                                    // when the mode last changed (the dance clock)

    final int[] radarCm = new int[RADAR_STEPS];
    final long[] radarAt = new long[RADAR_STEPS];      // when each direction was last seen (0 = never)
    int radarLook = RADAR_STEPS / 2;                   // where the sensor looked last

    CarState() {
        reset();
    }

    void reset() {
        mode = "";
        song = "";
        muted = football = slow = batteryLow = false;
        Arrays.fill(radarCm, 999);
        Arrays.fill(radarAt, 0);
        radarLook = RADAR_STEPS / 2;
    }

    /** Reads one line from the car; says which kind it was. */
    int read(String line, long now) {
        if (line.startsWith("~")) {
            int bar = line.indexOf('|');
            if (bar < 0 || line.length() < bar + 5)
                return NOTHING;
            String newMode = line.substring(1, bar).trim();
            if (!newMode.equals(mode))
                modeSince = now;
            mode = newMode;
            muted = line.charAt(bar + 1) == '1';
            football = line.charAt(bar + 2) == '1';
            slow = line.charAt(bar + 3) == '1';
            batteryLow = line.charAt(bar + 4) == '1';
            return STATUS;
        }
        if (line.startsWith("#S:")) {
            song = line.substring(3).trim();
            return SONG;
        }
        if (line.startsWith("#R:") && line.length() >= 5) {
            int k = line.charAt(3) - 'a';
            if (k < 0 || k >= RADAR_STEPS)
                return NOTHING;
            try {
                radarCm[k] = Integer.parseInt(line.substring(4).trim());
            } catch (NumberFormatException e) {
                return NOTHING;
            }
            radarAt[k] = now;
            radarLook = k;
            return RADAR;
        }
        return NOTHING;
    }

    /** The nearest thing the radar saw in the last few seconds, in cm (or -1). */
    int radarNearest(long now) {
        int best = -1;
        for (int k = 0; k < RADAR_STEPS; k++)
            if (radarAt[k] != 0 && now - radarAt[k] < 4000 && radarCm[k] < RADAR_RANGE_CM && (best < 0 || radarCm[k] < best))
                best = radarCm[k];
        return best;
    }
}
