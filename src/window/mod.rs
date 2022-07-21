extern crate sdl2;

mod font;
mod colour;

use sdl2::event::Event;
use sdl2::rect::{Rect, Point};
use sdl2::pixels::Color;
use sdl2::{render, mouse};

use std::sync::{Arc, Mutex};

use crate::song::BEAT_DIVISIONS;
use crate::{player::Player, song};

use colour::*;
use font::FONT_HEIGHT;

pub const SCREEN_WIDTH: u32 = 300;
pub const SCREEN_HEIGHT: u32 = 250;
pub const SCREEN_SCALE: u32 = 4;
pub const REAL_WIDTH: u32 = SCREEN_WIDTH * SCREEN_SCALE;
pub const REAL_HEIGHT: u32 = SCREEN_HEIGHT * SCREEN_SCALE;

pub const SPECTRUM_WIDTH: u32 = 128;
pub const SPECTRUM_HEIGHT: u32 = 32;

pub struct WindowState {
    song: song::Song,
    mouse_x: u32,
    mouse_y: u32,
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

pub struct Window {
    canvas: render::WindowCanvas,
    texture: render::Texture,
    root: Box<dyn Element>,
    state: WindowState,
}

#[derive(Clone)]
pub struct InputEvent {
    real_x: i32,
    real_y: i32,
    x: i32,
    y: i32,
    event: Event,
}

pub trait Element {
    fn render(&mut self, buf: &mut [u8], state: &WindowState);
    fn rect(&self) -> Rect;
    fn handle(&mut self, event: InputEvent, state: &mut WindowState);
}

pub struct Panel {
    rect: Rect,
    children: Vec<Box<dyn Element>>,
    background: Color,
    border: Option<Color>,
    corner: Option<Color>,
}

pub struct Spectrum {
    rect: Rect,
    player: Arc<Mutex<Player>>,
    index: usize,
    wave_scale: f32,
}

pub struct Sequencer {
    rect: Rect,
    scroll_x: f32,
    scroll_y: f32,
    cell_width: u32,
    cell_height: u32,
    num_octaves: u32,
    player: Arc<Mutex<Player>>,
    current_part: usize,
    drag_start: Option<Point>,
    drag_end: Option<Point>,
    temp_note: Option<song::Note>,
    place_dur: u32,
    beat_quantize: u32,
    to_delete: Option<usize>,
}

pub struct Stepper {
    rect: Rect,
    value: i32,
    min_value: i32,
    max_value: i32,
    background: Color,
    background_hover: Color,
    foreground: Color,
    on_change: Box<dyn FnMut(i32, &mut WindowState) -> ()>,
}

pub struct Label {
    position: Point,
    text: String,
    colour: Color,
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
        let division = (p * BEAT_DIVISIONS as f32) as u32;
        song::Time::new(beat, division)
    }
    
    pub fn t_to_x(&self, t: song::Time) -> u32 {
        (self.cell_width as f32 * (t.beat as f32 + t.division as f32 / BEAT_DIVISIONS as f32)) as u32
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
                    SEQ_BACKGROUND[12 * if col % state.song.beats_per_bar == 0 { 0 } else { 1 } + row % 12]
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
            let p = self.player.lock().unwrap();
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
                        
                        if mousestate.left() && dur >= (BEAT_DIVISIONS / self.beat_quantize) as i32 {
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
        
        draw_rect(buf, self.rect, if hover { self.background_hover } else { self.background }, None, Some(TRANSPARENT));
        draw_text(buf, self.rect.x as u32 + 2, self.rect.y as u32 + 1, self.foreground,
            &format!("{}", self.value)[..]);
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

impl Window {
    pub fn new(video: sdl2::VideoSubsystem, player_mutex: Arc<Mutex<Player>>)
    -> Result<Window, String> {
        let win = video
            .window("Cancrizans", REAL_WIDTH, REAL_HEIGHT)
            .allow_highdpi()
            .position_centered()
            .build()
            .map_err(|e| e.to_string())?;
        
        let canvas = win
            .into_canvas()
            .present_vsync()
            .build()
            .map_err(|e| e.to_string())?;
        
        let texture = canvas.texture_creator()
            .create_texture_streaming(None, SCREEN_WIDTH, SCREEN_HEIGHT)
            .map_err(|e| e.to_string())?;
        
        let mut root = Panel {
            rect: Rect::new(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
            children: Vec::new(),
            background: WIN_BG,
            border: None,
            corner: None,
        };
        
        root.children.push(Box::new(Panel {
            rect: Rect::new(1, 1, (SPECTRUM_WIDTH + 2) + 4, (SPECTRUM_HEIGHT + 2) * 4 + 7),
            children: (0..4).map(|i| {
                Box::new(Spectrum {
                    rect: Rect::new(
                        3, 3 + i * (SPECTRUM_HEIGHT + 3) as i32,
                        SPECTRUM_WIDTH + 2, SPECTRUM_HEIGHT + 2),
                    player: player_mutex.clone(),
                    index: i as usize,
                    wave_scale: 12.0,
                }) as Box<dyn Element>
            }).collect(),
            background: PANEL_BG,
            border: Some(BORDER),
            corner: Some(CORNER),
        }));
        
        root.children.push(Box::new(Panel {
            rect: Rect::new((SPECTRUM_WIDTH + 2) as i32 + 6, 1,
                SCREEN_WIDTH - (SPECTRUM_WIDTH + 2) - 7, (SPECTRUM_HEIGHT + 2) * 4 + 7),
            children: Vec::new(),
            background: PANEL_BG,
            border: Some(BORDER),
            corner: Some(CORNER),
        }));
        
        root.children.push(Box::new(Panel {
            rect: Rect::new(1, (SPECTRUM_HEIGHT + 2) as i32 * 4 + 9,
                SCREEN_WIDTH - 2, SCREEN_HEIGHT - (SPECTRUM_HEIGHT + 2) * 4 - 10),
            children: Vec::from([
                Box::new(Panel {
                    rect: Rect::new(
                        2,
                        (SPECTRUM_HEIGHT + 2) as i32 * 4 + 10,
                        SCREEN_WIDTH - 4,
                        8),
                    children: Vec::from([
                        Box::new(Stepper {
                            rect: Rect::new(
                                3,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 11,
                                15,
                                7,
                            ),
                            value: 60,
                            min_value: 1,
                            max_value: 999,
                            background: CONTROL_BG,
                            background_hover: CONTROL_HOVER,
                            foreground: FG2,
                            on_change: Box::new(|x, s| {
                                s.song.bpm = x as u32;
                            }),
                        }) as Box<dyn Element>,
                        Box::new(Label {
                            position: Point::new(
                                20,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 12,
                            ),
                            text: String::from("bpm"),
                            colour: DIM_LABEL,
                        })
                    ]),
                    background: PANEL_BG,
                    border: None,
                    corner: None,
                }) as Box<dyn Element>,
                Box::new(Sequencer {
                    rect: Rect::new(
                        1,
                        (SPECTRUM_HEIGHT + 2) as i32 * 4 + 19,
                        SCREEN_WIDTH - 2,
                        SCREEN_HEIGHT - (SPECTRUM_HEIGHT + 2) * 4 - 20),
                    player: player_mutex.clone(),
                    scroll_x: 0.0,
                    scroll_y: 160.0,
                    cell_width: 16,
                    cell_height: 4,
                    num_octaves: 9,
                    current_part: 0,
                    drag_start: None,
                    drag_end: None,
                    temp_note: None,
                    place_dur: BEAT_DIVISIONS,
                    beat_quantize: 4,
                    to_delete: None,
                }) as Box<dyn Element>
            ]),
            background: PANEL_BG,
            border: Some(BORDER),
            corner: Some(CORNER),
        }));
        
        Ok(Window {
            canvas,
            texture,
            root: Box::new(root),
            state: WindowState {
                mouse_x: 0,
                mouse_y: 0,
                song: song::Song::new(4, 60, 4),
            }
        })
    }
    
    pub fn start(&mut self, sdl: sdl2::Sdl) -> Result<(), String> {
        let mut events = sdl.event_pump()?;
        
        'run:
        loop {
            for e in events.poll_iter() {
                match e {
                    Event::Quit { .. } => break 'run,
                    Event::MouseMotion { x, y, .. } => {
                        self.state.mouse_x = x as u32 / SCREEN_SCALE;
                        self.state.mouse_y = y as u32 / SCREEN_SCALE;
                    },
                    _ => {}
                }
                
                match e {
                    Event::MouseMotion { .. } |
                    Event::MouseButtonUp { .. } |
                    Event::MouseButtonDown { .. } |
                    Event::MouseWheel { .. } => self.send_event(InputEvent {
                        real_x: self.state.mouse_x as i32,
                        real_y: self.state.mouse_y as i32,
                        x: self.state.mouse_x as i32 - self.root.rect().x,
                        y: self.state.mouse_y as i32 - self.root.rect().y,
                        event: e }),
                    _ => {}
                }
            }
            
            self.texture.with_lock(None, |buf: &mut [u8], _pitch: usize| {
                self.root.render(buf, &self.state);
            })?;
            
            self.canvas.clear();
            self.canvas.copy(&self.texture, None, None)?;
            self.canvas.present();
        }
        
        Ok(())
    }
    
    fn send_event(&mut self, e: InputEvent) {
        self.root.handle(e, &mut self.state);
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
            for j in 0..FONT_HEIGHT {
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