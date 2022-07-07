#ifndef H_FM_FILTER
#define H_FM_FILTER

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#include "common.h"

#define EQ_MAX_PEAKS 8

// a biquad filter, able to take the form of many LTI filters including
// the filters required for EQ (low-pass, high-pass, and peak).
typedef struct fm_biquad {
    // the filter's coefficients. a is the denominator, b is the numerator.
    // a[0] is ignored and assumed to be 1, since it should always be.
    double a[3], b[3];

    // the input values to the filter.
    // x[0] is the current, and x[1-2] are the previous two values.
    float x[3];

    // the output values from the filter.
    // y[0] is the current, and y[1-2] are the previous two.
    // y[0] is only accurate after the filter has been run.
    float y[3];
} fm_biquad;

// an equaliser, consisting of multiple biquad filters.
typedef struct fm_eq {
    // the frequency and peak gain of the EQ's low- and high-pass filters.
    // if the frequency is negative, the filter is ignored.
    double lowpass_hz, lowpass_Q;
    double highpass_hz, highpass_Q;

    // the output volume of the EQ filter.
    double gain;

    // the frequency, Q, and A values for each peak filter in the EQ.
    double peaks_hz[EQ_MAX_PEAKS], peaks_Q[EQ_MAX_PEAKS], peaks_A[EQ_MAX_PEAKS];

    // the total number of peak filters.
    int num_peaks;

    // the sequence of biquad filters which form the EQ. these are
    // generated from the EQ settings, i.e. the rest of the struct.
    fm_biquad *biquads;

    // the number of biquad filters currently allocated for the EQ.
    int biquads_cap;

    // the number of biquad filters currently in use by the EQ.
    int num_biquads;
} fm_eq;

// a four-channel feedback-delay-network. four inputs are given at each frame,
// and are delayed by different amounts. feedback from these delays is taken and
// added to the inputs before feeding into the delay lines.
typedef struct fm_fdn {
    // the lengths of the four delay lines.
    int delay_length[4];

    // the final gain of the four feedback lines, before adding with new samples.
    float feedback_gain[4];

    // the feedback matrix of the FDN, aka "Q". the feedback into channel n is:
    // (Q[n][0] * y[0] + Q[n][1] * y[1] + Q[n][2] * y[2] + Q[n][3] * y[3]) * fb_gain[n]
    float feedback_matrix[4][4];

    // the filters to apply to each sample finally before feeding back into the delay
    // lines.
    fm_biquad feedback_filter[4];

    // the four delay lines. delays[n] is delay_length[n] samples long. they are
    // implemented as circular buffers, growing towards negative indices.
    float * delays[4];
    int delay_heads[4];

    // the latest output from the FDN, from each channel.
    float output[4];
} fm_fdn;

// a reverb effect, implemented with a feedback delay network. the single output
// is spread into the four FDN channels, and the output of the FDN is combined
// back into a single output, passed through an EQ filter, and combined with the
// original signal.
typedef struct fm_reverb {
    // the internal feedback delay network.
    fm_fdn fdn;

    // the gains to input to four channels, and to combine them back into one.
    // on the input side, the input sample is multiplied by each in_gain, and these
    // are added to the corresponding channel. the opposite is done on the output.
    float in_gain[4], out_gain[4];

    // the proportion of the final output which is the reverb signal. the rest is
    // the dry signal. this should be between 0 and 1.
    float mix;
} fm_reverb;

// constructs a new equaliser. the settings of the EQ should be set
// afterwards, and then the biquads can be generated.
fm_eq fm_new_eq();

// constructs a new empty biquad filter. the constants a and b need to be set
// afterwards, since they are not initialised during this function.
fm_biquad fm_new_biquad();

// makes a new feedback delay network, with the specified delay lengths. the
// gains and feedback matrix should be set separately before use.
fm_fdn fm_new_fdn(int l0, int l1, int l2, int l3);

// makes a new reverb effect, with default settings, but a given mix amount.
fm_reverb fm_new_reverb(float mix);

// generates the required biquad filters to simulate the specified EQ
// settings.
void fm_eq_bake(fm_eq *eq);

// passes a sample through all of the filters in an EQ.
float fm_eq_run(fm_eq *eq, float sample);

// convenience functions for setting values or adding filters to the EQ.
void fm_eq_lowpass(fm_eq *eq, double hz, double Q);
void fm_eq_highpass(fm_eq *eq, double hz, double Q);
void fm_eq_add_peak(fm_eq *eq, double hz, double Q, double A);

// sets a biquad filter to act as a simple constant gain filter. each frequency
// is scaled by the same amount.
void fm_biquad_gain(fm_biquad *bq, double gain);

// sets a biquad filter to act as a low/high-pass filter about the given
// frequency, and with the given peak gain, Q.
// this relies on the "dt" = 1/sample_rate.
void fm_biquad_lowpass(fm_biquad *bq, double hz, double Q);
void fm_biquad_highpass(fm_biquad *bq, double hz, double Q);

// sets a biquad filter to act as a peak filter about the given frequency,
// with a peak gain of A and a given Q (how "peaky" the peak is).
void fm_biquad_peak(fm_biquad *bq, double hz, double Q, double A);

// sets a biquad filter to act as a high-shelf filter about the given
// frequency and with a given shelf linear gain, Q. this is a first-order
// filter, so a[2] and b[2] remain 0.
void fm_biquad_highshelf(fm_biquad *bq, double hz, double Q);

// pushes the given sample to the beginning of the input history
// array and then runs the filter, returning the processed sample.
float fm_biquad_run(fm_biquad *bq, float x0);

// runs one frame of a feedback delay network, by feeding in four parallel
// samples.
void fm_fdn_run(fm_fdn *fdn, float x0, float x1, float x2, float x3);

// sets a feedback delay network's feedback matrix to be a 4x4 hadamard
// matrix. this is useful in reverb.
void fm_fdn_hadamard(fm_fdn *fdn);

// runs a sample through the reverb effect.
float fm_reverb_run(fm_reverb *rv, float sample);

#endif
