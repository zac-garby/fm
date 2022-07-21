use std::sync::mpsc;

pub const BEAT_DIVISIONS: u32 = 96;
pub const NUM_PARTS: usize = 4;
pub const C0: f32 = 16.35159783;

pub const NOTE_NAMES: [&str; 12] = [
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
];

/// a particular point of time, quantized as a moment in a song.
#[derive(Copy, Clone)]
pub struct Time {
    pub beat: u32,
    pub division: u32,
}

impl Time {
    pub fn new(beat: u32, division: u32) -> Time {
        Time { beat, division }
    }
    
    pub fn add(&self, divisions: u32) -> Time {
        Time {
            beat: self.beat + divisions / BEAT_DIVISIONS,
            division: self.division + divisions % BEAT_DIVISIONS,
        }
    }
    
    pub fn diff(&self, t: Time) -> i32 {
        self.as_divs() as i32 - t.as_divs() as i32
    }
    
    pub fn as_divs(&self) -> u32 {
        self.beat * BEAT_DIVISIONS + self.division
    }
}

/// an individual note in a song.
#[derive(Copy, Clone)]
pub struct Note {
    /// the pitch of the note. C0 is represented as 0, and each successive
    /// pitch goes up by one semitone.
    /// the frequency is therefore: C0 * 2.0^(pitch / 12.0)
    pub pitch: u32,
    
    /// the start time of the note.
    pub start: Time,
    
    /// the duration of the note, as a number of divisions. a quarter-note
    /// is represented by BEAT_DIVISIONS (i.e. one whole beat.)
    pub duration: u32,
    
    /// the velocity (loudness) of the note.
    pub velocity: f32
}

/// a song, consisting of NUM_PARTS parts, each of which are a list of
/// notes to be played.
#[derive(Clone)]
pub struct Song {
    /// the beats-per-minute of the song.
    pub bpm: u32,
    bps: f64,
    
    /// the amount of beats in a bar (e.g. 4 for a 4/4 piece, or 3 for a 3/4.) a
    /// beat is always a quarter note.
    pub beats_per_bar: u32,
    
    /// the parts making up the song, each represented as a vector of notes.
    pub parts: Vec<Vec<Note>>,
}

impl Note {
    pub fn new(pitch: u32, beat: u32, division: u32, duration: u32, velocity: f32) -> Note {
        Note { pitch, start: Time::new(beat, division), duration, velocity }
    }
    
    /// get the real frequency of the note (as opposed to its pitch) in Hz.
    pub fn freq(&self) -> f32 {
        C0 * (2.0 as f32).powf((self.pitch as f32) / 12.0)
    }
    
    /// get the time (in seconds) at which the note should start to play.
    pub fn start_time(&self, bps: f64) -> f64 {
        ((self.start.beat as f64) + (self.start.division as f64) / (BEAT_DIVISIONS as f64)) / bps
    }
    
    /// get the time (in seconds) at which the note should finish playing.
    pub fn end_time(&self, bps: f64) -> f64 {
        self.start_time(bps) + self.real_duration(bps)
    }
    
    /// get the duration (in seconds) of the note.
    pub fn real_duration(&self, bps: f64) -> f64 {
        ((self.duration as f64) / BEAT_DIVISIONS as f64) / bps
    }
    
    /// check if two notes overlap.
    pub fn overlap(&self, note: Note) -> bool {
        let s1 = self.start.as_divs();
        let e1 = s1 + self.duration;
        let s2 = note.start.as_divs();
        let e2 = s2 + note.duration;
        
        (s1 < e2 && e1 > s2) || (s2 < e1 && e2 > s1)
    }
    
    /// check if the note contains a particular time.
    pub fn contains(&self, time: Time) -> bool {
        time.as_divs() >= self.start.as_divs() && time.as_divs() < self.start.as_divs() + self.duration
    }
    
    /// gets the name of the note's pitch (e.g. "C#")
    pub fn name(&self) -> &str {
        NOTE_NAMES[(self.pitch % 12) as usize]
    }
    
    /// gets the octave which the note is in
    pub fn octave(&self) -> u32 {
        self.pitch / 12
    }
}

impl Song {
    pub fn new(num_parts: usize, bpm: u32, beats_per_bar: u32) -> Song {
        let mut parts = Vec::new();
        for _ in 0..num_parts {
            parts.push(Vec::new());
        }
        
        Song {
            bpm,
            bps: bpm as f64 / 60.0,
            beats_per_bar,
            parts,
        }
    }
    
    /// appends a note to a given part in the song.
    pub fn add_note(&mut self, part: usize, note: Note) {
        self.parts[part].push(note);
    }
    
    /// calculates the total duration of the song, in seconds.
    pub fn duration(&self, bps: f64) -> f64 {
        self.parts.iter().filter_map(|p| p.last().map(|n| n.end_time(bps))).reduce(f64::max).unwrap_or(0.0)
    }
    
    /// gets the song's BPS, or beats per second.
    pub fn get_bps(&self) -> f64 {
        self.bps
    }
    
    /// sends all of the notes in a song to a player's note channel.
    pub fn sequence(&self, chan: mpsc::Sender<(usize, Note)>) {
        // collect all notes from all parts into one vector, and note their
        // start times.
        let mut notes = self.parts
            .iter()
            .enumerate()
            .map(|(i, part)| part.iter().map(move |n| (i, n, n.start_time(self.bps))))
            .flatten()
            .collect::<Vec<(usize, &Note, f64)>>();
        
        // sort all of the notes by their start time.
        notes.sort_by(|(_, _, a), (_, _, b)| a.partial_cmp(b).unwrap());
        
        // play the notes in the order in which they begin.
        notes.iter().for_each(|(i, note, _)| chan.send((*i, **note))
                                                 .expect("could not send note"));
    }
}