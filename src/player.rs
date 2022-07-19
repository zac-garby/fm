use crate::synth;
use crate::song;

use std::sync::mpsc;

const QUANTIZE: u32 = 256;

/// a signal which, when sent to a player's control signal, will
/// have some effect.
pub enum ControlSignal {
    /// toggles between playing and paused states.
    PlayPause,
    
    /// toggles between muted and unmuted states.
    MuteUnmute,
    
    /// sets the player's volume.
    Volume(f32),
}

pub struct Player {
    pub bps: f64,
    pub volume: f32,
    pub mute: bool,
    pub paused: bool,
    pub instruments: Vec<synth::Instrument>,
    
    pub playhead: f64,
    note_recv: mpsc::Receiver<(usize, song::Note)>,
    signals: mpsc::Receiver<ControlSignal>,
    next_note: Option<(usize, song::Note, f64)>,
    quantize_count: u32,
}

impl Player {
    pub fn new() -> (Player,
                     mpsc::Sender<(usize, song::Note)>,
                     mpsc::Sender<ControlSignal>) {
        let (tx_note, rx_note) = mpsc::channel();
        let (tx_sig, rx_sig) = mpsc::channel();
        
        (Player {
            bps: 2.0,
            volume: 1.0,
            mute: false,
            paused: true,
            instruments: Vec::new(),
            
            playhead: 0.0,
            note_recv: rx_note,
            signals: rx_sig,
            next_note: None,
            quantize_count: QUANTIZE,
        }, tx_note, tx_sig)
    }
    
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
    
    fn quantum(&mut self) {
        for sig in self.signals.try_iter() {
            match sig {
                ControlSignal::PlayPause => {
                    self.paused = !self.paused
                },
                ControlSignal::MuteUnmute => {
                    self.mute = !self.mute
                },
                ControlSignal::Volume(vol) => {
                    self.volume = vol;
                }
            }
        }
        
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
    }
}
