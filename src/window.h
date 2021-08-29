#ifndef H_FM_WINDOW
#define H_FM_WINDOW

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <kissfft/kiss_fftr.h>
#include <math.h>

#include "player.h"
#include "synth.h"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

#define PANEL_BORDER_WIDTH 3
#define PANEL_BORDER_INSET 4
#define PANEL_BORDER_PADDING 2

#define SPECTRUM_VERTICAL_SCALE 0.5

#define WINDOW_BACKGROUND_COLOUR 0, 0, 0, 255
#define GUI_BACKGROUND_COLOUR 0, 0, 0, 255
#define GUI_BORDER_COLOUR 200, 200, 200, 255
#define SPECTRUM_BAR_COLOUR 255, 255, 255, 255

#define WINDOW_LEFT_WIDTH 650
#define SPECTRUM_PANEL_HEIGHT 300

#define FRAMES_PER_FFT 1

#define FFT_BAR_WIDTH 2
#define FFT_SCALE_STEP 100
#define FFT_SCALE_SKIP 10
#define FFT_SCALE_MAX (48000 / 2)

#define WAVEFORM_RESOLUTION SCREEN_WIDTH
#define WAVEFORM_SEGMENT (HOLD_BUFFER_SIZE)

static TTF_Font *font;

struct fm_window;
struct fm_gui_panel;

typedef void fm_panel_renderer(struct fm_window*, struct fm_gui_panel*);

typedef struct fm_gui_panel {
    SDL_Rect rect;
    fm_panel_renderer *render;
} fm_gui_panel;

typedef struct fm_window {
    SDL_Window *window;
    SDL_Renderer *renderer;

    fm_player *player;
    kiss_fftr_cfg fft_cfg;

    fm_gui_panel *panels;
    int num_panels;
} fm_window;

fm_window fm_create_window(fm_player *player);

// needs to be called in the main thread
void fm_window_loop(fm_window *win);

#endif
