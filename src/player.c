#include "player.h"

void fm_player_outstream_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    UNUSED(frame_count_min);
    
    fm_player *p = (fm_player*) outstream->userdata;

    const struct SoundIoChannelLayout *layout = &outstream->layout;
    struct SoundIoChannelArea *areas;
    int frames_left = frame_count_max;
    int frames_per_quantum = outstream->sample_rate / TIME_QUANTIZE;
    int err;

    while (frames_left > 0) {
        int frame_count = frames_left;

        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fprintf(stderr, "error: %s\n", soundio_strerror(err));
            exit(1);
        }

        if (!frame_count) break;

        for (int frame = 0; frame < frame_count; frame++) {
            if (p->quantize_counter++ >= frames_per_quantum) {
                fm_player_schedule(p, fm_config.dt * frames_per_quantum);
                p->quantize_counter = 0;
            }

			float sample = 0;

            if (!p->paused) {
                for (int i = 0; i < p->num_instrs; i++) {
                    sample += fm_instr_get_next_output(&p->instrs[i],
                                                       p->playhead + frame * fm_config.dt);
                }
                
                sample *= p->volume;
            }

            for (int channel = 0; channel < layout->channel_count; channel++) {
                float *ptr = (float*) (areas[channel].ptr + areas[channel].step * frame);
                *ptr = sample;
            }
        }

        if (!p->paused)
            p->playhead += fm_config.dt * frame_count;

        if ((err = soundio_outstream_end_write(outstream))) {
            fprintf(stderr, "error: %s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
    }
}

fm_player* fm_new_player(int num_instrs, struct SoundIoDevice *device) {
    fm_player *p = malloc(sizeof(fm_player));
    int err;

    p->instrs = malloc(sizeof(fm_instrument) * num_instrs);
    p->num_instrs = num_instrs;
    p->next_notes = calloc(num_instrs, sizeof(int));
    p->bps = 1.0;
    p->playhead = 0;
	p->volume = 1.0;
    p->paused = true;
    
    p->outstream = soundio_outstream_create(device);
    if (!p->outstream) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    if ((err = soundio_outstream_open(p->outstream))) {
        fprintf(stderr, "unable to open device: %s\n", soundio_strerror(err));
        exit(1);
    }

    printf("sample rate: %dHz\n", p->outstream->sample_rate);

    fm_config.sample_rate = p->outstream->sample_rate;
    fm_config.dt = 1.0 / (double) fm_config.sample_rate;

    p->outstream->format = SoundIoFormatFloat32NE;
    p->outstream->userdata = p;
    p->outstream->write_callback = fm_player_outstream_callback;
    p->quantize_counter = p->outstream->sample_rate;
    
    return p;
}

void fm_player_loop(void *player_ptr) {
    fm_player *p = (fm_player*) player_ptr;
    int err;
    p->playing = true;

    if ((err = soundio_outstream_start(p->outstream))) {
        fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
        exit(1);
    }
    
    while (p->playing) {
        soundio_flush_events(p->outstream->device->soundio);
    }
}

void fm_player_schedule(fm_player *p, double time_per_quantum) {
    for (int i = 0; i < p->num_instrs; i++) {
        fm_instrument *instr = &p->instrs[i];
        fm_song_part part = p->song.parts[i];

        // while there are notes remaining in this part which have a start time before or
        // at the current playhead, play them.
        while (p->next_notes[i] < part.num_notes) {            
            if (part.notes[p->next_notes[i]].start >= (p->playhead + time_per_quantum) * p->bps) {
                break;
            }
            
            // find the best candidate note to remove from the synth, which
            // ideally will be one which has already finished playing.
            double earliest_finish = DBL_MAX;
            int earliest_idx = 0;

            // get the note which needs to be played.
            fm_note note = part.notes[p->next_notes[i]];

            for (int n = 0; n < MAX_POLYPHONY; n++) {
                fm_note candidate = instr->voices[n].note;
                double finish = candidate.start + (double) candidate.duration;                

                if (candidate.freq == note.freq) {
                    earliest_finish = finish;
                    earliest_idx = n;
                    break;
                }
                
                if (finish < earliest_finish) {
                    earliest_finish = finish;
                    earliest_idx = n;
                }
            }

            // replace the note that finished longest ago
            double error = p->playhead - note.start;
            note.start = p->playhead;
            note.duration = (note.duration / p->bps) - error;
            instr->voices[earliest_idx].note = note;
            p->next_notes[i]++;
        }
    }
}

void fm_player_pause(fm_player *p) {
    p->playing = false;
    soundio_outstream_pause(p->outstream, true);
}

void fm_player_close(fm_player *p) {
    struct SoundIoDevice *device = p->outstream->device;
    struct SoundIo *soundio = device->soundio;
    soundio_outstream_destroy(p->outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
}

void fm_player_reset(fm_player *p) {
    fm_player_pause(p);
    p->playhead = 0;
    p->quantize_counter = p->outstream->sample_rate;

    for (int i = 0; i < p->num_instrs; i++) {
        fm_instrument *instr = &p->instrs[i];
        p->next_notes[i] = 0;

        for (int v = 0; v < MAX_POLYPHONY; v++) {
            fm_synth *voice = &instr->voices[v];
            voice->hold_index = HOLD_BUFFER_SIZE;
            
            voice->note.start = 0;
            voice->note.duration = 0;
            voice->note.velocity = 0;
            voice->note.freq = 0;

            for (int j = 0; j < N_CHANNELS; j++) {
                voice->channels[j] = 0;
                voice->channels_back[j] = 0;
            }
        }
    }
}
