use sdl2::pixels::Color;

pub const FG: Color = Color { r: 210, g: 210, b: 210, a: 255 };
pub const FG2: Color = Color { r: 139, g: 139, b: 139, a: 139 };
pub const PANEL_BG: Color = Color { r: 29, g: 24, b: 30, a: 255 };
pub const WIN_BG: Color = Color { r: 21, g: 20, b: 18, a: 255 };
pub const EMPTY_SPECTRUM_BG: Color = Color { r: 22, g: 18, b: 23, a: 255 };
pub const BORDER: Color = Color { r: 48, g: 41, b: 50, a: 255 };
pub const CORNER: Color = Color { r: 34, g: 29, b: 36, a: 255 };

pub const SEQ_NOTE: Color = Color { r: 203, g: 185, b: 191, a: 255 };
pub const SEQ_GHOST_NOTE: Color = Color { r: 121, g: 89, b: 128, a: 255 };
pub const SEQ_PLAYHEAD: Color = Color { r: 240, g: 44, b: 44, a: 255 };
pub const SEQ_DIVIDER: Color = Color { r: 65, g: 57, b: 66, a: 255 };

pub const TRANSPARENT: Color = Color { r: 0, g: 0, b: 0, a: 0 };

/// the background colours for sequencer cells. the first 12
/// colours are for the first beat of a bar, the next 12 are for
/// anything else.
pub const SEQ_BACKGROUND: [Color; 24] = [
    Color { r: 70, g: 59, b: 72, a: 255 }, // octave
    Color { r: 75, g: 63, b: 74, a: 255 },
    Color { r: 70, g: 59, b: 72, a: 255 },
    Color { r: 75, g: 63, b: 74, a: 255 },
    Color { r: 70, g: 59, b: 72, a: 255 },
    Color { r: 75, g: 63, b: 74, a: 255 },
    Color { r: 70, g: 59, b: 72, a: 255 },
    Color { r: 75, g: 63, b: 74, a: 255 },
    Color { r: 70, g: 59, b: 72, a: 255 },
    Color { r: 75, g: 63, b: 74, a: 255 },
    Color { r: 70, g: 59, b: 72, a: 255 },
    Color { r: 75, g: 63, b: 74, a: 255 },
    
    Color { r: 84, g: 65, b: 87, a: 255 }, // octave
    Color { r: 85, g: 76, b: 87, a: 255 },
    Color { r: 78, g: 70, b: 83, a: 255 },
    Color { r: 85, g: 76, b: 87, a: 255 },
    Color { r: 78, g: 70, b: 83, a: 255 },
    Color { r: 85, g: 76, b: 87, a: 255 },
    Color { r: 78, g: 70, b: 83, a: 255 },
    Color { r: 85, g: 76, b: 87, a: 255 },
    Color { r: 78, g: 70, b: 83, a: 255 },
    Color { r: 85, g: 76, b: 87, a: 255 },
    Color { r: 78, g: 70, b: 83, a: 255 },
    Color { r: 85, g: 76, b: 87, a: 255 },
];

pub const SPECTRUM_FG: [Color; 4] = [
    Color { r: 252, g: 131, b: 131, a: 255 },
    Color { r: 145, g: 224, b: 145, a: 255 },
    Color { r: 126, g: 144, b: 238, a: 255 },
    Color { r: 255, g: 251, b: 181, a: 255 },
];