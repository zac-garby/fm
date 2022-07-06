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

// constructs a new equaliser. the settings of the EQ should be set
// afterwards, and then the biquads can be generated.
fm_eq fm_new_eq();

// generates the required biquad filters to simulate the specified EQ
// settings.
void fm_eq_bake(fm_eq *eq);

// passes a sample through all of the filters in an EQ.
float fm_eq_run(fm_eq *eq, float sample);

// convenience functions for setting values or adding filters to the EQ.
void fm_eq_lowpass(fm_eq *eq, double hz, double Q);
void fm_eq_highpass(fm_eq *eq, double hz, double Q);
void fm_eq_add_peak(fm_eq *eq, double hz, double Q, double A);

// constructs a new empty biquad filter. the constants a and b need to be set
// afterwards, since they are not initialised during this function.
fm_biquad fm_new_biquad();

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

#endif
