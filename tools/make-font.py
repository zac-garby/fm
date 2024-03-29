#!/usr/bin/env python3

# this script generates the font data file "font.rs" which is used
# in the GUI for my synth. it requires Pillow to run.

# and yes
# i realise this code is horrible

import sys
from PIL import Image

CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,()[]{}<>/\\|!-_+=;:?'\"%#~*\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16\x17"
SPACE_WIDTH = 2

TEMPLATE = """// font.rs
// this file is generated by tools/make-font.py.
// do not manually edit this file!

/// the height of a character in the font.
pub const FONT_HEIGHT: usize = 5;

/// the maximum width of a character in the font.
pub const FONT_WIDTH: usize = 12;

/// stores the raw pixel data for one character in a font.
pub struct CharData {
    pub width: u32,
    pub data: [u32; FONT_HEIGHT * FONT_WIDTH],
}

pub const FONT_DATA: [Option<CharData>; 256] = [
%s
];
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
            if p != 0:
                empty = False
                break

    data = [[] for i in range(5)]
            
    for x in range(x1, x2):
        for y in range(5):
            data[y].append(1 if im.getpixel((x, y)) == 1 else 0)

    all_data[ord(c)] = data
   
            
    x1 = x2 + 1

s = ""

for i in range(256):
    c = chr(i)
    if c in "abcdefghijklmnopqrstuvwxyz":
        c = c.upper()

    d = all_data[ord(c)]
    
    if d != None:
        l = [ e for ds in d for e in ds ]
        l += [0] * (5 * 12 - len(l))
        
        s += "    Some(CharData { width: %d, data: [%s] }),\n" % (
            len(d[0]),
            "".join([ str(e) + ", " for e in l])[:-2]
        )
    else:
        s += "    None,\n"
    
print(TEMPLATE % s)
