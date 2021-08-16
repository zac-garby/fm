#include "note.h"

fm_note fm_make_note(float freq, double start, float duration) {
    fm_note n;

    n.freq = freq;
    n.start = start;
    n.duration = duration;
    
    return n;
}
