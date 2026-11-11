package io.github.mayaanmalik2486.robotcar;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Where everything sits on the main screen, for any screen or window size (in dp).
 * Everything is placed on an 8 dp grid: 8 or 16 dp from the edges, 8 dp between parts.
 *
 * Wide screens (phones and tablets held sideways):
 * <pre>
 *   [connect] [song ..........] [controller] [speed] [style] [settings] [help]
 *   +--------+  +----------------------+  +--------+
 *   |        |  | LCD                  |  |  STOP  |
 *   | stick  |  +----------------------+  |   Y    |       two sticks: the right column is the
 *   |        |  | Moves|Play|Radar|Head|  | X    RT|       steering stick, and STOP + the four
 *   |        |  | page of buttons      |  |   A    |       action buttons sit in a row under the LCD
 *   +--------+  +----------------------+  +--------+
 * </pre>
 * Narrow or square windows (a Chromebook window, a tablet held upright): everything is
 * stacked, with the stick(s) at the bottom.
 *
 * No Android in here, so it can be tested on a computer (App\tests\LayoutTests).
 */
final class Layout {

    /** A rectangle in dp. */
    static final class Box {
        final int x, y, w, h;

        Box(int x, int y, int w, int h) {
            this.x = x;
            this.y = y;
            this.w = w;
            this.h = h;
        }

        int right() {
            return x + w;
        }

        int bottom() {
            return y + h;
        }

        /** The space between two boxes (negative = they overlap). */
        int gap(Box o) {
            int dx = Math.max(o.x - right(), x - o.right());
            int dy = Math.max(o.y - bottom(), y - o.bottom());
            return Math.max(dx, dy);
        }

        @Override
        public String toString() {
            return x + "," + y + " " + w + "x" + h;
        }
    }

    static final int ONE_STICK = 0, TWO_STICKS = 1, TILT = 2;

    static final String CONNECT = "connect", SONG = "song", PAD = "pad", SPEED = "speed", STYLE = "style",
        SETTINGS = "settings", HELP = "help", LCD = "lcd", TAB0 = "tab0", TAB1 = "tab1", TAB2 = "tab2", TAB3 = "tab3",
        PAGES = "pages", STICK = "stick", STICK2 = "stick2", STOP = "stop", DANCE = "dance", KICK = "kick",
        HORN = "horn", TURBO = "turbo";

    /** The parts that are tapped (they must be at least 44 dp each way). */
    static final String[] TOUCHED = {CONNECT, PAD, SPEED, STYLE, SETTINGS, HELP, TAB0, TAB1, TAB2, TAB3, STOP, DANCE, KICK, HORN, TURBO};

    static final int GAP = 8;
    static final int MIN_TOUCH = 44;
    static final int MIN_STICK = 120;
    static final int RADAR_BUTTON = 112;      // the radar page: its start / stop button beside the radar (wide pages)

    /** Everything on the main screen, for a screen w x h dp and a control style. */
    static Map<String, Box> screen(int w, int h, int style) {
        Map<String, Box> out = new LinkedHashMap<>();
        boolean two = style == TWO_STICKS;
        int m = w < 720 || h < 400 ? 8 : 16;
        int topH = h < 420 ? 40 : 48;
        topBar(out, m, m, w - 2 * m, topH);

        int y0 = m + topH + GAP;
        int bodyH = h - y0 - m;
        if (w >= h * 1.25f)
            wide(out, m, y0, w - 2 * m, bodyH, two);
        else
            stacked(out, m, y0, w - 2 * m, bodyH, two);
        return out;
    }

    private static void topBar(Map<String, Box> out, int x, int y, int w, int h) {
        boolean roomy = w >= 784, tight = w < 600;
        int connect = roomy ? 168 : tight ? 120 : 136;
        int pad = roomy ? 168 : 48;
        int chip = roomy ? 104 : tight ? 64 : 80;
        int icon = Math.max(h, MIN_TOUCH);
        int fixed = connect + pad + 2 * chip + 2 * icon + 6 * GAP;
        int song = w - fixed;
        int right = x + w;
        int cy = y + (h - Math.max(h, MIN_TOUCH)) / 2;
        int ih = Math.max(h, MIN_TOUCH);

        out.put(CONNECT, new Box(x, cy, connect, ih));
        right -= icon;
        out.put(HELP, new Box(right, cy, icon, ih));
        right -= GAP + icon;
        out.put(SETTINGS, new Box(right, cy, icon, ih));
        right -= GAP + chip;
        out.put(STYLE, new Box(right, cy, chip, ih));
        right -= GAP + chip;
        out.put(SPEED, new Box(right, cy, chip, ih));
        right -= GAP + pad;
        out.put(PAD, new Box(right, cy, pad, ih));
        if (song >= 96)
            out.put(SONG, new Box(x + connect + GAP, cy, song, ih));
    }

    private static void wide(Map<String, Box> out, int x, int y, int w, int h, boolean two) {
        int side = snap(Math.min(w * 0.28f, h * 1.1f));
        side = Math.max(152, Math.min(360, side));
        int cw = w - 2 * side - 2 * GAP;
        if (cw < 256) {
            side = snap((w - 2 * GAP - 256) / 2f);
            cw = w - 2 * side - 2 * GAP;
        }
        int cx = x + side + GAP;
        // the 3D scene gets everything the button rows and the pages don't need
        int rows = (two ? 56 : 0) + 56 + GAP;
        int pagesH = Math.max(104, Math.min(264, snap(h * 0.36f)));
        int lcdH = Math.max(64, Math.min(400, h - rows - pagesH));

        int cy = y;
        out.put(LCD, new Box(cx, cy, cw, lcdH));
        cy += lcdH + GAP;
        if (two) {
            actionRow(out, cx, cy, cw, 48);
            cy += 48 + GAP;
        }
        tabs(out, cx, cy, cw, 48);
        cy += 48 + GAP;
        out.put(PAGES, new Box(cx, cy, cw, y + h - cy));

        int rx = x + w - side;
        out.put(STICK, new Box(x, y, side, h));
        if (two) {
            out.put(STICK2, new Box(rx, y, side, h));
            return;
        }
        out.put(STOP, new Box(rx, y, side, 48));
        diamond(out, rx, y + 48 + GAP, side, h - 48 - GAP);
    }

    private static void stacked(Map<String, Box> out, int x, int y, int w, int h, boolean two) {
        int lcdH = Math.max(64, Math.min(240, snap(h * 0.22f)));
        int stickH = Math.max(136, Math.min(320, snap(Math.min(w * 0.5f, h * 0.38f))));
        int cy = y;
        out.put(LCD, new Box(x, cy, w, lcdH));
        cy += lcdH + GAP;
        actionRow(out, x, cy, w, 48);
        cy += 48 + GAP;
        tabs(out, x, cy, w, 48);
        cy += 48 + GAP;
        int by = y + h - stickH;
        out.put(PAGES, new Box(x, cy, w, by - GAP - cy));
        if (two) {
            out.put(STICK, new Box(x, by, stickH, stickH));
            out.put(STICK2, new Box(x + w - stickH, by, stickH, stickH));
        } else {
            out.put(STICK, new Box(x + (w - stickH) / 2, by, stickH, stickH));
        }
    }

    // STOP (a bit wider) and dance, kick, horn, turbo in one row
    private static void actionRow(Map<String, Box> out, int x, int y, int w, int h) {
        float unit = (w - 4 * GAP) / 5.4f;
        int stop = Math.round(unit * 1.4f);
        int each = (w - 4 * GAP - stop) / 4;
        stop = w - 4 * GAP - 4 * each;
        out.put(STOP, new Box(x, y, stop, h));
        String[] names = {DANCE, KICK, HORN, TURBO};
        int bx = x + stop + GAP;
        for (String n : names) {
            out.put(n, new Box(bx, y, each, h));
            bx += each + GAP;
        }
    }

    private static void tabs(Map<String, Box> out, int x, int y, int w, int h) {
        int each = (w - 3 * GAP) / 4;
        out.put(TAB0, new Box(x, y, each, h));
        out.put(TAB1, new Box(x + each + GAP, y, each, h));
        out.put(TAB2, new Box(x + 2 * (each + GAP), y, each, h));
        out.put(TAB3, new Box(x + 3 * (each + GAP), y, w - 3 * (each + GAP), h));
    }

    // like the controller's face buttons: horn on top, kick left, turbo right, dance below
    private static void diamond(Map<String, Box> out, int x, int y, int w, int h) {
        int sq = Math.min(w, h);
        int cell = sq / 3;
        int b = Math.min(cell - 4, 96);
        int cx = x + w / 2, cy = y + h / 2;
        out.put(HORN, new Box(cx - b / 2, cy - cell - b / 2, b, b));
        out.put(KICK, new Box(cx - cell - b / 2, cy - b / 2, b, b));
        out.put(TURBO, new Box(cx + cell - b / 2, cy - b / 2, b, b));
        out.put(DANCE, new Box(cx - b / 2, cy + cell - b / 2, b, b));
    }

    /**
     * A grid for n buttons in a box: {columns, rows, cell width, cell height, left offset, top offset}.
     * The cells are as big as possible (but not giant on tablets), centred in the box.
     */
    static int[] grid(int n, int w, int h) {
        int bestCols = 1;
        float best = -1;
        for (int cols = 1; cols <= n; cols++) {
            int rows = (n + cols - 1) / cols;
            float cw = (w - (cols - 1) * GAP) / (float) cols;
            float ch = (h - (rows - 1) * GAP) / (float) rows;
            float score = Math.min(cw / 1.3f, ch);
            if (score > best) {
                best = score;
                bestCols = cols;
            }
        }
        int cols = bestCols, rows = (n + cols - 1) / cols;
        int cw = Math.min(240, (w - (cols - 1) * GAP) / cols);
        int ch = Math.min(160, (h - (rows - 1) * GAP) / rows);
        int ox = (w - (cols * cw + (cols - 1) * GAP)) / 2;
        int oy = (h - (rows * ch + (rows - 1) * GAP)) / 2;
        return new int[]{cols, rows, cw, ch, ox, oy};
    }

    /** Rounds down to the 8 dp grid. */
    static int snap(float v) {
        return (int) (v / 8) * 8;
    }

    private Layout() {
    }
}
