#include "window.h"

fm_window fm_create_window(fm_player *player) {
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
    win.player = player;
    win.fft_cfg = kiss_fftr_alloc(HOLD_BUFFER_SIZE, 0, NULL, NULL);
    
    return win;
}

void fm_window_loop(fm_window *win) {
    bool quit = false;
    SDL_Event e;
    kiss_fft_cpx *freq = malloc(sizeof(kiss_fft_cpx) * FREQ_DOMAIN);
    SDL_Rect rect;
    int fft_timer = 0;
    float fft_peak = 0;

    rect.w = 1;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_FillRect(win->surface, NULL, SDL_MapRGB(win->surface->format, 0x00, 0x00, 0x00));

        if (fft_timer == 0) {
            kiss_fftr(win->fft_cfg, win->player->synths[0].hold_buf[0], freq);
            fft_peak = 0;

            for (int i = 0; i < FREQ_DOMAIN; i++) {
                double m = hypot(freq[i].r, freq[i].i);
                if (m > fft_peak) {
                    fft_peak = m;
                }
            }
        }

        fft_timer = (fft_timer + 1) % FRAMES_PER_FFT;

        for (int i = 0; i < FREQ_DOMAIN; i++) {
            double h = hypotf(freq[i].r, freq[i].i) * (280.0f / fft_peak);
            rect.x = i;
            rect.y = 300 - (int) h;
            rect.h = (int) h;
            SDL_FillRect(win->surface, &rect, SDL_MapRGB(win->surface->format, 0xff, 0xff, 0xff));
        }

        SDL_UpdateWindowSurface(win->window);
    }

    kiss_fftr_free(win->fft_cfg);
    SDL_DestroyWindow(win->window);
    SDL_Quit();
}

