#ifndef H_FM_WINDOW
#define H_FM_WINDOW

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <kissfft/kiss_fftr.h>
#include <math.h>

#include "player.h"
#include "synth.h"
#include "font.h"

#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 250
#define SCREEN_SCALE 4
#define REAL_WIDTH (SCREEN_WIDTH * SCREEN_SCALE)
#define REAL_HEIGHT (SCREEN_HEIGHT * SCREEN_SCALE)

#define SPECTRUM_VERT_SCALE 0.08f
#define SPECTRUM_MIN_SCALE 0.01f
#define SPECTRUM_MAX_SCALE 2.0f
#define SPECTRUM_SCALE_DELTA 0.01f
#define SPECTRUM_FALLOFF 0.9

#define WAVE_VERT_SCALE 60
#define WAVE_MIN_SCALE 1
#define WAVE_MAX_SCALE 100
#define WAVE_SCALE_DELTA 3

#define FG_COLOUR 210, 210, 210, 255
#define BG_COLOUR 21, 20, 18, 255
#define BORDER_COLOUR 48, 41, 50, 255
#define BORDER_CORNER_COLOUR 34, 29, 36, 255
#define WIN_BORDER_COLOUR 78, 71, 80, 255
#define PANEL_COLOUR 29, 24, 30, 255

#define SPECTRO_W 128
#define SPECTRO_H 30

#define INSTR0_COLOUR 252, 131, 131, 255
#define INSTR1_COLOUR 145, 224, 145, 255
#define INSTR2_COLOUR 126, 144, 238, 255
#define INSTR3_COLOUR 255, 251, 181, 255

#define SEQ_CELL_W 8
#define SEQ_CELL_H 5
#define SEQ_NUM_OCTAVES 9

#define SEQ_CELL_BG_COLOUR_1 85, 76, 87, 255
#define SEQ_CELL_BG_COLOUR_2 76, 68, 80, 255
#define SEQ_CELL_BG_COLOUR_FIRST_1 75, 63, 74, 255
#define SEQ_CELL_BG_COLOUR_FIRST_2 70, 59, 72, 255
#define SEQ_CELL_BG_COLOUR_OCTAVE 84, 65, 87, 255
#define SEQ_CELL_DIVIDER_COLOUR 65, 57, 66, 255
#define SEQ_NOTE_COLOUR 203, 185, 191, 255

struct fm_window;
struct fm_gui_panel;

typedef struct fm_spectrum_data {
    int synth_index;
    float bins[SPECTRO_W];
    bool show_wave;
    float spectrum_scale, wave_scale;
    char *title;
} fm_spectrum_data;

typedef struct fm_sequencer_data {
    int part_index;

    float scroll_x, scroll_y;
    int song_length;
    SDL_Surface *canvas;
    bool needs_redraw;
} fm_sequencer_data;

typedef void fm_panel_renderer(struct fm_window*, struct fm_gui_panel*);
typedef void fm_panel_event_handler(struct fm_window*, struct fm_gui_panel*, SDL_Event e);

typedef struct fm_gui_panel {
    SDL_Rect rect;
    fm_panel_renderer *render;
    fm_panel_event_handler *handler;
    void *data;

    struct fm_gui_panel *parent;
    struct fm_gui_panel *children;
    int num_children;

    Uint32 bg, border, corner;
} fm_gui_panel;

typedef struct fm_window {
    SDL_Window *window;
    SDL_Surface *surf, *win_surf;
    fm_font font;
    int mouse_x, mouse_y;

    fm_player *player;

    fm_gui_panel root;
} fm_window;

fm_window fm_create_window(fm_player *player);

fm_gui_panel fm_make_panel(int x, int y, int w, int h,
                           int num_children,
                           fm_window *win,
                           fm_panel_renderer *render,
                           fm_panel_event_handler *handler);

// needs to be called in the main thread
void fm_window_loop(fm_window *win);

void fm_draw_tooltip(fm_window *win, int x, int y, char *text);
void send_mouse_event(fm_window*, fm_gui_panel*, int x, int y, SDL_Event e);
void setup_panels(fm_window *win);
void draw_rect(SDL_Surface *s, SDL_Rect *r, Uint32 bg, Uint32 border, Uint32 corner);
void render_spectrum(fm_window *win, fm_gui_panel *panel);
void render_children(fm_window *win, fm_gui_panel *panel);
void render_box(fm_window *win, fm_gui_panel *panel);
SDL_Rect make_rect(int x, int y, int w, int h);
bool point_in_rect(int x, int y, SDL_Rect *r);
SDL_Rect get_safe_area(fm_gui_panel*);
void set_pixel(SDL_Surface *s, int x, int y, Uint32 colour);

#endif
