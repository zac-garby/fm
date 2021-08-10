#include "window.h"

fm_window fm_create_window() {
    fm_window win;

    win.window = SDL_CreateWindow("FM Synthesizer",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  SCREEN_WIDTH, SCREEN_HEIGHT,
                                  SDL_WINDOW_SHOWN);
    if (win.window == NULL) {
        fprintf(stderr, "could not create a window: %s\n", SDL_GetError());
        exit(1);
    }

    win.surface = SDL_GetWindowSurface(win.window);
    
    return win;
}

void fm_window_loop(fm_window *win) {
    SDL_FillRect(win->surface, NULL, SDL_MapRGB(win->surface->format, 0x00, 0x00, 0x00));

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_UpdateWindowSurface(win->window);
    }
    
    SDL_DestroyWindow(win->window);
    SDL_Quit();
}

