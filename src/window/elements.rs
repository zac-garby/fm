use std::sync::{Arc, Mutex};

use sdl2::{mouse, keyboard};
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
    pub rect: Rect,
    pub children: Vec<Box<dyn Element>>,
    pub background: Color,
    pub border: Option<Color>,
    pub corner: Option<Color>,
}

pub struct Spectrum {
    pub rect: Rect,
    pub player: Arc<Mutex<Player>>,
    pub index: usize,
    pub wave_scale: f32,
}

pub struct Sequencer {
    pub rect: Rect,
    pub scroll_x: f32,
    pub scroll_y: f32,
    pub cell_width: u32,
    pub cell_height: u32,
    pub num_octaves: u32,
    pub drag_start: Option<Point>,
    pub drag_end: Option<Point>,
    pub temp_note: Option<song::Note>,
    pub place_dur: u32,
    pub beat_quantize: u32,
    pub to_delete: Option<usize>,
    pub on_change: Box<dyn FnMut(&mut WindowState) -> ()>,
}

pub struct Stepper {
    pub rect: Rect,
    pub value: i32,
    pub min_value: i32,
    pub max_value: i32,
    pub background: Color,
    pub background_hover: Color,
    pub foreground: Color,
    pub on_change: Box<dyn FnMut(i32, &mut WindowState) -> ()>,
}

pub struct Slider {
    pub rect: Rect,
    pub state: ButtonState,
    pub value: i32,
    pub min_value: i32,
    pub max_value: i32,
    pub background: Color,
    pub track: Color,
    pub handle: Color,
    pub handle_hover: Color,
    pub handle_active: Color,
    pub on_change: Box<dyn FnMut(i32, &mut WindowState) -> ()>,
}

pub enum ButtonType {
    Momentary {
        label: String,
    },
    
    Toggle {
        on_label: String,
        off_label: String,
        off_foreground: Color,
    },
}

#[derive(PartialEq)]
pub enum ButtonState {
    Off, Active,
}

pub struct Button {
    pub rect: Rect,
    pub kind: ButtonType,
    pub state: ButtonState,
    pub value: bool,
    pub background: Color,
    pub background_hover: Color,
    pub background_active: Color,
    pub foreground: Color,
    pub on_change: Box<dyn FnMut(bool, &mut WindowState) -> ()>,
}

pub struct Label {
    pub position: Point,
    pub text: String,
    pub tooltip: Option<String>,
    pub colour: Color,
}

pub struct WindowState {
    pub player: Arc<Mutex<Player>>,
    pub song: song::Song,
    pub selected_instrument: usize,
    pub mouse_x: u32,
    pub mouse_y: u32,
}

#[derive(Clone)]
pub struct InputEvent {
    pub real_x: i32,
    pub real_y: i32,
    pub x: i32,
    pub y: i32,
    pub event: Event,
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
            
            let propagate_anyway = match e.event {
                Event::MouseButtonUp { .. } | Event::MouseMotion { .. } => true,
                _ => false,
            };
            
            e.x = event.x - (child.rect().x - self.rect.x);
            e.y = event.y - (child.rect().y - self.rect.y);
            
            if propagate_anyway || (e.real_x >= child.rect().left() && e.real_x < child.rect().right() &&
                e.real_y >= child.rect().top() && e.real_y < child.rect().bottom()) {
                child.handle(e, state);
            }
        }
    }
}

impl Element for Spectrum {
    fn render(&mut self, buf: &mut [u8], state: &WindowState) {
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
            
            if state.selected_instrument == self.index {
                draw_text(buf, safe.left() as u32 + 1, safe.top() as u32 + 1, FG,
                    &format!("\x05INSTR-{}", self.index + 1)[..]);
            } else {
                draw_text(buf, safe.left() as u32 + 1, safe.top() as u32 + 1, DIM_LABEL,
                    &format!("INSTR-{}", self.index + 1)[..]);
            }
        } else {
            draw_rect(buf, self.rect, EMPTY_SPECTRUM_BG, Some(BORDER), Some(CORNER));
        }
    }
    
    fn rect(&self) -> Rect {
        self.rect
    }
    
    fn handle(&mut self, event: InputEvent, state: &mut WindowState) {
        match event.event {
            Event::MouseWheel { y, .. } => {
                self.wave_scale = (self.wave_scale + y as f32 * 0.5).clamp(1.0, 60.0);
            },
            Event::MouseButtonDown { mouse_btn: mouse::MouseButton::Left, .. } => {
                state.selected_instrument = self.index;
            }
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
        
        for note in &state.song.parts[state.selected_instrument] {
            self.draw_note(buf, note, SEQ_NOTE);
        }
        
        if safe.contains_point(Point::new(state.mouse_x as i32, state.mouse_y as i32)) {
            if let Some(note) = self.temp_note {
                self.draw_note(buf, &note, SEQ_GHOST_NOTE);
                
                draw_tooltip(buf, state.mouse_x, state.mouse_y, Vec::from([
                    format!("\x00 {}:{}, bar {}", note.start.beat, note.start.division, 1 + note.start.beat / state.song.beats_per_bar),
                    format!("\x04 {} {}", note.name(), note.octave()),
                ]));
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
        
        if !safe.contains_point(Point::new(event.real_x, event.real_y)) {
            return;
        }
        
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
                self.drag_start = Some(point);
                let t = self.x_to_t(point.x as u32);
                let pitch = self.y_to_pitch(point.y as u32);
                
                self.temp_note = Some(song::Note::new(pitch, t.beat, t.division, self.place_dur, 1.0));
            },
            Event::MouseMotion { mousestate, .. } => {
                self.to_delete = None;
                
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
                } else if let Some((i, hovered)) = state.find_note(state.selected_instrument, t, pitch) {
                    self.temp_note = Some(hovered);
                    self.to_delete = Some(i);
                } else {
                    self.temp_note = Some(song::Note::new(pitch, t.beat, t.division, self.place_dur, 1.0));
                }
            },
            Event::MouseButtonUp { mouse_btn: mouse::MouseButton::Left, .. } => {
                if let Some(i) = self.to_delete {
                    state.song.parts[state.selected_instrument].remove(i);
                    self.to_delete = None;
                } else if let Some(note) = self.temp_note {
                    state.add_note(state.selected_instrument, note);
                    (self.on_change)(state);
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
            Event::MouseWheel { x, y, .. } => {
                self.value = (self.value + x + y).clamp(self.min_value, self.max_value);
                (self.on_change)(self.value, state);
            },
            Event::KeyDown { keycode: Some(keyboard::Keycode::Up), .. } |
            Event::KeyDown { keycode: Some(keyboard::Keycode::Right), .. } => {
                self.value = (self.value + 1).clamp(self.min_value, self.max_value);
                (self.on_change)(self.value, state);
            },
            Event::KeyDown { keycode: Some(keyboard::Keycode::Down), .. } |
            Event::KeyDown { keycode: Some(keyboard::Keycode::Left), .. } => {
                self.value = (self.value - 1).clamp(self.min_value, self.max_value);
                (self.on_change)(self.value, state);
            }
            _ => {},
        }
    }
}

impl Element for Slider {
    fn render(&mut self, buf: &mut [u8], state: &WindowState) {
        let mouse = Point::new(state.mouse_x as i32, state.mouse_y as i32);
        
        let input_rect = Rect::new(self.rect.x + 2, self.rect.y + 1, self.rect.width() - 4, self.rect.height() - 2);
        let track = Rect::new(input_rect.x, input_rect.y + input_rect.h / 2, input_rect.width(), 1);
        draw_rect(buf, self.rect, self.background, None, Some(TRANSPARENT));
        draw_rect(buf, track, self.track, None, None);
        
        let p = (self.value as f32 - self.min_value as f32) / (self.max_value as f32 - self.min_value as f32);
        let handle = Rect::new(track.x + (p * track.w as f32) as i32 - 1, input_rect.y, 2, input_rect.height());
        let handle_colour = match self.state {
            ButtonState::Off => if handle.contains_point(mouse) {
                self.handle_hover
            } else {
                self.handle
            },
            ButtonState::Active => self.handle_active,
        };
        
        draw_rect(buf, handle, handle_colour, None, None);
    }

    fn rect(&self) -> Rect {
        self.rect
    }

    fn handle(&mut self, event: InputEvent, state: &mut WindowState) {
        let mouse = Point::new(state.mouse_x as i32, state.mouse_y as i32);
        let input_rect = Rect::new(self.rect.x + 2, self.rect.y + 1, self.rect.width() - 4, self.rect.height() - 2);
        let track = Rect::new(input_rect.x, input_rect.y + input_rect.h / 2, input_rect.width(), 1);
        
        match event.event {
            Event::MouseButtonDown { mouse_btn: mouse::MouseButton::Left, .. } => {
                if input_rect.contains_point(mouse) {
                    self.state = ButtonState::Active;
                    let p_new = (mouse.x as f32 - track.left() as f32) / (track.width() as f32);
                    self.value = ((p_new * (self.max_value as f32 - self.min_value as f32)) as i32 + self.min_value)
                        .clamp(self.min_value, self.max_value);
                }
            },
            Event::MouseButtonUp { mouse_btn: mouse::MouseButton::Left, .. } => {
                self.state = ButtonState::Off;
            },
            Event::MouseMotion { .. } => {
                if self.state == ButtonState::Active {
                    let p_new = (mouse.x as f32 - track.left() as f32) / (track.width() as f32);
                    self.value = ((p_new * (self.max_value as f32 - self.min_value as f32)) as i32 + self.min_value)
                        .clamp(self.min_value, self.max_value);
                }
            }
            _ => {},
        }
        
        (self.on_change)(self.value, state);
    }
}

impl Element for Button {
    fn render(&mut self, buf: &mut [u8], state: &WindowState) {
        let mouse = Point::new(state.mouse_x as i32, state.mouse_y as i32);
        
        let background = match self.state {
            ButtonState::Off => if self.rect.contains_point(mouse) {
                self.background_hover
            } else { 
                self.background
            },
            ButtonState::Active => self.background_active,
        };
        
        let (label, fg) = match &self.kind {
            ButtonType::Momentary { label } => (label, self.foreground),
            ButtonType::Toggle { on_label, off_label, off_foreground } => if self.value {
                (on_label, self.foreground)
            } else {
                (off_label, *off_foreground)
            },
        };
        
        let label_w = measure_text(&label[..]);
        
        draw_rect(buf, self.rect, background, None, Some(TRANSPARENT));
        draw_text(buf, (self.rect.x + self.rect.w / 2) as u32 - label_w / 2, self.rect.y as u32 + 1, fg, &label[..]);
    }

    fn rect(&self) -> Rect {
        self.rect
    }

    fn handle(&mut self, event: InputEvent, state: &mut WindowState) {
        match event.event {
            Event::MouseButtonDown { mouse_btn: mouse::MouseButton::Left, .. } => {
                self.state = ButtonState::Active;
                
                match self.kind {
                    ButtonType::Momentary { .. } => {
                        self.value = true;
                        (self.on_change)(self.value, state);
                    },
                    ButtonType::Toggle { .. } => {},
                }
            },
            Event::MouseButtonUp { mouse_btn: mouse::MouseButton::Left, .. } => {
                if self.state == ButtonState::Active {
                    if self.rect.contains_point(Point::new(state.mouse_x as i32, state.mouse_y as i32)) {
                        match self.kind {
                            ButtonType::Momentary { .. } => {
                                self.state = ButtonState::Off;
                                self.value = false;
                            },
                            ButtonType::Toggle { .. } => {
                                self.state = ButtonState::Off;
                                self.value = !self.value;
                            },
                        }
                        
                        (self.on_change)(self.value, state);
                    } else {
                        self.state = ButtonState::Off;
                    }
                }
            }
            _ => {},
        }
    }
}

impl Element for Label {
    fn render(&mut self, buf: &mut [u8], state: &WindowState) {
        draw_text(buf, self.position.x as u32, self.position.y as u32, self.colour, &self.text[..]);
        
        if let Some(tooltip) = &self.tooltip {
            if self.rect().contains_point(Point::new(state.mouse_x as i32, state.mouse_y as i32)) {
                draw_tooltip(buf, state.mouse_x, state.mouse_y, vec![tooltip.clone()]);
            }
        }
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

pub(crate) fn draw_rect(buf: &mut [u8], rect: Rect,
    bg: Color, border: Option<Color>, corner: Option<Color>) {
    if let Some(rect) = clamp_rect(rect, Rect::new(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT)) {
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
}

pub(crate) fn draw_text(buf: &mut [u8], mut x: u32, y: u32, colour: Color, str: &str) {
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

pub(crate) fn draw_tooltip(buf: &mut [u8], x: u32, y: u32, text: Vec<String>) {
    if let Some(text_width) = text.iter().map(|s| measure_text(&s[..])).max() {
        let text_height = (text.len() * (font::FONT_HEIGHT + 2) - 2) as u32;
        
        let rect = Rect::new(
            x as i32 + 4,
            y as i32 - text_height as i32 - 8,
            text_width + 4,
            text_height + 4
        );
        
        draw_rect(buf, rect, TOOLTIP_BG, None, Some(TRANSPARENT));
        
        for (i, line) in text.iter().enumerate() {
            draw_text(
                buf,
                rect.x as u32 + 2,
                rect.y as u32 + 2 + (i * (font::FONT_HEIGHT + 2)) as u32,
                FG,
                &line[..],
            );
        }
    }
}

pub(crate) fn measure_text(str: &str) -> u32 {
    let mut w = 0;
    
    for char in str.chars() {
        if let Some(data) = &font::FONT_DATA[char as usize] {
            w += data.width + 1;
        }
    }
    
    w - 1
}

#[inline]
pub(crate) fn set_pixel(buf: &mut [u8], x: u32, y: u32, colour: Color) {
    if x < SCREEN_WIDTH && y < SCREEN_HEIGHT {
        let idx = coord_index(x, y);
        buf[idx + 0] = colour.b;
        buf[idx + 1] = colour.g;
        buf[idx + 2] = colour.r;
        buf[idx + 3] = colour.a;
    }
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
