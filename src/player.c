#include "player.h"

void fm_player_outstream_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    UNUSED(frame_count_min);
    
    fm_player *p = (fm_player*) outstream->userdata;

    const struct SoundIoChannelLayout *layout = &outstream->layout;
    float time_per_frame = 1.0f / outstream->sample_rate;
    struct SoundIoChannelArea *areas;
    int frames_left = frame_count_max;
    int err;

    while (frames_left > 0) {
        int frame_count = frames_left;

        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fprintf(stderr, "error: %s\n", soundio_strerror(err));
            exit(1);
        }

        if (!frame_count) break;

        for (int frame = 0; frame < frame_count; frame++) {
            float sample = fm_synth_get_next_output(&p->synths[0],
                                                    p->playhead + frame * time_per_frame,
                                                    time_per_frame);

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

fm_player fm_new_player(int num_synths, struct SoundIoDevice *device) {
    fm_player p;

    p.synths = malloc(sizeof(fm_synth) * num_synths);
    p.num_synths = num_synths;
    
    p.outstream = soundio_outstream_create(device);
    if (!p.outstream) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    p.outstream->format = SoundIoFormatFloat32NE;
    p.outstream->userdata = &p;
    p.outstream->write_callback = fm_player_outstream_callback;
    
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


