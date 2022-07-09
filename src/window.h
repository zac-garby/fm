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
#define SPECTRUM_MIN_SCALE 0.05f
#define SPECTRUM_MAX_SCALE 2.0f
#define SPECTRUM_SCALE_DELTA 0.05f

#define WAVE_VERT_SCALE 60
#define WAVE_MIN_SCALE 1
#define WAVE_MAX_SCALE 100
#define WAVE_SCALE_DELTA 3

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
    float spectrum_scale, wave_scale;
} fm_spectrum_data;

typedef struct fm_sequencer_data {
    int synth_index;
    fm_song song;
} fm_sequencer_data;

typedef void fm_panel_renderer(struct fm_window*, struct fm_gui_panel*);
typedef void fm_panel_event_handler(struct fm_window*, struct fm_gui_panel*, SDL_Event e);

typedef struct fm_gui_panel {
    SDL_Rect rect;
    fm_panel_renderer *render;
    fm_panel_event_handler *handler;
    void *data;

    struct fm_gui_panel *children;
    int num_children;
} fm_gui_panel;

typedef struct fm_window {
    SDL_Window *window;
    SDL_Surface *surf, *win_surf;
    int mouse_x, mouse_y;

    fm_player *player;

    fm_gui_panel root;
} fm_window;

fm_window fm_create_window(fm_player *player);

fm_gui_panel fm_make_panel(int x, int y, int w, int h,
                           int num_children,
                           fm_panel_renderer *render,
                           fm_panel_event_handler *handler);

// needs to be called in the main thread
void fm_window_loop(fm_window *win);

#endif
