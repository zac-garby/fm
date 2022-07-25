use std::f64::consts::PI;

/// an effect, used to process a signal sample-by-sample.
pub trait Effect : Send {
    /// processes a single audio sample, and updates the internal state.
    fn process(&mut self, sample: f32) -> f32;
    
    /// resets an effect's internal state.
    fn reset(&mut self);
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

    fn reset(&mut self) {
        self.delay.reset();
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

    fn reset(&mut self) {
        self.biquads.iter_mut().for_each(|b| b.reset());
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
    
    pub fn allpass() -> Biquad {
        Biquad::from(1.0, 1.0, 1.0, 1.0, 1.0, 1.0)
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

    fn reset(&mut self) {
        self.x = [0.0, 0.0, 0.0];
        self.y = [0.0, 0.0, 0.0];
    }
}

pub struct Reverb {
    fdn: FeedbackDelayNetwork,
    pub in_gain: [f32; 4],
    pub out_gain: [f32; 4],
    pub mix: f32,
}

impl Reverb {
    pub fn new(mix: f32, gain: f32) -> Reverb {
        Reverb {
            fdn: {
                let mut fdn = FeedbackDelayNetwork::new(3041, 3385, 4481, 5477);
                
                for i in 0..4 {
                    fdn.feedback_filter[i] = Biquad::lowpass(5600.0, 0.707, 1.0 / 44100.0);
                    fdn.feedback_gain[i] = gain;
                }
                
                fdn
            },
            in_gain: [ 0.4, 0.3, 0.2, 0.2 ],
            out_gain: [ 0.5, 0.5, 0.3, 0.1 ],
            mix,
        }
    }
}

impl Effect for Reverb {
    fn process(&mut self, sample: f32) -> f32 {
        self.fdn.run([
            sample * self.in_gain[0],
            sample * self.in_gain[1],
            sample * self.in_gain[2],
            sample * self.in_gain[3],
        ]);
        
        let out: f32 = (0..4)
            .map(|i| self.fdn.output[i] * self.out_gain[i])
            .sum();
        
        self.mix * out + (1.0 - self.mix) * sample
    }

    fn reset(&mut self) {
        self.fdn.reset();
    }
}

/// a four-channel feedback-delay-network. four inputs are given at each frame,
/// and are delayed by different amounts. feedback from these delays is taken and
/// added to the inputs before feeding into the delay lines.
struct FeedbackDelayNetwork {
    /// the final gain of the four feedback lines, before adding with the new samples.
    feedback_gain: [f32; 4],
    
    /// the FDN's feedback matrix, aka "Q" in literature. used to mix channels together.
    feedback_matrix: [[f32; 4]; 4],
    
    /// the filters to process each sample before feeding back into the delay lines.
    feedback_filter: [Biquad; 4],
    
    /// the four delay lines.
    delays: [Delay; 4],
    
    /// the latest output from each FDN channel.
    output: [f32; 4],
}

impl FeedbackDelayNetwork {
    fn new(l0: usize, l1: usize, l2: usize, l3: usize) -> FeedbackDelayNetwork {
        FeedbackDelayNetwork {
            feedback_gain: [1.0, 1.0, 1.0, 1.0],
            feedback_matrix: FeedbackDelayNetwork::hadamard(),
            feedback_filter: [Biquad::lowpass(10000.0, 0.517, 1.0 / 44100.0); 4],
            delays: [
                Delay::new(l0, 0.0),
                Delay::new(l1, 0.0),
                Delay::new(l2, 0.0),
                Delay::new(l3, 0.0),
            ],
            output: [0.0, 0.0, 0.0, 0.0],
        }
    }
    
    fn run(&mut self, x: [f32; 4]) {
        let mut fb: [f32; 4] = [0.0, 0.0, 0.0, 0.0];
        
        for i in 0..4 {
            self.output[i] = self.delays[i].peek();
        }
        
        for i in 0..4 {
            fb[i] =
                ( self.feedback_matrix[i][0] * self.output[0]
                + self.feedback_matrix[i][1] * self.output[1]
                + self.feedback_matrix[i][2] * self.output[2]
                + self.feedback_matrix[i][3] * self.output[3] )
                * self.feedback_gain[i];
            
            fb[i] = self.feedback_filter[i].process(fb[i]);
        }
        
        for i in 0..4 {
            self.delays[i].push(fb[i] + x[i]);
        }
    }
    
    fn hadamard() -> [[f32; 4]; 4] {
        [
            [ 0.5,  0.5,  0.5,  0.5],
            [-0.5,  0.5, -0.5,  0.5],
            [-0.5, -0.5,  0.5,  0.5],
            [ 0.5, -0.5, -0.5,  0.5],
        ]
    }
    
    fn reset(&mut self) {
        self.delays.iter_mut().for_each(|d| d.reset());
        self.output = [0.0; 4];
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
    
    fn push(&mut self, sample: f32) -> f32 {
        let out = self.line[self.head];
        self.line[self.head] = out * self.ratio + sample;
        self.head = (self.head + self.len() - 1) % self.len();
        out
    }
    
    fn peek(&self) -> f32 {
        self.line[self.head]
    }
    
    fn len(&self) -> usize {
        self.line.len()
    }
}

impl Effect for Delay {
    fn process(&mut self, sample: f32) -> f32 {
        self.push(sample)
    }

    fn reset(&mut self) {
        self.line.iter_mut().for_each(|x| *x = 0.0);
        self.head = 0;
    }
}