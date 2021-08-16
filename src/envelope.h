#ifndef H_FM_ENVELOPE
#define H_FM_ENVELOPE

#define MIN(x, y) ((x)<(y)?(x):(y))
#define MAX(x, y) ((x)<(y)?(y):(x))

// an envelope. if attack < 0, the envelope is treated specially as a constant "1"
typedef struct fm_envelope {
    float attack;
    float decay;
    float sustain;
    float release;
} fm_envelope;

fm_envelope fm_make_envelope(float attack, float decay, float sustain, float release);

float fm_envelope_evaluate(fm_envelope *env, float t, float hold_time);

#endif