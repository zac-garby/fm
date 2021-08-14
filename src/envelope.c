#include "envelope.h"

fm_envelope fm_make_envelope(float attack, float decay, float sustain, float release) {
    fm_envelope e;
    e.attack = attack;
    e.decay = decay;
    e.sustain = sustain;
    e.release = release;
    return e;
}

float fm_envelope_evaluate(fm_envelope *env, float t, float hold_time) {
    if (hold_time < env->attack + env->decay && t > hold_time && t < hold_time + env->release) {
        float rf = (1 - (t - hold_time) / env->release);
        if (hold_time < env->attack) return (hold_time / env->attack) * rf;
        else return (1 - ((1 - env->sustain) * (hold_time - env->attack)) / env->decay) * rf;
    } else {
        if (t < env->attack) return t / env->attack;
        else if (t < env->attack + env->decay) return 1 - ((1 - env->sustain) * (t - env->attack)) / env->decay;
        else if (t < hold_time) return env->sustain;
        else if (t < hold_time + env->release) return env->sustain * (1 - (t - hold_time) / env->release);
    }

    return 0;
}
