#include "window.h"

static SDL_Surface* render_text_scale(fm_window *win);

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

    // TODO: maybe move font stuff to some other file?
    font = TTF_OpenFont("assets/NotoSansJP-Regular.otf", 10);
    if (font == NULL) {
        fprintf(stderr, "could not load font: %s\n", TTF_GetError());
        exit(1);
    }
    
    return win;
}

void fm_window_loop(fm_window *win) {
    bool quit = false;
    SDL_Event e;
    kiss_fft_cpx *freq = malloc(sizeof(kiss_fft_cpx) * FREQ_DOMAIN);
    SDL_Rect rect;
    int fft_timer = 0;
    float fft_peak = 0;
    
    SDL_Rect scaleRect;
    scaleRect.x = 0;
    scaleRect.y = SCREEN_HEIGHT - 20;
    scaleRect.w = SCREEN_WIDTH;
    scaleRect.h = 20;
        
    SDL_Surface *scale = render_text_scale(win);

    rect.w = 1;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_FillRect(win->surface, NULL, SDL_MapRGB(win->surface->format, 0x00, 0x00, 0x00));

        if (fft_timer == 0 && !win->player->synths[0].hold_buf_dirty) {
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
            rect.y = SCREEN_HEIGHT - 20 - (int) h;
            rect.h = (int) h;
            SDL_FillRect(win->surface, &rect, SDL_MapRGB(win->surface->format, 0xff, 0xff, 0xff));
        }

        SDL_BlitSurface(scale, NULL, win->surface, &scaleRect);

        SDL_UpdateWindowSurface(win->window);
    }

    kiss_fftr_free(win->fft_cfg);
    SDL_FreeSurface(scale);
    SDL_DestroyWindow(win->window);
    SDL_Quit();

    TTF_CloseFont(font);
    TTF_Quit();
}

static SDL_Surface* render_text_scale(fm_window *win) {
    SDL_Color fg = { 0x00, 0x00, 0x00, 0xff };
    SDL_Color bg = { 0xff, 0xff, 0xff, 0xff };
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, 20, 32, SDL_PIXELFORMAT_RGBA32);
    char *label = malloc(8);
    if (surf == NULL) {
        fprintf(stderr, "could not create surface. %s\n", SDL_GetError());
        exit(1);
    }
    
    SDL_Rect tick;
    tick.w = 1;
    tick.y = 0;

    SDL_FillRect(surf, NULL, SDL_MapRGB(win->surface->format, bg.r, bg.g, bg.b));

    int i = 0;
    for (int f = 0; f <= FFT_SCALE_MAX; f += FFT_SCALE_STEP) {
        int x = (int) (((float) f / (float) FFT_SCALE_MAX) * (float) FREQ_DOMAIN);
        tick.x = x;
        tick.h = i % FFT_SCALE_SKIP == 0 ? 15 : 5;
        SDL_FillRect(surf, &tick, SDL_MapRGB(win->surface->format, fg.r, fg.g, fg.b));

        if (i % FFT_SCALE_SKIP == 0) {
            if (f >= 1000) {
                sprintf(label, "%.1fk", (float) f / 1000.0f);
            } else {
                sprintf(label, "%dHz", f);
            }
            
            SDL_Surface *text = TTF_RenderText_Blended(font, label, fg);
            SDL_Rect dest;
            dest.x = x + 2;
            dest.y = 5;
            dest.w = text->w;
            dest.h = text->h;
            SDL_BlitSurface(text, NULL, surf, &dest);
            SDL_FreeSurface(text);
        }

        i++;
    }

    free(label);

    return surf;
}

