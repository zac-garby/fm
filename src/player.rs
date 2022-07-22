use crate::synth;
use crate::song;

use std::sync::mpsc;

const QUANTIZE: u32 = 256;

/// a player collects together a number of instruments and plays
/// them together, allocating notes to them and handling control
/// signals.
pub struct Player {
    /// how many beats (quarter notes) are played per second?
    pub bps: f64,
    
    /// the volume of the output. 1.0 is normal.
    pub volume: f32,
    
    /// whether the player is muted. a muted player will still
    /// synthesise its instruments, it just won't output them to the
    /// speakers.
    pub mute: bool,
    
    /// whether the player is paused. a paused instrument will not
    /// advance its playhead, and will not run its synthesisers.
    pub paused: bool,
    
    /// the set of instruments which the player owns.
    pub instruments: Vec<synth::Instrument>,
    
    /// the location, in seconds, of the player's playhead.
    pub playhead: f64,
    
    note_recv: mpsc::Receiver<(usize, song::Note)>,
    next_note: Option<(usize, song::Note, f64)>,
    quantize_count: u32,
}

impl Player {
    /// constructs a new player with default settings, and returns a channel
    /// for sending notes. by default, the player is paused.
    pub fn new() -> (Player, mpsc::Sender<(usize, song::Note)>) {
        let (tx_note, rx_note) = mpsc::channel();
        
        (Player {
            bps: 1.0,
            volume: 1.0,
            mute: false,
            paused: true,
            instruments: Vec::new(),
            
            playhead: 0.0,
            note_recv: rx_note,
            next_note: None,
            quantize_count: QUANTIZE,
        }, tx_note)
    }
    
    /// advances the player's instruments and playhead, and returns
    /// the next sample. the playhead is advanced by `dt` seconds.
    pub fn sample(&mut self, dt: f64) -> f32 {
        if self.quantize_count >= QUANTIZE {
            self.quantum();
            self.quantize_count = 0;
        } else {
            self.quantize_count += 1;
        }
        
        let mut s = 0.0;
        
        if !self.paused {
            for instr in &mut self.instruments {
                s += instr.next_output(self.playhead, dt);
            }
    
            self.playhead += dt;
        }
        
        if self.mute {
            0.0
        } else {
            s * self.volume
        }
    }
    
    /// empties the note input stream, so a new song can be started
    /// without interference.
    pub fn flush_notes(&mut self) {
        for _ in self.note_recv.try_iter() {}
        
        self.next_note = None;
        self.quantize_count = QUANTIZE;
        
        for instr in self.instruments.iter_mut() {
            instr.flush();
        }
    }
    
    fn quantum(&mut self) {
        if let Some((i, next, start)) = self.next_note {
            if start <= self.playhead {
                if let Some(instr) = self.instruments.get_mut(i) {
                    instr.schedule(next, self.bps);
                }
                
                self.next_note = None;
            }
        }
        
        if self.next_note.is_none() {
            for (i, note) in self.note_recv.try_iter() {
                let start = note.start_time(self.bps);
                
                if start > self.playhead {
                    self.next_note = Some((i, note, start));
                    break;
                }
                
                if let Some(instr) = self.instruments.get_mut(i) {
                    instr.schedule(note, self.bps.into());
                }
            }
        }
    }
}
