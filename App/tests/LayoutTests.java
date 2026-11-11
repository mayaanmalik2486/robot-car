package io.github.mayaanmalik2486.robotcar;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Checks the main screen's layout for many screen and window sizes: nothing outside the
 * screen, nothing overlapping, every button big enough to tap, the sticks big enough to use,
 * and the button pages fit.
 */
public final class LayoutTests {

    static int passed, failed;

    public static void main(String[] args) {
        int[][] sizes = {
            // phones held sideways (dp)
            {640, 360}, {692, 360}, {720, 360}, {731, 411}, {760, 360}, {800, 360}, {808, 360}, {851, 393}, {869, 412}, {915, 412}, {960, 540},
            // tablets and Chromebooks
            {1024, 600}, {1024, 768}, {1138, 712}, {1280, 800}, {1366, 768}, {1366, 1024}, {1440, 900}, {1920, 1080}, {1920, 1200}, {2560, 1600},
            // windows of odd shapes (split screen, a resized window, a tablet held upright)
            {640, 480}, {700, 700}, {600, 900}, {800, 1280}, {768, 1024}, {480, 640}, {540, 720}, {1000, 400}, {2000, 500}, {900, 600},
        };
        String[] styles = {"one stick", "two sticks", "tilt"};
        for (int[] s : sizes)
            for (int style = 0; style < 3; style++)
                check(s[0], s[1], style, styles[style]);
        System.out.println();
        System.out.println(failed == 0 ? "ALL " + passed + " LAYOUT CHECKS PASSED (" + sizes.length + " sizes x 3 styles)"
            : failed + " LAYOUT CHECK(S) FAILED, " + passed + " passed");
        System.exit(failed == 0 ? 0 : 1);
    }

    static void check(int w, int h, int style, String name) {
        Map<String, Layout.Box> boxes = Layout.screen(w, h, style);
        List<String> problems = new ArrayList<>();
        List<String> keys = new ArrayList<>(boxes.keySet());

        for (String k : keys) {
            Layout.Box b = boxes.get(k);
            if (b.x < 0 || b.y < 0 || b.right() > w || b.bottom() > h)
                problems.add(k + " is off the screen (" + b + ")");
            if (b.w <= 0 || b.h <= 0)
                problems.add(k + " has no size (" + b + ")");
        }
        for (int i = 0; i < keys.size(); i++)
            for (int j = i + 1; j < keys.size(); j++) {
                Layout.Box a = boxes.get(keys.get(i)), b = boxes.get(keys.get(j));
                if (a.gap(b) < 4)
                    problems.add(keys.get(i) + " and " + keys.get(j) + " touch or overlap (" + a + " / " + b + ")");
            }
        for (String k : Layout.TOUCHED) {
            Layout.Box b = boxes.get(k);
            if (b == null)
                problems.add(k + " is missing");
            else if (b.w < Layout.MIN_TOUCH || b.h < Layout.MIN_TOUCH)
                problems.add(k + " is too small to tap (" + b + ")");
        }
        for (String k : new String[]{Layout.STICK, Layout.STICK2}) {
            Layout.Box b = boxes.get(k);
            if (b == null) {
                if (k.equals(Layout.STICK) || style == Layout.TWO_STICKS)
                    problems.add(k + " is missing");
                continue;
            }
            if (Math.min(b.w, b.h) < Layout.MIN_STICK)
                problems.add(k + " is too small to use (" + b + ")");
        }
        if (style != Layout.TWO_STICKS && boxes.containsKey(Layout.STICK2))
            problems.add("a second stick in " + name + " mode");
        Layout.Box lcd = boxes.get(Layout.LCD);
        if (lcd.h < 64 || lcd.w < 200)
            problems.add("the LCD is too small (" + lcd + ")");
        Layout.Box pages = boxes.get(Layout.PAGES);
        for (int n : new int[]{8, 6}) {
            // (the head page: a dial at least 96 dp high beside or above its button - the same room as the radar)
            int[] g = Layout.grid(n, pages.w, pages.h);
            if (g[2] < Layout.MIN_TOUCH || g[3] < Layout.MIN_TOUCH)
                problems.add("the page with " + n + " buttons is too small: " + g[2] + "x" + g[3] + " in " + pages);
            if (g[4] < 0 || g[5] < 0 || g[0] * g[2] + (g[0] - 1) * 8 > pages.w || g[1] * g[3] + (g[1] - 1) * 8 > pages.h)
                problems.add("the page with " + n + " buttons does not fit in " + pages);
        }
        // the radar page: the radar beside or above its button
        boolean side = pages.w > pages.h * 1.6f;
        int radarH = side ? pages.h : pages.h - 48 - 8;
        int radarW = side ? pages.w - Layout.RADAR_BUTTON - 8 : pages.w;
        if (radarH < 96 || radarW < 140)
            problems.add("the radar is too small: " + radarW + "x" + radarH);
        Layout.Box song = boxes.get(Layout.SONG);
        if (song != null && song.w < 96)
            problems.add("the song name has no room (" + song + ")");

        String label = w + "x" + h + " " + name;
        if (problems.isEmpty()) {
            passed++;
            System.out.println("  PASS  " + label);
        } else {
            failed++;
            System.out.println("  FAIL  " + label);
            for (String p : problems)
                System.out.println("          - " + p);
        }
    }

    private LayoutTests() {
    }
}
