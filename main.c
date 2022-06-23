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
#include "src/export.h"

static fm_player *player;

struct SoundIoDevice* init_audio();

void make_flute(fm_instrument*);
void make_lute(fm_instrument*);
void make_organ(fm_instrument*);

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL failed to initialise: %s\n", SDL_GetError());
        exit(1);
    }

    struct SoundIoDevice *device = init_audio();

    player = fm_new_player(1, device);

    if (!parse_song("assets/runescape.txt", &player->song)) {
        return 0;
    }

    player->volume = 0.15;
    player->bps = (float) player->song.bpm / 60.0f;
    
    make_flute(&player->instrs[0]);
    // make_flute(&player->instrs[1]);
    // make_flute(&player->instrs[2]);
    // make_organ(&player->instrs[3]);
    // make_organ(&player->instrs[4]);
    // make_organ(&player->instrs[5]);
    // make_lute(&player->instrs[6]);
    // make_lute(&player->instrs[7]);
    // make_flute(&player->instrs[1]);
    // make_lute(&player->instrs[2]);

    fm_window win = fm_create_window(player);

    pthread_t player_thread;
    pthread_create(&player_thread, 0, fm_player_loop, player);
    fm_window_loop(&win);
    fm_player_pause(player);
    pthread_join(player_thread, NULL);

    fm_export_wav("out.wav", player, 44100, 16, 216);
    
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
    carr.recv_level[0] = 4.0f;
    carr.recv_type[0] = FM_RECV_MODULATE;
    carr.recv[1] = 3;
    carr.recv_level[1] = 4;
    carr.send[0] = 0;
    carr.send_level[0] = 0.7f;
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
    fb.send_level[0] = 0.15f;
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
    op.envelope = fm_make_envelope(0.01f, 0.6, 0.3, 0.55f);
    op.send[0] = 0;
    op.send_level[0] = 0.40f;
    instr->ops[0] = op;

    fm_operator op2 = fm_new_op(0, 1, false, 2.0f);
    op2.wave_type = FN_SQUARE;
    op2.envelope = fm_make_envelope(0.01f, 0.1f, 0.15f, 0.8f);
    op2.send[0] = 0;
    op2.send_level[0] = 0.05f;
    instr->ops[1] = op2;
}

void make_organ(fm_instrument *instr) {
    fm_new_instr(instr, 8);

    for (int i = 0; i < 4; i++) {
        fm_operator op = fm_new_op(1, 1, false, powf(2.0f, (float) i));
        op.envelope = fm_make_envelope(0.2, 0.2, 1.0, 0.35);
        op.recv[0] = i + 1;
        op.recv_level[0] = 4;
        op.recv_type[0] = FM_RECV_MODULATE;
        op.send[0] = 0;
        op.send_level[0] = 0.5 - (float) i / 20;
        instr->ops[i] = op;

        fm_operator feedback = fm_new_op(1, 1, false, pow(2.0f, (float) i));
        feedback.envelope = fm_make_envelope(0.05, 0.2, 1, 0.35);
        feedback.recv[0] = i + 1;
        feedback.recv_level[0] = 0.65;
        feedback.send[0] = i + 1;
        feedback.send_level[0] = 1;
        instr->ops[i + 4] = feedback;
    }
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
