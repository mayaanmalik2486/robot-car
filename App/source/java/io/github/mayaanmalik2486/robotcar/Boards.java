package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * The containers that place things exactly where Layout says (so what the tests checked is
 * what you see). They follow the window: rotate the phone, split the screen or resize a
 * Chromebook window and everything moves to its new place.
 */
final class Boards {

    /** The main screen: every part has a name from Layout (Layout.STICK, Layout.LCD ...). */
    static final class Screen extends ViewGroup {
        interface Style {
            int style();
        }

        private final Map<String, View> parts = new LinkedHashMap<>();
        private final Style style;
        private Map<String, Layout.Box> boxes;

        Screen(Context c, Style style) {
            super(c);
            this.style = style;
            setClipChildren(false);
        }

        void put(String name, View v) {
            parts.put(name, v);
            addView(v);
        }

        @Override
        protected void onMeasure(int ws, int hs) {
            int w = MeasureSpec.getSize(ws), h = MeasureSpec.getSize(hs);
            float d = getResources().getDisplayMetrics().density;
            boxes = Layout.screen(Math.round(w / d), Math.round(h / d), style.style());
            for (Map.Entry<String, View> e : parts.entrySet()) {
                Layout.Box b = boxes.get(e.getKey());
                View v = e.getValue();
                if (b == null) {
                    v.setVisibility(GONE);
                    continue;
                }
                v.setVisibility(VISIBLE);
                v.measure(MeasureSpec.makeMeasureSpec(Math.round(b.w * d), MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(Math.round(b.h * d), MeasureSpec.EXACTLY));
            }
            setMeasuredDimension(w, h);
        }

        @Override
        protected void onLayout(boolean changed, int l, int t, int r, int b) {
            if (boxes == null)
                return;
            float d = getResources().getDisplayMetrics().density;
            for (Map.Entry<String, View> e : parts.entrySet()) {
                Layout.Box box = boxes.get(e.getKey());
                if (box == null)
                    continue;
                int x = Math.round(box.x * d), y = Math.round(box.y * d);
                View v = e.getValue();
                v.layout(x, y, x + v.getMeasuredWidth(), y + v.getMeasuredHeight());
            }
        }
    }

    /** A page of buttons in a grid (Layout.grid). */
    static final class Grid extends ViewGroup {
        private int[] g;

        Grid(Context c) {
            super(c);
            setClipChildren(false);
        }

        @Override
        protected void onMeasure(int ws, int hs) {
            int w = MeasureSpec.getSize(ws), h = MeasureSpec.getSize(hs);
            float d = getResources().getDisplayMetrics().density;
            g = Layout.grid(getChildCount(), Math.round(w / d), Math.round(h / d));
            for (int i = 0; i < getChildCount(); i++)
                getChildAt(i).measure(MeasureSpec.makeMeasureSpec(Math.round(g[2] * d), MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(Math.round(g[3] * d), MeasureSpec.EXACTLY));
            setMeasuredDimension(w, h);
        }

        @Override
        protected void onLayout(boolean changed, int l, int t, int r, int b) {
            float d = getResources().getDisplayMetrics().density;
            for (int i = 0; i < getChildCount(); i++) {
                int col = i % g[0], row = i / g[0];
                int x = Math.round((g[4] + col * (g[2] + Layout.GAP)) * d);
                int y = Math.round((g[5] + row * (g[3] + Layout.GAP)) * d);
                View v = getChildAt(i);
                v.layout(x, y, x + v.getMeasuredWidth(), y + v.getMeasuredHeight());
            }
        }
    }

    /** The radar page: the radar, with its start / stop button under it - or beside it on a wide, low page. */
    static final class RadarPage extends ViewGroup {
        private final View radar, button;
        private boolean side;

        RadarPage(Context c, View radar, View button) {
            super(c);
            this.radar = radar;
            this.button = button;
            addView(radar);
            addView(button);
        }

        @Override
        protected void onMeasure(int ws, int hs) {
            int w = MeasureSpec.getSize(ws), h = MeasureSpec.getSize(hs);
            float d = getResources().getDisplayMetrics().density;
            int gap = Math.round(Layout.GAP * d);
            side = w > h * 2;
            int bw = side ? Math.round(Layout.RADAR_BUTTON * d) : w, bh = side ? h : Math.round(48 * d);
            button.measure(MeasureSpec.makeMeasureSpec(bw, MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(bh, MeasureSpec.EXACTLY));
            radar.measure(MeasureSpec.makeMeasureSpec(side ? w - bw - gap : w, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(side ? h : h - bh - gap, MeasureSpec.EXACTLY));
            setMeasuredDimension(w, h);
        }

        @Override
        protected void onLayout(boolean changed, int l, int t, int r, int b) {
            radar.layout(0, 0, radar.getMeasuredWidth(), radar.getMeasuredHeight());
            int w = r - l, h = b - t;
            button.layout(w - button.getMeasuredWidth(), h - button.getMeasuredHeight(), w, h);
        }
    }

    private Boards() {
    }
}
