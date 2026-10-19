package io.github.mayaanmalik2486.robotcar;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.os.Handler;
import android.os.Looper;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * The Bluetooth line to the car's HC-05: connect, send letters, and hand every line the car
 * sends back to the listener. All listener calls happen on the main (screen) thread.
 */
final class CarLink {

    interface Listener {
        /** Connecting, connected or disconnected (see isConnected / isConnecting). */
        void onLinkChanged();

        /** The connection ended without us asking: lost = it was connected, else it never answered. */
        void onLinkBroken(boolean lost);

        void onLine(String line);
    }

    private static final UUID SERIAL_PORT = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    private final BluetoothAdapter adapter;
    private final Listener listener;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private final ExecutorService sender = Executors.newSingleThreadExecutor();

    private volatile BluetoothSocket socket;
    private volatile OutputStream out;
    private volatile boolean connected;
    private volatile boolean connecting;
    private volatile boolean closingOnPurpose;
    private volatile String address;

    CarLink(BluetoothAdapter adapter, Listener listener) {
        this.adapter = adapter;
        this.listener = listener;
    }

    boolean isConnected() {
        return connected;
    }

    boolean isConnecting() {
        return connecting;
    }

    String address() {
        return address;
    }

    void connect(final String address) {
        if (connected || connecting)
            return;
        this.address = address;
        connecting = true;
        closingOnPurpose = false;
        listener.onLinkChanged();

        new Thread(() -> {
            BluetoothSocket s = null;
            boolean wasConnected = false;
            try {
                BluetoothDevice car = adapter.getRemoteDevice(address);
                try {
                    s = car.createRfcommSocketToServiceRecord(SERIAL_PORT);
                    socket = s;                             // (so disconnect() can stop it)
                    s.connect();
                } catch (IOException first) {
                    close(s);
                    if (closingOnPurpose)
                        throw first;
                    Method m = car.getClass().getMethod("createRfcommSocket", int.class);   // some HC-05s only answer on channel 1
                    s = (BluetoothSocket) m.invoke(car, 1);
                    socket = s;
                    s.connect();
                }
                out = s.getOutputStream();
                connected = true;
                connecting = false;
                wasConnected = true;
                ui.post(listener::onLinkChanged);
                send(Car.ASK);                              // "what are you doing, which song?"
                readLines(s.getInputStream());
            } catch (Exception e) {
                close(s);
            }

            connected = false;
            connecting = false;
            out = null;
            final boolean lost = wasConnected;
            final boolean onPurpose = closingOnPurpose;
            ui.post(() -> {
                listener.onLinkChanged();
                if (!onPurpose)
                    listener.onLinkBroken(lost);
            });
        }).start();
    }

    void disconnect() {
        closingOnPurpose = true;
        close(socket);
    }

    /** Sends one letter (does nothing when not connected). */
    void send(final char c) {
        if (!connected)
            return;
        try {
            sender.execute(() -> {
                try {
                    OutputStream o = out;
                    if (o != null) {
                        o.write(c);
                        o.flush();
                    }
                } catch (IOException e) {
                    // the reading thread notices the lost connection
                }
            });
        } catch (Exception e) {
            // the app is closing
        }
    }

    /** For when the app closes for good. */
    void shutdown() {
        disconnect();
        sender.shutdownNow();
    }

    private void readLines(InputStream in) throws IOException {
        StringBuilder line = new StringBuilder();
        byte[] buf = new byte[64];
        while (true) {
            int n = in.read(buf);
            if (n < 0)
                throw new IOException("closed");
            for (int i = 0; i < n; i++) {
                char c = (char) (buf[i] & 0xFF);
                if (c == '\n') {
                    final String l = line.toString().trim();
                    line.setLength(0);
                    ui.post(() -> listener.onLine(l));
                } else if (line.length() < 64) {
                    line.append(c);
                }
            }
        }
    }

    private static void close(BluetoothSocket s) {
        if (s == null)
            return;
        try {
            s.close();
        } catch (IOException e) {
            // already closed
        }
    }
}
