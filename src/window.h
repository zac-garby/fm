#ifndef H_FM_WINDOW
#define H_FM_WINDOW

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <kissfft/kiss_fftr.h>
#include <math.h>

#include "player.h"
#include "synth.h"

#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 250
#define SCREEN_SCALE 4
#define REAL_WIDTH (SCREEN_WIDTH * SCREEN_SCALE)
#define REAL_HEIGHT (SCREEN_HEIGHT * SCREEN_SCALE)

#define SPECTRUM_VERT_SCALE 0.15f
#define WAVE_VERT_SCALE 10

#define BG_COLOUR 21, 20, 18, 255
#define BORDER_COLOUR 48, 41, 50, 255
#define PANEL_COLOUR 29, 24, 30, 255

#define SPECTRO_W 128
#define SPECTRO_H 32

#define INSTR0_COLOUR 252, 131, 131, 255
#define INSTR1_COLOUR 145, 224, 145, 255
#define INSTR2_COLOUR 126, 144, 238, 255
#define INSTR3_COLOUR 255, 251, 181, 255

struct fm_window;
struct fm_gui_panel;

typedef struct fm_spectrum_data {
    int synth_index;
    float bins[SPECTRO_W];
    bool show_wave;
} fm_spectrum_data;

typedef void fm_panel_renderer(struct fm_window*, struct fm_gui_panel*);
typedef void fm_panel_event_handler(struct fm_window*, struct fm_gui_panel*, SDL_Event e);

typedef struct fm_gui_panel {
    SDL_Rect rect;
    fm_panel_renderer *render;
    fm_panel_event_handler *handler;
    void *data;
} fm_gui_panel;

typedef struct fm_window {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surf, *win_surf;

    fm_player *player;

    fm_gui_panel *panels;
    int num_panels;
} fm_window;

fm_window fm_create_window(fm_player *player);

// needs to be called in the main thread
void fm_window_loop(fm_window *win);

#endif
