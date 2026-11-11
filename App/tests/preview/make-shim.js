'use strict';
// Writes the small stand-in Android classes the preview tool needs (android.* drawn with the
// PC's own Java 2D), into the folder given. See Preview.java.
const fs = require('fs');
const path = require('path');
const out = process.argv[2];
const files = {
'util/DisplayMetrics.java': `package android.util;
public class DisplayMetrics { public float density = 2f; public int widthPixels = 1600, heightPixels = 800; }`,
'content/res/Resources.java': `package android.content.res;
public class Resources { private final android.util.DisplayMetrics m = new android.util.DisplayMetrics(); public android.util.DisplayMetrics getDisplayMetrics() { return m; } }`,
'content/Context.java': `package android.content;
public class Context { private final android.content.res.Resources r = new android.content.res.Resources(); public android.content.res.Resources getResources() { return r; } }`,
'os/SystemClock.java': `package android.os;
public class SystemClock { public static long now = 100000; public static long uptimeMillis() { return now; } }`,
'os/Build.java': `package android.os;
public class Build { public static class VERSION { public static int SDK_INT = 21; } }`,
'text/TextUtils.java': `package android.text;
public class TextUtils { public enum TruncateAt { END } public static boolean equals(CharSequence a, CharSequence b) { return a == b || (a != null && b != null && a.toString().equals(b.toString())); } }`,
'text/Spanned.java': `package android.text;
public interface Spanned extends CharSequence { int SPAN_EXCLUSIVE_EXCLUSIVE = 33; }`,
'text/SpannableString.java': `package android.text;
import java.util.*;
public class SpannableString implements Spanned {
    public final String s; public final List<Object[]> spans = new ArrayList<>();
    public SpannableString(CharSequence c) { s = c.toString(); }
    public void setSpan(Object what, int start, int end, int flags) { spans.add(new Object[]{what, start, end}); }
    public int length() { return s.length(); } public char charAt(int i) { return s.charAt(i); }
    public CharSequence subSequence(int a, int b) { return s.substring(a, b); } public String toString() { return s; }
}`,
'text/style/RelativeSizeSpan.java': `package android.text.style;
public class RelativeSizeSpan { public final float f; public RelativeSizeSpan(float f) { this.f = f; } }`,
'text/style/ForegroundColorSpan.java': `package android.text.style;
public class ForegroundColorSpan { public final int c; public ForegroundColorSpan(int c) { this.c = c; } }`,
'text/style/TypefaceSpan.java': `package android.text.style;
public class TypefaceSpan { public final String f; public TypefaceSpan(String f) { this.f = f; } }`,
'view/animation/DecelerateInterpolator.java': `package android.view.animation;
public class DecelerateInterpolator { public DecelerateInterpolator() { } public DecelerateInterpolator(float f) { } }`,
'view/animation/OvershootInterpolator.java': `package android.view.animation;
public class OvershootInterpolator { public OvershootInterpolator(float f) { } }`,
'animation/ValueAnimator.java': `package android.animation;
public class ValueAnimator {
    public interface AnimatorUpdateListener { void onAnimationUpdate(ValueAnimator a); }
    public static ValueAnimator ofFloat(float... v) { return new ValueAnimator(); }
    public ValueAnimator setDuration(long d) { return this; } public void addUpdateListener(AnimatorUpdateListener l) { } public void start() { }
    public Object getAnimatedValue() { return 1f; }
}`,
'view/Gravity.java': `package android.view;
public class Gravity { public static final int CENTER = 17, CENTER_VERTICAL = 16, CENTER_HORIZONTAL = 1, LEFT = 3, TOP = 48, BOTTOM = 80; }`,
'view/HapticFeedbackConstants.java': `package android.view;
public class HapticFeedbackConstants { public static final int VIRTUAL_KEY = 1, LONG_PRESS = 0; }`,
'view/MotionEvent.java': `package android.view;
public class MotionEvent {
    public static final int ACTION_DOWN = 0, ACTION_UP = 1, ACTION_MOVE = 2, ACTION_CANCEL = 3, ACTION_POINTER_UP = 6;
    public static final int AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 11, AXIS_RX = 12, AXIS_RY = 13, AXIS_RZ = 14, AXIS_HAT_X = 15, AXIS_HAT_Y = 16,
        AXIS_LTRIGGER = 17, AXIS_RTRIGGER = 18, AXIS_GAS = 22, AXIS_BRAKE = 23;
    public final float[] axes = new float[32];
    public int getSource() { return InputDevice.SOURCE_JOYSTICK; } public InputDevice getDevice() { return new InputDevice(); }
    public int getHistorySize() { return 0; } public float getAxisValue(int a) { return axes[a]; } public float getHistoricalAxisValue(int a, int pos) { return axes[a]; }
    public int action; public float x, y;
    public MotionEvent(int action, float x, float y) { this.action = action; this.x = x; this.y = y; }
    public int getActionMasked() { return action; } public int getPointerId(int i) { return 0; } public int findPointerIndex(int id) { return 0; }
    public int getActionIndex() { return 0; } public float getX() { return x; } public float getY() { return y; } public float getX(int i) { return x; } public float getY(int i) { return y; }
}`,
'view/InputDevice.java': `package android.view;
public class InputDevice {
    public static final int SOURCE_GAMEPAD = 0x401, SOURCE_JOYSTICK = 0x1000010, SOURCE_DPAD = 0x201, SOURCE_KEYBOARD = 0x101;
    public boolean isVirtual() { return false; } public int getSources() { return SOURCE_GAMEPAD | SOURCE_JOYSTICK; } public int getId() { return 7; }
    public String getName() { return "GameSir-Cyclone 2"; } public String getDescriptor() { return "preview"; }
    public android.os.Vibrator getVibrator() { return null; } public android.os.VibratorManager getVibratorManager() { return null; }
}`,
'view/KeyEvent.java': `package android.view;
public class KeyEvent {
    public static final int ACTION_DOWN = 0, ACTION_UP = 1, ACTION_MULTIPLE = 2;
    public static final int KEYCODE_HOME = 3, KEYCODE_BACK = 4, KEYCODE_DPAD_UP = 19, KEYCODE_DPAD_DOWN = 20, KEYCODE_DPAD_LEFT = 21, KEYCODE_DPAD_RIGHT = 22,
        KEYCODE_VOLUME_UP = 24, KEYCODE_VOLUME_DOWN = 25, KEYCODE_MENU = 82, KEYCODE_BUTTON_A = 96, KEYCODE_BUTTON_B = 97, KEYCODE_BUTTON_X = 99, KEYCODE_BUTTON_Y = 100,
        KEYCODE_BUTTON_L1 = 102, KEYCODE_BUTTON_R1 = 103, KEYCODE_BUTTON_L2 = 104, KEYCODE_BUTTON_R2 = 105, KEYCODE_BUTTON_THUMBL = 106, KEYCODE_BUTTON_THUMBR = 107,
        KEYCODE_BUTTON_START = 108, KEYCODE_BUTTON_SELECT = 109, KEYCODE_BUTTON_MODE = 110;
    public int code, action; public KeyEvent(int action, int code) { this.action = action; this.code = code; }
    public int getKeyCode() { return code; } public int getAction() { return action; } public int getRepeatCount() { return 0; }
    public int getSource() { return InputDevice.SOURCE_GAMEPAD; } public InputDevice getDevice() { return new InputDevice(); }
}`,
'os/Vibrator.java': `package android.os;
public class Vibrator { public boolean hasVibrator() { return false; } public void vibrate(VibrationEffect e) { } public void vibrate(long[] p, int r) { } }`,
'os/VibrationEffect.java': `package android.os;
public class VibrationEffect { public static VibrationEffect createWaveform(long[] p, int r) { return new VibrationEffect(); } }`,
'os/VibratorManager.java': `package android.os;
public class VibratorManager { public int[] getVibratorIds() { return new int[0]; } public void vibrate(CombinedVibration c) { } }`,
'os/CombinedVibration.java': `package android.os;
public class CombinedVibration { public static CombinedVibration createParallel(VibrationEffect e) { return new CombinedVibration(); } }`,
'view/ViewParent.java': `package android.view;
public interface ViewParent { void requestDisallowInterceptTouchEvent(boolean b); }`,
'view/ViewPropertyAnimator.java': `package android.view;
public class ViewPropertyAnimator {
    public ViewPropertyAnimator scaleX(float v) { return this; } public ViewPropertyAnimator scaleY(float v) { return this; }
    public ViewPropertyAnimator alpha(float v) { return this; } public ViewPropertyAnimator translationY(float v) { return this; }
    public ViewPropertyAnimator setDuration(long d) { return this; } public ViewPropertyAnimator setStartDelay(long d) { return this; }
    public ViewPropertyAnimator setInterpolator(Object i) { return this; } public void start() { }
}`,
'view/View.java': `package android.view;
import android.content.Context;
import android.graphics.Canvas;
// a view that can be measured, laid out and drawn - no touch handling by the system, no animations
public class View implements ViewParent {
    public static final int VISIBLE = 0, GONE = 8;
    public static class MeasureSpec {
        public static final int EXACTLY = 1 << 30;
        public static int makeMeasureSpec(int size, int mode) { return size; }
        public static int getSize(int spec) { return spec; }
    }
    public interface OnClickListener { void onClick(View v); }
    public interface OnLongClickListener { boolean onLongClick(View v); }
    private final Context context;
    public int left, top, width, height, measuredW, measuredH, visibility;
    private boolean pressed;
    public View parent;
    public View(Context c) { context = c; }
    public Context getContext() { return context; }
    public android.content.res.Resources getResources() { return context.getResources(); }
    public int getWidth() { return width; } public int getHeight() { return height; }
    public int getMeasuredWidth() { return measuredW; } public int getMeasuredHeight() { return measuredH; }
    public void measure(int ws, int hs) { onMeasure(ws, hs); }
    protected void onMeasure(int ws, int hs) { setMeasuredDimension(ws, hs); }
    protected void setMeasuredDimension(int w, int h) { measuredW = w; measuredH = h; }
    public void layout(int l, int t, int r, int b) {
        boolean changed = l != left || t != top || r - l != width || b - t != height;
        int ow = width, oh = height;
        left = l; top = t; width = r - l; height = b - t;
        if (ow != width || oh != height) onSizeChanged(width, height, ow, oh);
        onLayout(changed, l, t, r, b);
    }
    protected void onLayout(boolean changed, int l, int t, int r, int b) { }
    protected void onSizeChanged(int w, int h, int ow, int oh) { }
    protected void onAttachedToWindow() { }
    protected void onDraw(Canvas c) { }
    public void draw(Canvas c) { if (visibility == VISIBLE) onDraw(c); }
    public void invalidate() { } public void requestLayout() { } public void postInvalidateOnAnimation() { } public void postInvalidateDelayed(long ms) { }
    public boolean postDelayed(Runnable r, long ms) { return true; }
    public void setVisibility(int v) { visibility = v; } public int getVisibility() { return visibility; }
    public void setClickable(boolean b) { } public void setHapticFeedbackEnabled(boolean b) { } public boolean performHapticFeedback(int k) { return true; }
    public void setBackground(android.graphics.drawable.Drawable d) { } public void setWillNotDraw(boolean b) { }
    public void setOnClickListener(OnClickListener l) { } public void setOnLongClickListener(OnLongClickListener l) { }
    public void setSystemGestureExclusionRects(java.util.List<android.graphics.Rect> r) { }
    public boolean isPressed() { return pressed; } public void setPressed(boolean p) { pressed = p; }
    public void setEnabled(boolean e) { } public void setAlpha(float a) { } public void setScaleX(float v) { } public void setScaleY(float v) { } public void setTranslationY(float v) { }
    public ViewPropertyAnimator animate() { return new ViewPropertyAnimator(); }
    public boolean onTouchEvent(MotionEvent e) { return true; }
    public ViewParent getParent() { return this; }
    public void requestDisallowInterceptTouchEvent(boolean b) { }
    public boolean isShown() { return true; }
    public void setContentDescription(CharSequence c) { }
}`,
'view/ViewGroup.java': `package android.view;
import android.content.Context;
import android.graphics.Canvas;
import java.util.*;
public abstract class ViewGroup extends View {
    public final List<View> children = new ArrayList<>();
    public ViewGroup(Context c) { super(c); }
    public void addView(View v) { children.add(v); v.parent = this; }
    public void addView(View v, Object params) { addView(v); }
    public int getChildCount() { return children.size(); } public View getChildAt(int i) { return children.get(i); }
    public void setClipChildren(boolean b) { }
    @Override public void draw(Canvas c) {
        if (visibility != VISIBLE) return;
        onDraw(c);
        for (View v : children) { c.save(); c.translate(v.left, v.top); v.draw(c); c.restore(); }
    }
}`,
'widget/FrameLayout.java': `package android.widget;
import android.content.Context;
import android.view.*;
public class FrameLayout extends ViewGroup {
    public static class LayoutParams { public LayoutParams(int w, int h) { } }
    public FrameLayout(Context c) { super(c); }
    @Override protected void onMeasure(int ws, int hs) { for (View v : children) v.measure(ws, hs); setMeasuredDimension(ws, hs); }
    @Override protected void onLayout(boolean changed, int l, int t, int r, int b) { for (View v : children) v.layout(0, 0, r - l, b - t); }
}`,
'widget/TextView.java': `package android.widget;
import android.content.Context;
import android.graphics.*;
import android.text.SpannableString;
import android.text.style.*;
import android.view.*;
// just enough of a TextView: one or two lines of text, placed by gravity and padding; the first
// line big, a second line with its own size and colour (the spans of Ui.twoLines)
public class TextView extends View {
    public enum BufferType { NORMAL }
    private CharSequence text = "";
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private float sizePx; private int color = 0xFFFFFFFF, gravity = Gravity.CENTER, pl, pt, pr, pb;
    public TextView(Context c) { super(c); setTextSize(14); }
    public final void setText(CharSequence t) { setText(t, BufferType.NORMAL); }
    public void setText(CharSequence t, BufferType type) { text = t == null ? "" : t; }
    public CharSequence getText() { return text; }
    public void setTextSize(float sp) { sizePx = sp * getResources().getDisplayMetrics().density; paint.setTextSize(sizePx); }
    public float getTextSize() { return sizePx; }
    public void setTextSize(int unit, float px) { sizePx = px; paint.setTextSize(px); }
    public Paint getPaint() { return paint; }
    public void setTextColor(int c) { color = c; } public void setTypeface(Typeface t) { paint.setTypeface(t); }
    public void setGravity(int g) { gravity = g; } public void setIncludeFontPadding(boolean b) { } public void setMaxLines(int n) { }
    public void setEllipsize(android.text.TextUtils.TruncateAt t) { } public void setSingleLine(boolean b) { }
    public void setLetterSpacing(float f) { paint.setLetterSpacing(f); } public void setAllCaps(boolean b) { }
    public void setPadding(int l, int t, int r, int b) { pl = l; pt = t; pr = r; pb = b; }
    @Override protected void onDraw(Canvas c) {
        String[] lines = text.toString().split("\\n");
        float[] sizes = new float[lines.length]; int[] colors = new int[lines.length]; boolean[] mono = new boolean[lines.length];
        float total = 0;
        for (int i = 0; i < lines.length; i++) {
            sizes[i] = sizePx; colors[i] = color;
            if (i > 0 && text instanceof SpannableString)
                for (Object[] s : ((SpannableString) text).spans) {
                    if (s[0] instanceof RelativeSizeSpan) sizes[i] = sizePx * ((RelativeSizeSpan) s[0]).f;
                    if (s[0] instanceof ForegroundColorSpan) colors[i] = ((ForegroundColorSpan) s[0]).c;
                    if (s[0] instanceof TypefaceSpan) mono[i] = true;
                }
            total += sizes[i] * 1.18f;
        }
        float areaH = getHeight() - pt - pb;
        float y = (gravity & Gravity.TOP) == Gravity.TOP ? pt : pt + (areaH - total) / 2;
        Typeface own = paint.getTypeface();
        for (int i = 0; i < lines.length; i++) {
            paint.setTextSize(sizes[i]); paint.setColor(colors[i]);
            paint.setTypeface(mono[i] ? Typeface.MONOSPACE : own);
            float w = paint.measureText(lines[i]);
            float x = (gravity & 7) == Gravity.LEFT ? pl : pl + (getWidth() - pl - pr - w) / 2;
            paint.setTextAlign(Paint.Align.LEFT);
            c.drawText(lines[i], x, y + sizes[i] * 0.92f, paint);
            y += sizes[i] * 1.18f;
        }
        paint.setTextSize(sizePx); paint.setTypeface(own);
    }
}`,
'graphics/Color.java': `package android.graphics;
public class Color {
    public static int alpha(int c) { return c >>> 24; } public static int red(int c) { return (c >> 16) & 255; } public static int green(int c) { return (c >> 8) & 255; } public static int blue(int c) { return c & 255; }
    public static int rgb(int r, int g, int b) { return 0xFF000000 | (r << 16) | (g << 8) | b; }
    public static int argb(int a, int r, int g, int b) { return (a << 24) | (r << 16) | (g << 8) | b; }
    public static int HSVToColor(float[] h) { return 0xFF000000 | (java.awt.Color.HSBtoRGB(h[0] / 360f, h[1], h[2]) & 0xFFFFFF); }
}`,
'graphics/RectF.java': `package android.graphics;
public class RectF {
    public float left, top, right, bottom;
    public RectF() { } public RectF(float l, float t, float r, float b) { set(l, t, r, b); }
    public void set(float l, float t, float r, float b) { left = l; top = t; right = r; bottom = b; }
    public float width() { return right - left; } public float height() { return bottom - top; }
    public float centerX() { return (left + right) / 2; } public float centerY() { return (top + bottom) / 2; }
    public void inset(float dx, float dy) { left += dx; right -= dx; top += dy; bottom -= dy; }
    public boolean contains(float x, float y) { return x >= left && x < right && y >= top && y < bottom; }
}`,
'graphics/Rect.java': `package android.graphics;
public class Rect { public Rect(int l, int t, int r, int b) { } }`,
'graphics/Typeface.java': `package android.graphics;
public class Typeface {
    public static final int NORMAL = 0, BOLD = 1;
    public static final Typeface MONOSPACE = new Typeface("Consolas", 0), DEFAULT_BOLD = new Typeface("Segoe UI", 1);
    public final String family; public final int style;
    Typeface(String f, int s) { family = f; style = s; }
    public static Typeface create(String name, int style) {
        return new Typeface(name.contains("condensed") ? "Bahnschrift Condensed" : name.contains("mono") ? "Consolas" : "Segoe UI", style);
    }
    public static Typeface create(Typeface t, int style) { return new Typeface(t.family, style); }
}`,
'graphics/Shader.java': `package android.graphics;
public class Shader { public enum TileMode { CLAMP } public java.awt.Paint paint; }`,
'graphics/LinearGradient.java': `package android.graphics;
public class LinearGradient extends Shader {
    public LinearGradient(float x0, float y0, float x1, float y1, int c0, int c1, TileMode m) { this(x0, y0, x1, y1, new int[]{c0, c1}, null, m); }
    public LinearGradient(float x0, float y0, float x1, float y1, int[] colors, float[] pos, TileMode m) {
        if (x0 == x1 && y0 == y1) { paint = new java.awt.Color(colors[0], true); return; }
        paint = new java.awt.LinearGradientPaint(x0, y0, x1, y1, RadialGradient.stops(colors.length, pos), RadialGradient.colors(colors));
    }
}`,
'graphics/RadialGradient.java': `package android.graphics;
public class RadialGradient extends Shader {
    public RadialGradient(float cx, float cy, float r, int c0, int c1, TileMode m) { this(cx, cy, r, new int[]{c0, c1}, null, m); }
    public RadialGradient(float cx, float cy, float r, int[] colors, float[] pos, TileMode m) {
        paint = new java.awt.RadialGradientPaint(cx, cy, Math.max(r, 0.001f), stops(colors.length, pos), colors(colors));
    }
    static float[] stops(int n, float[] pos) {
        float[] f = new float[n];
        for (int i = 0; i < n; i++) { f[i] = pos == null ? i / (float) (n - 1) : pos[i]; if (i > 0 && f[i] <= f[i - 1]) f[i] = Math.min(1f, f[i - 1] + 0.0001f); }
        return f;
    }
    static java.awt.Color[] colors(int[] c) { java.awt.Color[] o = new java.awt.Color[c.length]; for (int i = 0; i < c.length; i++) o[i] = new java.awt.Color(c[i], true); return o; }
}`,
'graphics/drawable/Drawable.java': `package android.graphics.drawable;
public class Drawable { }`,
'graphics/drawable/GradientDrawable.java': `package android.graphics.drawable;
public class GradientDrawable extends Drawable {
    public enum Orientation { TOP_BOTTOM }
    public GradientDrawable() { } public GradientDrawable(Orientation o, int[] c) { }
    public void setColor(int c) { } public void setCornerRadius(float r) { } public void setStroke(int w, int c) { }
}`,
'graphics/Path.java': `package android.graphics;
import java.awt.geom.*;
public class Path {
    public enum Direction { CW }
    public Path2D.Float p = new Path2D.Float();
    public void reset() { p.reset(); }
    public void moveTo(float x, float y) { p.moveTo(x, y); } public void lineTo(float x, float y) { p.lineTo(x, y); }
    public void cubicTo(float a, float b, float c, float d, float e, float f) { p.curveTo(a, b, c, d, e, f); }
    public void close() { p.closePath(); }
    public void addRoundRect(RectF r, float rx, float ry, Direction d) { addRoundRect(r, new float[]{rx, ry, rx, ry, rx, ry, rx, ry}, d); }
    // radii: top-left, top-right, bottom-right, bottom-left (x and y each)
    public void addRoundRect(RectF r, float[] q, Direction d) {
        float k = 0.5523f;
        p.moveTo(r.left + q[0], r.top);
        p.lineTo(r.right - q[2], r.top);
        p.curveTo(r.right - q[2] * (1 - k), r.top, r.right, r.top + q[3] * (1 - k), r.right, r.top + q[3]);
        p.lineTo(r.right, r.bottom - q[5]);
        p.curveTo(r.right, r.bottom - q[5] * (1 - k), r.right - q[4] * (1 - k), r.bottom, r.right - q[4], r.bottom);
        p.lineTo(r.left + q[6], r.bottom);
        p.curveTo(r.left + q[6] * (1 - k), r.bottom, r.left, r.bottom - q[7] * (1 - k), r.left, r.bottom - q[7]);
        p.lineTo(r.left, r.top + q[1]);
        p.curveTo(r.left, r.top + q[1] * (1 - k), r.left + q[0] * (1 - k), r.top, r.left + q[0], r.top);
        p.closePath();
    }
}`,
'graphics/Paint.java': `package android.graphics;
public class Paint {
    public static final int ANTI_ALIAS_FLAG = 1;
    public enum Style { FILL, STROKE } public enum Cap { BUTT, ROUND } public enum Join { MITER, ROUND } public enum Align { LEFT, CENTER, RIGHT }
    public int color = 0xFF000000; public Style style = Style.FILL; public float strokeWidth = 1, textSize = 12, letterSpacing;
    public Cap cap = Cap.BUTT; public Join join = Join.MITER; public Align align = Align.LEFT; public Shader shader; public Typeface typeface; public boolean fakeBold;
    public float alphaMul = 1;
    public Paint() { } public Paint(int flags) { }
    public void setColor(int c) { color = c; alphaMul = 1; } public int getColor() { return color; }
    public void setAlpha(int a) { if (shader != null) alphaMul = a / 255f; else color = (color & 0xFFFFFF) | (a << 24); }
    public void setStyle(Style s) { style = s; } public void setStrokeWidth(float w) { strokeWidth = w; } public float getStrokeWidth() { return strokeWidth; }
    public void setStrokeCap(Cap c) { cap = c; } public void setStrokeJoin(Join j) { join = j; }
    public void setShader(Shader s) { shader = s; if (s == null) alphaMul = 1; }
    public void setTextSize(float s) { textSize = s; } public float getTextSize() { return textSize; }
    public void setTextAlign(Align a) { align = a; } public void setTypeface(Typeface t) { typeface = t; } public Typeface getTypeface() { return typeface; }
    public void setFakeBoldText(boolean b) { fakeBold = b; } public void setLetterSpacing(float f) { letterSpacing = f; } public float getLetterSpacing() { return letterSpacing; }
    public java.awt.Font font() {
        Typeface t = typeface == null ? new Typeface("Segoe UI", 0) : typeface;
        java.util.Map<java.awt.font.TextAttribute, Object> a = new java.util.HashMap<>();
        a.put(java.awt.font.TextAttribute.FAMILY, t.family);
        a.put(java.awt.font.TextAttribute.SIZE, textSize);
        if (t.style == Typeface.BOLD || fakeBold) a.put(java.awt.font.TextAttribute.WEIGHT, java.awt.font.TextAttribute.WEIGHT_BOLD);
        if (letterSpacing != 0) a.put(java.awt.font.TextAttribute.TRACKING, letterSpacing);
        return new java.awt.Font(a);
    }
    private static final java.awt.font.FontRenderContext FRC = new java.awt.font.FontRenderContext(null, true, true);
    public float measureText(String s) { return (float) font().getStringBounds(s, FRC).getWidth(); }
    public float measureText(String s, int a, int b) { return measureText(s.substring(a, b)); }
}`,
'graphics/Canvas.java': `package android.graphics;
import java.awt.*;
import java.awt.geom.*;
import java.util.ArrayDeque;
// android.graphics.Canvas drawn with Java 2D
public class Canvas {
    private final Graphics2D g;
    private final ArrayDeque<Object[]> stack = new ArrayDeque<>();
    public Canvas(Graphics2D g) {
        this.g = g;
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
        g.setRenderingHint(RenderingHints.KEY_STROKE_CONTROL, RenderingHints.VALUE_STROKE_PURE);
        g.setRenderingHint(RenderingHints.KEY_FRACTIONALMETRICS, RenderingHints.VALUE_FRACTIONALMETRICS_ON);
    }
    public int save() { stack.push(new Object[]{g.getTransform(), g.getClip()}); return stack.size(); }
    public void restore() { Object[] s = stack.pop(); g.setTransform((AffineTransform) s[0]); g.setClip((Shape) s[1]); }
    public void translate(float x, float y) { g.translate(x, y); } public void scale(float x, float y) { g.scale(x, y); }
    public void rotate(float deg) { g.rotate(Math.toRadians(deg)); }
    public boolean clipPath(android.graphics.Path p) { g.clip(p.p); return true; }
    private void paint(android.graphics.Paint p, Shape s) {
        if (p.shader != null) g.setPaint(p.shader.paint); else g.setColor(new java.awt.Color(p.color, true));
        Composite old = g.getComposite();
        if (p.alphaMul < 1) g.setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, Math.max(0, p.alphaMul)));
        if (p.style == android.graphics.Paint.Style.FILL) g.fill(s);
        else {
            g.setStroke(new BasicStroke(Math.max(0.01f, p.strokeWidth), p.cap == android.graphics.Paint.Cap.ROUND ? BasicStroke.CAP_ROUND : BasicStroke.CAP_BUTT,
                p.join == android.graphics.Paint.Join.ROUND ? BasicStroke.JOIN_ROUND : BasicStroke.JOIN_MITER));
            g.draw(s);
        }
        g.setComposite(old);
    }
    public void drawRect(float l, float t, float r, float b, android.graphics.Paint p) { paint(p, new Rectangle2D.Float(l, t, r - l, b - t)); }
    public void drawRoundRect(RectF r, float rx, float ry, android.graphics.Paint p) {
        rx = Math.max(0, Math.min(rx, r.width() / 2)); ry = Math.max(0, Math.min(ry, r.height() / 2));
        paint(p, new RoundRectangle2D.Float(r.left, r.top, r.width(), r.height(), rx * 2, ry * 2));
    }
    public void drawCircle(float cx, float cy, float rad, android.graphics.Paint p) { paint(p, new Ellipse2D.Float(cx - rad, cy - rad, rad * 2, rad * 2)); }
    public void drawOval(RectF r, android.graphics.Paint p) { paint(p, new Ellipse2D.Float(r.left, r.top, r.width(), r.height())); }
    public void drawLine(float x0, float y0, float x1, float y1, android.graphics.Paint p) {
        android.graphics.Paint.Style s = p.style; p.style = android.graphics.Paint.Style.STROKE;
        paint(p, new Line2D.Float(x0, y0, x1, y1)); p.style = s;
    }
    public void drawPath(android.graphics.Path path, android.graphics.Paint p) { paint(p, path.p); }
    // Android: angles in degrees, clockwise, 0 = 3 o'clock. Java 2D: counter-clockwise.
    public void drawArc(RectF r, float start, float sweep, boolean useCenter, android.graphics.Paint p) {
        paint(p, new Arc2D.Float(r.left, r.top, r.width(), r.height(), -start, -sweep, useCenter ? Arc2D.PIE : Arc2D.OPEN));
    }
    public void drawText(String s, float x, float y, android.graphics.Paint p) {
        Font f = p.font();
        g.setFont(f);
        float w = p.measureText(s);
        if (p.align == android.graphics.Paint.Align.CENTER) x -= w / 2; else if (p.align == android.graphics.Paint.Align.RIGHT) x -= w;
        g.setColor(new java.awt.Color(p.color, true));
        if (f.canDisplayUpTo(s) >= 0) g.setFont(new Font("Segoe UI Emoji", Font.PLAIN, 1).deriveFont(p.textSize));   // (emoji: the PC's own, without colour)
        g.drawString(s, x, y);
    }
}`,
};
for (const [name, body] of Object.entries(files)) {
  const f = path.join(out, 'android', name);
  fs.mkdirSync(path.dirname(f), { recursive: true });
  fs.writeFileSync(f, body + '\n');
}
console.log('wrote ' + Object.keys(files).length + ' stand-in classes to ' + out);
