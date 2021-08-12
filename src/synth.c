#include "synth.h"

fm_synth fm_new_synth(int n_ops) {
    fm_synth s;

    s.channels = malloc(sizeof(float) * N_CHANNELS);
    s.channels_back = malloc(sizeof(float) * N_CHANNELS);
    s.hold_index = HOLD_BUFFER_SIZE;
    
    s.ops = malloc(sizeof(fm_operator) * n_ops);
    s.n_ops = n_ops;
    s.stop = false;
    
    for (int i = 0; i < N_CHANNELS; i++) {
        s.channels[i] = 0;
        s.channels_back[i] = 0;
    }

    return s;
}

void fm_synth_swap_buffers(fm_synth *s) {
    float *temp = s->channels;
    s->channels = s->channels_back;
    s->channels_back = temp;

    for (int i = 0; i < N_CHANNELS; i++) {
        s->channels_back[i] = 0.0f;
    }
}

void fm_synth_frame(fm_synth *s, double time) {
    for (int i = 0; i < s->n_ops; i++) {
        fm_operator *op = &s->ops[i];
        float freq;
        double mod = 0;
        if (op->fixed) {
            freq = op->transpose;
        } else {
            freq = s->freq * op->transpose;
        }

        for (int n = 0; n < op->recv_n; n++) {
            mod += s->channels[op->recv[n]] * op->recv_level[n];
        }

        float sample = sin(freq * time * 2.0 * PI + mod);

        for (int n = 0; n < op->send_n; n++) {
            s->channels_back[op->send[n]] += op->send_level[n] * sample;
        }
    }

    fm_synth_swap_buffers(s);
}

void fm_synth_fill_hold_buffer(fm_synth *s, double start_time, double seconds_per_frame) {
    for (int frame = 0; frame < HOLD_BUFFER_SIZE; frame++) {
        double time = start_time + seconds_per_frame * frame;
        fm_synth_frame(s, time);

        for (int c = 0; c < N_CHANNELS; c++) {
            s->hold_buf[c][frame] = s->channels[c];
        }
    }

    s->hold_index = 0;
}

float fm_synth_get_next_output(fm_synth *s, double start_time, double seconds_per_frame) {
    if (s->hold_index >= HOLD_BUFFER_SIZE) {
        fm_synth_fill_hold_buffer(s, start_time, seconds_per_frame);
    }

    float out = s->hold_buf[0][s->hold_index];
    s->hold_index++;
    return out;
}

