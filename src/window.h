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
#define SCREEN_HEIGHT 200
#define SCREEN_SCALE 4
#define REAL_WIDTH (SCREEN_WIDTH * SCREEN_SCALE)
#define REAL_HEIGHT (SCREEN_HEIGHT * SCREEN_SCALE)

#define SPECTRUM_VERT_SCALE 0.15f

#define BG_COLOUR 21, 20, 18, 255
#define BORDER_COLOUR 48, 41, 50, 255
#define PANEL_COLOUR 29, 24, 30, 255

#define SPECTRO_W 128
#define SPECTRO_H 32

#define SPECTRO0_BORDER 252, 27, 27, 255
#define SPECTRO0_BG 255, 213, 213, 255
#define SPECTRO0_FG 107, 0, 0, 255

#define SPECTRO1_BORDER 62, 237, 121, 255
#define SPECTRO1_BG 242, 255, 242, 255
#define SPECTRO1_FG 0, 77, 2, 255

#define SPECTRO2_BORDER 58, 108, 249, 255
#define SPECTRO2_BG 229, 233, 255, 255
#define SPECTRO2_FG 0, 2, 92, 255

#define SPECTRO3_BORDER 208, 172, 3, 255
#define SPECTRO3_BG 255, 253, 209, 255
#define SPECTRO3_FG 92, 87, 8, 255

struct fm_window;
struct fm_gui_panel;

typedef void fm_panel_renderer(struct fm_window*, struct fm_gui_panel*);

typedef struct fm_gui_panel {
    SDL_Rect rect;
    fm_panel_renderer *render;
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
