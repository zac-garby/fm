extern crate sdl2;
extern crate nfd;

mod font;
mod constants;
mod elements;

use sdl2::event::Event;
use sdl2::rect::{Rect, Point};
use sdl2::render;

use std::fs::OpenOptions;
use std::path::Path;
use std::sync::{Arc, Mutex, mpsc};

use crate::song::Song;
use crate::synth;
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
    pub fn new(video: sdl2::VideoSubsystem,
        player_mutex: Arc<Mutex<Player>>,
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
        
        let mut win = Window {
            canvas,
            texture,
            root: Box::new(Label {
                position: Point::new(2, 2),
                text: String::from("loading..."),
                tooltip: None,
                colour: FG2,
            }),
            state: WindowState {
                mouse_x: 0,
                mouse_y: 0,
                selected_instrument: 0,
                player: player_mutex,
                song: song::Song::new(4, 60, 4),
                seq_scale_x: 12,
                seq_scale_y: 4,
                seq_quantize: 4,
                filename: None,
                note_channel,
                current_op: 0,
            }
        };
        
        win.load_elements();
        
        Ok(win)
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
    
    fn load_elements(&mut self) {
        let mut root = Panel {
            rect: Rect::new(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
            children: Vec::new(),
            background: WIN_BG,
            border: None,
            corner: None,
        };
        
        root.children.push(Box::new(Panel {
            rect: Rect::new(
                0, 0, SCREEN_WIDTH, 10,
            ),
            children: Vec::from([
                Box::new(DynamicLabel {
                    rect: Rect::new(3, 3, 48, 5),
                    tooltip: None,
                    colour: FG2,
                    get_text: Box::new(|state| {
                        let filename = state.filename.clone().unwrap_or(String::from("unnamed.crz"));
                        let path = Path::new(&filename[..]);
                        format!("\x10 {}", path.file_name().and_then(|f| f.to_str()).unwrap_or("unnamed.crz"))
                    }),
                }) as Box<dyn Element>,
                Box::new(Button {
                    rect: Rect::new(
                        SCREEN_WIDTH as i32 - 110, 2,
                        20, 7,
                    ),
                    kind: ButtonType::Momentary { label: String::from("save") },
                    state: ButtonState::Off,
                    value: false,
                    background: CONTROL_BG,
                    background_hover: CONTROL_HOVER,
                    background_active: CONTROL_ACTIVE,
                    foreground: FG2,
                    on_change: Box::new(|pressed, state| {
                        if !pressed {
                            if let Some(path) = match &state.filename {
                                Some(f) => Some(f.clone()),
                                None => {
                                    match nfd::open_save_dialog(Some("crz"), Some("crz")) {
                                        Ok(nfd::Response::Okay(f)) => Some(f),
                                        Ok(nfd::Response::OkayMultiple(fs)) => Some(fs[0].clone()),
                                        Ok(nfd::Response::Cancel) => None,
                                        Err(_) => {
                                            eprintln!("could not open file dialog");
                                            None
                                        },
                                    }
                                },
                            } {                                
                                match OpenOptions::new().read(false).write(true).create(true).open(&path) {
                                    Ok(file) => {
                                        file.set_len(0).unwrap();
                                        
                                        match serde_json::to_writer_pretty(file, &state.song) {
                                            Ok(_) => state.filename = Some(path),
                                            Err(err) => eprintln!("couldn't write to file: {}", err),
                                        }
                                    },
                                    Err(err) => eprintln!("could not open file: {}", err),
                                }
                            }
                        }
                    }),
                }) as Box<dyn Element>,
                Box::new(Button {
                    rect: Rect::new(
                        SCREEN_WIDTH as i32 - 88, 2,
                        38, 7,
                    ),
                    kind: ButtonType::Momentary { label: String::from("new song") },
                    state: ButtonState::Off,
                    value: false,
                    background: CONTROL_BG,
                    background_hover: CONTROL_HOVER,
                    background_active: CONTROL_ACTIVE,
                    foreground: FG2,
                    on_change: Box::new(|pressed, state| {
                        if !pressed {
                            state.filename = None;
                            state.song = Song::new(4, 60, 4);
                            state.player.lock().unwrap().reset();
                        }
                    }),
                }) as Box<dyn Element>,
                Box::new(Button {
                    rect: Rect::new(
                        SCREEN_WIDTH as i32 - 48, 2,
                        46, 7,
                    ),
                    kind: ButtonType::Momentary { label: String::from("open song...") },
                    state: ButtonState::Off,
                    value: false,
                    background: CONTROL_BG,
                    background_hover: CONTROL_HOVER,
                    background_active: CONTROL_ACTIVE,
                    foreground: FG2,
                    on_change: Box::new(|pressed, state| {
                        if !pressed {
                            match nfd::open_file_dialog(None, None) {
                                Ok(nfd::Response::Okay(f)) => {
                                    match OpenOptions::new().read(true).open(f.clone()) {
                                        Ok(file) => {
                                            match serde_json::from_reader(file) {
                                                Ok(s) => {
                                                    state.song = s;
                                                    state.player.lock().unwrap().bps = state.song.get_bps();
                                                    state.filename = Some(f);
                                                },
                                                Err(err) => eprintln!("error reading file: {}", err),
                                            }
                                        },
                                        Err(err) => eprintln!("could not open file: {}", err),
                                    }
                                },
                                Ok(nfd::Response::OkayMultiple(_fs)) => {},
                                Ok(nfd::Response::Cancel) => {},
                                Err(_) => todo!(),
                            }
                        }
                    }),
                })
            ]),
            background: PANEL_BG,
            border: None,
            corner: None,
        }) as Box<dyn Element>);
        
        root.children.push(Box::new(Panel {
            rect: Rect::new(1, 11, (SPECTRUM_WIDTH + 2) + 4, (SPECTRUM_HEIGHT + 2) * 4 + 7),
            children: (0..4).map(|i| {
                Box::new(Spectrum {
                    rect: Rect::new(
                        3, 13 + i * (SPECTRUM_HEIGHT + 3) as i32,
                        SPECTRUM_WIDTH + 2, SPECTRUM_HEIGHT + 2),
                    player: self.state.player.clone(),
                    index: i as usize,
                    wave_scale: 12.0,
                }) as Box<dyn Element>
            }).collect(),
            background: PANEL_BG,
            border: Some(BORDER),
            corner: Some(CORNER),
        }));
        
        root.children.push(Box::new(Panel {
            rect: Rect::new((SPECTRUM_WIDTH + 2) as i32 + 6, 11,
                SCREEN_WIDTH - (SPECTRUM_WIDTH + 2) - 7, (SPECTRUM_HEIGHT + 2) * 4 + 7),
            children: {
                let mut elems: Vec<Box<dyn Element>> = Vec::new();
                
                elems.push(Box::new(Button {
                    rect: Rect::new(
                        SPECTRUM_WIDTH as i32 + 11,
                        14, 7, 7,
                    ),
                    kind: ButtonType::Momentary { label: String::from("<") },
                    state: ButtonState::Off,
                    value: false,
                    background: CONTROL_BG,
                    background_hover: CONTROL_HOVER,
                    background_active: CONTROL_ACTIVE,
                    foreground: FG2,
                    on_change: Box::new(|pressed, s| {
                        if !pressed {
                            let p = s.player.lock().unwrap();
                            let num_ops = p.instruments[s.selected_instrument].operators.len();
                            s.current_op = (s.current_op + num_ops - 1) % num_ops;
                        }
                    }),
                }) as Box<dyn Element>);
                
                elems.push(Box::new(DynamicLabel {
                    rect: Rect::new(
                        SPECTRUM_WIDTH as i32 + 20,
                        15, 22, 5,
                    ),
                    tooltip: None,
                    colour: FG2,
                    get_text: Box::new(|s| {
                        let p = s.player.lock().unwrap();
                        let num_ops = p.instruments[s.selected_instrument].operators.len();
                        format!("op: {}/{}", s.current_op + 1, num_ops)
                    }),
                }) as Box<dyn Element>);
                
                elems.push(Box::new(Button {
                    rect: Rect::new(
                        SPECTRUM_WIDTH as i32 + 61,
                        14, 7, 7,
                    ),
                    kind: ButtonType::Momentary { label: String::from(">") },
                    state: ButtonState::Off,
                    value: false,
                    background: CONTROL_BG,
                    background_hover: CONTROL_HOVER,
                    background_active: CONTROL_ACTIVE,
                    foreground: FG2,
                    on_change: Box::new(|pressed, s| {
                        if !pressed {
                            let p = s.player.lock().unwrap();
                            let num_ops = p.instruments[s.selected_instrument].operators.len();
                            s.current_op = (s.current_op + 1) % num_ops;
                        }
                    }),
                }) as Box<dyn Element>);
                
                elems.push(Box::new(Button {
                    rect: Rect::new(
                        SPECTRUM_WIDTH as i32 + 69,
                        14, 7, 7,
                    ),
                    kind: ButtonType::Momentary { label: String::from("\x16") },
                    state: ButtonState::Off,
                    value: false,
                    background: CONTROL_BG,
                    background_hover: CONTROL_HOVER,
                    background_active: CONTROL_ACTIVE,
                    foreground: FG2,
                    on_change: Box::new(|_, _| {
                        
                    }),
                }) as Box<dyn Element>);
                
                elems.push(Box::new(Button {
                    rect: Rect::new(
                        SPECTRUM_WIDTH as i32 + 77,
                        14, 7, 7,
                    ),
                    kind: ButtonType::Momentary { label: String::from("+") },
                    state: ButtonState::Off,
                    value: false,
                    background: CONTROL_BG,
                    background_hover: CONTROL_HOVER,
                    background_active: CONTROL_ACTIVE,
                    foreground: FG2,
                    on_change: Box::new(|_, _| {
                        
                    }),
                }) as Box<dyn Element>);
                
                elems.push(Box::new(Label {
                    position: Point::new(
                        SPECTRUM_WIDTH as i32 + 11, 24,
                    ),
                    text: String::from("wave:"),
                    tooltip: None,
                    colour: FG2,
                }) as Box<dyn Element>);
                
                elems.push(Box::new(Stepper {
                    rect: Rect::new(
                        SPECTRUM_WIDTH as i32 + 33, 23, 24, 7,
                    ),
                    value: DynVar::new(
                        |s| {
                            let p = s.player.lock().unwrap();
                            p.instruments[s.selected_instrument]
                                .operators[s.current_op]
                                .transpose as i32
                        },
                        |s, x| {
                            let mut p = s.player.lock().unwrap();
                            p.instruments[s.selected_instrument]
                                .operators[s.current_op]
                                .transpose = x as f32;
                        }
                    ),
                    min_value: 1,
                    max_value: 48000,
                    background: CONTROL_BG,
                    background_hover: CONTROL_HOVER,
                    foreground: FG,
                }) as Box<dyn Element>);
                
                elems.push(Box::new(Choice {
                    rect: Rect::new(
                        SPECTRUM_WIDTH as i32 + 58, 23, 12, 7,
                    ),
                    value: DynVar::new(
                        |s| {
                            let p = s.player.lock().unwrap();
                            p.instruments[s.selected_instrument]
                                .operators[s.current_op]
                                .fixed.into()
                        },
                        |s, x| {
                            let mut p = s.player.lock().unwrap();
                            p.instruments[s.selected_instrument]
                                .operators[s.current_op]
                                .fixed = x == 1;
                        }
                    ),
                    num_values: 2,
                    background: CONTROL_BG,
                    background_hover: CONTROL_HOVER,
                    foreground: FG,
                    make_label: Box::new(|x| {
                        match x {
                            0 => format!("*"),
                            _ => format!("\x17"),
                        }
                    }),
                }) as Box<dyn Element>);
                
                elems.push(Box::new(Choice {
                    rect: Rect::new(
                        SPECTRUM_WIDTH as i32 + 72, 23, 15, 7,
                    ),
                    value: DynVar::new(
                        |s| {
                            let p = s.player.lock().unwrap();
                            p.instruments[s.selected_instrument]
                                .operators[s.current_op]
                                .wave
                                .to_u32()
                        },
                        |s, x| {
                            let mut p = s.player.lock().unwrap();
                            p.instruments[s.selected_instrument]
                                .operators[s.current_op]
                                .wave = synth::WaveType::from(x);
                        }
                    ),
                    num_values: 4,
                    background: CONTROL_BG,
                    background_hover: CONTROL_HOVER,
                    foreground: FG,
                    make_label: Box::new(|x| {
                        match x {
                            0 => format!("\x11"),
                            1 => format!("\x13"),
                            2 => format!("\x12"),
                            _ => format!("\x14"),
                        }
                    }),
                }) as Box<dyn Element>);
                
                for i in 0..synth::NUM_CHANNELS as i32 {
                    let left = SPECTRUM_WIDTH as i32 + 12;
                    let top = ((SPECTRUM_HEIGHT + 2) * 4 + 7) as i32 - 15 * (synth::NUM_CHANNELS as i32 - i - 1) - 1;
                    
                    elems.push(Box::new(Label {
                        position: Point::new(
                            left, top + 1,
                        ),
                        text: format!("{}", i + 1),
                        tooltip: None,
                        colour: FG2,
                    }) as Box<dyn Element>);
                    
                    elems.push(Box::new(Choice {
                        rect: Rect::new(
                            left + 6, top,
                            38, 7,
                        ),
                        value: DynVar::new(
                            move |s| {
                                let p = s.player.lock().unwrap();
                                p.instruments[s.selected_instrument]
                                    .operators[s.current_op]
                                    .connections[i as usize]
                                    .kind
                                    .to_u32()
                            },
                            move |s, n| {
                                let mut p = s.player.lock().unwrap();
                                p.instruments[s.selected_instrument]
                                    .operators[s.current_op]
                                    .connections[i as usize]
                                    .kind = synth::ReceiveKind::from(n);
                            },
                        ),
                        num_values: 3,
                        background: CONTROL_BG,
                        background_hover: CONTROL_HOVER,
                        foreground: FG2,
                        make_label: Box::new(move |n| match n {
                            0 => format!("normal"),
                            1 => format!("modulate"),
                            2 => format!("vibrato"),
                            _ => format!("<error>"),
                        }),
                    }));
                    
                    elems.push(Box::new(Knob {
                        center: Point::new(
                            left + 53,
                            top + 3,
                        ),
                        radius: 5,
                        border_width: 1,
                        background: KNOB_BG,
                        border: KNOB_BORDER,
                        state: ButtonState::Off,
                        min_value: 0.0,
                        max_value: 4.0,
                        value: DynVar::new(
                            move |s| {
                                let p = s.player.lock().unwrap();
                                p.instruments[s.selected_instrument]
                                    .operators[s.current_op]
                                    .connections[i as usize]
                                    .receive
                            },
                            move |s, n| {
                                let mut p = s.player.lock().unwrap();
                                p.instruments[s.selected_instrument]
                                    .operators[s.current_op]
                                    .connections[i as usize]
                                    .receive = n;
                            }),
                        make_tooltip: Box::new(move |x, _s| {
                            format!("recv {:.2}", x)
                        }),
                    }));
                    
                    elems.push(Box::new(Knob {
                        center: Point::new(
                            left + 67,
                            top + 3,
                        ),
                        radius: 5,
                        border_width: 1,
                        background: KNOB_BG,
                        border: KNOB_BORDER,
                        state: ButtonState::Off,
                        min_value: 0.0,
                        max_value: 4.0,
                        value: DynVar::new(
                            move |s| {
                                let p = s.player.lock().unwrap();
                                p.instruments[s.selected_instrument]
                                    .operators[s.current_op]
                                    .connections[i as usize]
                                    .send
                            },
                            move |s, n| {
                                let mut p = s.player.lock().unwrap();
                                p.instruments[s.selected_instrument]
                                    .operators[s.current_op]
                                    .connections[i as usize]
                                    .send = n;
                            }),
                        make_tooltip: Box::new(move |x, _s| {
                            format!("send {:.2}", x)
                        }),
                    }));
                }
                
                elems
            },
            background: PANEL_BG,
            border: Some(BORDER),
            corner: Some(CORNER),
        }));
        
        root.children.push(Box::new(Panel {
            rect: Rect::new(1, (SPECTRUM_HEIGHT + 2) as i32 * 4 + 19,
                SCREEN_WIDTH - 2, SCREEN_HEIGHT - (SPECTRUM_HEIGHT + 2) * 4 - 20),
            children: Vec::from([
                Box::new(Panel {
                    rect: Rect::new(
                        2,
                        (SPECTRUM_HEIGHT + 2) as i32 * 4 + 20,
                        SCREEN_WIDTH - 4,
                        8),
                    children: Vec::from([
                        Box::new(Stepper {
                            rect: Rect::new(
                                3,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 21,
                                15,
                                7,
                            ),
                            value: DynVar::new(
                                |s| s.song.bpm as i32,
                                |s, v| {
                                    s.song.bpm = v as u32;
                                    let mut player = s.player.lock().unwrap();
                                    let old_bps = player.bps;
                                    let new_bps = v as f64 / 60.0;
                                    player.bps = new_bps;
                                    player.playhead /= new_bps / old_bps;
                                },
                            ),
                            min_value: 1,
                            max_value: 999,
                            background: CONTROL_BG,
                            background_hover: CONTROL_HOVER,
                            foreground: FG2,
                        }) as Box<dyn Element>,
                        Box::new(Label {
                            position: Point::new(
                                20,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 22,
                            ),
                            text: String::from("bpm"),
                            tooltip: Some(String::from("beats per minute")),
                            colour: DIM_LABEL,
                        }),
                        Box::new(Stepper {
                            rect: Rect::new(
                                38,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 21,
                                11,
                                7,
                            ),
                            value: DynVar::new(
                                |s| s.song.beats_per_bar as i32,
                                |s, v| s.song.beats_per_bar = v as u32,
                            ),
                            min_value: 1,
                            max_value: 64,
                            background: CONTROL_BG,
                            background_hover: CONTROL_HOVER,
                            foreground: FG2,
                        }) as Box<dyn Element>,
                        Box::new(Label {
                            position: Point::new(
                                50,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 22,
                            ),
                            text: String::from("/4"),
                            tooltip: Some(String::from("time signature")),
                            colour: DIM_LABEL,
                        }),
                        Box::new(Slider {
                            rect: Rect::new(
                                62,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 21,
                                16,
                                7,
                            ),
                            value: DynVar::new(
                                |s|  [1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 96]
                                    .iter()
                                    .position(|x| *x == s.seq_quantize)
                                    .unwrap() as i32,
                                |s, v| {
                                    let old_v = s.seq_scale_x / s.seq_quantize;
                                    s.seq_quantize =  [1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 96][v as usize];
                                    s.seq_scale_x = old_v * s.seq_quantize;
                                },
                            ),
                            min_value: 0,
                            max_value: 11,
                            state: ButtonState::Off,
                            background: CONTROL_BG,
                            track: BORDER,
                            handle: SLIDER_HANDLE,
                            handle_hover: CONTROL_HOVER,
                            handle_active: CONTROL_ACTIVE,
                            make_tooltip: Box::new(|_value, s| {
                                format!("{} per beat", s.seq_quantize)
                            }),
                        }) as Box<dyn Element>,
                        Box::new(Label {
                            position: Point::new(
                                80,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 22,
                            ),
                            text: String::from("QZ."),
                            tooltip: Some(String::from("quanta per beat")),
                            colour: DIM_LABEL,
                        }),
                        Box::new(Button {
                            rect: Rect::new(
                                SCREEN_WIDTH as i32 / 2 - 35,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 21,
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
                                let chan = self.state.note_channel.clone();
                                
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
                                SCREEN_WIDTH as i32 / 2 - 24,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 21,
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
                                let chan = self.state.note_channel.clone();
                                
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
                                SCREEN_WIDTH as i32 / 2 - 13,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 21,
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
                                SCREEN_WIDTH as i32 / 2 + 1,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 21,
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
                                SCREEN_WIDTH as i32 / 2 + 12,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 21,
                                36,
                                7,
                            ),
                            state: ButtonState::Off,
                            value: DynVar::new(
                                |s| (s.player.lock().unwrap().volume * 16.0) as i32,
                                |s, v| s.player.lock().unwrap().volume = v as f32 / 16.0,
                            ),
                            min_value: 0,
                            max_value: 32,
                            background: CONTROL_BG,
                            track: BORDER,
                            handle: SLIDER_HANDLE,
                            handle_hover: CONTROL_HOVER,
                            handle_active: CONTROL_ACTIVE,
                            make_tooltip: Box::new(|val, _s| {
                                format!("volume: {}%", (100.0 * val as f32 / 16.0) as u32)
                            }),
                        }),
                        Box::new(Label {
                            position: Point::new(
                                SCREEN_WIDTH as i32 / 2 + 85,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 22,
                            ),
                            text: String::from("\x08"),
                            tooltip: Some(String::from("vertical scale")),
                            colour: DIM_LABEL,
                        }),
                        Box::new(Slider {
                            rect: Rect::new(
                                SCREEN_WIDTH as i32 / 2 + 89,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 21,
                                24,
                                7,
                            ),
                            state: ButtonState::Off,
                            value: DynVar::new(
                                |s| s.seq_scale_y as i32,
                                |s, v| s.seq_scale_y = v as u32,
                            ),
                            min_value: 2,
                            max_value: 12,
                            background: CONTROL_BG,
                            track: BORDER,
                            handle: SLIDER_HANDLE,
                            handle_hover: CONTROL_HOVER,
                            handle_active: CONTROL_ACTIVE,
                            make_tooltip: Box::new(|val, _s| {
                                format!("vertical scale: {}", val)
                            }),
                        }),
                        Box::new(Label {
                            position: Point::new(
                                SCREEN_WIDTH as i32 / 2 + 117,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 22,
                            ),
                            text: String::from("\x09"),
                            tooltip: Some(String::from("horizontal scale")),
                            colour: DIM_LABEL,
                        }),
                        Box::new(Slider {
                            rect: Rect::new(
                                SCREEN_WIDTH as i32 / 2 + 123,
                                (SPECTRUM_HEIGHT + 2) as i32 * 4 + 21,
                                24,
                                7,
                            ),
                            state: ButtonState::Off,
                            value: DynVar::new(
                                |s| (s.seq_scale_x / s.seq_quantize) as i32,
                                |s, v| s.seq_scale_x = v as u32 * s.seq_quantize,
                            ),
                            min_value: 1,
                            max_value: 11,
                            background: CONTROL_BG,
                            track: BORDER,
                            handle: SLIDER_HANDLE,
                            handle_hover: CONTROL_HOVER,
                            handle_active: CONTROL_ACTIVE,
                            make_tooltip: Box::new(|_val, s| {
                                format!("horizontal scale: {}", s.seq_scale_x)
                            }),
                        })
                    ]),
                    background: PANEL_BG,
                    border: None,
                    corner: None,
                }) as Box<dyn Element>,
                Box::new(Sequencer {
                    rect: Rect::new(
                        1,
                        (SPECTRUM_HEIGHT + 2) as i32 * 4 + 29,
                        SCREEN_WIDTH - 2,
                        SCREEN_HEIGHT - (SPECTRUM_HEIGHT + 2) * 4 - 30),
                    scroll_x: 0.0,
                    scroll_y: 160.0,
                    num_octaves: 9,
                    drag_start: None,
                    drag_end: None,
                    temp_note: None,
                    place_dur: song::BEAT_DIVISIONS,
                    to_delete: None,
                    on_change: {
                        let chan = self.state.note_channel.clone();
                        
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
        
        self.root = Box::new(root);
    }
    
    fn send_event(&mut self, e: InputEvent) {
        self.root.handle(e, &mut self.state);
    }
}
