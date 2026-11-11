package io.github.mayaanmalik2486.robotcar;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.RadialGradient;
import android.graphics.RectF;
import android.graphics.Shader;
import android.os.SystemClock;
import android.view.View;
import android.view.ViewGroup;

import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Random;

/**
 * The containers that place things exactly where Layout says (so what the tests checked is
 * what you see). They follow the window: rotate the phone, split the screen or resize a
 * Chromebook window and everything moves to its new place.
 */
final class Boards {

    /**
     * The main screen: every part has a name from Layout (Layout.STICK, Layout.LCD ...).
     * It also paints what is behind them: the night sky with slow bubbles drifting up, and
     * the track of the three tabs with the orange pill that slides to the chosen one.
     */
    static final class Screen extends ViewGroup {
        interface Style {
            int style();
        }

        private final Map<String, View> parts = new LinkedHashMap<>();
        private final Style style;
        private Map<String, Layout.Box> boxes;

        // the bubbles: where along the bottom, how big, how fast, which colour, where in their climb
        private static final int BUBBLES = 12;
        private static final int[] HUES = {24, 174, 205, 325, 45};
        private final float[] bx = new float[BUBBLES], bs = new float[BUBBLES], bv = new float[BUBBLES], bt = new float[BUBBLES], sway = new float[BUBBLES];
        private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final RectF r = new RectF();
        private Shader sky, warm, cool, deep, shine;
        private int shaderW, shaderH;
        private long lastFrame;
        private int tab;
        private float pillX = -1, pillW;

        Screen(Context c, Style style) {
            super(c);
            this.style = style;
            setClipChildren(false);
            setWillNotDraw(false);
            Random rnd = new Random(7);
            for (int i = 0; i < BUBBLES; i++) {
                bx[i] = rnd.nextFloat();
                bs[i] = 0.05f + rnd.nextFloat() * 0.13f;
                bv[i] = 1f / (16 + rnd.nextFloat() * 20);
                bt[i] = rnd.nextFloat();
                sway[i] = rnd.nextFloat() * 6.28f;
            }
        }

        void put(String name, View v) {
            parts.put(name, v);
            addView(v);
        }

        /** Which tab is chosen (the orange pill slides there). */
        void setTab(int tab) {
            this.tab = tab;
            invalidate();
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
            if (changed)
                pillX = -1;                              // (a new size: the pill jumps to its place)
        }

        @Override
        protected void onDraw(Canvas c) {
            float w = getWidth(), h = getHeight(), d = getResources().getDisplayMetrics().density;
            if (w == 0 || h == 0)
                return;
            long now = SystemClock.uptimeMillis();
            float dt = lastFrame == 0 ? 0.016f : Math.min(0.05f, (now - lastFrame) / 1000f);
            lastFrame = now;

            // the night sky, with a warm and a cool glow in the top corners and deep blue rising from below
            if (sky == null || shaderW != getWidth() || shaderH != getHeight()) {
                shaderW = getWidth();
                shaderH = getHeight();
                sky = new LinearGradient(0, 0, 0, h, Ui.BG, 0xFF0B1230, Shader.TileMode.CLAMP);
                warm = new RadialGradient(w * 0.85f, -h * 0.1f, Math.max(w, h) * 0.6f, 0x33FF7A1A, 0x00FF7A1A, Shader.TileMode.CLAMP);
                cool = new RadialGradient(w * 0.08f, 0, Math.max(w, h) * 0.5f, 0x242EE6D6, 0x002EE6D6, Shader.TileMode.CLAMP);
                deep = new RadialGradient(w * 0.5f, h * 1.1f, Math.max(w, h) * 0.75f, 0xFF1B2A6B, 0x001B2A6B, Shader.TileMode.CLAMP);
                shine = new RadialGradient(-0.36f, -0.44f, 0.5f, 0x8CFFFFFF, 0x00FFFFFF, Shader.TileMode.CLAMP);
            }
            p.setStyle(Paint.Style.FILL);
            for (Shader s : new Shader[]{sky, deep, warm, cool}) {
                p.setShader(s);
                c.drawRect(0, 0, w, h, p);
            }
            p.setShader(null);

            // the bubbles
            float unit = Math.min(w, h);
            for (int i = 0; i < BUBBLES; i++) {
                bt[i] += bv[i] * dt;
                if (bt[i] > 1)
                    bt[i] -= 1;
                float t = bt[i], rad = bs[i] * unit / 2 * (0.85f + 0.25f * t);
                float x = bx[i] * w + (float) Math.sin(t * 6.28f + sway[i]) * unit * 0.05f;
                float y = h + rad - t * (h + rad * 2.4f);
                float fade = Math.min(1, Math.min(t / 0.08f, (1 - t) / 0.08f));
                int col = Ui.hsl(HUES[i % HUES.length], 0.95f, 0.62f);
                p.setColor(Ui.alpha(col, 0.11f * fade));
                c.drawCircle(x, y, rad, p);
                p.setStyle(Paint.Style.STROKE);
                p.setStrokeWidth(d);
                p.setColor(Ui.alpha(Ui.lighter(col, 0.4f), 0.24f * fade));
                c.drawCircle(x, y, rad, p);
                p.setStyle(Paint.Style.FILL);
                c.save();                                    // its little shine
                c.translate(x, y);
                c.scale(rad, rad);
                p.setShader(shine);
                p.setAlpha(Math.round(255 * fade));
                c.drawCircle(-0.36f, -0.44f, 0.5f, p);
                p.setShader(null);
                p.setAlpha(255);
                c.restore();
            }

            // the tabs' track, and the pill on the chosen tab
            Layout.Box t0 = boxes == null ? null : boxes.get(Layout.TAB0), t2 = boxes == null ? null : boxes.get(Layout.TAB3);
            Layout.Box tc = boxes == null ? null : boxes.get("tab" + tab);
            if (t0 != null && t2 != null && tc != null) {
                r.set(t0.x * d, t0.y * d, t2.right() * d, t0.bottom() * d);
                float rad = 18 * d;
                p.setColor(0x80030612);
                c.drawRoundRect(r, rad, rad, p);
                p.setStyle(Paint.Style.STROKE);
                p.setStrokeWidth(d);
                p.setColor(0x1FFFFFFF);
                c.drawRoundRect(r, rad, rad, p);
                p.setStyle(Paint.Style.FILL);
                float wantX = (tc.x + 4) * d, wantW = (tc.w - 8) * d;
                if (pillX < 0) {
                    pillX = wantX;
                    pillW = wantW;
                }
                float k = 1 - (float) Math.exp(-dt * 16);
                pillX += (wantX - pillX) * k;
                pillW += (wantW - pillW) * k;
                float top = (tc.y + 4) * d, bottom = (tc.bottom() - 4) * d;
                for (int i = 3; i >= 1; i--) {               // its glow
                    float g = i * 3 * d;
                    p.setColor(Ui.alpha(Ui.ACCENT, 0.09f));
                    r.set(pillX - g * 0.3f, top + 3 * d - g * 0.3f, pillX + pillW + g * 0.3f, bottom + g);
                    c.drawRoundRect(r, 14 * d + g, 14 * d + g, p);
                }
                r.set(pillX, top, pillX + pillW, bottom);
                p.setShader(new LinearGradient(0, top, 0, bottom, 0xFFFF9A3D, 0xFFFF6A13, Shader.TileMode.CLAMP));
                c.drawRoundRect(r, 14 * d, 14 * d, p);
                p.setShader(new LinearGradient(0, top, 0, (top + bottom) / 2, 0x66FFFFFF, 0x00FFFFFF, Shader.TileMode.CLAMP));
                r.set(pillX + 3 * d, top + 2 * d, pillX + pillW - 3 * d, (top + bottom) / 2);
                c.drawRoundRect(r, 12 * d, 12 * d, p);
                p.setShader(null);
            }
            postInvalidateOnAnimation();                     // (the bubbles never stop)
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
            setClipChildren(false);
            addView(radar);
            addView(button);
        }

        @Override
        protected void onMeasure(int ws, int hs) {
            int w = MeasureSpec.getSize(ws), h = MeasureSpec.getSize(hs);
            float d = getResources().getDisplayMetrics().density;
            int gap = Math.round(Layout.GAP * d);
            side = w > h * 1.6f;
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
