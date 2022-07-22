extern crate sdl2;

mod font;
mod constants;
mod elements;

use sdl2::event::Event;
use sdl2::rect::{Rect, Point};
use sdl2::render;

use std::sync::{Arc, Mutex, mpsc};

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
    pub fn new(video: sdl2::VideoSubsystem, player_mutex: Arc<Mutex<Player>>,
        note_channel: mpsc::Sender<(usize, song::Note)>)
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
                                let mut player = s.player.lock().unwrap();
                                let old_bps = player.bps;
                                let new_bps = x as f64 / 60.0;
                                player.bps = new_bps;
                                player.playhead /= new_bps / old_bps;
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
                        Box::new(Button {
                            rect: Rect::new(
                                SCREEN_WIDTH as i32 / 2 - 16,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 11,
                                11,
                                7,
                            ),
                            kind: ButtonType::Momentary { label: String::from("\x02") },
                            state: ButtonState::Off,
                            value: false,
                            background: CONTROL_BG,
                            background_hover: CONTROL_HOVER,
                            background_active: CONTROL_ACTIVE,
                            foreground: FG,
                            on_change: {
                                let chan = note_channel.clone();
                                
                                Box::new(move |pressed, s| {
                                    if !pressed {
                                        let mut player = s.player.lock().unwrap();
                                        
                                        if !player.paused {
                                            player.flush_notes();
                                            s.song.sequence(chan.clone());
                                        }
                                        
                                        player.playhead = 0.0;
                                    }
                                })
                            },
                        }),
                        Box::new(Button {
                            rect: Rect::new(
                                SCREEN_WIDTH as i32 / 2 - 5,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 11,
                                11,
                                7,
                            ),
                            kind: ButtonType::Toggle {
                                on_label: String::from("\x01"),
                                off_label: String::from("\x00"),
                                off_foreground: FG,
                            },
                            state: ButtonState::Off,
                            value: false,
                            background: CONTROL_BG,
                            background_hover: CONTROL_HOVER,
                            background_active: CONTROL_ACTIVE,
                            foreground: FG,
                            on_change: {
                                let chan = note_channel.clone();
                                
                                Box::new(move |playing, s| {
                                    let mut player = s.player.lock().unwrap();
                                    
                                    if playing {
                                        player.flush_notes();
                                        s.song.sequence(chan.clone());
                                    }
                                    
                                    player.paused = !playing;
                                })
                            },
                        }),
                        Box::new(Button {
                            rect: Rect::new(
                                SCREEN_WIDTH as i32 / 2 + 6,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 11,
                                11,
                                7,
                            ),
                            kind: ButtonType::Momentary { label: String::from("\x03") },
                            state: ButtonState::Off,
                            value: false,
                            background: CONTROL_BG,
                            background_hover: CONTROL_HOVER,
                            background_active: CONTROL_ACTIVE,
                            foreground: FG,
                            on_change: Box::new(|_pressed, _s| {
                                
                            }),
                        }),
                        Box::new(Button {
                            rect: Rect::new(
                                SCREEN_WIDTH as i32 / 2 + 33,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 11,
                                11,
                                7,
                            ),
                            kind: ButtonType::Toggle {
                                on_label: String::from("\x06\x07"),
                                off_label: String::from("\x06"),
                                off_foreground: DIM_LABEL,
                            },
                            state: ButtonState::Off,
                            value: true,
                            background: CONTROL_BG,
                            background_hover: CONTROL_HOVER,
                            background_active: CONTROL_ACTIVE,
                            foreground: FG,
                            on_change: Box::new(|on, s| {
                                s.player.lock().unwrap().mute = !on;
                            }),
                        }),
                        Box::new(Slider {
                            rect: Rect::new(
                                SCREEN_WIDTH as i32 / 2 + 44,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 11,
                                36,
                                7,
                            ),
                            state: ButtonState::Off,
                            value: 16,
                            min_value: 0,
                            max_value: 32,
                            background: CONTROL_BG,
                            track: BORDER,
                            handle: SLIDER_HANDLE,
                            handle_hover: CONTROL_HOVER,
                            handle_active: CONTROL_ACTIVE,
                            on_change: Box::new(|val, s| {
                                let vol = val as f32 / 16.0;
                                s.player.lock().unwrap().volume = vol;
                            }),
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
                    drag_start: None,
                    drag_end: None,
                    temp_note: None,
                    place_dur: song::BEAT_DIVISIONS,
                    beat_quantize: 4,
                    to_delete: None,
                    on_change: {
                        let chan = note_channel.clone();
                        
                        Box::new(move |s| {
                            let mut player = s.player.lock().unwrap();
                            
                            if !player.paused {
                                player.flush_notes();
                                s.song.sequence(chan.clone());
                            }
                        })
                    }
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
                selected_instrument: 0,
                player: player_mutex,
                song: song::Song::new(4, 60, 4),
            }
        })
    }
    
    pub fn start(&mut self, sdl: sdl2::Sdl) -> Result<(), String> {
        let mut events = sdl.event_pump()?;
        
        self.canvas.window().grab();
        
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
                    Event::MouseWheel { .. } |
                    Event::KeyDown { .. } |
                    Event::KeyUp { .. } => self.send_event(InputEvent {
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
