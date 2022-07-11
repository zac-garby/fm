#ifndef H_FM_FONT
#define H_FM_FONT

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <math.h>

#include "font-data.h"

typedef struct fm_font {
    SDL_Surface * surfaces[256];
} fm_font;

fm_font fm_make_font(int r, int g, int b, int a);
void fm_font_write(SDL_Surface *surf, fm_font *font, int x, int y, char *text);
int fm_font_measure(fm_font *font, char *text);

#endif
