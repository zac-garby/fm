#ifndef H_FM_PLAYER
#define H_FM_PLAYER

#include <soundio/soundio.h>
#include <stdbool.h>
#include <float.h>

#include "synth.h"
#include "song.h"

#define UNUSED(x) (void)(x)

#define TIME_QUANTIZE 1024

typedef struct fm_player {
    // an array of instruments.
    fm_instrument *instrs;
    int num_instrs;

    // the soundio outstream struct.
    struct SoundIoOutStream *outstream;

    // the playhead position in seconds.
    double playhead;

    // the beats per second to play the song at. the note times in each song
    // part are given in beats rather than seconds, and this scales them to
    // real time which is required to play them.
    double bps;

    // the volume of the player. 0 is silent, 1 is maximum.
    double volume;

    // the song to play, represented as a number (num_synths) of song parts.
    // each part corresponds to the synth with the same index.
    fm_song song;

    // the index for each song part's next note to play. initially all 0.
    int *next_notes;

    // time is quantized into TIME_QUANTIZE chunks per second.
    // this counter is used to determine when to roll over to the next
    // quantum.
    int quantize_counter;

    // whether or not the player is playing at the moment.
    // set this to false to stop the play loop.
    bool playing;
} fm_player;

fm_player* fm_new_player(int num_instrs, struct SoundIoDevice *device);

void fm_player_loop(void *player_ptr);
void fm_player_schedule(fm_player *p, double time_per_quantum);

#endif
