#include "window.h"

static SDL_Texture* render_text_scale(fm_window *win);

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

    win.renderer = SDL_CreateRenderer(win.window, -1, SDL_RENDERER_ACCELERATED);
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
        
    SDL_Texture *scale = render_text_scale(win);
    SDL_Point *waveform = malloc(sizeof(SDL_Point) * WAVEFORM_RESOLUTION);

    rect.w = SCREEN_WIDTH / FFT_RESOLUTION;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_SetRenderDrawColor(win->renderer, 0x00, 0x00, 0x00, 0xff);
        SDL_RenderClear(win->renderer);

        // recompute the FFT every n frames, if the hold buffer is not being computed.
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

        SDL_SetRenderDrawColor(win->renderer, 0xff, 0xff, 0xff, 0xff);
        
        // render the FFT frequency domain
        for (int x = 0; x < FFT_RESOLUTION; x++) {
            float p = (float) x / (float) FFT_RESOLUTION;
            int i = (int) ((float) FREQ_DOMAIN * p);
            double h = hypotf(freq[i].r, freq[i].i) * (280.0f / fft_peak);
            rect.x = x * rect.w;
            rect.y = SCREEN_HEIGHT - 20 - (int) h;
            rect.h = (int) h;
            SDL_RenderFillRect(win->renderer, &rect);
        }

        // render the frequency domain x-axis scale
        SDL_RenderCopy(win->renderer, scale, NULL, &scaleRect);

        // render the output (channel 0) waveform, once the hold buffer is written.
        while (!win->player->synths[0].hold_buf_dirty);
        
        float wave_peak = 0;
        for (int x = 0; x < HOLD_BUFFER_SIZE; x++) {
            float sample = fabs(win->player->synths[0].hold_buf[0][x]);
            if (sample > wave_peak) {
                wave_peak = sample;
            }
        }
        
        for (int x = 0; x < WAVEFORM_RESOLUTION; x++) {
            float p = (float) x / (float) WAVEFORM_RESOLUTION;
            int i = (int) ((float) WAVEFORM_SEGMENT * p);
            waveform[x].x = (int) ((float) SCREEN_WIDTH * p);
            waveform[x].y = (int) (130.0f * win->player->synths[0].hold_buf[0][i] / wave_peak) + 150;
        }

        SDL_RenderDrawLine(win->renderer, 0, 150, SCREEN_WIDTH, 150);

        SDL_RenderDrawLines(win->renderer, waveform, WAVEFORM_RESOLUTION);

        SDL_RenderPresent(win->renderer);
    }

    kiss_fftr_free(win->fft_cfg);
    free(waveform);
    SDL_DestroyWindow(win->window);
    SDL_Quit();

    TTF_CloseFont(font);
    TTF_Quit();
}

static SDL_Texture* render_text_scale(fm_window *win) {
    SDL_Color fg = { 0x00, 0x00, 0x00, 0xff };
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, 20, 32, SDL_PIXELFORMAT_RGBA32);
    char *label = malloc(8);
    if (surf == NULL) {
        fprintf(stderr, "could not create surface. %s\n", SDL_GetError());
        exit(1);
    }

    int ppb = SCREEN_WIDTH / FFT_RESOLUTION;
    
    SDL_Rect tick;
    tick.w = 1;
    tick.y = 0;

    SDL_FillRect(surf, NULL, 0xffffffff);

    int i = 0;
    for (int f = 0; f <= FFT_SCALE_MAX; f += FFT_SCALE_STEP * ppb) {
        int x = (int) (((float) f / (float) FFT_SCALE_MAX) * (float) FREQ_DOMAIN) * ppb;
        tick.x = x;
        tick.h = i % FFT_SCALE_SKIP == 0 ? 15 : 5;
        SDL_FillRect(surf, &tick, 0x000000ff);

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

    SDL_Texture *tex = SDL_CreateTextureFromSurface(win->renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

