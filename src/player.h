#ifndef H_FM_PLAYER
#define H_FM_PLAYER

#include <soundio/soundio.h>
#include <stdbool.h>

#include "synth.h"

#define UNUSED(x) (void)(x)

typedef struct fm_player {
    // an array of synths
    fm_synth *synths;
    int num_synths;

    // the soundio outstream struct.
    struct SoundIoOutStream *outstream;

    // the playhead position in seconds
    double playhead;

    // whether or not the player is playing at the moment.
    // set this to false to stop the play loop.
    bool playing;
} fm_player;

fm_player fm_new_player(int num_synths, struct SoundIoDevice *device);

void fm_player_loop(void *player_ptr);

#endif
