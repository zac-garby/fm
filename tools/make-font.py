#!/usr/bin/env python3

# this script generates the font data file "font.h" which is used
# in the GUI for my synth. it requires Pillow to run.

# and yes
# i realise this code is horrible

import sys
from PIL import Image

CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,()[]{}<>/\\|!-_+=;:?'\"%#~"
SPACE_WIDTH = 2

TEMPLATE = """// font-data.h
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
%s};

#endif
"""

if len(sys.argv) < 2:
    print("usage: make-font.py <font.png>")
    sys.exit(1)
    
im = Image.open(sys.argv[1])
x1 = 0

all_data = [ None for i in range(256) ]

all_data[ord(' ')] = [ [0] * SPACE_WIDTH ] * 5

for c in CHARSET:
    x2 = x1
    empty = False

    while not empty:
        x2 += 1
        empty = True
        
        for y in range(5):
            p = im.getpixel((x2, y))
            if p == 1:
                empty = False
                break

    data = [[] for i in range(5)]
            
    for x in range(x1, x2):
        for y in range(5):
            data[y].append(im.getpixel((x, y)))

    all_data[ord(c)] = data
   
            
    x1 = x2 + 1

s = ""

for i in range(256):
    c = chr(i)
    if c in "abcdefghijklmnopqrstuvwxyz":
        c = c.upper()

    d = all_data[ord(c)]
    
    if d != None:
        s += "    { %d, %d, { %s } },\n" % (
            i,
            len(d[0]),
            "".join([ str(e) + ", " for ds in d for e in ds])[:-2]
        )
    else:
        s += "    { %d, -1, { } },\n" % i
    
print(TEMPLATE % s)