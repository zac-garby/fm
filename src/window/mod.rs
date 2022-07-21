extern crate sdl2;

mod font;
mod constants;
mod elements;

use sdl2::event::Event;
use sdl2::rect::{Rect, Point};
use sdl2::render;

use std::sync::{Arc, Mutex};

use crate::{player::Player, song};

use constants::*;
use elements::*;

pub struct Window {
    canvas: render::WindowCanvas,
    texture: render::Texture,
    root: Box<dyn Element>,
    state: WindowState,
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
                        }),
                        Box::new(Stepper {
                            rect: Rect::new(
                                38,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 11,
                                11,
                                7,
                            ),
                            value: 4,
                            min_value: 1,
                            max_value: 64,
                            background: CONTROL_BG,
                            background_hover: CONTROL_HOVER,
                            foreground: FG2,
                            on_change: Box::new(|x, s| {
                                s.song.beats_per_bar = x as u32;
                            }),
                        }) as Box<dyn Element>,
                        Box::new(Label {
                            position: Point::new(
                                50,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 12,
                            ),
                            text: String::from("/4"),
                            colour: DIM_LABEL,
                        }),
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
                    scroll_x: 0.0,
                    scroll_y: 160.0,
                    cell_width: 16,
                    cell_height: 4,
                    num_octaves: 9,
                    current_part: 0,
                    drag_start: None,
                    drag_end: None,
                    temp_note: None,
                    place_dur: song::BEAT_DIVISIONS,
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
                player: player_mutex,
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
