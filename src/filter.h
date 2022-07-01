#ifndef H_FM_FILTER
#define H_FM_FILTER

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

// constructs a new empty biquad filter. the constants a and b need to be set
// afterwards, since they are not initialised during this function.
fm_biquad fm_new_biquad();

// sets a biquad filter to act as a passthrough, i.e. does nothing to the
// input.
void fm_biquad_passthrough(fm_biquad *bq);

// pushes the given sample to the beginning of the input history
// array and then runs the filter, returning the processed sample.
float fm_biquad_run(fm_biquad *bq, float x0);

#endif
