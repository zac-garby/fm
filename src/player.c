#include "player.h"

void fm_player_outstream_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    UNUSED(frame_count_min);
    
    fm_player *p = (fm_player*) outstream->userdata;

    const struct SoundIoChannelLayout *layout = &outstream->layout;
    double time_per_frame = 1.0 / outstream->sample_rate;
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
                fm_player_schedule(p, time_per_frame * frames_per_quantum);
                p->quantize_counter = 0;
            }
            
            float sample = fm_synth_get_next_output(&p->synths[0],
                                                    p->playhead + frame * time_per_frame,
                                                    time_per_frame);

            //sample = 0;

            for (int channel = 0; channel < layout->channel_count; channel++) {
                float *ptr = (float*) (areas[channel].ptr + areas[channel].step * frame);
                *ptr = sample;
            }
        }

        p->playhead += time_per_frame * frame_count;

        if ((err = soundio_outstream_end_write(outstream))) {
            fprintf(stderr, "error: %s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
    }
}

fm_player* fm_new_player(int num_synths, struct SoundIoDevice *device) {
    fm_player *p = malloc(sizeof(fm_player));

    p->synths = malloc(sizeof(fm_synth) * num_synths);
    p->num_synths = num_synths;
    p->song_parts = malloc(sizeof(fm_song_part) * num_synths);
    p->next_notes = calloc(num_synths, sizeof(int));
    p->bps = 1.0;
    p->playhead = 0;
    
    p->outstream = soundio_outstream_create(device);
    if (!p->outstream) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

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
    
    if ((err = soundio_outstream_open(p->outstream))) {
        fprintf(stderr, "unable to open device: %s\n", soundio_strerror(err));
        exit(1);
    }

    if ((err = soundio_outstream_start(p->outstream))) {
        fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
        exit(1);
    }

    while (p->playing) {
        soundio_flush_events(p->outstream->device->soundio);
    }
}

void fm_player_schedule(fm_player *p, double time_per_quantum) {
    for (int i = 0; i < p->num_synths; i++) {        
        fm_synth *s = &p->synths[i];
        fm_song_part part = p->song_parts[i];

        // while there are notes remaining in this part which have a start time before or
        // at the current playhead, play them.
        while (p->next_notes[i] < part.num_notes) {            
            if (part.notes[p->next_notes[i]].start > (p->playhead - time_per_quantum) * p->bps) {
                break;
            }
            
            // find the best candidate note to remove from the synth, which
            // ideally will be one which has already finished playing.
            double earliest_finish = DBL_MAX;
            int earliest_idx = 0;

            for (int n = 0; n < MAX_POLYPHONY; n++) {
                fm_note candidate = s->notes[n];
                double finish = candidate.start + (double) candidate.duration;
                if (finish < earliest_finish) {
                    earliest_finish = finish;
                    earliest_idx = n;
                }
            }

            // replace the note that finished longest ago
            fm_note note = part.notes[p->next_notes[i]];
            note.start /= p->bps;
            note.duration /= p->bps;
            printf("scheudling note (%d) @ %f: %f %f %f\n", earliest_idx, p->playhead, note.freq, note.start, note.duration);
            s->notes[earliest_idx] = note;
            p->next_notes[i]++;
        }
    }
}
