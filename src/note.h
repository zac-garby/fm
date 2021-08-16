#ifndef H_FM_NOTE
#define H_FM_NOTE

#include "envelope.h"

typedef struct fm_note {
    float freq;
    double start;
    float duration;
} fm_note;

fm_note fm_make_note(float freq, double start, float duration);

#endif
