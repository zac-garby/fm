#ifndef H_FM_OPERATOR
#define H_FM_OPERATOR

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "envelope.h"

#define FN_SIN 0
#define FN_SQUARE 1
#define FN_TRIANGLE 2
#define FN_NOISE 3
#define FN_SAWTOOTH 4

// the normal receipt type for an oscillator. the phase
// delta will be multiplied only by the recv_level value.
#define FM_RECV_NORMAL 0

// used for modulating a signal into a carrier. the phase
// delta will be multiplied additionally by the frequency
// of the note. in this case, the recv_level acts as an
// FM index.
#define FM_RECV_MODULATE 1

// used for vibrato. functionally identical to FM_RECV_NORMAL.
#define FM_RECV_VIBRATO 2

// stores the *settings* of an FM operator.
// the operator is used in an fm_synth, where the actual
// oscillation is performed (see fm_synth.phases.)
typedef struct fm_operator {
    // an array of channels to receive from, and
    // the level of each one to be summed. recv_n is
    // the amount of channels in the array. also,
    // includes a type for each receipt, to determine
    // whether the oscillator's frequency should be
    // multiplied with it.
    int *recv, recv_n;
    float *recv_level;
    int *recv_type;

    // as with recv, etc. but for sending to channels.
    int *send, send_n;
    float *send_level;

    // if false, the osc. frequency is tied to the current note.
    // otherwise, the frequency = the value of transpose.
    bool fixed;

	// the wave function to use.
	int wave_type;

    // a frequency scaling factor.
    float transpose;

    // the envelope that the operator follows.
    fm_envelope envelope;
} fm_operator;

fm_operator fm_new_op(int recv_n, int send_n, int fixed, float transpose);

#endif
