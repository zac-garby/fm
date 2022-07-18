use crate::synth;
use crate::song;

use std::sync::mpsc;

pub struct Player {
    pub bps: f64,
    pub volume: f32,
    pub mute: bool,
    pub instruments: Vec<synth::Instrument>,
    
    pub playhead: f64,
    note_recv: mpsc::Receiver<(usize, song::Note)>,
    next_note: Option<(usize, song::Note, f64)>,
}

impl Player {
    pub fn new() -> (Player, mpsc::Sender<(usize, song::Note)>) {
        let (tx, rx) = mpsc::channel();
        
        (Player {
            bps: 1.0,
            volume: 1.0,
            mute: false,
            instruments: Vec::new(),
            
            playhead: 0.0,
            note_recv: rx,
            next_note: None,
        }, tx)
    }
    
    pub fn sample(&mut self, dt: f64) -> f32 {
        if let Some((i, next, start)) = &self.next_note {
            if *start <= self.playhead {
                self.instruments.get_mut(*i).map(|i| i.schedule(*next, self.bps));
                self.next_note = None;
            }
        }
        
        if self.next_note.is_none() {
            for (i, note) in self.note_recv.try_iter() {
                let start = note.start_time(self.bps);
            
                if note.start_time(self.bps) > self.playhead {
                    self.next_note = Some((i, note, start));
                    break;
                }
            
                if let Some(instr) = self.instruments.get_mut(i) {
                    instr.schedule(note, self.bps.into());
                }
            }
        }
        
        let mut s = 0.0;
        
        for instr in &mut self.instruments {
            s += instr.next_output(self.playhead, dt);
        }

        self.playhead += dt;
        
        if self.mute {
            0.0
        } else {
            s * self.volume
        }
    }
}
