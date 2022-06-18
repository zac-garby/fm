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

static fm_player *player;

struct SoundIoDevice* init_audio();

void crab_canon(fm_player *p);
fm_synth make_synth1();
fm_synth make_synth2();
fm_synth make_synth3();

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

    player = fm_new_player(3, device);
    player->bps = 2.0;
	player->volume = 0.1;

	crab_canon(player);
    
    player->synths[0] = make_synth1();
	player->synths[1] = make_synth2();
    player->synths[2] = make_synth3();

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

void crab_canon(fm_player *p) {
    float notes[90];
	double durs[90];

	// notes (forwards)
	notes[0] = C4;
	notes[1] = EB4;
	
	notes[2] = G4;
	notes[3] = AB4;

	notes[4] = B3;
	notes[5] = G4;
	
	notes[6] = FS4;
	notes[7] = F4;

	notes[8] = E4;
	notes[9] = EB4;

	notes[10] = D4;
	notes[11] = DB4;
	notes[12] = C4;

	notes[13] = B3;
	notes[14] = G3;
	notes[15] = C4;
	notes[16] = F4;

	notes[17] = EB4;
	notes[18] = D4;

	notes[19] = C4;
	notes[20] = EB4;

	notes[21] = G4;
	notes[22] = F4;
	notes[23] = G4;
	notes[24] = C5;
	notes[25] = G4;
	notes[26] = EB4;
	notes[27] = D4;
	notes[28] = EB4;

	notes[29] = F4;
	notes[30] = G4;
	notes[31] = A4;
	notes[32] = B4;
	notes[33] = C5;
	notes[34] = EB4;
	notes[35] = F4;
	notes[36] = G4;

	notes[37] = AB4;
	notes[38] = D4;
	notes[39] = EB4;
	notes[40] = F4;
	notes[41] = G4;
	notes[42] = F4;
	notes[43] = EB4;
	notes[44] = D4;

	notes[45] = EB4;
	notes[46] = F4;
	notes[47] = G4;
	notes[48] = AB4;
	notes[49] = BB4;
	notes[50] = AB4;
	notes[51] = G4;
	notes[52] = F4;

	notes[53] = G4;
	notes[54] = AB4;
	notes[55] = BB4;
	notes[56] = C5;
	notes[57] = DB5;
	notes[58] = BB4;
	notes[59] = AB4;
	notes[60] = G4;

	notes[61] = A4;
	notes[62] = B4;
	notes[63] = C5;
	notes[64] = D5;
	notes[65] = EB5;
	notes[66] = C5;
	notes[67] = B4;
	notes[68] = A4;

	notes[69] = B4;
	notes[70] = C5;
	notes[71] = D5;
	notes[72] = EB5;
	notes[73] = F5;
	notes[74] = D5;
	notes[75] = G4;
	notes[76] = D5;

	notes[77] = C5;
	notes[78] = D5;
	notes[79] = EB5;
	notes[80] = F5;
	notes[81] = EB5;
	notes[82] = D5;
	notes[83] = C5;
	notes[84] = B4;
	
	notes[85] = C5;
	notes[86] = G4;
	notes[87] = EB4;
	notes[88] = C4;


	// durations (forwards)
	durs[0] = 2;
	durs[1] = 2;
	
	durs[2] = 2;
	durs[3] = 2;

	durs[4] = 3;
	durs[5] = 2;

	durs[6] = 2;
	durs[7] = 2;
	
	durs[8] = 2;
	durs[9] = 2;

	durs[10] = 1;
	durs[11] = 1;
	durs[12] = 1;

	durs[13] = 1;
	durs[14] = 1;
	durs[15] = 1;
	durs[16] = 1;

	durs[17] = 2;
	durs[18] = 2;

	durs[19] = 2;
	durs[20] = 2;

	for (int i = 21; i < 85; i++) {
	    durs[i] = 0.5f;
	}

	durs[85] = 1;
	durs[86] = 1;
	durs[87] = 1;
	durs[88] = 1;


	// set the notes
	
	p->song_parts[0].num_notes = 89;
	p->song_parts[0].notes = malloc(sizeof(fm_note) * 89);

	p->song_parts[1].num_notes = 89;
	p->song_parts[1].notes = malloc(sizeof(fm_note) * 89);

    p->song_parts[2].num_notes = 18 * 4;
    p->song_parts[2].notes = malloc(sizeof(fm_note) * 18 * 4);

	double pos1 = 0;
	double pos2 = 0;
	
	for (int i = 0; i < 89; i++) {
	    int j = 88 - i;
        p->song_parts[0].notes[i] = fm_make_note(notes[i], pos1, durs[i]);
		p->song_parts[1].notes[i] = fm_make_note(notes[j], pos2, durs[j]);
		pos1 += durs[i];
		pos2 += durs[j];
	}

    for (int i = 0; i < 18 * 4; i++) {
        p->song_parts[2].notes[i] =
            fm_make_note(i % 4 == 0 ? C0 : C1, (float) i, 1.0f);
    }
}

fm_synth make_synth1() {
    fm_synth s = fm_new_synth(2);

    fm_operator op = fm_new_op(1, 1, false, 1.0f);
    op.wave_type = FN_SAWTOOTH;
    op.envelope = fm_make_envelope(0.1, 0.8, 0.8, 0.2f);
    op.recv[0] = 1;
    op.recv_level[0] = 1.0f;
    op.send[0] = 0;
    op.send_level[0] = 0.75f;
    s.ops[0] = op;

    fm_operator vib = fm_new_op(0, 1, true, 10.0f);
    vib.wave_type = FN_SIN;
    vib.envelope = fm_make_envelope(0.0, 1.0, 0.2, 1.0);
    vib.send[0] = 1;
    vib.send_level[0] = 5.0f;
    s.ops[1] = vib;

	return s;
}

fm_synth make_synth2() {
    fm_synth s = fm_new_synth(1);

    fm_operator op = fm_new_op(0, 1, false, 1.0f);
    op.wave_type = FN_SQUARE;
    op.envelope = fm_make_envelope(0.0f, 0.8, 0.8, 0.2f);
    op.send[0] = 0;
    op.send_level[0] = 0.25f;
    s.ops[0] = op;

	return s;
}

fm_synth make_synth3() {
    fm_synth s = fm_new_synth(2);

    fm_operator op = fm_new_op(0, 2, true, 1.0f);
    op.wave_type = FN_NOISE;
    op.envelope = fm_make_envelope(0.0f, 0.6f, 0.3f, 0.3f);
    op.send[0] = 1;
    op.send_level[0] = 4.0f;
    op.send[1] = 0;
    op.send_level[1] = 0.1f;
    s.ops[0] = op;

    fm_operator op2 = fm_new_op(1, 1, false, 1.0f);
    op2.wave_type = FN_SQUARE;
    op2.envelope = fm_make_envelope(0.0f, 0.2f, 0.4f, 0.0f);
    op2.recv[0] = 1;
    op2.recv_level[0] = 2.0f;
    op2.send[0] = 0;
    op2.send_level[0] = 0.6f;
    s.ops[1] = op2;

    return s;
}
