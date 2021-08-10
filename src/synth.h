#ifndef H_FM_SYNTH
#define H_FM_SYNTH

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <soundio/soundio.h>
#include <pthread.h>

#include "operator.h"

static const float PI = 3.1415926535f;

#define N_CHANNELS 16
#define HOLD_BUFFER_SIZE 1024

typedef struct fm_synth {
    // the array of recv/send channels.
    float *channels;

    // the "back buffer" of recv/send channels.
    // this swaps with 'channels' each frame so that sends
    // can go into a blank buffer.
    float *channels_back;

    // the hold buffer, i.e. the history of the synth channels.
    // the history goes back HOLD_BUFFER_SIZE frames, and is used
    // to allow spectral analysis on the synth output.
    float hold_buf[N_CHANNELS][HOLD_BUFFER_SIZE];

    // the current playhead into the hold buffer.
    int hold_index;

    // the array of operators.
    fm_operator *ops;
    int n_ops;

    // the frequency of the current note to play.
    float freq;

    // the thread to run the synth in.
    pthread_t thread;

    // whether to stop the synth thread running.
    bool stop;
} fm_synth;

fm_synth fm_new_synth(int n_ops);

void fm_synth_start(fm_synth *s);
void fm_synth_stop(fm_synth *s);
void fm_synth_swap_buffers(fm_synth *s);
void fm_synth_frame(fm_synth *s, float time);
void fm_synth_fill_hold_buffer(fm_synth *s, float start_time, float seconds_per_frame);
float fm_synth_get_next_output(fm_synth *s, float start_time, float seconds_per_frame);

#endif
