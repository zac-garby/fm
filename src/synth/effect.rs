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