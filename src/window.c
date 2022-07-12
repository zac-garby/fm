#include "window.h"

fm_window fm_create_window(fm_player *player) {
    fm_window win;

    win.window = SDL_CreateWindow("Synthesizer",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  REAL_WIDTH, REAL_HEIGHT,
                                  SDL_WINDOW_ALLOW_HIGHDPI);
    if (win.window == NULL) {
        fprintf(stderr, "could not create a window: %s\n", SDL_GetError());
        exit(1);
    }

    win.surf = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT,
                                    32, 0, 0, 0, 0);
    
    win.win_surf = SDL_GetWindowSurface(win.window);
    win.player = player;

    win.mouse_x = 0;
    win.mouse_y = 0;

    win.font = fm_make_font(FG_COLOUR);

    setup_panels(&win);
    
    return win;
}

void fm_window_loop(fm_window *win) {
    bool quit = false;
   
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;

            case SDL_MOUSEMOTION:
                win->mouse_x = e.motion.x / SCREEN_SCALE;
                win->mouse_y = e.motion.y / SCREEN_SCALE;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEWHEEL:
                send_mouse_event(win, &win->root, win->mouse_x, win->mouse_y, e);
                break;

            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_SPACE) {
                    win->player->paused = !win->player->paused;
                }
            }
        }

        SDL_FillRect(win->surf, NULL, SDL_MapRGBA(win->surf->format, BG_COLOUR));
        
        fm_gui_panel* root = &win->root;
        root->render(win, root);
        
        SDL_BlitScaled(win->surf, NULL, win->win_surf, NULL);
        SDL_UpdateWindowSurface(win->window);
    }

    SDL_DestroyWindow(win->window);
    SDL_Quit();
}

void send_mouse_event(fm_window *win, fm_gui_panel *panel,
                      int x, int y, SDL_Event e) {
    if (panel->handler != NULL) {
        panel->handler(win, panel, e);
    }
    
    for (int i = 0; i < panel->num_children; i++) {
        fm_gui_panel *child = &panel->children[i];
        
        if (x >= child->rect.x && y >= child->rect.y &&
            x < child->rect.x + child->rect.w &&
            y < child->rect.y + child->rect.h) {
            
            send_mouse_event(win, child, x, y, e);
        }
    }
}

void render_children(fm_window *win, fm_gui_panel *panel) {
    for (int i = 0; i < panel->num_children; i++) {
        fm_gui_panel *child = &panel->children[i];
        child->render(win, child);
    }
}

void render_spectrum(fm_window *win, fm_gui_panel *panel) {
    fm_spectrum_data *data;
    Uint32 fg;
    SDL_Rect safe;

    data = (fm_spectrum_data*) panel->data;
    safe = get_safe_area(panel);

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

    render_box(win, panel);

    if (data->synth_index < win->player->num_instrs) {
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
            
            SDL_FillRect(win->surf, &axis, panel->border);
            
            for (int i = 0; i < SPECTRO_W; i++) {
                float p = (float) i / (float) SPECTRO_W;
                int j = (int) (p * HOLD_BUFFER_SIZE);
                
                float sample = instr->hold_buf[j] * data->wave_scale;
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
                data->bins[i] *= SPECTRUM_FALLOFF;
            }
            
            SDL_Rect bar;
            bar.w = 1;
            
            if (!win->player->paused) {
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
            }
            
            for (int x = 0; x < SPECTRO_W; x++) {
                int h = (int) (data->bins[x] * data->spectrum_scale) + 1;
                if (h > safe.h) h = safe.h;
                
                bar.x = x + safe.x;
                bar.y = safe.h - h + safe.y;
                bar.h = h;
                SDL_FillRect(win->surf, &bar, fg);
            }
        }
    }

    fm_font_write(win->surf, &win->font, panel->rect.x + 2, panel->rect.y + 2, data->title);
}

void spectrum_handle_event(fm_window *win, fm_gui_panel *panel, SDL_Event e) {
    UNUSED(win);
    
    fm_spectrum_data *data = (fm_spectrum_data*) panel->data;

    switch (e.type) {
    case SDL_MOUSEBUTTONUP:
        data->show_wave = !data->show_wave;
        break;
    case SDL_MOUSEWHEEL:
        if (data->show_wave) {
            data->wave_scale += e.wheel.preciseY * WAVE_SCALE_DELTA;
            if (data->wave_scale < WAVE_MIN_SCALE)
                data->wave_scale = WAVE_MIN_SCALE;
            if (data->wave_scale > WAVE_MAX_SCALE)
                data->wave_scale = WAVE_MAX_SCALE;
        } else {
            data->spectrum_scale += e.wheel.preciseY * SPECTRUM_SCALE_DELTA;
            if (data->spectrum_scale < SPECTRUM_MIN_SCALE)
                data->spectrum_scale = SPECTRUM_MIN_SCALE;
            if (data->spectrum_scale > SPECTRUM_MAX_SCALE)
                data->spectrum_scale = SPECTRUM_MAX_SCALE;
        }
        break;
    }
}

void sequencer_render(fm_window *win, fm_gui_panel *panel) {
    render_box(win, panel);

    fm_sequencer_data *data = (fm_sequencer_data*) panel->data;
    SDL_Rect safe = get_safe_area(panel);
    
    if (data->needs_redraw) {
        data->needs_redraw = false;
        
        if (data->canvas == NULL) {
            data->canvas =
                SDL_CreateRGBSurface(0, (SEQ_CELL_W + 1) * data->song_length,
                                     SEQ_CELL_H * SEQ_NUM_OCTAVES * 12,
                                     32, 0, 0, 0, 0);

        }

        Uint32 cell_div = SDL_MapRGBA(data->canvas->format, SEQ_CELL_DIVIDER_COLOUR);
        
        Uint32 cell_bg[24] = {
            // normal colours
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_OCTAVE),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_1),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_1),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_1),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_1),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_1),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_1),

            // colours for the first in a bar
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_1),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_1),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_1),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_1),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_1),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_2),
            SDL_MapRGBA(data->canvas->format, SEQ_CELL_BG_COLOUR_FIRST_1),
        };
        
        int bg_index = 0;

        SDL_Rect cell;
        cell.w = SEQ_CELL_W;
        cell.h = SEQ_CELL_H;

        SDL_FillRect(data->canvas, NULL, cell_div);

        for (int row = 0; row < SEQ_NUM_OCTAVES * 12; row++) {
            // (needs to be flipped vertically...)
            cell.y = data->canvas->h - (row * SEQ_CELL_H + SEQ_CELL_H);
            
            for (int col = 0; col < data->song_length; col++) {
                cell.x = col * (SEQ_CELL_W + 1);

                if (col % data->song.beats_per_bar == 0) {
                    SDL_FillRect(data->canvas, &cell, cell_bg[bg_index + 12]);
                } else {
                    SDL_FillRect(data->canvas, &cell, cell_bg[bg_index]);
                }
            }
            
            bg_index = (bg_index + 1) % 12;
        }
    }

    SDL_Rect clip = get_safe_area(panel);
    clip.x = (int) data->scroll_x;
    clip.y = data->canvas->h - safe.h - (int) data->scroll_y;

    SDL_BlitSurface(data->canvas, &clip, win->surf, &safe);

    if (point_in_rect(win->mouse_x, win->mouse_y, &safe)) {
        int x = win->mouse_x - safe.x;
        int y = win->mouse_y - safe.y;
        
        float cell_x_frac = ((float) x + clip.x) / (SEQ_CELL_W + 1);
        int cell_x = (int) cell_x_frac;
        int bar = cell_x / data->song.beats_per_bar;
        int beat = cell_x % data->song.beats_per_bar;
        int cell_subdiv = (int) (10 * (cell_x_frac - (float) cell_x));
        int cell_y = SEQ_NUM_OCTAVES * 12 - 1 - (y + clip.y) / SEQ_CELL_H;
        int octave = cell_y / 12;
        int note = cell_y % 12;

        if (cell_subdiv < 8) {
            // inside a beat (>= 8 would be between beats)
            char text[16];
            sprintf(text, "%s%d", NOTE_NAMES[note], octave);
            fm_draw_tooltip(win, win->mouse_x + 6, win->mouse_y, text);
            sprintf(text, "%d:%d.%d", bar + 1, beat + 1, cell_subdiv + 1);
            fm_draw_tooltip(win, win->mouse_x + 6, win->mouse_y - 7, text);
        }
    }
}

void sequencer_handler(fm_window *win, fm_gui_panel *panel, SDL_Event e) {
    UNUSED(win);
    
    fm_sequencer_data *data = (fm_sequencer_data*) panel->data;
    SDL_Rect safe = get_safe_area(panel);

    switch (e.type) {
    case SDL_MOUSEWHEEL:
        data->scroll_x = CLAMP(data->scroll_x + e.wheel.preciseX,
                               0, data->canvas->w - safe.w);
        
        data->scroll_y = CLAMP(data->scroll_y + e.wheel.preciseY,
                               0, data->canvas->h - safe.h);
        
        break;
    }
}

void render_box(fm_window *win, fm_gui_panel *panel) {
    draw_rect(win->surf, &panel->rect, panel->bg, panel->border, panel->corner);
    render_children(win, panel);
}

fm_gui_panel fm_make_panel(int x, int y, int w, int h,
                           int num_children,
                           fm_window *win,
                           fm_panel_renderer *render,
                           fm_panel_event_handler *handler) {
    fm_gui_panel p;

    p.rect = make_rect(x, y, w, h);
    p.render = render;
    p.handler = handler;

    p.children = malloc(sizeof(fm_gui_panel) * num_children);
    p.num_children = num_children;

    p.bg = SDL_MapRGBA(win->surf->format, PANEL_COLOUR);
    p.border = SDL_MapRGBA(win->surf->format, BORDER_COLOUR);
    p.corner = SDL_MapRGBA(win->surf->format, BORDER_CORNER_COLOUR);

    return p;
}

void setup_panels(fm_window *win) {
    win->root = fm_make_panel(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                              6, win, render_box, NULL);
    win->root.border = SDL_MapRGBA(win->surf->format, WIN_BORDER_COLOUR);
    win->root.corner = win->root.border;
    win->root.bg = SDL_MapRGBA(win->surf->format, BG_COLOUR);
    win->root.parent = NULL;

    // spectrum/waveform windows
    for (int i = 0; i < 4; i++) {
        win->root.children[i] =
            fm_make_panel(2, 2 + i * (SPECTRO_H + 3),
                          SPECTRO_W + 2, SPECTRO_H + 2,
                          0, win, render_spectrum, spectrum_handle_event);

        fm_spectrum_data *data = malloc(sizeof(fm_spectrum_data));
        data->synth_index = i;
        data->show_wave = false;
        data->spectrum_scale = SPECTRUM_VERT_SCALE;
        data->wave_scale = WAVE_VERT_SCALE;
        data->title = malloc(sizeof(char) * 8);
        sprintf(data->title, "INSTR-%d", i);
        for (int i = 0; i < SPECTRO_W; i++) data->bins[i] = 0;
        
        win->root.children[i].data = data;
        win->root.children[i].parent = &win->root;
        win->root.children[i].bg = SDL_MapRGBA(win->surf->format, BG_COLOUR);
    }

    // sequencer / player panel
    win->root.children[4] =
        fm_make_panel(2, 2 + 4 * (SPECTRO_H + 3),
                      SCREEN_WIDTH - 4,
                      SCREEN_HEIGHT - 2 - (2 + 4 * (SPECTRO_H + 3)),
                      2, win, render_children, NULL);
    win->root.children[4].parent = &win->root;

    win->root.children[4].children[0] =
        fm_make_panel(2, 2 + 4 * (SPECTRO_H + 3),
                      SCREEN_WIDTH - 4,
                      11,
                      0, win, render_box, NULL);
    win->root.children[4].children[0].parent = &win->root.children[4];

    win->root.children[4].children[1] =
        fm_make_panel(2, 14 + 4 * (SPECTRO_H + 3),
                      SCREEN_WIDTH - 4,
                      SCREEN_HEIGHT - (14 + 4 * (SPECTRO_H + 3)) - 2,
                      0, win, sequencer_render, sequencer_handler);
    fm_sequencer_data *seq_data = malloc(sizeof(fm_sequencer_data));
    seq_data->part_index = 0;
    seq_data->song = fm_new_song(4, 120);
    seq_data->scroll_x = 0;
    seq_data->scroll_y = 12 * 3 * SEQ_CELL_H;
    seq_data->needs_redraw = true;
    seq_data->song_length = 64;
    seq_data->canvas = NULL;

    win->root.children[4].children[1].parent = &win->root.children[4];
    win->root.children[4].children[1].data = seq_data;

    // instrument controls panel
    win->root.children[5] =
        fm_make_panel(SPECTRO_W + 5, 2,
                      SCREEN_WIDTH - 7 - SPECTRO_W,
                      4 * (SPECTRO_H + 3) - 1,
                      0, win, render_box, NULL);

    win->root.children[5].parent = &win->root;
}

void draw_rect(SDL_Surface *s, SDL_Rect *r, Uint32 bg, Uint32 border, Uint32 corner) {
    if (border != 0) {
        SDL_FillRect(s, r, border);
    }

    if (bg != 0) {
        SDL_Rect inner;
        if (border == 0) {
            inner = *r;
        } else {
            inner.x = r->x + 1;
            inner.y = r->y + 1;
            inner.w = r->w - 2;
            inner.h = r->h - 2;
        }

        SDL_FillRect(s, &inner, bg);
    }

    if (corner != 0) {
        set_pixel(s, r->x, r->y, corner);
        set_pixel(s, r->x + r->w - 1, r->y, corner);
        set_pixel(s, r->x, r->y + r->h - 1, corner);
        set_pixel(s, r->x + r->w - 1, r->y + r->h - 1, corner);
    }
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

bool point_in_rect(int x, int y, SDL_Rect *r) {
    return x >= r->x && y >= r->y && x < r->x + r->w && y < r->y + r->h;
}

void set_pixel(SDL_Surface *surface, int x, int y, Uint32 colour) {
    Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                              + y * surface->pitch
                                              + x * surface->format->BytesPerPixel);
    *target_pixel = colour;
}

void fm_draw_tooltip(fm_window *win, int x, int y, char *text) {
    int text_w = fm_font_measure(&win->font, text);
    
    while (x + text_w >= SCREEN_WIDTH - 4) x--;
    
    SDL_Rect popup = make_rect(x, y - 7, text_w + 2, 7);
    
    draw_rect(win->surf, &popup,
              SDL_MapRGBA(win->surf->format, PANEL_COLOUR),
              0, 0);
    
    fm_font_write(win->surf, &win->font, x + 1, y - 6, text);
}
