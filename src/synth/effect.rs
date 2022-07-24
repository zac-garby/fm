use std::f64::consts::PI;

/// an effect, used to process a signal sample-by-sample.
pub trait Effect : Send {
    /// processes a single audio sample, and updates the internal state.
    fn process(&mut self, sample: f32) -> f32;
}

pub struct Echo {
    pub amount: f32,
    delay: Delay,
}

impl Echo {
    pub fn new(length: usize, amount: f32) -> Echo {
        Echo {
            amount,
            delay: Delay::new(length, amount),
        }
    }
}

impl Effect for Echo {
    fn process(&mut self, sample: f32) -> f32 {
        self.delay.push(sample * self.amount) + sample
    }
}

pub struct EQ {
    pub biquads: Vec<Biquad>,
}

impl EQ {
    pub fn new() -> EQ {
        EQ {
            biquads: Vec::new(),
        }
    }
}

impl Effect for EQ {
    fn process(&mut self, sample: f32) -> f32 {
        self.biquads
            .iter_mut()
            .fold(sample, |s, eff| eff.process(s))
    }
}

/// a biquad filter, able to take the form of many LTI filters including
/// the filters required for EQ (low-pass, high-pass, etc.)
#[derive(Clone, Copy)]
pub struct Biquad {
    pub a: [f64; 3],
    pub b: [f64; 3],
    x: [f64; 3],
    y: [f64; 3],
}

impl Biquad {
    fn from(a0: f64, a1: f64, a2: f64, b0: f64, b1: f64, b2: f64) -> Biquad {
        Biquad {
            a: [1.0, a1 / a0, a2 / a0],
            b: [b0 / a0, b1 / a0, b2 / a0],
            x: [0.0, 0.0, 0.0],
            y: [0.0, 0.0, 0.0],
        }
    }
    
    pub fn lowpass(hz: f64, q: f64, dt: f64) -> Biquad {
        let w = 2.0 * PI * hz * dt;
        let alpha = w.sin() / (2.0 * q);
        let cos_w = w.cos();
        
        Biquad::from(
            1.0 + alpha,
            -2.0 * cos_w,
            1.0 - alpha,
            
            (1.0 - cos_w) / 2.0,
            1.0 - cos_w,
            (1.0 - cos_w) / 2.0,
        )
    }
    
    pub fn highpass(hz: f64, q: f64, dt: f64) -> Biquad {
        let w = 2.0 * PI * hz * dt;
        let alpha = w.sin() / (2.0 * q);
        let cos_w = w.cos();
        
        Biquad::from(
            1.0 + alpha,
            -2.0 * cos_w,
            1.0 - alpha,
            
            (1.0 + cos_w) / 2.0,
            -1.0 - cos_w,
            (1.0 + cos_w) / 2.0,
        )
    }
    
    pub fn peak(hz: f64, g: f64, scale: f64, dt: f64) -> Biquad {
        let sqrt_gain = g.sqrt();
        let w = 2.0 * PI * hz * dt;
        let cos_w = w.cos();
        let bandwidth = scale * w / (if sqrt_gain >= 1.0 { sqrt_gain } else { 1.0 / sqrt_gain });
        let alpha = (bandwidth / 2.0).tan();
        
        Biquad::from(
            1.0 + alpha / sqrt_gain,
            -2.0 * cos_w,
            1.0 - alpha / sqrt_gain,
            
            1.0 + alpha * sqrt_gain,
            -2.0 * cos_w,
            1.0 - alpha * sqrt_gain,
        )
    }
}

impl Effect for Biquad {
    fn process(&mut self, sample: f32) -> f32 {
        self.x[2] = self.x[1];
        self.x[1] = self.x[0];
        self.x[0] = sample as f64;
        
        let y0
            = self.b[0] * self.x[0]
            + self.b[1] * self.x[1]
            + self.b[2] * self.x[2]
            - self.a[1] * self.y[0]
            - self.a[2] * self.y[1];
        
        self.y[2] = self.y[1];
        self.y[1] = self.y[0];
        self.y[0] = y0;
        
        y0 as f32
    }
}

/// a delay line, used for numerous effects
struct Delay {
    /// the internal state of the delay line, implemented as a circular buffer.
    /// the buffer grows backwards, so line[head] is the next element and line[head+1]
    /// is the one after that.
    line: Vec<f32>,
    
    /// the location of the next item in the delay line to return.
    head: usize,
    
    /// the update ratio - the proportion of the new sample in the delay line which is
    /// the actual new sample.
    ratio: f32,
}

impl Delay {
    fn new(length: usize, ratio: f32) -> Self {
        Delay {
            line: std::iter::repeat(Default::default()).take(length).collect(),
            head: 0,
            ratio,
        }
    }
    
    fn len(&self) -> usize {
        self.line.len()
    }
    
    fn push(&mut self, sample: f32) -> f32 {
        let out = self.line[self.head];
        self.line[self.head] = out * (1.0 - self.ratio) + sample * self.ratio;
        self.head = (self.head + self.len() - 1) % self.len();
        out
    }
}