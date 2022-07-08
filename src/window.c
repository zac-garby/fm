#include "window.h"

void send_mouse_event(fm_window *win, int x, int y, SDL_Event e);
void setup_panels(fm_window *win);
void draw_rect(SDL_Surface *s, SDL_Rect *r, Uint32 bg, Uint32 border);
void render_spectrum(fm_window *win, fm_gui_panel *panel);
SDL_Rect make_rect(int x, int y, int w, int h);
SDL_Rect get_safe_area(fm_gui_panel*);
void set_pixel(SDL_Surface *s, int x, int y, Uint32 colour);

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
            int x, y;
            
            switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                x = e.button.x;
                y = e.button.y;
            case SDL_MOUSEMOTION:
                x = e.motion.x;
                y = e.motion.y;

                send_mouse_event(win, x, y, e);
                
                break;
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

void send_mouse_event(fm_window *win, int screen_x, int screen_y, SDL_Event e) {
    int x = screen_x / SCREEN_SCALE;
    int y = screen_y / SCREEN_SCALE;
    
    for (int i = 0; i < win->num_panels; i++) {
        fm_gui_panel *panel = &win->panels[i];
        
        if (x >= panel->rect.x && y >= panel->rect.y &&
            x < panel->rect.x + panel->rect.w &&
            y < panel->rect.y + panel->rect.h) {
            
            panel->handler(win, panel, e);
        }
    }
}

void render_spectrum(fm_window *win, fm_gui_panel *panel) {
    fm_spectrum_data *data;
    Uint32 bg, border, fg;
    SDL_Rect safe;

    data = (fm_spectrum_data*) panel->data;
    safe = get_safe_area(panel);

    bg = SDL_MapRGBA(win->surf->format, BG_COLOUR);
    border = SDL_MapRGBA(win->surf->format, BORDER_COLOUR);

    switch (data->synth_index) {
    case 0:
        fg = SDL_MapRGBA(win->surf->format, INSTR0_COLOUR);
        break;
    case 1:
        fg = SDL_MapRGBA(win->surf->format, INSTR1_COLOUR);
        break;
    case 2:
        fg = SDL_MapRGBA(win->surf->format, INSTR2_COLOUR);
        break;
    case 3:
        fg = SDL_MapRGBA(win->surf->format, INSTR3_COLOUR);
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

    if (data->show_wave) {
        SDL_Rect axis;
        axis.w = SPECTRO_W;
        axis.h = 1;
        axis.x = safe.x;
        axis.y = safe.y + safe.h / 2;

        SDL_FillRect(win->surf, &axis, border);

        for (int i = 0; i < SPECTRO_W; i++) {
            float p = (float) i / (float) SPECTRO_W;
            int j = (int) (p * HOLD_BUFFER_SIZE);

            float sample = instr->hold_buf[j] * WAVE_VERT_SCALE;
            int sy = sample < 0 ? (int) sample : 0;
            int ey = sample < 0 ? 0 : (int) sample;
            if (sy < -safe.h / 2) sy = -safe.h / 2;
            if (ey > safe.h / 2) ey = safe.h / 2;
            
            for (int y = sy; y <= ey; y++) {
                set_pixel(win->surf, axis.x + i, axis.y + y, fg);
            }
        }
    } else {
        for (int i = 0; i < SPECTRO_W; i++) {
            data->bins[i] *= 0.5f;
        }
        
        SDL_Rect bar;
        bar.w = 1;
        
        static float freqs[FREQ_DOMAIN];
        for (int i = 0; i < FREQ_DOMAIN; i++) {
            freqs[i] = hypot(instr->spectrum[i].r, instr->spectrum[i].i);
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
}

void spectrum_handle_event(fm_window *win, fm_gui_panel *panel, SDL_Event e) {
    UNUSED(win);
    
    fm_spectrum_data *data = (fm_spectrum_data*) panel->data;
    
    if (e.type == SDL_MOUSEBUTTONUP) {
        data->show_wave = !data->show_wave;
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
        win->panels[i].handler = spectrum_handle_event;

        fm_spectrum_data *data = malloc(sizeof(fm_spectrum_data));
        data->synth_index = i;
        data->show_wave = false;
        
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

void set_pixel(SDL_Surface *surface, int x, int y, Uint32 colour) {
    Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                              + y * surface->pitch
                                              + x * surface->format->BytesPerPixel);
    *target_pixel = colour;
}

/*
  plotLine(x0, y0, x1, y1)
    dx = x1 - x0
    dy = y1 - y0
    D = 2*dy - dx
    y = y0

    for x from x0 to x1
        plot(x,y)
        if D > 0
            y = y + 1
            D = D - 2*dx
        end if
        D = D + 2*dy
void draw_line(SDL_Surface *s, int x0, int y0, int x1, int y1, Uint32 colour) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int D = 2 * dy - dx;
    int y = y0;

    for (int x = x0; x <= x1; x++) {
        set_pixel(s, x, y, colour);

        if (D > 0) {
            y++;
            D -= 2 * dx;
        }

        D += 2 * dy;
    }
}
*/
