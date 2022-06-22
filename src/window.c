#include "window.h"

void setup_panels(fm_window *win);
void draw_rect(SDL_Surface *s, SDL_Rect *r, Uint32 bg, Uint32 border);
void render_spectrum(fm_window *win, fm_gui_panel *panel);
SDL_Rect make_rect(int x, int y, int w, int h);
SDL_Rect get_safe_area(fm_gui_panel*);

typedef struct fm_spectrum_data {
    int synth_index;
    kiss_fft_cpx *freq;
    float *bins;
} fm_spectrum_data;

fm_window fm_create_window(fm_player *player) {
    fm_window win;

    win.window = SDL_CreateWindow("FM Synthesizer",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  REAL_WIDTH, REAL_HEIGHT,
                                  SDL_WINDOW_SHOWN);
    if (win.window == NULL) {
        fprintf(stderr, "could not create a window: %s\n", SDL_GetError());
        exit(1);
    }

    win.surf = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                    32, 0, 0, 0, 0);
    
    win.win_surf = SDL_GetWindowSurface(win.window);
    win.player = player;

    setup_panels(&win);
    
    return win;
}

void fm_window_loop(fm_window *win) {
    bool quit = false;
   
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_FillRect(win->surf, NULL, SDL_MapRGBA(win->surf->format, BG_COLOUR));

        for (int i = 0; i < win->num_panels; i++) {
            fm_gui_panel* panel = &win->panels[i];
            panel->render(win, panel);
        }
        
        SDL_BlitScaled(win->surf, NULL, win->win_surf, NULL);
        SDL_UpdateWindowSurface(win->window);
    }

    SDL_DestroyWindow(win->window);
    SDL_Quit();
}

void render_spectrum(fm_window *win, fm_gui_panel *panel) {
    static float hold_buf[HOLD_BUFFER_SIZE];
    
    fm_spectrum_data *data;
    Uint32 bg, border, fg;
    SDL_Rect safe;

    data = (fm_spectrum_data*) panel->data;
    safe = get_safe_area(panel);

    switch (data->synth_index) {
    case 0:
        bg = SDL_MapRGBA(win->surf->format, SPECTRO0_BG);
        border = SDL_MapRGBA(win->surf->format, SPECTRO0_BORDER);
        fg = SDL_MapRGBA(win->surf->format, SPECTRO0_FG);
        break;
    case 1:
        bg = SDL_MapRGBA(win->surf->format, SPECTRO1_BG);
        border = SDL_MapRGBA(win->surf->format, SPECTRO1_BORDER);
        fg = SDL_MapRGBA(win->surf->format, SPECTRO1_FG);
        break;
    case 2:
        bg = SDL_MapRGBA(win->surf->format, SPECTRO2_BG);
        border = SDL_MapRGBA(win->surf->format, SPECTRO2_BORDER);
        fg = SDL_MapRGBA(win->surf->format, SPECTRO2_FG);
        break;
    case 3:
        bg = SDL_MapRGBA(win->surf->format, SPECTRO3_BG);
        border = SDL_MapRGBA(win->surf->format, SPECTRO3_BORDER);
        fg = SDL_MapRGBA(win->surf->format, SPECTRO3_FG);
        break;
    default:
        return;
    }

    draw_rect(win->surf, &panel->rect, bg, border);

    if (data->synth_index >= win->player->num_instrs) {
        return;
    }

    if (SPECTRO_W != safe.w) {
        printf("the width is wrong\n");
        return;
    }

    fm_instrument *instr = &win->player->instrs[data->synth_index];

    for (int n = 0; n < HOLD_BUFFER_SIZE; n++) {
        hold_buf[n] = 0;
        
        for (int i = 0; i < MAX_POLYPHONY; i++) {
            hold_buf[n] += instr->voices[i].hold_buf[n];
        }
    }

    kiss_fftr_cfg fft_cfg = kiss_fftr_alloc(HOLD_BUFFER_SIZE, 0, NULL, NULL);
    kiss_fftr(fft_cfg, hold_buf, data->freq);
    kiss_fftr_free(fft_cfg);

    for (int i = 0; i < SPECTRO_W; i++) {
        data->bins[i] *= 0.5f;
    }
    
    SDL_Rect bar;
    bar.w = 1;

    static float freqs[FREQ_DOMAIN];
    for (int i = 0; i < FREQ_DOMAIN; i++) {
        freqs[i] = hypot(data->freq[i].r, data->freq[i].i);
    }

    int f = 1, fn = 0;
    for (int i = 0; i < SPECTRO_W; i++) {
        float p = (float) (i + 1) / (float) SPECTRO_W;
        fn = (int) (powf((float) FREQ_DOMAIN, p));

        float sum = 0.0f;
        int n = 0;
        for (int j = f; j < fn + 1; j++) {
            sum += freqs[j];
            n++;
        }
        data->bins[i] += sum / n;

        f = fn;
    }

    for (int x = 0; x < SPECTRO_W; x++) {
        int h = (int) (data->bins[x] * SPECTRUM_VERT_SCALE) + 1;
        if (h > safe.h) h = safe.h;
        
        bar.x = x + safe.x;
        bar.y = safe.h - h + safe.y;
        bar.h = h;
        SDL_FillRect(win->surf, &bar, fg);
    }
}

void setup_panels(fm_window *win) {
    win->num_panels = 4;
    win->panels = malloc(sizeof(fm_gui_panel) * win->num_panels);

    for (int i = 0; i < 4; i++) {
        win->panels[i].rect = make_rect(2, 2 + i * (SPECTRO_H + 3),
                                        SPECTRO_W + 2,
                                        SPECTRO_H + 2);
        
        win->panels[i].render = render_spectrum;

        fm_spectrum_data *data = malloc(sizeof(fm_spectrum_data));
        data->synth_index = i;
        data->freq = malloc(sizeof(kiss_fft_cpx) * FREQ_DOMAIN);
        data->bins = malloc(sizeof(float) * SPECTRO_W);
        
        win->panels[i].data = data;
    }
}

void draw_rect(SDL_Surface *s, SDL_Rect *r, Uint32 bg, Uint32 border) {
    SDL_FillRect(s, r, border);

    SDL_Rect inner;
    inner.x = r->x + 1;
    inner.y = r->y + 1;
    inner.w = r->w - 2;
    inner.h = r->h - 2;

    SDL_FillRect(s, &inner, bg);
}

SDL_Rect get_safe_area(fm_gui_panel *panel) {
    return make_rect(panel->rect.x + 1,
                     panel->rect.y + 1,
                     panel->rect.w - 2,
                     panel->rect.h - 2);
}

SDL_Rect make_rect(int x, int y, int w, int h) {
    SDL_Rect r;

    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    
    return r;
}
