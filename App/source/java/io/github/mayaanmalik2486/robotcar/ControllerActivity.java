package io.github.mayaanmalik2486.robotcar;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.drawable.ColorDrawable;
import android.hardware.input.InputManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * The controller screen: a live drawing of the GameSir Cyclone 2 with every button's job, and
 * a way to change them ("remap"). On this screen the buttons don't drive the car - try them
 * all safely.
 */
public class ControllerActivity extends Activity implements ControllerInput.Listener, ControllerView.Listener,
        InputManager.InputDeviceListener {

    private Settings settings;
    private ControllerInput pad;
    private ControllerView drawing;
    private TextView title, hint;
    private PadButton shiftB;
    private InputManager inputs;
    private final Handler ui = new Handler(Looper.getMainLooper());

    private char waitingJob;                         // remap: the job waiting for a button (0 = none)
    private int heldShift = -1;                      // remap: a SHIFT button went down while waiting
    private boolean otherPressed;

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().setBackgroundDrawable(new ColorDrawable(Ui.BG));
        settings = new Settings(this);
        pad = new ControllerInput(this);
        pad.enabled = false;                         // buttons are shown, not used
        inputs = (InputManager) getSystemService(INPUT_SERVICE);

        int m = Ui.dp(this, 16), g = Ui.dp(this, 8);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(m, Ui.dp(this, 8), m, Ui.dp(this, 8));
        root.setBackground(Ui.sky());

        LinearLayout bar = new LinearLayout(this);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        PadButton back = button("‹  Back");
        back.setOnClickListener(v -> finish());
        bar.addView(back, new LinearLayout.LayoutParams(Ui.dp(this, 96), Ui.dp(this, 44)));
        title = new TextView(this);
        title.setTextColor(Ui.TEXT);
        title.setTextSize(20);
        title.setTypeface(Ui.BOLD);
        title.setSingleLine(true);
        title.setEllipsize(TextUtils.TruncateAt.END);
        LinearLayout.LayoutParams tp = new LinearLayout.LayoutParams(0, -2, 1f);
        tp.leftMargin = Ui.dp(this, 16);
        bar.addView(title, tp);
        shiftB = button("LB layer");
        shiftB.setOnClickListener(v -> {
            drawing.showShift = !drawing.showShift;
            shiftB.setOn(drawing.showShift);
            drawing.invalidate();
        });
        PadButton remap = button("Remap");
        remap.setOnClickListener(v -> chooseJob(-1));
        PadButton reset = button("Reset");
        reset.setOnClickListener(v -> new AlertDialog.Builder(this)
            .setMessage("Put every button back to its first job?")
            .setPositiveButton("Reset", (d, w) -> {
                pad.map.reset();
                save();
            })
            .setNegativeButton("Cancel", null)
            .show());
        for (View v : new View[]{shiftB, remap, reset}) {
            LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(Ui.dp(this, 104), Ui.dp(this, 44));
            p.leftMargin = g;
            bar.addView(v, p);
        }
        root.addView(bar);

        drawing = new ControllerView(this, pad, this);
        LinearLayout.LayoutParams dp = new LinearLayout.LayoutParams(-1, 0, 1f);
        dp.topMargin = g;
        root.addView(drawing, dp);

        hint = new TextView(this);
        hint.setTextColor(Ui.LABEL);
        hint.setTextSize(13);
        hint.setGravity(Gravity.CENTER);
        hint.setMaxLines(2);
        root.addView(hint, new LinearLayout.LayoutParams(-1, -2));
        setContentView(root);
        find();
    }

    private PadButton button(String s) {
        PadButton b = new PadButton(this, s, PadButton.GLASS, false);
        b.setTextSize(14);
        return b;
    }

    @Override
    protected void onResume() {
        super.onResume();
        inputs.registerInputDeviceListener(this, ui);
        find();
    }

    @Override
    protected void onPause() {
        super.onPause();
        inputs.unregisterInputDeviceListener(this);
    }

    @Override
    public void finish() {
        super.finish();
        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
    }

    // which controller is there, and its own map
    private void find() {
        InputDevice found = null;
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice d = InputDevice.getDevice(id);
            if (ControllerInput.isGamepad(d)) {
                found = d;
                break;
            }
        }
        if (found != null && pad.deviceId != found.getId()) {
            pad.deviceId = found.getId();
            pad.name = found.getName();
            pad.descriptor = found.getDescriptor();
        }
        if (found == null)
            pad.deviceId = -1;
        settings.loadMap(pad.map, pad.descriptor);
        pad.deadZone = settings.padDeadZone(pad.descriptor);
        title.setText(found == null ? "No controller connected" : "🎮 " + found.getName() + "   ·   connected");
        title.setTextColor(found == null ? Ui.WARN : Ui.TEXT);
        showHint();
        drawing.invalidate();
    }

    private void showHint() {
        if (waitingJob != 0)
            hint.setText("Press the button for \"" + ControllerMap.jobName(waitingJob) + "\" now (hold LB first to put it on the LB layer).   Back = cancel");
        else if (pad.deviceId < 0)
            hint.setText("Cyclone 2: hold HOME + A for 2 s until the light flashes YELLOW, then pick \"GameSir-Cyclone 2\" in Bluetooth.   "
                + "Or plug in its 2.4 GHz dongle with a USB-C adapter (light GREEN).");
        else
            hint.setText("Press anything: it lights up. Tap a button to give it another job. (These buttons don't drive the car here.)");
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent e) {
        if (pad.onKey(e))
            return true;
        if (e.getKeyCode() == KeyEvent.KEYCODE_BACK && waitingJob != 0 && e.getAction() == KeyEvent.ACTION_UP) {
            waitingJob = 0;
            drawing.highlight = -1;
            showHint();
            return true;
        }
        return super.dispatchKeyEvent(e);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent e) {
        if (pad.onMotion(e))
            return true;
        return super.dispatchGenericMotionEvent(e);
    }

    // ------------------------------------------------------------------ remapping

    /** Choose a job: for one button (tapped on the drawing), or (button -1) then press the button. */
    private void chooseJob(int button) {
        String[] names = ControllerMap.JOB_NAMES.clone();
        boolean layer = drawing.showShift;
        new AlertDialog.Builder(this)
            .setTitle(button < 0 ? "Remap: choose a job" : ControllerMap.BUTTON_NAMES[button] + (layer ? " (with LB held)" : "") + " does ...")
            .setItems(names, (d, which) -> {
                char job = ControllerMap.JOBS[which];
                if (button < 0) {
                    waitingJob = job;
                    heldShift = -1;
                    showHint();
                } else {
                    pad.map.set(button, layer, job);
                    save();
                }
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void assign(int button, boolean layer) {
        pad.map.set(button, layer, waitingJob);
        waitingJob = 0;
        heldShift = -1;
        drawing.highlight = button;
        ui.postDelayed(() -> {
            drawing.highlight = -1;
            drawing.invalidate();
        }, 700);
        save();
    }

    private void save() {
        settings.saveMap(pad.map, pad.descriptor);
        showHint();
        drawing.invalidate();
    }

    @Override
    public void onPadButton(int button, boolean down) {
        if (waitingJob == 0)
            return;
        boolean isShift = pad.map.job(button, false) == ControllerMap.SHIFT;
        if (down) {
            if (isShift && waitingJob != ControllerMap.SHIFT && heldShift < 0) {
                heldShift = button;                  // maybe holding LB for the second layer - wait and see
                otherPressed = false;
                return;
            }
            otherPressed = true;
            assign(button, heldShift >= 0 && button != heldShift);
        } else if (button == heldShift) {
            if (!otherPressed)
                assign(button, false);               // LB alone: LB itself gets the job
            heldShift = -1;
        }
    }

    @Override
    public void onTapButton(int button) {
        chooseJob(button);
    }

    @Override
    public void onPadJob(char job, int button) {
    }

    @Override
    public void onPadDrive(float throttle, float steer) {
    }

    @Override
    public void onPadHead(float turn) {
    }

    @Override
    public void onPadChanged() {
        drawing.invalidate();
    }

    @Override
    public void onPadDevice(InputDevice d) {
        find();
    }

    @Override
    public void onInputDeviceAdded(int id) {
        find();
    }

    @Override
    public void onInputDeviceRemoved(int id) {
        if (id == pad.deviceId)
            pad.lost();
        find();
    }

    @Override
    public void onInputDeviceChanged(int id) {
        find();
    }
}
