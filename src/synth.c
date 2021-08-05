#include "synth.h"

fm_synth fm_new_synth(int n_ops) {
    fm_synth s;

    s.channels = malloc(sizeof(float) * N_CHANNELS);
    s.channels_back = malloc(sizeof(float) * N_CHANNELS);
    
    s.ops = malloc(sizeof(fm_operator) * n_ops);
    s.n_ops = n_ops;
    
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

void fm_synth_frame(fm_synth *s, float time) {
    for (int i = 0; i < s->n_ops; i++) {
        fm_operator *op = &s->ops[i];
        float freq;
        float mod = 0;
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
