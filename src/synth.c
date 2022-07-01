#pragma clang diagnostic ignored "-Wincompatible-pointer-types"

#include "synth.h"

void fm_new_instr(fm_instrument *instr, int n_ops) {
    instr->n_ops = n_ops;
    instr->ops = malloc(sizeof(fm_operator) * n_ops);

    instr->voices = malloc(sizeof(fm_synth) * MAX_POLYPHONY);

    for (int i = 0; i < MAX_POLYPHONY; i++) {
        instr->voices[i] = fm_new_synth(instr);
    }

    instr->hold_index = HOLD_BUFFER_SIZE;

    instr->hold_buf = calloc(HOLD_BUFFER_SIZE, sizeof(float));
    instr->hold_buf_back = calloc(HOLD_BUFFER_SIZE, sizeof(float));

    instr->fft_cfg = kiss_fftr_alloc(HOLD_BUFFER_SIZE,
                                     0, NULL, NULL);

    instr->bq = fm_new_biquad();
    fm_biquad_passthrough(&instr->bq);
}

float fm_instr_get_next_output(fm_instrument *instr,
                              double start_time,
                              double seconds_per_frame) {
    if (instr->hold_index >= HOLD_BUFFER_SIZE) {
        fm_instr_fill_hold_buffer(instr, start_time, seconds_per_frame);
    }
    
    return instr->hold_buf[instr->hold_index++];
}

void fm_instr_fill_hold_buffer(fm_instrument *instr,
                               double start_time,
                               double seconds_per_frame) {
    for (int i = 0; i < HOLD_BUFFER_SIZE; i++) {
        instr->hold_buf_back[i] = 0;
    }

    for (int i = 0; i < MAX_POLYPHONY; i++) {
        fm_synth_fill_hold_buffer(&instr->voices[i],
                                  start_time,
                                  seconds_per_frame);
    }

    // apply filters to the whole buffer at once
    for (int i = 0; i < HOLD_BUFFER_SIZE; i++) {
        X(i) = fm_biquad_run(&instr->bq, X(i));
    }

    fm_instr_swap_buffers(instr);
    
    instr->hold_index = 0;
}

void fm_instr_swap_buffers(fm_instrument *instr) {
    // swap the hold buffer and the back-buffer.
    // the hold buffer now contains the new samples.
    float *temp = instr->hold_buf;
    instr->hold_buf = instr->hold_buf_back;
    instr->hold_buf_back = temp;

    // also, computes the spectrum of the hold buffer.
    kiss_fftr(instr->fft_cfg, instr->hold_buf, instr->spectrum);
}



fm_synth fm_new_synth(fm_instrument *instr) {
    fm_synth s;

    s.channels = calloc(N_CHANNELS, sizeof(float));
    s.channels_back = calloc(N_CHANNELS, sizeof(float));
    
    s.instr = instr;
    s.phases = calloc(MAX_OPERATORS, sizeof(float));

    s.note.freq = 0;
    s.note.start = 0;
    s.note.duration = 0;
    s.note.velocity = 0;
    
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

void fm_synth_frame(fm_synth *s, double time, double seconds_per_frame) {
    for (int i = 0; i < s->instr->n_ops; i++) {
        fm_operator *op = &s->instr->ops[i];
        
        for (int n = 0; n < op->recv_n; n++) {
            float mod = s->channels[op->recv[n]] * op->recv_level[n] * seconds_per_frame;
            if (op->recv_type[n] == FM_RECV_MODULATE) mod *= s->note.freq;
            s->phases[i] += mod;
        }

        while (s->phases[i] > 2 * PI) s->phases[i] -= 2 * PI;

        float sample = 0;

        if (s->note.freq > 0) {
            float env = fm_envelope_evaluate(&op->envelope,
                                             time - s->note.start,
                                             s->note.duration);
            float vel = env * s->note.velocity;
            double f = op->fixed ? op->transpose : s->note.freq * op->transpose;
            double t = f * time + (double) s->phases[i];
            float wave;

            switch (op->wave_type) {
            case FN_SIN:
                wave = -cos(2.0f * PI * t);
                break;
            case FN_SQUARE:
                wave = 2 * ((int)(2 * t) % 2) - 1;
                break;
            case FN_TRIANGLE:
                wave = 1 - 2 * fabs(2 * (t - floor(t)) - 1);
                break;
            case FN_NOISE:
                wave = 2 * (rand() % 2) - 1;
                break;
            case FN_SAWTOOTH:
                wave = t - floor(t);
                break;
            }

            sample += wave * vel;
        }

        for (int n = 0; n < op->send_n; n++) {
            s->channels_back[op->send[n]] += op->send_level[n] * sample;
        }
    }

    fm_synth_swap_buffers(s);
}

void fm_synth_fill_hold_buffer(fm_synth *s, double start_time, double seconds_per_frame) {
    for (int frame = 0; frame < HOLD_BUFFER_SIZE; frame++) {
        double time = start_time + seconds_per_frame * frame;

        fm_synth_frame(s, time, seconds_per_frame);

        s->instr->hold_buf_back[frame] += s->channels[0];
    }
}

