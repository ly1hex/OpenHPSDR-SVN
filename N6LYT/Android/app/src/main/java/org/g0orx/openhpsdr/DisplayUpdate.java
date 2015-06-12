package org.g0orx.openhpsdr;

import org.g0orx.openhpsdr.wdsp.WDSP;

import android.util.Log;

import java.util.Timer;
import java.util.TimerTask;

public class DisplayUpdate extends Thread {

    public DisplayUpdate(VFOView vfoView,PanadapterView panadapterView, WaterfallView waterfallView, MeterView meterView, FrequencyView frequencyView, BandscopeView bandscopeView, Metis metis, int pixels) {
        this.vfoView = vfoView;
        this.panadapterView = panadapterView;
        this.waterfallView = waterfallView;
        this.meterView = meterView;
        this.frequencyView = frequencyView;
        this.bandscopeView = bandscopeView;
        this.metis = metis;
        this.pixels = pixels;
        configuration = Configuration.getInstance();

        wdsp = WDSP.getInstance();

        samples = new float[pixels];
        Log.i("DisplayUpdate", "id:" + this.getId() + " name:" + this.getName() + " fps=" + configuration.fps);
    }

    public void startTimer() {
        running = true;
        final Timer timer = new Timer();
        time = System.currentTimeMillis();
        timer.scheduleAtFixedRate(new TimerTask() {
            public void run() {
                if (running) {
                    update();
                } else {
                    timer.cancel();
                    Log.i("DisplayUpdate", "Cancel timer");
                }
            }
        }, 0, 1000 / configuration.fps);
    }

    private void update() {
        if (firsttime) {
            if(frequencyView.update() && vfoView.update()) {
                firsttime = false;
            }
        }
        if (metis.isTransmitting()) {
            if (metis.Process_Panadapter(Display.TX, samples)) {
                if(panadapterView!=null) {
                    panadapterView.plotSpectrum(samples);
                }
                meterView.setPower(metis.getPenelopeForwardPower(), metis.getAlexForwardPower(), metis.getAlexReversePower());
            }
        } else {
            if (metis.Process_Panadapter(Display.RX, samples)) {
                if(panadapterView!=null) {
                    panadapterView.plotSpectrum(samples);
                }
                if (waterfallView!=null) {
                    waterfallView.update(samples);
                }
                int meter = (int) wdsp.GetRXAMeter(configuration.subrx ? Channel.SUBRX : Channel.RX, WDSP.S_AV);
                meterView.setMeter(meter);
            }

            if(bandscopeView!=null) {
                if(metis.Process_Panadapter(Display.BS, samples)) {
                    bandscopeView.update(samples);
                } else {
                    Log.i("DisplayUpdate", "Process_Panadapter FALSE");
                }
            }
        }
        /*
        frames++;
        if (frames == configuration.fps) {
            long elapsed = System.currentTimeMillis() - time;
            if (elapsed > 1000L) {
                Log.i("DisplayUpdate", "time for " + configuration.fps + " =" + elapsed + "ms");
            }
            frames = 0;
            time = System.currentTimeMillis();
        }
        */
    }

    public boolean isRunning() {
        return running;
    }

    public void terminate() {
        running = false;
    }

    private Configuration configuration;

    private int pixels;

    private boolean running;
    private float[] samples;
    private int meter = 0;

    private VFOView vfoView;
    private PanadapterView panadapterView;
    private WaterfallView waterfallView;
    private MeterView meterView;
    private FrequencyView frequencyView;
    private BandscopeView bandscopeView;
    private Metis metis;
    private WDSP wdsp;

    private int frames = 0;

    private long time;

    private boolean firsttime = true;
}
