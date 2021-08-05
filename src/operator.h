#ifndef H_FM_OPERATOR
#define H_FM_OPERATOR

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define RECV_MAX 8
#define SEND_MAX 8

typedef struct fm_operator {
    // an array of channels to receive from, and
    // the level of each one to be summed. recv_n is
    // the amount of channels in the array.
    int *recv, recv_n;
    float *recv_level;

    // as with recv, etc. but for sending to channels.
    int *send, send_n;
    float *send_level;

    // if false, the osc. frequency is tied to the current note.
    // otherwise, the frequency = the value of transpose.
    bool fixed;

    // a frequency scaling factor.
    float transpose;
} fm_operator;

fm_operator fm_new_op(int recv_n, int send_n, int fixed, float transpose);

#endif
