#ifndef H_FM_SYNTH
#define H_FM_SYNTH

#include <stdlib.h>
#include <math.h>

#include "operator.h"

static const float PI = 3.1415926535f;

#define N_CHANNELS 16

typedef struct fm_synth {
    // the array of recv/send channels.
    float *channels;

    // the "back buffer" of recv/send channels.
    // this swaps with 'channels' each frame so that sends
    // can go into a blank buffer.
    float *channels_back;

    // the array of operators.
    fm_operator *ops;
    int n_ops;

    // the frequency of the current note to play.
    float freq;
} fm_synth;

fm_synth fm_new_synth(int n_ops);

void fm_synth_swap_buffers(fm_synth *s);
void fm_synth_frame(fm_synth *s, float time);

#endif
