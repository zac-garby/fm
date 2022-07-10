#include "font.h"

fm_font fm_make_font(int r, int g, int b, int a) {
    fm_font f;

    Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    for (int ch = 0; ch < 256; ch++) {
        fm_font_char data = FONT_DATA[ch];

        if (data.width == -1) {
            f.surfaces[ch] = NULL;
            continue;
        }

        f.surfaces[ch] = SDL_CreateRGBSurface(0, data.width, 5, 32, rmask, gmask, bmask, amask);

        for (int x = 0; x < data.width; x++) {
            for (int y = 0; y < 5; y++) {
                Uint32 colour = SDL_MapRGBA(f.surfaces[ch]->format, r, g, b, a);
                
                Uint32 *target_pixel = (Uint32 *) ((Uint8 *) f.surfaces[ch]->pixels
                                                   + y * f.surfaces[ch]->pitch
                                                   + x * f.surfaces[ch]->format->BytesPerPixel);

                *target_pixel = data.data[y * data.width + x] ? colour : 0;
            }
        }
    }

    return f;
}

void fm_font_write(SDL_Surface *surf, fm_font *font, int x, int y, char *text) {
    SDL_Rect r;
    char c;

    r.x = x;
    r.y = y;

    while ((c = *(text++)) != '\0') {
        SDL_BlitSurface(font->surfaces[(int) c], NULL, surf, &r);
        r.x += font->surfaces[(int) c]->w + 1;
    }
}
