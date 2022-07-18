extern crate sdl2;

use sdl2::event::Event;
use sdl2::rect::Rect;
use sdl2::pixels::Color;
use sdl2::render;

use std::sync::{Arc, Mutex};

use crate::player::Player;

pub const SCREEN_WIDTH: u32 = 300;
pub const SCREEN_HEIGHT: u32 = 250;
pub const SCREEN_SCALE: u32 = 4;
pub const REAL_WIDTH: u32 = SCREEN_WIDTH * SCREEN_SCALE;
pub const REAL_HEIGHT: u32 = SCREEN_HEIGHT * SCREEN_SCALE;

pub const SPECTRUM_WIDTH: u32 = 128;
pub const SPECTRUM_HEIGHT: u32 = 32;

pub const PANEL_BG: Color = Color { r: 29, g: 24, b: 30, a: 255 };
pub const WIN_BG: Color = Color { r: 21, g: 20, b: 18, a: 255 };
pub const EMPTY_SPECTRUM_BG: Color = Color { r: 22, g: 18, b: 23, a: 255 };
pub const BORDER: Color = Color { r: 48, g: 41, b: 50, a: 255 };
pub const WIN_BORDER: Color = Color { r: 78, g: 71, b: 80, a: 255 };
pub const CORNER: Color = Color { r: 34, g: 29, b: 36, a: 255 };

pub const SPECTRUM_FG: [Color; 4] = [
    Color { r: 252, g: 131, b: 131, a: 255 },
    Color { r: 145, g: 224, b: 145, a: 255 },
    Color { r: 126, g: 144, b: 238, a: 255 },
    Color { r: 255, g: 251, b: 181, a: 255 },
];

pub struct Window {
    canvas: render::WindowCanvas,
    texture: render::Texture,
    root: Box<dyn Element>,
    mouse_x: u32,
    mouse_y: u32,
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
    fn render(&self, buf: &mut [u8]);
    fn rect(&self) -> Rect;
    fn handle(&mut self, event: InputEvent);
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

impl Element for Panel {
    fn render(&self, buf: &mut [u8]) {
        draw_rect(buf, self.rect, self.background, self.border, self.corner);
        
        for child in &self.children {
            child.render(buf);
        }
    }
    
    fn rect(&self) -> Rect {
        self.rect
    }

    fn handle(&mut self, event: InputEvent) {
        for child in &mut self.children {
            let mut e = event.clone();
            
            e.x = event.x - (child.rect().x - self.rect.x);
            e.y = event.y - (child.rect().y - self.rect.y);
            
            if e.real_x >= child.rect().left() && e.real_x < child.rect().right() &&
                e.real_y >= child.rect().top() && e.real_y < child.rect().bottom() {
                child.handle(e);
            }
        }
    }
}

impl Element for Spectrum {
    fn render(&self, buf: &mut [u8]) {
        let player = self.player.lock().unwrap();
        
        if self.index < player.instruments.len() {
            draw_rect(buf, self.rect, WIN_BG, Some(BORDER), Some(CORNER));
            
            let safe = safe_area(self.rect());
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
        } else {
            draw_rect(buf, self.rect, EMPTY_SPECTRUM_BG, Some(BORDER), Some(CORNER));
        }
    }
    
    fn rect(&self) -> Rect {
        self.rect
    }

    fn handle(&mut self, event: InputEvent) {
        match event.event {
            Event::MouseWheel { y, .. } => {
                self.wave_scale = (self.wave_scale + y as f32 * 0.5).clamp(1.0, 60.0);
            },
            _ => {},
        }
    }
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
                    wave_scale: 20.0,
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
            children: Vec::new(),
            background: PANEL_BG,
            border: Some(BORDER),
            corner: Some(CORNER),
        }));
        
        Ok(Window {
            canvas,
            texture,
            root: Box::new(root),
            mouse_x: 0,
            mouse_y: 0,
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
                        self.mouse_x = x as u32 / SCREEN_SCALE;
                        self.mouse_y = y as u32 / SCREEN_SCALE;
                    },
                    _ => {}
                }
                
                match e {
                    Event::MouseMotion { .. } |
                    Event::MouseButtonUp { .. } |
                    Event::MouseButtonDown { .. } |
                    Event::MouseWheel { .. } => self.send_event(InputEvent {
                        real_x: self.mouse_x as i32,
                        real_y: self.mouse_y as i32,
                        x: self.mouse_x as i32 - self.root.rect().x,
                        y: self.mouse_y as i32 - self.root.rect().y,
                        event: e }),
                    _ => {}
                }
            }
            
            self.texture.with_lock(None, |buf: &mut [u8], _pitch: usize| {
                self.root.render(buf);
            })?;
            
            self.canvas.clear();
            self.canvas.copy(&self.texture, None, None)?;
            self.canvas.present();
        }
        
        Ok(())
    }
    
    fn send_event(&mut self, e: InputEvent) {
        self.root.handle(e);
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
            
            buf[i0 + x * 4 + 0] = color.b;
            buf[i0 + x * 4 + 1] = color.g;
            buf[i0 + x * 4 + 2] = color.r;
            buf[i0 + x * 4 + 3] = color.a;
        }
    }
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