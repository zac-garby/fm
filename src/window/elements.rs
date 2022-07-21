use std::sync::{Arc, Mutex};

use sdl2::mouse;
use sdl2::event::Event;
use sdl2::rect::{Rect, Point};
use sdl2::pixels::Color;

use crate::player::*;
use crate::song;

use super::font;
use super::constants::*;

pub trait Element {
    fn render(&mut self, buf: &mut [u8], state: &WindowState);
    fn rect(&self) -> Rect;
    fn handle(&mut self, event: InputEvent, state: &mut WindowState);
}

pub struct Panel {
    pub(crate) rect: Rect,
    pub(crate) children: Vec<Box<dyn Element>>,
    pub(crate) background: Color,
    pub(crate) border: Option<Color>,
    pub(crate) corner: Option<Color>,
}

pub struct Spectrum {
    pub(crate) rect: Rect,
    pub(crate) player: Arc<Mutex<Player>>,
    pub(crate) index: usize,
    pub(crate) wave_scale: f32,
}

pub struct Sequencer {
    pub(crate) rect: Rect,
    pub(crate) scroll_x: f32,
    pub(crate) scroll_y: f32,
    pub(crate) cell_width: u32,
    pub(crate) cell_height: u32,
    pub(crate) num_octaves: u32,
    pub(crate) current_part: usize,
    pub(crate) drag_start: Option<Point>,
    pub(crate) drag_end: Option<Point>,
    pub(crate) temp_note: Option<song::Note>,
    pub(crate) place_dur: u32,
    pub(crate) beat_quantize: u32,
    pub(crate) to_delete: Option<usize>,
}

pub struct Stepper {
    pub(crate) rect: Rect,
    pub(crate) value: i32,
    pub(crate) min_value: i32,
    pub(crate) max_value: i32,
    pub(crate) background: Color,
    pub(crate) background_hover: Color,
    pub(crate) foreground: Color,
    pub(crate) on_change: Box<dyn FnMut(i32, &mut WindowState) -> ()>,
}

pub struct Label {
    pub(crate) position: Point,
    pub(crate) text: String,
    pub(crate) colour: Color,
}

pub struct WindowState {
    pub(crate) player: Arc<Mutex<Player>>,
    pub(crate) song: song::Song,
    pub(crate) mouse_x: u32,
    pub(crate) mouse_y: u32,
}

#[derive(Clone)]
pub struct InputEvent {
    pub(crate) real_x: i32,
    pub(crate) real_y: i32,
    pub(crate) x: i32,
    pub(crate) y: i32,
    pub(crate) event: Event,
}

impl Element for Panel {
    fn render(&mut self, buf: &mut [u8], state: &WindowState) {
        draw_rect(buf, self.rect, self.background, self.border, self.corner);
        
        for child in &mut self.children {
            child.render(buf, state);
        }
    }
    
    fn rect(&self) -> Rect {
        self.rect
    }
    
    fn handle(&mut self, event: InputEvent, state: &mut WindowState) {
        for child in &mut self.children {
            let mut e = event.clone();
            
            e.x = event.x - (child.rect().x - self.rect.x);
            e.y = event.y - (child.rect().y - self.rect.y);
            
            if e.real_x >= child.rect().left() && e.real_x < child.rect().right() &&
            e.real_y >= child.rect().top() && e.real_y < child.rect().bottom() {
                child.handle(e, state);
            }
        }
    }
}

impl Element for Spectrum {
    fn render(&mut self, buf: &mut [u8], _state: &WindowState) {
        let player = self.player.lock().unwrap();
        let safe = safe_area(self.rect());
        
        if self.index < player.instruments.len() {
            draw_rect(buf, self.rect, WIN_BG, Some(BORDER), Some(CORNER));
            
            assert_eq!(SPECTRUM_WIDTH, safe.width());
            
            let colour = SPECTRUM_FG[self.index];
            let axis = safe.y + safe.h / 2;
            
            draw_rect(buf, Rect::new(safe.x, safe.y + safe.h / 2, safe.width(), 1), colour, None, None);
            
            let samples = &player.instruments[self.index].hold_buf;
            for i in 0..SPECTRUM_WIDTH as usize {
                let sample = ((samples[i] * self.wave_scale) as i32 + axis).clamp(safe.top(), safe.bottom());
                
                let (sy, ey) = if sample < axis {
                    (sample, axis)
                } else {
                    (axis, sample)
                };
                
                for y in sy..ey+1 {
                    set_pixel(buf, safe.x as u32 + i as u32, y as u32, colour);
                }
            }
            
            draw_text(buf, safe.left() as u32 + 1, safe.top() as u32 + 1, FG,
                &format!("INSTR.{}", self.index)[..]);
        } else {
            draw_rect(buf, self.rect, EMPTY_SPECTRUM_BG, Some(BORDER), Some(CORNER));
        }
    }
    
    fn rect(&self) -> Rect {
        self.rect
    }
    
    fn handle(&mut self, event: InputEvent, _state: &mut WindowState) {
        match event.event {
            Event::MouseWheel { y, .. } => {
                self.wave_scale = (self.wave_scale + y as f32 * 0.5).clamp(1.0, 60.0);
            },
            _ => {},
        }
    }
}

impl Sequencer {
    pub fn x_to_t(&self, x: u32) -> song::Time {
        let beat = x / self.cell_width;
        let pixel = x - beat as u32 * self.cell_width;
        let p = (self.beat_quantize as f32 * pixel as f32 / self.cell_width as f32).floor() / self.beat_quantize as f32;
        let division = (p * song::BEAT_DIVISIONS as f32) as u32;
        song::Time::new(beat, division)
    }
    
    pub fn t_to_x(&self, t: song::Time) -> u32 {
        (self.cell_width as f32 * (t.beat as f32 + t.division as f32 / song::BEAT_DIVISIONS as f32)) as u32
    }
    
    pub fn y_to_pitch(&self, y: u32) -> u32 {
        y / self.cell_height
    }
    
    fn draw_note(&self, buf: &mut [u8], note: &song::Note, bg: Color) {
        let safe = safe_area(self.rect);
        
        let mut rect = Rect::new(
            self.t_to_x(note.start) as i32,
            ((note.pitch + 1) * self.cell_height) as i32,
            self.t_to_x(song::Time::new(0, note.duration)),
            self.cell_height,
        );
        
        rect.x = rect.x + safe.x - self.scroll_x as i32;
        rect.y = safe.y + safe.h - (rect.y - self.scroll_y as i32);
        
        if let Some(rect) = clamp_rect(rect, safe) {
            draw_rect(buf, rect, bg, None, if rect.w <= 1 { None } else { Some(TRANSPARENT) });
        }
    }
}

impl Element for Sequencer {
    fn render(&mut self, buf: &mut [u8], state: &WindowState) {
        let safe = safe_area(self.rect);
        draw_rect(buf, self.rect, SEQ_BACKGROUND[0], Some(BORDER), Some(CORNER));
        
        for real_y in 0..safe.height() {
            let y = real_y + self.scroll_y as u32;
            let row = (y / self.cell_height) as usize;
            
            for real_x in 0..safe.width() {
                let x = real_x + self.scroll_x as u32;
                let col = x / self.cell_width;
                let subdiv = x - col as u32 * self.cell_width;
                
                let bg = if subdiv == self.cell_width-1 {
                    SEQ_DIVIDER
                } else {
                    let is_first = state.song.beats_per_bar > 1 && col % state.song.beats_per_bar == 0;
                    SEQ_BACKGROUND[12 * if is_first { 0 } else { 1 } + row % 12]
                };
                
                set_pixel(buf, real_x + safe.x as u32, safe.height() - real_y - 1 + safe.y as u32, bg);
            }
        }
        
        for note in &state.song.parts[self.current_part] {
            self.draw_note(buf, note, SEQ_NOTE);
        }
        
        if safe.contains_point(Point::new(state.mouse_x as i32, state.mouse_y as i32)) {
            if let Some(note) = self.temp_note {
                self.draw_note(buf, &note, SEQ_GHOST_NOTE);
            }
        }
        
        if let Some(playhead) = {
            let p = state.player.lock().unwrap();
            let cell_x = p.playhead * p.bps;
            let div = (cell_x.fract() * self.cell_width as f64) as i32;
            let head_x = cell_x as i32 * self.cell_width as i32 + div;
            
            clamp_rect(Rect::new(
                safe.x + head_x - self.scroll_x as i32,
                safe.y, 1, safe.height()), safe)
        } {
            draw_rect(buf, playhead, SEQ_PLAYHEAD, None, None);
            
            if playhead.right() >= safe.right() {
                self.scroll_x += safe.w as f32 * 0.75;
            }
        }
    }
    
    fn rect(&self) -> Rect {
        self.rect
    }
    
    fn handle(&mut self, event: InputEvent, state: &mut WindowState) {
        let safe = safe_area(self.rect);
        
        let point = Point::new(
            event.x + self.scroll_x as i32 - 1,
            safe.h - event.y + self.scroll_y as i32,
        );
        
        match event.event {
            Event::MouseWheel { x, y, .. } => {
                self.scroll_x = (self.scroll_x + x as f32).max(0.0);
                self.scroll_y = (self.scroll_y + y as f32)
                    .clamp(0.0, (self.cell_height * 12 * self.num_octaves - (self.rect.height() - 2)) as f32);
            },
            Event::MouseButtonDown { mouse_btn: mouse::MouseButton::Left, .. } => {
                if safe.contains_point(Point::new(event.real_x, event.real_y)) {
                    self.drag_start = Some(point);
                    let t = self.x_to_t(point.x as u32);
                    let pitch = self.y_to_pitch(point.y as u32);
                    
                    self.temp_note = Some(song::Note::new(pitch, t.beat, t.division, self.place_dur, 1.0));
                }
            },
            Event::MouseMotion { mousestate, .. } => {
                self.to_delete = None;
                
                if safe.contains_point(Point::new(event.real_x, event.real_y)) {
                    let t = self.x_to_t(point.x as u32);
                    let pitch = self.y_to_pitch(point.y as u32);
                    
                    if let Some(start) = self.drag_start {
                        let start_t = self.x_to_t(start.x as u32);
                        let dur = t.diff(start_t);
                        
                        if mousestate.left() && dur >= (song::BEAT_DIVISIONS / self.beat_quantize) as i32 {
                            self.drag_end = Some(point);
                            
                            self.temp_note = self.temp_note.map(|mut n| {
                                let diff = t.diff(n.start);
                                if diff > 0 {
                                    n.duration = diff as u32;
                                }
                                n
                            });
                        }
                    } else if let Some((i, hovered)) = state.find_note(self.current_part, t, pitch) {
                        self.temp_note = Some(hovered);
                        self.to_delete = Some(i);
                    } else {
                        self.temp_note = Some(song::Note::new(pitch, t.beat, t.division, self.place_dur, 1.0));
                    }
                }
            },
            Event::MouseButtonUp { mouse_btn: mouse::MouseButton::Left, .. } => {
                if let Some(i) = self.to_delete {
                    state.song.parts[self.current_part].remove(i);
                    self.to_delete = None;
                } else if let Some(note) = self.temp_note {
                    state.add_note(self.current_part, note);
                    self.place_dur = note.duration;
                }
                
                self.drag_start = None;
                self.drag_end = None;
                self.temp_note = None;
            }
            _ => {},
        }
    }
}

impl Element for Stepper {
    fn render(&mut self, buf: &mut [u8], state: &WindowState) {
        let hover = self.rect.contains_point(Point::new(state.mouse_x as i32, state.mouse_y as i32));
        let text = &format!("{}", self.value)[..];
        let text_width = measure_text(text);
        let text_offset = self.rect.w as u32 - 2 - text_width;
        
        draw_rect(buf, self.rect, if hover { self.background_hover } else { self.background }, None, Some(TRANSPARENT));
        draw_text(buf, self.rect.x as u32 + text_offset, self.rect.y as u32 + 1, self.foreground, text);
    }
    
    fn rect(&self) -> Rect {
        self.rect
    }
    
    fn handle(&mut self, event: InputEvent, state: &mut WindowState) {
        match event.event {
            Event::MouseWheel { y, .. } => {
                self.value = (self.value + y).clamp(self.min_value, self.max_value);
                (self.on_change)(self.value, state);
            },
            _ => {},
        }
    }
}

impl Element for Label {
    fn render(&mut self, buf: &mut [u8], _state: &WindowState) {
        draw_text(buf, self.position.x as u32, self.position.y as u32, self.colour, &self.text[..]);
    }
    
    fn rect(&self) -> Rect {
        Rect::new(
            self.position.x,
            self.position.y,
            measure_text(&self.text[..]),
            5,
        )
    }
    
    fn handle(&mut self, _event: InputEvent, _state: &mut WindowState) {}
}

impl WindowState {
    fn add_note(&mut self, part: usize, note: song::Note) {
        let mut overlaps = Vec::new();
        let part = &mut self.song.parts[part];
        
        for (i, existing) in part.iter().enumerate() {
            if existing.pitch == note.pitch && existing.overlap(note) {
                overlaps.push(i);
            }
        }
        
        for (n, i) in overlaps.iter().enumerate() {
            part.remove(i - n);
        }
        
        part.push(note);
    }
    
    fn find_note(&mut self, part: usize, t: song::Time, pitch: u32) -> Option<(usize, song::Note)> {
        for (i, note) in self.song.parts[part].iter().enumerate() {
            if note.pitch == pitch && note.contains(t) {
                return Some((i, note.clone()))
            }
        }
        
        None
    }
}

fn draw_rect(buf: &mut [u8], rect: Rect,
    bg: Color, border: Option<Color>, corner: Option<Color>) {
        for y in rect.y..rect.y + rect.h {
            let i0 = coord_index(rect.x as u32, y as u32);
            
            for x in 0..rect.w as usize {
                let vert = x == 0 || x == rect.w as usize - 1;
                let horiz = y == rect.y || y == rect.y + rect.h - 1;
                let color = if vert && horiz {
                    corner.unwrap_or(bg)
                } else if vert || horiz {
                    border.unwrap_or(bg)
                } else {
                    bg
                };
                
                if color.a == 255 {
                    buf[i0 + x * 4 + 0] = color.b;
                    buf[i0 + x * 4 + 1] = color.g;
                    buf[i0 + x * 4 + 2] = color.r;
                    buf[i0 + x * 4 + 3] = color.a;
                }
            }
        }
    }

fn draw_text(buf: &mut [u8], mut x: u32, y: u32, colour: Color, str: &str) {
    for ch in str.chars() {
        if let Some(data) = &font::FONT_DATA[ch as usize] {
            for j in 0..font::FONT_HEIGHT {
                for i in 0..data.width as usize {
                    if data.data[j * data.width as usize + i] == 1 {
                        set_pixel(buf, x + i as u32, y + j as u32, colour);
                    }
                }
            }
            
            x += data.width + 1;
        }
    }
}

fn measure_text(str: &str) -> u32 {
    let mut w = 0;
    
    for char in str.chars() {
        if let Some(data) = &font::FONT_DATA[char as usize] {
            w += data.width + 1;
        }
    }
    
    w - 1
}

#[inline]
fn set_pixel(buf: &mut [u8], x: u32, y: u32, colour: Color) {
    let idx = coord_index(x, y);
    buf[idx + 0] = colour.b;
    buf[idx + 1] = colour.g;
    buf[idx + 2] = colour.r;
    buf[idx + 3] = colour.a;
}

fn coord_index(x: u32, y: u32) -> usize {
    (y as usize) * 4 * SCREEN_WIDTH as usize + (x as usize) * 4
}

fn safe_area(rect: Rect) -> Rect {
    Rect::new(rect.x + 1, rect.y + 1, rect.w as u32 - 2, rect.h as u32 - 2)
}

fn clamp_rect(rect: Rect, inside: Rect) -> Option<Rect> {
    if rect.right() < inside.left() || rect.left() >= inside.right() ||
    rect.top() >= inside.bottom() || rect.bottom() < inside.top() {
        None
    } else {
        let x = rect.x.clamp(inside.left(), inside.right());
        let y = rect.y.clamp(inside.top(), inside.bottom());
        let w = rect.width().min((inside.right() - rect.x) as u32) as i32 + (rect.x - x);
        let h = rect.height().min((inside.bottom() - rect.y) as u32) as i32 + (rect.y - y);
        
        if w <= 0 || h <= 0 {
            None
        } else {
            Some(Rect::new(x, y, w as u32, h as u32))
        }
    }
}