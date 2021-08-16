#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "src/synth.h"
#include "src/player.h"
#include "src/operator.h"
#include "src/window.h"
#include "src/envelope.h"

static fm_player player;

struct SoundIoDevice* init_audio();

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL failed to initialise: %s\n", SDL_GetError());
        exit(1);
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "SDL ttf failed to initialise\n");
        exit(1);
    }

    struct SoundIoDevice *device = init_audio();

    player = fm_new_player(1, device);
    
    fm_window win = fm_create_window(&player);
    
    player.synths[0] = fm_new_synth(5);
    player.synths[0].notes[0] = 440.0f;
    player.synths[0].notes[1] = 6000.0f;
    player.synths[0].num_notes = 2;
    
    fm_operator op = fm_new_op(0, 1, true, 80.0f);
    op.envelope = fm_make_envelope(0.2, 0.4, 0.3, 1.0);
    op.send[0] = 1;
    op.send_level[0] = 1.0f;
    player.synths[0].ops[0] = op;

    fm_operator op2 = fm_new_op(1, 1, false, 1.0f);
    op2.envelope = fm_make_envelope(0.2, 0.4, 0.3, 1.0);
    op2.recv[0] = 0;
    op2.recv_level[0] = 0.0f;
    op2.send[0] = 0;
    op2.send_level[0] = 0.43f;
    player.synths[0].ops[1] = op2;

    fm_operator op3 = fm_new_op(1, 1, false, 2.01f);
    op3.envelope = fm_make_envelope(0.2, 0.4, 0.3, 1.0);
    op3.recv[0] = 1;
    op3.recv_level[0] = 0.0f;
    op3.send[0] = 0;
    op3.send_level[0] = 0.30f;
    player.synths[0].ops[2] = op3;

    fm_operator op4 = fm_new_op(1, 1, false, 4.01f);
    op4.envelope = fm_make_envelope(0.2, 0.4, 0.3, 1.0);
    op4.recv[0] = 1;
    op4.recv_level[0] = 0.0f;
    op4.send[0] = 0;
    op4.send_level[0] = 0.22f;
    player.synths[0].ops[3] = op4;

    fm_operator op5 = fm_new_op(1, 1, false, 8.01f);
    op5.envelope = fm_make_envelope(0.2, 0.4, 0.3, 1.0);
    op5.recv[0] = 0;
    op5.recv_level[0] = 250.0f;
    op5.send[0] = 0;
    op5.send_level[0] = 0.15f;
    player.synths[0].ops[4] = op5;

    pthread_t player_thread;
    pthread_create(&player_thread, 0, fm_player_loop, &player);
    fm_window_loop(&win);
    player.playing = false;
    pthread_join(player_thread, NULL);
    
    return 0;
}

struct SoundIoDevice* init_audio() {
    int err;
    struct SoundIo *soundio = soundio_create();
    if (!soundio) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    if ((err = soundio_connect(soundio))) {
        fprintf(stderr, "error connecting: %s\n", soundio_strerror(err));
        exit(1);
    }

    soundio_flush_events(soundio);

    int default_out_device_index = soundio_default_output_device_index(soundio);
    if (default_out_device_index < 0) {
        fprintf(stderr, "no output device found\n");
        exit(1);
    }

    struct SoundIoDevice *device = soundio_get_output_device(soundio, default_out_device_index);
    if (!device) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    fprintf(stderr, "Output device: %s\n", device->name);

    return device;
}
