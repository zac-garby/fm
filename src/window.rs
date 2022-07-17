extern crate sdl2;

use sdl2::event::Event;
use sdl2::render;
use std::time::Duration;

pub const SCREEN_WIDTH: u32 = 300;
pub const SCREEN_HEIGHT: u32 = 250;
pub const SCREN_SCALE: u32 = 4;
pub const REAL_WIDTH: u32 = SCREEN_WIDTH * SCREN_SCALE;
pub const REAL_HEIGHT: u32 = SCREEN_HEIGHT * SCREN_SCALE;

pub struct Window {
    canvas: render::WindowCanvas,
}

impl Window {
    pub fn new(video: sdl2::VideoSubsystem) -> Result<Window, String> {        
        let win = video
            .window("Cancrizans", REAL_WIDTH, REAL_HEIGHT)
            .position_centered()
            .build()
            .map_err(|e| e.to_string())?;
        
        let canvas = win
            .into_canvas()
            .build()
            .map_err(|e| e.to_string())?;
        
        Ok(Window {
            canvas, 
        })
    }
    
    pub fn start(&mut self, sdl: sdl2::Sdl) -> Result<(), String> {
        let mut events = sdl.event_pump()?;
        
        'run:
        loop {
            for e in events.poll_iter() {
                match e {
                    Event::Quit { .. } => break 'run,
                    _ => {}
                }
            }
            
            self.canvas.clear();
            self.canvas.present();
            std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 30));
        }
        
        Ok(())
    }
}