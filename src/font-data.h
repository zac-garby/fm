// font-data.h
// this file is generated by tools/make-font.py.
// do not manually edit this file! it will be overwritten when the Makefile is run.

#ifndef H_FM_FONT_DATA
#define H_FM_FONT_DATA

// a struct containing the raw data for a character in a font.
// each character is 5 pixels high and up to 5 pixels wide.
typedef struct fm_font_char {
    // the character represented by this data.
    char ch;

    // the width, in pixels, of this character. or, -1, if the character
    // is not defined
    int width;

    // the raw pixel data, 0 for transparent, 1 for black. for a character
    // of width W, only 5 * W slots of this array are used. the rows of the
    // character are written one after the other, from top to bottom.
    int data[25];
} fm_font_char;

static const fm_font_char FONT_DATA[256] = {
    { 0, -1, { } },
    { 1, -1, { } },
    { 2, -1, { } },
    { 3, -1, { } },
    { 4, -1, { } },
    { 5, -1, { } },
    { 6, -1, { } },
    { 7, -1, { } },
    { 8, -1, { } },
    { 9, -1, { } },
    { 10, -1, { } },
    { 11, -1, { } },
    { 12, -1, { } },
    { 13, -1, { } },
    { 14, -1, { } },
    { 15, -1, { } },
    { 16, -1, { } },
    { 17, -1, { } },
    { 18, -1, { } },
    { 19, -1, { } },
    { 20, -1, { } },
    { 21, -1, { } },
    { 22, -1, { } },
    { 23, -1, { } },
    { 24, -1, { } },
    { 25, -1, { } },
    { 26, -1, { } },
    { 27, -1, { } },
    { 28, -1, { } },
    { 29, -1, { } },
    { 30, -1, { } },
    { 31, -1, { } },
    { 32, 2, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { 33, 2, { 1, 1, 1, 1, 1, 0, 0, 0, 1, 0 } },
    { 34, 4, { 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { 35, 5, { 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0 } },
    { 36, -1, { } },
    { 37, 5, { 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1 } },
    { 38, -1, { } },
    { 39, 1, { 1, 1, 0, 0, 0 } },
    { 40, 2, { 0, 1, 1, 0, 1, 0, 1, 0, 0, 1 } },
    { 41, 2, { 1, 0, 0, 1, 0, 1, 0, 1, 1, 0 } },
    { 42, -1, { } },
    { 43, 3, { 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0 } },
    { 44, 1, { 0, 0, 0, 1, 1 } },
    { 45, 2, { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 } },
    { 46, 1, { 0, 0, 0, 0, 1 } },
    { 47, 3, { 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { 48, 3, { 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0 } },
    { 49, 3, { 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1 } },
    { 50, 3, { 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1 } },
    { 51, 3, { 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1 } },
    { 52, 3, { 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1 } },
    { 53, 3, { 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0 } },
    { 54, 3, { 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1 } },
    { 55, 3, { 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { 56, 3, { 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0 } },
    { 57, 3, { 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1 } },
    { 58, 1, { 0, 1, 0, 1, 0 } },
    { 59, 1, { 0, 1, 0, 1, 1 } },
    { 60, 3, { 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1 } },
    { 61, 3, { 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0 } },
    { 62, 3, { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0 } },
    { 63, 3, { 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0 } },
    { 64, -1, { } },
    { 65, 3, { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1 } },
    { 66, 3, { 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1 } },
    { 67, 3, { 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1 } },
    { 68, 3, { 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1 } },
    { 69, 3, { 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1 } },
    { 70, 3, { 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { 71, 3, { 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1 } },
    { 72, 3, { 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1 } },
    { 73, 1, { 1, 1, 1, 1, 1 } },
    { 74, 3, { 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0 } },
    { 75, 3, { 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1 } },
    { 76, 3, { 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1 } },
    { 77, 5, { 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1 } },
    { 78, 4, { 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1 } },
    { 79, 3, { 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0 } },
    { 80, 3, { 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0 } },
    { 81, 4, { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1 } },
    { 82, 3, { 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1 } },
    { 83, 3, { 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0 } },
    { 84, 3, { 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { 85, 3, { 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1 } },
    { 86, 4, { 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0 } },
    { 87, 5, { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1 } },
    { 88, 3, { 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1 } },
    { 89, 3, { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0 } },
    { 90, 4, { 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 } },
    { 91, 2, { 1, 1, 1, 0, 1, 0, 1, 0, 1, 1 } },
    { 92, 3, { 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1 } },
    { 93, 2, { 1, 1, 0, 1, 0, 1, 0, 1, 1, 1 } },
    { 94, -1, { } },
    { 95, 3, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 } },
    { 96, -1, { } },
    { 97, 3, { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1 } },
    { 98, 3, { 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1 } },
    { 99, 3, { 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1 } },
    { 100, 3, { 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1 } },
    { 101, 3, { 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1 } },
    { 102, 3, { 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { 103, 3, { 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1 } },
    { 104, 3, { 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1 } },
    { 105, 1, { 1, 1, 1, 1, 1 } },
    { 106, 3, { 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0 } },
    { 107, 3, { 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1 } },
    { 108, 3, { 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1 } },
    { 109, 5, { 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1 } },
    { 110, 4, { 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1 } },
    { 111, 3, { 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0 } },
    { 112, 3, { 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0 } },
    { 113, 4, { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1 } },
    { 114, 3, { 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1 } },
    { 115, 3, { 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0 } },
    { 116, 3, { 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { 117, 3, { 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1 } },
    { 118, 4, { 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0 } },
    { 119, 5, { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1 } },
    { 120, 3, { 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1 } },
    { 121, 3, { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0 } },
    { 122, 4, { 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0 } },
    { 123, 3, { 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1 } },
    { 124, 1, { 1, 1, 1, 1, 1 } },
    { 125, 3, { 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0 } },
    { 126, 4, { 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { 127, -1, { } },
    { 128, -1, { } },
    { 129, -1, { } },
    { 130, -1, { } },
    { 131, -1, { } },
    { 132, -1, { } },
    { 133, -1, { } },
    { 134, -1, { } },
    { 135, -1, { } },
    { 136, -1, { } },
    { 137, -1, { } },
    { 138, -1, { } },
    { 139, -1, { } },
    { 140, -1, { } },
    { 141, -1, { } },
    { 142, -1, { } },
    { 143, -1, { } },
    { 144, -1, { } },
    { 145, -1, { } },
    { 146, -1, { } },
    { 147, -1, { } },
    { 148, -1, { } },
    { 149, -1, { } },
    { 150, -1, { } },
    { 151, -1, { } },
    { 152, -1, { } },
    { 153, -1, { } },
    { 154, -1, { } },
    { 155, -1, { } },
    { 156, -1, { } },
    { 157, -1, { } },
    { 158, -1, { } },
    { 159, -1, { } },
    { 160, -1, { } },
    { 161, -1, { } },
    { 162, -1, { } },
    { 163, -1, { } },
    { 164, -1, { } },
    { 165, -1, { } },
    { 166, -1, { } },
    { 167, -1, { } },
    { 168, -1, { } },
    { 169, -1, { } },
    { 170, -1, { } },
    { 171, -1, { } },
    { 172, -1, { } },
    { 173, -1, { } },
    { 174, -1, { } },
    { 175, -1, { } },
    { 176, -1, { } },
    { 177, -1, { } },
    { 178, -1, { } },
    { 179, -1, { } },
    { 180, -1, { } },
    { 181, -1, { } },
    { 182, -1, { } },
    { 183, -1, { } },
    { 184, -1, { } },
    { 185, -1, { } },
    { 186, -1, { } },
    { 187, -1, { } },
    { 188, -1, { } },
    { 189, -1, { } },
    { 190, -1, { } },
    { 191, -1, { } },
    { 192, -1, { } },
    { 193, -1, { } },
    { 194, -1, { } },
    { 195, -1, { } },
    { 196, -1, { } },
    { 197, -1, { } },
    { 198, -1, { } },
    { 199, -1, { } },
    { 200, -1, { } },
    { 201, -1, { } },
    { 202, -1, { } },
    { 203, -1, { } },
    { 204, -1, { } },
    { 205, -1, { } },
    { 206, -1, { } },
    { 207, -1, { } },
    { 208, -1, { } },
    { 209, -1, { } },
    { 210, -1, { } },
    { 211, -1, { } },
    { 212, -1, { } },
    { 213, -1, { } },
    { 214, -1, { } },
    { 215, -1, { } },
    { 216, -1, { } },
    { 217, -1, { } },
    { 218, -1, { } },
    { 219, -1, { } },
    { 220, -1, { } },
    { 221, -1, { } },
    { 222, -1, { } },
    { 223, -1, { } },
    { 224, -1, { } },
    { 225, -1, { } },
    { 226, -1, { } },
    { 227, -1, { } },
    { 228, -1, { } },
    { 229, -1, { } },
    { 230, -1, { } },
    { 231, -1, { } },
    { 232, -1, { } },
    { 233, -1, { } },
    { 234, -1, { } },
    { 235, -1, { } },
    { 236, -1, { } },
    { 237, -1, { } },
    { 238, -1, { } },
    { 239, -1, { } },
    { 240, -1, { } },
    { 241, -1, { } },
    { 242, -1, { } },
    { 243, -1, { } },
    { 244, -1, { } },
    { 245, -1, { } },
    { 246, -1, { } },
    { 247, -1, { } },
    { 248, -1, { } },
    { 249, -1, { } },
    { 250, -1, { } },
    { 251, -1, { } },
    { 252, -1, { } },
    { 253, -1, { } },
    { 254, -1, { } },
    { 255, -1, { } },
};

#endif

