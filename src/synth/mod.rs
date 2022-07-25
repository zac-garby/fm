pub mod effect;

use std::f32::consts::PI;

use crate::song;

pub const NUM_CHANNELS: usize = 8;
pub const MAX_OPERATORS: usize = 8;
pub const HOLD_BUFFER_SIZE: usize = 256;

#[derive(Clone)]
/// the type of wave for a synth to play. the default and most
/// common is a sine wave, but others are available.
pub enum WaveType {
    Sine,
    Square,
    Triangle,
    Sawtooth,
}

#[derive(Clone)]
/// the way in which a receipt from a channel should be computed.
/// normally, the channel is simply added to the phase (after being
/// multiplied by the delta time), but for modulation, it is also
/// multiplied by the note's base frequency.
pub enum ReceiveKind {
    /// normal receiving (e.g. for feedback, or combining signals.)
    Normal,
    
    /// frequency modulation.
    Modulate,
    
    /// vibrato. functionally identical to normal.
    Vibrato,
}

#[derive(Clone)]
/// a note which has been placed into a synth and is currently being
/// played. mostly differs from a `Note` by having its frequency calculated.
struct PlayedNote {
    pitch: u32,
    freq: f32,
    velocity: f32,
    start: f64,
    duration: f64,
}

impl PlayedNote {
    fn from(note: song::Note, bps: f64) -> PlayedNote {
        PlayedNote {
            pitch: note.pitch,
            freq: note.freq(),
            velocity: note.velocity,
            start: note.start_time(bps),
            duration: note.real_duration(bps),
        }
    }
}

#[derive(Clone)]
pub struct Receive {
    channel: usize,
    level: f32,
    kind: ReceiveKind,
}

#[derive(Clone)]
pub struct Send {
    channel: usize,
    level: f32,
}

/// a synthesiser for a single voice in an instrument.
#[derive(Clone)]
pub struct Voice {
    channels: [f32; NUM_CHANNELS],
    channels_back: [f32; NUM_CHANNELS],
    phases: [f32; MAX_OPERATORS],
    note: PlayedNote,
}

/// collects together a number of identical synths, and allocates
/// notes between them. each voice acts upon the same hold buffer.
pub struct Instrument {
    voices: Vec<Voice>,
    operators: Vec<Operator>,
    effects: Vec<Box<dyn effect::Effect>>,
    
    pub hold_buf: [f32; HOLD_BUFFER_SIZE],
    hold_buf_back: [f32; HOLD_BUFFER_SIZE],
    hold_index: usize,
}

/// an FM operator, which sends to and receives from channels, and outputs a frequency.
#[derive(Clone)]
pub struct Operator {
    /// the recieves into the operator.
    pub receives: Vec<Receive>,
    
    /// the sends from the operator.
    pub sends: Vec<Send>,
    
    /// whether the frequency is fixed, or dependent upon the note being played.
    pub fixed: bool,
    
    /// the type of wave to produce.
    pub wave: WaveType,
    
    /// a frequency scaling factor. or, if `fixed`, the oscillator's base frequency.
    pub transpose: f32,
    
    /// the envelope which the operator's output amplitude follows.
    pub envelope: Envelope,
}

/// a parameterised ADSR envelope.
#[derive(Clone)]
pub struct Envelope {
    pub attack: f32,
    pub decay: f32,
    pub sustain: f32,
    pub release: f32,
}

impl Instrument {
    pub fn new(num_voices: usize) -> Instrument {
        Instrument {
            voices: vec![Voice::new(); num_voices],
            operators: Vec::new(),
            effects: vec![
                Box::new(effect::Reverb::new(0.8, 0.95)),
                // Box::new(effect::Biquad::lowpass(1000.0, 1.0 / SQRT_2, 1.0 / 44100.0)),
                // Box::new(effect::Biquad::highpass(250.0, 1.0 / SQRT_2, 1.0 / 44100.0)),
                // Box::new(effect::Biquad::peak(440.0, 1.0 / SQRT_2, 3.0, 1.0 / 44100.0))
            ],
            
            hold_buf: [0.0; HOLD_BUFFER_SIZE],
            hold_buf_back: [0.0; HOLD_BUFFER_SIZE],
            hold_index: HOLD_BUFFER_SIZE,
        }
    }
    
    /// places a note into the "best" voice and, if it is to be played at
    /// the current time, starts to play it.
    pub fn schedule(&mut self, note: song::Note, bps: f64) {        
        let mut best_finish = f64::MAX;
        let mut best_index = 0;
        
        for (n, voice) in self.voices.iter().enumerate() {
            let finish = voice.note.start + voice.note.duration;
            
            if voice.note.pitch == note.pitch {
                best_index = n;
                break;
            }
            
            if finish < best_finish {
                best_finish = finish;
                best_index = n;
            }
        }
        
        self.voices[best_index].note = PlayedNote::from(note, bps);
    }
    
    /// flushes all notes from the instrument's voices.
    pub fn flush(&mut self) {
        for voice in self.voices.iter_mut() {
            voice.note.freq = 0.0;
            voice.note.pitch = 0;
            voice.note.duration = 0.0;
            voice.note.start = -0.0;
            voice.note.velocity = 0.0;
        }
        
        for effect in self.effects.iter_mut() {
            effect.reset();
        }
    }
    
    pub fn add_operator(&mut self, op: Operator) {
        if self.operators.len() < MAX_OPERATORS {
            self.operators.push(op);
        }
    }
    
    pub fn next_output(&mut self, time: f64, dt: f64) -> f32 {
        if self.hold_index >= HOLD_BUFFER_SIZE {
            self.fill_hold_buffer(time, dt);
        }
        
        let out = self.hold_buf[self.hold_index];
        self.hold_index += 1;
        
        out
    }
    
    fn fill_hold_buffer(&mut self, time: f64, dt: f64) {
        for i in 0..HOLD_BUFFER_SIZE {
            self.hold_buf_back[i] = 0.0;
        }
        
        for voice in &mut self.voices {
            voice.fill_hold_buffer(time, dt, &mut self.hold_buf_back, &self.operators);
        }
        
        for i in 0..HOLD_BUFFER_SIZE {
            self.hold_buf_back[i] = self.effects
                .iter_mut()
                .fold(self.hold_buf_back[i], |s, eff| eff.process(s));
        }
        
        self.swap_buffers();
        self.hold_index = 0;
    }
    
    fn swap_buffers(&mut self) {
        std::mem::swap(&mut self.hold_buf, &mut self.hold_buf_back);
    }
}

impl Voice {
    pub fn new() -> Voice {
        Voice {
            channels: [0.0; NUM_CHANNELS],
            channels_back: [0.0; NUM_CHANNELS],
            phases: [0.0; MAX_OPERATORS],
            note: PlayedNote {
                pitch: 0,
                freq: 0.0,
                duration: 0.0,
                velocity: 0.0,
                start: 0.0,
            }
        }
    }
    
    #[inline]
    fn fill_hold_buffer(&mut self, time: f64, dt: f64,
        buf: &mut [f32; HOLD_BUFFER_SIZE], ops: &Vec<Operator>) {
        let mut t = time;
        
        for frame in 0..HOLD_BUFFER_SIZE {
            self.frame(t, dt, ops);
            buf[frame] += self.channels[0];
            t += dt;
        }
    }
        
    #[inline]
    fn frame(&mut self, time: f64, dt: f64, ops: &Vec<Operator>) {
        for (i, op) in ops.iter().enumerate() {
            for recv in &op.receives {
                let modulation = match recv.kind {
                    ReceiveKind::Normal | ReceiveKind::Vibrato
                    => self.channels[recv.channel] * recv.level * dt as f32,
                    ReceiveKind::Modulate
                    => self.channels[recv.channel] * recv.level * dt as f32 * self.note.freq
                };
                
                self.phases[i] += modulation;
            }
            
            while self.phases[i] > 2.0 * PI {
                self.phases[i] -= 2.0 * PI;
            }
            
            // if the pitch is 0, the note hasn't been set yet.
            let sample = if self.note.pitch > 0 {
                let env = op.envelope.evaluate((time - self.note.start) as f32, self.note.duration as f32);
                let vel = env * self.note.velocity;
                
                let f = if op.fixed {
                    op.transpose
                } else {
                    self.note.freq * op.transpose
                } as f64;
                
                let t = f * time + self.phases[i] as f64;
                
                vel * match op.wave {
                    WaveType::Sine => -f64::cos(2.0 * std::f64::consts::PI * t) as f32,
                    WaveType::Square => 2.0 * ((2.0 * t) as u32 % 2) as f32 - 1.0,
                    WaveType::Triangle => 1.0 - 2.0 * f32::abs(2.0 * (t - t.floor()) as f32 - 1.0),
                    WaveType::Sawtooth => (t - t.floor()) as f32,
                }
            } else {
                0.0
            };
            
            for send in &op.sends {
                self.channels_back[send.channel] += send.level * sample;
            }
        }
        
        self.swap_buffers();
    }
    
    fn swap_buffers(&mut self) {
        std::mem::swap(&mut self.channels, &mut self.channels_back);
        self.channels_back = [0.0; NUM_CHANNELS];
    }
}

impl Operator {
    pub fn new(wave: WaveType, fixed: bool, transpose: f32) -> Operator {
        Operator {
            receives: Vec::new(),
            sends: Vec::new(),
            fixed,
            wave,
            transpose,
            envelope: Envelope { attack: 0.0, decay: 0.0, sustain: 1.0, release: 0.0 }
        }
    }
    
    pub fn wave(&mut self, wave: WaveType) -> &mut Operator {
        self.wave = wave;
        self
    }
    
    pub fn send(&mut self, channel: usize, level: f32) -> &mut Operator {
        self.sends.push(Send { channel, level });
        self
    }
    
    pub fn recv(&mut self, channel: usize, level: f32, kind: ReceiveKind) -> &mut Operator {
        self.receives.push(Receive { channel, level, kind });
        self
    }
    
    pub fn env(&mut self, attack: f32, decay: f32, sustain: f32, release: f32) -> &mut Operator {
        self.envelope = Envelope {
            attack, decay, sustain, release,
        };
        
        self
    }
    
    pub fn add(&mut self, instr: &mut Instrument) {
        instr.operators.push(self.clone());
    }
}

impl Envelope {
    pub fn evaluate(&self, t: f32, hold_time: f32) -> f32 {
        if t < 0.0 || t >= hold_time + self.release {
            return 0.0;
        }
        
        if self.attack < 0.0 {
            return 1.0;
        }
        
        if hold_time < self.attack + self.decay && t >= hold_time {
            let rf = 1.0 - (t - hold_time) / self.release;
            
            if hold_time < self.attack {
                return (hold_time / self.attack) * rf;
            } else {
                return (1.0 - ((1.0 - self.sustain) * (hold_time - self.attack)) / self.decay) * rf;
            }
        } else {
            if t < self.attack {
                return t / self.attack;
            } else if t < self.attack + self.decay {
                return 1.0 - ((1.0 - self.sustain) * (t - self.attack)) / self.decay;
            } else if t < hold_time {
                return self.sustain;
            } else if t < hold_time + self.release {
                return self.sustain * (1.0 - (t - hold_time) / self.release);
            }
            
            0.0
        }
    }
}
