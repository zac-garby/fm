#ifndef H_FM_WINDOW
#define H_FM_WINDOW

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <kissfft/kiss_fftr.h>
#include <math.h>

#include "player.h"
#include "synth.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600

#define FRAMES_PER_FFT 1

#define FFT_RESOLUTION 512
#define FFT_SCALE_STEP 100
#define FFT_SCALE_SKIP 5
#define FFT_SCALE_MAX (48000 / 2)

#define WAVEFORM_RESOLUTION SCREEN_WIDTH
#define WAVEFORM_SEGMENT (HOLD_BUFFER_SIZE)

static TTF_Font *font;

typedef struct fm_window {
    SDL_Window *window;
    SDL_Renderer *renderer;

    fm_player *player;
    kiss_fftr_cfg fft_cfg;
} fm_window;

fm_window fm_create_window(fm_player *player);

// needs to be called in the main thread
void fm_window_loop(fm_window *win);

#endif
