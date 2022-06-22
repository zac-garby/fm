#include "synth.h"

float sine_wave(float);
float square_wave(float);
float triangle_wave(float);
float noise_wave(float);
float sawtooth_wave(float);

void fm_new_instr(fm_instrument *instr, int n_ops) {
    instr->n_ops = n_ops;
    instr->ops = malloc(sizeof(fm_operator) * n_ops);

    instr->voices = malloc(sizeof(fm_synth) * MAX_POLYPHONY);

    for (int i = 0; i < MAX_POLYPHONY; i++) {
        instr->voices[i] = fm_new_synth(instr);
    }
}

float fm_instr_get_next_output(fm_instrument *instr,
                              double start_time,
                              double seconds_per_frame) {
    float sample = 0;

    for (int i = 0; i < MAX_POLYPHONY; i++) {
        sample += fm_synth_get_next_output(&instr->voices[i],
                                           start_time,
                                           seconds_per_frame);
    }

    return sample;
}

fm_synth fm_new_synth(fm_instrument *instr) {
    fm_synth s;

    s.channels = calloc(N_CHANNELS, sizeof(float));
    s.channels_back = calloc(N_CHANNELS, sizeof(float));
    s.hold_index = HOLD_BUFFER_SIZE;
    s.hold_buf_dirty = false;
    
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

		float (*wave)(float);
		switch (op->wave_type) {
		case FN_SIN:
			wave = sine_wave;
			break;
		case FN_SQUARE:
			wave = square_wave;
			break;
		case FN_TRIANGLE:
			wave = triangle_wave;
			break;
		case FN_NOISE:
			wave = noise_wave;
			break;
        case FN_SAWTOOTH:
            wave = sawtooth_wave;
            break;
		}

        float sample = 0;

        if (s->note.freq > 0) {
            float env = fm_envelope_evaluate(&op->envelope,
                                             time - s->note.start,
                                             s->note.duration);
            float vel = env * s->note.velocity;
            float f = op->fixed ? op->transpose : s->note.freq * op->transpose;
            float t = f * time + s->phases[i];

            sample += wave(t) * vel;
        }

        for (int n = 0; n < op->send_n; n++) {
            s->channels_back[op->send[n]] += op->send_level[n] * sample;
        }
    }

    fm_synth_swap_buffers(s);
}

void fm_synth_fill_hold_buffer(fm_synth *s, double start_time, double seconds_per_frame) {
    s->hold_buf_dirty = true;
    
    for (int frame = 0; frame < HOLD_BUFFER_SIZE; frame++) {
        double time = start_time + seconds_per_frame * frame;

        fm_synth_frame(s, time, seconds_per_frame);

        s->hold_buf[frame] = s->channels[0];
    }

    s->hold_index = 0;
    s->hold_buf_dirty = false;
}

float fm_synth_get_next_output(fm_synth *s, double start_time, double seconds_per_frame) {
    if (s->hold_index >= HOLD_BUFFER_SIZE) {
        fm_synth_fill_hold_buffer(s, start_time, seconds_per_frame);
    }

    float out = s->hold_buf[s->hold_index];
    s->hold_index++;

    return out;
}

float sine_wave(float t) {
	return -cos(2.0f * PI * t);
}

float square_wave(float t) {
	return 2 * ((int)(2 * t) % 2) - 1;
}

float triangle_wave(float t) {
	return 1 - 2 * fabs(2 * (t - floor(t)) - 1);
}

float noise_wave(float t) {
	return 2 * (rand() % 2) - 1;
}

float sawtooth_wave(float t) {
    return t - floor(t);
}
