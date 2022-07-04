#include "filter.h"

fm_eq fm_new_eq() {
    fm_eq eq;

    eq.lowpass_hz = -1;
    eq.highpass_hz = -1;

    eq.gain = 1.0;

    eq.num_peaks = 0;

    eq.biquads_cap = 0;
    eq.num_biquads = 0;
    eq.biquads = NULL;

    return eq;
}

void fm_eq_bake(fm_eq *eq) {
    int has_lp = eq->lowpass_hz >= 0;
    int has_hp = eq->highpass_hz >= 0;
    
    eq->num_biquads = eq->num_peaks + has_lp + has_hp;

    if (eq->num_biquads > eq->biquads_cap) {
        eq->biquads_cap = eq->num_biquads;
        eq->biquads = realloc(eq->biquads, sizeof(fm_biquad) * eq->biquads_cap);
    }

    int i = 0;

    if (has_lp) {
        fm_biquad_lowpass(&eq->biquads[i++], eq->lowpass_hz, eq->lowpass_Q);
    }

    if (has_hp) {
        fm_biquad_highpass(&eq->biquads[i++], eq->highpass_hz, eq->highpass_Q);
    }

    for (int j = 0; j < eq->num_peaks; j++) {
        fm_biquad_peak(&eq->biquads[i++],
                       eq->peaks_hz[j], eq->peaks_Q[j], eq->peaks_A[j]);
    }
}

float fm_eq_run(fm_eq *eq, float sample) {
    float out = sample;

    for (int i = 0; i < eq->num_biquads; i++) {
        out = fm_biquad_run(&eq->biquads[i], out);
    }

    return out * eq->gain;
}

void fm_eq_lowpass(fm_eq *eq, double hz, double Q) {
    eq->lowpass_hz = hz;
    eq->lowpass_Q = Q;
}

void fm_eq_highpass(fm_eq *eq, double hz, double Q) {
    eq->highpass_hz = hz;
    eq->highpass_Q = Q;
}

void fm_eq_add_peak(fm_eq *eq, double hz, double Q, double A) {
    if (eq->num_peaks >= EQ_MAX_PEAKS) {
        printf("could not add more peaks\n");
        return;
    }

    eq->peaks_hz[eq->num_peaks] = hz;
    eq->peaks_Q[eq->num_peaks] = Q;
    eq->peaks_A[eq->num_peaks] = A;

    eq->num_peaks++;
}

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
    bq->b[1] = -(1 + cos_w);
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
