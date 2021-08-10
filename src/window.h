#ifndef H_FM_WINDOW
#define H_FM_WINDOW

#include <SDL2/SDL.h>
#include <stdbool.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 300

typedef struct fm_window {
    SDL_Window *window;
    SDL_Surface *surface;
} fm_window;

fm_window fm_create_window();

// needs to be called in the main thread
void fm_window_loop(fm_window *win);

#endif
