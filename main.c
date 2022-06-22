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
#include "src/note.h"
#include "src/song.h"

static fm_player *player;

struct SoundIoDevice* init_audio();

void make_flute(fm_instrument*);
void make_lute(fm_instrument*);
// fm_instrument make_lute();
// fm_instrument make_brass();
// fm_instrument make_sine();

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL failed to initialise: %s\n", SDL_GetError());
        exit(1);
    }

    struct SoundIoDevice *device = init_audio();

    player = fm_new_player(2, device);

    if (!parse_song("assets/crab.txt", &player->song)) {
        return 0;
    }

    player->volume = 0.025;
    player->bps = (float) player->song.bpm / 60.0f;
    
    make_flute(&player->instrs[0]);
    make_lute(&player->instrs[1]);
    // player->instrs[1] = make_brass();

    fm_window win = fm_create_window(player);

    pthread_t player_thread;
    pthread_create(&player_thread, 0, fm_player_loop, player);
    fm_window_loop(&win);
    player->playing = false;
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

void make_flute(fm_instrument *instr) {
    fm_new_instr(instr, 5);

    fm_operator carr = fm_new_op(2, 1, false, 1.0f);
    carr.wave_type = FN_SIN;
    carr.envelope = fm_make_envelope(0.1, 0.45, 0.8, 0.35f);
    carr.recv[0] = 1;
    carr.recv_level[0] = 1550.0f;
    carr.recv[1] = 3;
    carr.recv_level[1] = 4;
    carr.send[0] = 0;
    carr.send_level[0] = 1.0f;
    instr->ops[0] = carr;

    fm_operator mod1 = fm_new_op(0, 1, false, 2.0f);
    mod1.envelope = fm_make_envelope(0.1, 0.45, 0.75, 0.35f);
    mod1.send[0] = 1;
    mod1.send_level[0] = 0.43f;
    instr->ops[1] = mod1;

    fm_operator mod2 = fm_new_op(0, 1, false, 1.0f);
    mod2.envelope = fm_make_envelope(0.03, 0.45, 0.75, 0.35f);
    mod2.send[0] = 1;
    mod2.send_level[0] = 0.37f;
    instr->ops[2] = mod2;

    fm_operator fb = fm_new_op(2, 1, false, 1.1f);
    fb.envelope = fm_make_envelope(0.04, 0.5, 0.1, 0.15f);
    fb.recv[0] = 2;
    fb.recv_level[0] = 1.0f;
    fb.send[0] = 2;
    fb.send_level[0] = 0.5f;
    fb.send[1] = 1;
    fb.send_level[0] = 0.03f;
    instr->ops[3] = fb;

    fm_operator vib = fm_new_op(0, 1, true, 4.0f);
    vib.envelope = fm_make_envelope(1.3, 0.2, 2.0, 0.0);
    vib.send[0] = 3;
    vib.send_level[0] = 1.0f;
    instr->ops[4] = vib;
}

void make_lute(fm_instrument *instr) {
    fm_new_instr(instr, 2);

    fm_operator op = fm_new_op(0, 1, false, 1.0f);
    op.wave_type = FN_TRIANGLE;
    op.envelope = fm_make_envelope(0.2f, 0.3, 0.6, 0.35f);
    op.send[0] = 0;
    op.send_level[0] = 0.45f;
    instr->ops[0] = op;

    fm_operator op2 = fm_new_op(0, 1, false, 0.5f);
    op2.wave_type = FN_SQUARE;
    op2.envelope = fm_make_envelope(0.01f, 0.3f, 0.4f, 0.8f);
    op2.send[0] = 0;
    op2.send_level[0] = 0.25f;
    instr->ops[1] = op2;
}

/*
fm_instrument make_brass() {
    fm_instrument instr = fm_new_instr(2);

    fm_operator op = fm_new_op(1, 1, false, 1.0f);
    op.wave_type = FN_SIN;
    op.envelope = fm_make_envelope(0.12f, 0.5f, 1.0f, 0.05f);
    op.recv[0] = 1;
    op.recv_level[0] = 700.0f;
    op.send[0] = 0;
    op.send_level[0] = 1.5f;
    instr.ops[0] = op;

    fm_operator feedback = fm_new_op(1, 1, false, 1.0f);
    feedback.wave_type = FN_TRIANGLE;
    feedback.envelope = fm_make_envelope(0.1f, 0.4f, 0.7f, 0.1f);
    feedback.recv[0] = 1;
    feedback.recv_level[0] = 1.0f;
    feedback.send[0] = 1;
    feedback.send_level[0] = 0.8f;
    instr.ops[1] = feedback;

    return instr;
}

fm_instrument make_sine() {
    fm_instrument instr = fm_new_instr(2);

    fm_operator op = fm_new_op(1, 1, false, 1.0f);
    op.wave_type = FN_SIN;
    op.envelope = fm_make_envelope(0.1f, 0.4, 5.0, 0.1);
    op.recv[0] = 1;
    op.recv_level[0] = 4.0;
    op.send[0] = 0;
    op.send_level[0] = 0.45f;
    instr.ops[0] = op;

    fm_operator vib = fm_new_op(0, 1, true, 8.0f);
    vib.wave_type = FN_SIN;
    vib.envelope = fm_make_envelope(-1, 0, 0, 0);
    vib.send[0] = 1;
    vib.send_level[0] = 1.0f;
    instr.ops[1] = vib;

	return instr;
}
*/
