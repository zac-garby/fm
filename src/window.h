#ifndef H_FM_WINDOW
#define H_FM_WINDOW

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <kissfft/kiss_fftr.h>
#include <math.h>

#include "player.h"
#include "synth.h"

#define SCREEN_WIDTH 1026
#define SCREEN_HEIGHT 300

#define FRAMES_PER_FFT 5

typedef struct fm_window {
    SDL_Window *window;
    SDL_Surface *surface;

    fm_player *player;
    kiss_fftr_cfg fft_cfg;
} fm_window;

fm_window fm_create_window(fm_player *player);

// needs to be called in the main thread
void fm_window_loop(fm_window *win);

#endif
