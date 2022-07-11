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

fm_biquad fm_new_biquad() {
    fm_biquad bq;

    for (int i = 0; i < 3; i++) {
        bq.x[i] = 0;
        bq.y[i] = 0;
        bq.a[i] = 0;
        bq.b[i] = 0;
    }

    return bq;
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

    bq->a[0] = 1 + alpha; // ignored
    
    bq->b[0] = ((1 - cos_w) / 2.0) / bq->a[0];
    bq->b[1] = (1 - cos_w) / bq->a[0];
    bq->b[2] = ((1 - cos_w) / 2.0) / bq->a[0];

    bq->a[1] = (-2.0 * cos_w) / bq->a[0];
    bq->a[2] = (1 - alpha) / bq->a[0];
}

void fm_biquad_highpass(fm_biquad *bq, double hz, double Q) {
    double w = (2 * PI * (double) hz) * fm_config.dt;
    double alpha = sin(w) / (2.0 * Q);
    double cos_w = cos(w);

    bq->a[0] = 1 + alpha; // ignored 
    
    bq->b[0] = ((1 + cos_w) / 2.0) / bq->a[0];
    bq->b[1] = (-(1 + cos_w)) / bq->a[0];
    bq->b[2] = ((1 + cos_w) / 2.0) / bq->a[0];

    bq->a[1] = (-2.0 * cos_w) / bq->a[0];
    bq->a[2] = (1 - alpha) / bq->a[0];
}

void fm_biquad_peak(fm_biquad *bq, double hz, double Q, double A) {
    double w = (2 * PI * (double) hz) * fm_config.dt;
    double alpha = sin(w) / (2.0 * Q);
    double cos_w = cos(w);

    bq->a[0] = 1 + alpha / A;

    bq->b[0] = (1 + alpha * A) / bq->a[0];
    bq->b[1] = (-2 * cos_w) / bq->a[0];
    bq->b[2] = (1 - alpha * A) / bq->a[0];

    bq->a[1] = (-2 * cos_w) / bq->a[0];
    bq->a[2] = (1 - alpha / A) / bq->a[0];
}

void fm_biquad_highshelf(fm_biquad *bq, double hz, double Q) {
    double w = (2 * PI * (double) hz) * fm_config.dt;
    double t = tan(w / 2.0);
    double sqrt_Q = sqrt(Q);
    double g = (t * sqrt_Q - 1) / (t * sqrt_Q + 1);

    bq->b[0] = (1 + g + Q * (1 - g)) / 2.0;
    bq->b[1] = (1 + g - Q * (1 - g)) / 2.0;
    bq->b[2] = 0;

    bq->a[0] = 1.0;
    bq->a[1] = g / bq->a[0];
    bq->a[2] = 0;
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


#define Q(i, n) (fdn->feedback_matrix[i][n])
#define Y(i) (fdn->output[i])

fm_fdn fm_new_fdn(int l0, int l1, int l2, int l3) {
    fm_fdn fdn;

    fdn.delay_length[0] = l0;
    fdn.delay_length[1] = l1;
    fdn.delay_length[2] = l2;
    fdn.delay_length[3] = l3;

    for (int i = 0; i < 4; i++) {
        fdn.delays[i] = malloc(fdn.delay_length[i] * sizeof(float));
        for (int j = 0; j < fdn.delay_length[i]; j++) {
            fdn.delays[i][j] = 0;
        }

        fdn.output[i] = 0;
        fdn.delay_heads[i] = 0;

        fdn.feedback_filter[i] = fm_new_biquad();
    }

    return fdn;
}

void fm_fdn_run(fm_fdn *fdn, float x0, float x1, float x2, float x3) {
    float fb[4];
    
    for (int i = 0; i < 4; i++) {
        int len = fdn->delay_length[i];
        fdn->delay_heads[i] = (fdn->delay_heads[i] + len - 1) % len;
        fdn->output[i] = fdn->delays[i][fdn->delay_heads[i]];
        
        fb[i] = (Q(i, 0) * Y(0)
               + Q(i, 1) * Y(1)
               + Q(i, 2) * Y(2)
               + Q(i, 3) * Y(3))
              * fdn->feedback_gain[i];
        
        fb[i] = fm_biquad_run(&fdn->feedback_filter[i], fb[i]);
    }

    fdn->delays[0][fdn->delay_heads[0]] = fb[0] + x0;
    fdn->delays[1][fdn->delay_heads[1]] = fb[1] + x1;
    fdn->delays[2][fdn->delay_heads[2]] = fb[2] + x2;
    fdn->delays[3][fdn->delay_heads[3]] = fb[3] + x3;
}

void fm_fdn_hadamard(fm_fdn *fdn) {
    Q(0, 0) = 0.5;
    Q(0, 1) = 0.5;
    Q(0, 2) = 0.5;
    Q(0, 3) = 0.5;

    Q(1, 0) = -0.5;
    Q(1, 1) = 0.5;
    Q(1, 2) = -0.5;
    Q(1, 3) = 0.5;

    Q(2, 0) = -0.5;
    Q(2, 1) = -0.5;
    Q(2, 2) = 0.5;
    Q(2, 3) = 0.5;

    Q(3, 0) = 0.5;
    Q(3, 1) = -0.5;
    Q(3, 2) = -0.5;
    Q(3, 3) = 0.5;
}

#undef Q
#undef Y

fm_reverb fm_new_reverb(float mix) {
    fm_reverb rv;

    rv.fdn = fm_new_fdn(3041, 3385, 4481, 5477);
    
    fm_fdn_hadamard(&rv.fdn);
    rv.fdn.feedback_gain[0] = 0.83;
    rv.fdn.feedback_gain[1] = 0.9;
    rv.fdn.feedback_gain[2] = 0.93;
    rv.fdn.feedback_gain[3] = 0.85;
    
    for (int i = 0; i < 4; i++) {
        fm_biquad_lowpass(&rv.fdn.feedback_filter[i], 5600, 1 / SQRT2);
    }

    rv.in_gain[0] = 0.4;
    rv.in_gain[1] = 0.3;
    rv.in_gain[2] = 0.2;
    rv.in_gain[3] = 0.2;

    rv.out_gain[0] = 0.5;
    rv.out_gain[1] = 0.5;
    rv.out_gain[2] = 0.3;
    rv.out_gain[3] = 0.1;

    rv.mix = mix;

    return rv;
}

float fm_reverb_run(fm_reverb *rv, float sample) {
    fm_fdn_run(&rv->fdn,
               sample * rv->in_gain[0],
               sample * rv->in_gain[1],
               sample * rv->in_gain[2],
               sample * rv->in_gain[3]);

    float fdn_out = rv->fdn.output[0] * rv->out_gain[0]
                  + rv->fdn.output[1] * rv->out_gain[1]
                  + rv->fdn.output[2] * rv->out_gain[2]
                  + rv->fdn.output[3] * rv->out_gain[3];

    return rv->mix * fdn_out + (1.0f - rv->mix) * sample;
}
