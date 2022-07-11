#ifndef H_FM_COMMON
#define H_FM_COMMON

#define PI 3.1415926535
#define SQRT2 1.41421356

#define CLAMP(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))

struct {
    int sample_rate;
    double dt;
} fm_config;

#endif
