#include "filter.h"

fm_biquad fm_new_biquad() {
    fm_biquad bq;

    for (int i = 0; i < 3; i++) {
        bq.x[i] = 0;
        bq.y[i] = 0;
    }

    return bq;
}

void fm_biquad_gain(fm_biquad *bq, double gain) {
    bq->b[0] = gain;
    bq->b[1] = 0;
    bq->b[2] = 0;
    
    bq->a[1] = 0;
    bq->a[2] = 0;
}

// the code of my lowpass, highpass, and peak filters are based heavily
// on beepbox's source code, found at:
//
// https://github.com/johnnesky/beepbox/blob/afc81d1c641dd0bb4409007a...
// ...34be2f85b28bf401/synth/filtering.ts
//
// also,
//
// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html

void fm_biquad_lowpass(fm_biquad *bq, double hz, double Q) {
    double w = (2 * PI * (double) hz) * fm_config.dt;
    double alpha = sin(w) / (2.0 * Q);
    double cos_w = cos(w);
    
    bq->b[0] = (1 - cos_w) / 2.0;
    bq->b[1] = 1 - cos_w;
    bq->b[2] = (1 - cos_w) / 2.0;

    bq->a[0] = 1 + alpha; // ignored
    bq->a[1] = (-2.0 * cos_w) / bq->a[0];
    bq->a[2] = (1 - alpha) / bq->a[0];
}

void fm_biquad_highpass(fm_biquad *bq, double hz, double Q) {
    double w = (2 * PI * (double) hz) * fm_config.dt;
    double alpha = sin(w) / (2.0 * Q);
    double cos_w = cos(w);
    
    bq->b[0] = (1 + cos_w) / 2.0;
    bq->b[1] = -(1 - cos_w);
    bq->b[2] = (1 + cos_w) / 2.0;

    bq->a[0] = 1 + alpha; // ignored
    bq->a[1] = (-2.0 * cos_w) / bq->a[0];
    bq->a[2] = (1 - alpha) / bq->a[0];
}

void fm_biquad_peak(fm_biquad *bq, double hz, double Q, double A) {
    double w = (2 * PI * (double) hz) * fm_config.dt;
    double alpha = sin(w) / (2.0 * Q);
    double cos_w = cos(w);

    bq->b[0] = 1 + alpha * A;
    bq->b[1] = -2 * cos_w;
    bq->b[2] = 1 - alpha * A;

    bq->a[0] = 1 + alpha / A;
    bq->a[1] = (-2 * cos_w) / bq->a[0];
    bq->a[2] = (1 - alpha / A) / bq->a[0];
}

float fm_biquad_run(fm_biquad *bq, float x0) {
    bq->x[2] = bq->x[1];
    bq->x[1] = bq->x[0];
    bq->x[0] = x0;

    float y0 = bq->b[0] * bq->x[0]
            + bq->b[1] * bq->x[1]
            + bq->b[2] * bq->x[2]
            - bq->a[1] * bq->y[0]
            - bq->a[2] * bq->y[1];

    bq->y[2] = bq->y[1];
    bq->y[1] = bq->y[0];
    bq->y[0] = y0;

    return y0;
}
