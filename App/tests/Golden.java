package io.github.mayaanmalik2486.robotcar;

import java.io.File;
import java.io.PrintWriter;
import java.util.Map;

/**
 * Writes down what the app's Java brain does - the layout for many screen sizes, the wheel
 * mixing, and long random drives - so the web page's JavaScript copy (Web\logic.js) can be
 * checked against it: Web\test-logic.js must get exactly the same.
 */
public final class Golden {

    private static int seed;

    static int rand() {
        seed = seed * 1103515245 + 12345;
        return seed & 0x7fffffff;
    }

    public static void main(String[] args) throws Exception {
        String repo = args.length > 0 ? args[0] : ".";
        StringBuilder j = new StringBuilder("{\n\"layouts\": [");
        int[][] sizes = {{640, 360}, {692, 360}, {731, 411}, {800, 360}, {851, 393}, {915, 412}, {960, 540}, {1024, 600}, {1280, 800},
            {1366, 768}, {1440, 900}, {1920, 1080}, {2560, 1600}, {640, 480}, {700, 700}, {600, 900}, {800, 1280}, {480, 640}, {1000, 400},
            {1536, 730}, {1280, 619}, {1366, 625}, {390, 844}};
        boolean first = true;
        for (int[] s : sizes)
            for (int style = 0; style < 3; style++) {
                Map<String, Layout.Box> b = Layout.screen(s[0], s[1], style);
                StringBuilder o = new StringBuilder();
                for (Map.Entry<String, Layout.Box> e : b.entrySet()) {
                    Layout.Box x = e.getValue();
                    o.append(o.length() == 0 ? "" : ",").append('"').append(e.getKey()).append("\":[").append(x.x).append(',').append(x.y)
                        .append(',').append(x.w).append(',').append(x.h).append(']');
                }
                Layout.Box p = b.get(Layout.PAGES);
                int[] g8 = Layout.grid(8, p.w, p.h), g6 = Layout.grid(6, p.w, p.h);
                j.append(first ? "" : ",").append("\n {\"w\":").append(s[0]).append(",\"h\":").append(s[1]).append(",\"style\":").append(style)
                    .append(",\"boxes\":{").append(o).append("},\"grid8\":").append(arr(g8)).append(",\"grid6\":").append(arr(g6)).append('}');
                first = false;
            }
        j.append("],\n\"mix\": [");
        int[] o = new int[2];
        first = true;
        for (int t = -10; t <= 10; t++)
            for (int s = -10; s <= 10; s++)
                for (int slow = 0; slow < 2; slow++) {
                    DriveLogic.mix(t / 10f, s / 10f, slow == 1, 0.6f, o);
                    j.append(first ? "" : ",").append('[').append(t).append(',').append(s).append(',').append(slow).append(',').append(o[0]).append(',').append(o[1]).append(']');
                    first = false;
                }
        j.append("],\n\"drives\": [");
        for (int run = 0; run < 6; run++) {
            seed = 1000 + run;
            final StringBuilder codes = new StringBuilder();
            final long[] clock = new long[1];
            DriveLogic d = new DriveLogic(c -> codes.append(codes.length() == 0 ? "" : ",").append(clock[0]).append(',').append(c));
            d.setSteering(run % 2 == 0);
            d.setOneStick(run >= 4);
            d.reset();
            char[] jobs = {'4', 'I', 'T', 'Y', 'X', '3', 'k', 'u', 'Q', 'W', 'O'};
            long now = 0;
            for (int step = 0; step < 4000; step++) {
                now += 4 + rand() % 57;
                clock[0] = now;
                int r = rand() % 1000;
                if (r < 650) {
                    // sticks: often centred, often held still, sometimes anywhere
                    int kind = rand() % 4;
                    float t = kind == 0 ? 0 : (rand() % 2001 - 1000) / 1000f;
                    float s = kind == 1 ? 0 : (rand() % 2001 - 1000) / 1000f;
                    if (kind == 3) {
                        t = 0;
                        s = 0;
                    }
                    d.input(t, s, now);
                } else if (r < 900) {
                    d.tick(now);
                } else if (r < 950) {
                    d.actionSent(jobs[rand() % jobs.length], now);
                } else if (r < 965) {
                    d.stopAll(now);
                } else if (r < 972) {
                    d.halt(now);
                } else if (r < 985) {
                    d.setSlowTop(rand() % 2 == 0);
                } else if (r < 992) {
                    d.carSpeedIs(rand() % 2 == 0);
                } else {
                    d.setSteering(rand() % 2 == 0);
                }
            }
            j.append(run == 0 ? "" : ",").append("\n [").append(codes).append(']');
        }
        j.append("]\n}\n");
        File out = new File(repo, "Tools/build/golden.json");
        if (out.getParentFile().isDirectory())
            try (PrintWriter w = new PrintWriter(out, "UTF-8")) {
                w.print(j);
            }
        System.out.println("  wrote " + out + " (" + sizes.length * 3 + " layouts, 882 mixes, 6 long drives)");
    }

    private static String arr(int[] a) {
        StringBuilder s = new StringBuilder("[");
        for (int i = 0; i < a.length; i++)
            s.append(i == 0 ? "" : ",").append(a[i]);
        return s.append(']').toString();
    }

    private Golden() {
    }
}
