#include "note.h"

fm_note fm_make_note(int pitch, int beat, int div, int dur, float velocity) {
    fm_note n;

    n.pitch = pitch;
    n.beat = beat;
    n.division = div;
    n.duration = dur;
    n.velocity = velocity;
    
    return n;
}

float fm_note_get_freq(fm_note *note) {
    return C0 * powf(2.0f, (float) note->pitch / 12.0f);
}

double fm_note_get_start_time(fm_note *note, double bps) {
    return ((double) note->beat + (double) note->division / FM_BEAT_DIVISIONS) / bps;
}

double fm_note_get_duration(fm_note *note, double bps) {
    return ((double) note->duration / FM_BEAT_DIVISIONS) / bps;
}

double fm_note_get_end_time(fm_note *note, double bps) {
    return fm_note_get_start_time(note, bps) + fm_note_get_duration(note, bps);
}
