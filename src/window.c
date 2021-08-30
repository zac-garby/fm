#include "window.h"

static SDL_Texture* render_text_scale(fm_window *win, SDL_Rect area);
static SDL_Rect panel_safe_area(fm_gui_panel *panel);
static void panel_render_background(fm_window *win, fm_gui_panel *panel);
static void setup_panels(fm_window *win);

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
    win.num_panels = 3;
    win.panels = malloc(sizeof(fm_gui_panel) * win.num_panels);
    setup_panels(&win);

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
   
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_SetRenderDrawColor(win->renderer, WINDOW_BACKGROUND_COLOUR);
        SDL_RenderClear(win->renderer);

        for (int i = 0; i < win->num_panels; i++) {
            fm_gui_panel *panel = &win->panels[i];

            if (panel->render != NULL) {
                panel_render_background(win, panel);
                panel->render(win, panel);
            }
        }

        SDL_RenderPresent(win->renderer);
    }

    SDL_DestroyWindow(win->window);
    SDL_Quit();

    TTF_CloseFont(font);
    TTF_Quit();
}

static SDL_Rect panel_safe_area(fm_gui_panel *panel) {
    SDL_Rect r;
    int d = PANEL_BORDER_WIDTH + PANEL_BORDER_INSET + PANEL_BORDER_PADDING;
    r.x = panel->rect.x + d;
    r.y = panel->rect.y + d;
    r.w = panel->rect.w - 2*d;
    r.h = panel->rect.h - 2*d;
    return r;
}

static void panel_render_background(fm_window *win, fm_gui_panel *panel) {
    SDL_SetRenderDrawColor(win->renderer, GUI_BACKGROUND_COLOUR);
    SDL_RenderFillRect(win->renderer, &panel->rect);

    SDL_SetRenderDrawColor(win->renderer, GUI_BORDER_COLOUR);
    
    SDL_Rect r;
    r.x = panel->rect.x + PANEL_BORDER_INSET;
    r.y = panel->rect.y + PANEL_BORDER_INSET;
    r.w = PANEL_BORDER_WIDTH;
    r.h = panel->rect.h - PANEL_BORDER_INSET * 2;
    SDL_RenderFillRect(win->renderer, &r);
    r.x = panel->rect.x + panel->rect.w - PANEL_BORDER_INSET - PANEL_BORDER_WIDTH;
    SDL_RenderFillRect(win->renderer, &r);

    r.x = panel->rect.x + PANEL_BORDER_INSET;
    r.w = panel->rect.w - PANEL_BORDER_INSET * 2;
    r.h = PANEL_BORDER_WIDTH;
    SDL_RenderFillRect(win->renderer, &r);
    r.y = panel->rect.y + panel->rect.h - PANEL_BORDER_INSET - PANEL_BORDER_WIDTH;
    SDL_RenderFillRect(win->renderer, &r);
}

void render_spectrum_panel(fm_window *win, fm_gui_panel *panel) {
    SDL_Rect area = panel_safe_area(panel);

    static kiss_fft_cpx *freq = NULL;
    if (freq == NULL) freq = malloc(sizeof(kiss_fft_cpx) * FREQ_DOMAIN);

    static SDL_Texture *scale = NULL;
    if (scale == NULL) scale = render_text_scale(win, area);
    
    kiss_fftr_cfg fft_cfg = kiss_fftr_alloc(HOLD_BUFFER_SIZE, 0, NULL, NULL);

    int fft_resolution = area.w / FFT_BAR_WIDTH;
    
    SDL_Rect rect;
    rect.w = FFT_BAR_WIDTH;

    SDL_Rect scaleRect;
    scaleRect.x = area.x;
    scaleRect.y = area.y + area.h - 20;
    scaleRect.w = area.w;
    scaleRect.h = 20;

    kiss_fftr(fft_cfg, win->player->synths[0].hold_buf[0], freq);

    SDL_SetRenderDrawColor(win->renderer, SPECTRUM_BAR_COLOUR);

    for (int x = 0; x < fft_resolution; x++) {
        SDL_RenderCopy(win->renderer, scale, NULL, &scaleRect);
        float p = (float) x / (float) fft_resolution;
        int i = (int) ((float) FREQ_DOMAIN * p);
        int h = (int) (hypotf(freq[i].r, freq[i].i) * SPECTRUM_VERTICAL_SCALE);
        rect.x = x * rect.w + area.x;
        rect.y = area.h - 20 - h + area.y;
        rect.h = h;
        SDL_RenderFillRect(win->renderer, &rect);
    }
}

static SDL_Texture* render_text_scale(fm_window *win, SDL_Rect area) {
    SDL_Color fg = { 0x00, 0x00, 0x00, 0xff };
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, area.w, 20, 32, SDL_PIXELFORMAT_RGBA32);
    char *label = malloc(8);
    if (surf == NULL) {
        fprintf(stderr, "could not create surface. %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Rect tick;
    tick.w = 1;
    tick.y = 0;

    int fft_resolution = area.w / FFT_BAR_WIDTH;

    SDL_FillRect(surf, NULL, 0xffffffff);

    int i = 0;
    float lastTick = -FFT_SCALE_STEP;
    for (int x = 0; x < fft_resolution; x += 1) {
        float p = (float) x / (float) fft_resolution;
        float f = p * FFT_SCALE_MAX;

        if (f >= lastTick + FFT_SCALE_STEP) {
            lastTick = f;
            
            tick.x = x * FFT_BAR_WIDTH;
            tick.h = i % FFT_SCALE_SKIP == 0 ? 15 : 5;
            SDL_FillRect(surf, &tick, 0x000000ff);
            
            if (i % FFT_SCALE_SKIP == 0) {
                if (f >= 1000) {
                    sprintf(label, "%.2fk", (float) f / 1000.0f);
                } else {
                    sprintf(label, "%dHz", (int) f);
                }
                
                SDL_Surface *text = TTF_RenderText_Blended(font, label, fg);
                SDL_Rect dest;
                dest.x = x * FFT_BAR_WIDTH + 2;
                dest.y = 5;
                dest.w = text->w;
                dest.h = text->h;
                SDL_BlitSurface(text, NULL, surf, &dest);
                SDL_FreeSurface(text);
            }
            
            i++;
        }
    }

    free(label);

    SDL_Texture *tex = SDL_CreateTextureFromSurface(win->renderer, surf);
    SDL_FreeSurface(surf);
    return tex;
}

void render_right_panel(fm_window *win, fm_gui_panel *panel) {
    //SDL_Rect area = panel_safe_area(panel);
}

void render_control_panel(fm_window *win, fm_gui_panel *panel) {
    //SDL_Rect area = panel_safe_area(panel);
}

static void setup_panels(fm_window *win) {
    SDL_Rect r1 = {0, 0, WINDOW_LEFT_WIDTH, SPECTRUM_PANEL_HEIGHT};
    SDL_Rect r2 = {WINDOW_LEFT_WIDTH, 0, SCREEN_WIDTH - WINDOW_LEFT_WIDTH, SCREEN_HEIGHT};
    SDL_Rect r3 = {0, SPECTRUM_PANEL_HEIGHT, WINDOW_LEFT_WIDTH,
        SCREEN_HEIGHT - SPECTRUM_PANEL_HEIGHT};
    
    win->panels[0].rect = r1;
    win->panels[0].render = render_spectrum_panel;
    
    win->panels[1].rect = r2;
    win->panels[1].render = render_right_panel;

    win->panels[2].rect = r3;
    win->panels[2].render = render_control_panel;
}
