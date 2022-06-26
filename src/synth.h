#ifndef H_FM_SYNTH
#define H_FM_SYNTH

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <soundio/soundio.h>
#include <pthread.h>
#include <kissfft/kiss_fftr.h>

#include "operator.h"
#include "note.h"

static const float PI = 3.1415926535f;

#define N_CHANNELS 8
#define HOLD_BUFFER_SIZE 1024
#define FREQ_DOMAIN (HOLD_BUFFER_SIZE / 2 + 1)
#define MAX_OPERATORS 8
#define MAX_POLYPHONY 8

struct fm_instrument;
struct fm_synth;

// collects together a number (MAX_POLYPHONY) of identical
// synths, and allocates notes between them to emulate
// polyphony.
typedef struct fm_instrument {
    // each potential voice the instrument can play is a
    // synth, because two notes cannot be played at the
    // same time on one synth. len: MAX_POLYPHONY.
    struct fm_synth *voices;

    // the number of operators in the instrument.
    int n_ops;

    // the operators making up the instrument. these are
    // shared to each of its synths.
    fm_operator *ops;

    // the instrument's hold buffer.
    // i.e. the history of the instrument's combined output from
    // all of its voices. allows for spectral analysis.
    // also the hold back-buffer, which is used for writing to.
    float *hold_buf, *hold_buf_back;

    // the current playhead into the hold buffer.
    int hold_index;

    // the spectral analysis of the current hold_buf, and
    // associated data.
    kiss_fft_cpx spectrum[FREQ_DOMAIN];
    kiss_fft_cfg fft_cfg;
} fm_instrument;

typedef struct fm_synth {
    // the array of recv/send channels.
    float *channels;

    // the "back buffer" of recv/send channels.
    // this swaps with 'channels' each frame so that sends
    // can go into a blank buffer.
    float *channels_back;

    // the current playhead into the hold buffer.
    int hold_index;

    // the instrument associated with this synth.
    struct fm_instrument *instr;

    // the phases of each of the synth's oscillators.
    float *phases;

    // a pointer to the current note to play.
    // if frequency is 0, play nothing.
    fm_note note;
} fm_synth;

void fm_new_instr(fm_instrument *instr, int n_ops);
fm_synth fm_new_synth(fm_instrument *instr);

float fm_instr_get_next_output(fm_instrument *instr,
                               double start_time,
                               double seconds_per_frame);
void fm_instr_fill_hold_buffer(fm_instrument *instr,
                               double start_time,
                               double seconds_per_frame);
void fm_instr_swap_buffers(fm_instrument *instr);

void fm_synth_start(fm_synth *s);
void fm_synth_stop(fm_synth *s);
void fm_synth_swap_buffers(fm_synth *s);
void fm_synth_frame(fm_synth *s, double time, double seconds_per_frame);
void fm_synth_fill_hold_buffer(fm_synth *s,
                               double start_time,
                               double seconds_per_frame);

#endif
