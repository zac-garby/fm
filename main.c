#include <soundio/soundio.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "src/synth.h"
#include "src/operator.h"

static float seconds_offset = 0.0f;

static fm_synth synth;

static void write_callback(struct SoundIoOutStream *outstream,
        int frame_count_min, int frame_count_max) {
    const struct SoundIoChannelLayout *layout = &outstream->layout;
    float float_sample_rate = outstream->sample_rate;
    float seconds_per_frame = 1.0f / float_sample_rate;
    struct SoundIoChannelArea *areas;
    int frames_left = frame_count_max;
    int err;

    while (frames_left > 0) {
        int frame_count = frames_left;

        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            exit(1);
        }

        if (!frame_count)
            break;

        float pitch = 440.0f;
        float radians_per_second = pitch * 2.0f * PI;
        for (int frame = 0; frame < frame_count; frame += 1) {
            float sample = fm_synth_get_next_output(&synth,
                                                    seconds_offset + frame * seconds_per_frame,
                                                    seconds_per_frame);
            
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);
                *ptr = sample;
            }
        }

        seconds_offset += seconds_per_frame * frame_count;
        //seconds_offset = fmod(seconds_offset + seconds_per_frame * frame_count, 10.0);

        if ((err = soundio_outstream_end_write(outstream))) {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
    }
}

int main() {
    synth = fm_new_synth(2);
    synth.freq = 440.0f;
    
    fm_operator op = fm_new_op(0, 1, false, 1.05f);
    op.send[0] = 1;
    op.send_level[0] = 1.0f;
    synth.ops[0] = op;

    fm_operator op2 = fm_new_op(1, 1, false, 1.0f);
    op2.recv[0] = 1;
    op2.recv_level[0] = 1.0f;
    op2.send[0] = 0;
    op2.send_level[0] = 0.1f;
    synth.ops[1] = op2;
    
    int err;
    struct SoundIo *soundio = soundio_create();
    if (!soundio) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if ((err = soundio_connect(soundio))) {
        fprintf(stderr, "error connecting: %s\n", soundio_strerror(err));
        return 1;
    }

    soundio_flush_events(soundio);

    int default_out_device_index = soundio_default_output_device_index(soundio);
    if (default_out_device_index < 0) {
        fprintf(stderr, "no output device found\n");
        return 1;
    }

    struct SoundIoDevice *device = soundio_get_output_device(soundio, default_out_device_index);
    if (!device) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    fprintf(stderr, "Output device: %s\n", device->name);

    struct SoundIoOutStream *outstream = soundio_outstream_create(device);
    if (!outstream) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    outstream->format = SoundIoFormatFloat32NE;
    outstream->write_callback = write_callback;

    if ((err = soundio_outstream_open(outstream))) {
        fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
        return 1;
    }

    if (outstream->layout_error)
        fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

    if ((err = soundio_outstream_start(outstream))) {
        fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
        return 1;
    }

    for (;;)
        soundio_wait_events(soundio);

    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
    return 0;
}
