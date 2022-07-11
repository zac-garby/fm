#ifndef H_FM_SONG
#define H_FM_SONG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "note.h"

typedef struct fm_song_part {
    int num_notes;
    fm_note *notes;
} fm_song_part;

typedef struct fm_song {
    int bpm;
    int beats_per_bar;
    int num_parts;
    fm_song_part *parts;
} fm_song;

fm_song fm_new_song(int num_parts, int bpm);
int fm_parse_song(char *filename, fm_song *song);
double fm_song_duration(fm_song *song);

#endif
