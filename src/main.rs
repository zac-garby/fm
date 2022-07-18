extern crate cpal;

pub mod song;
pub mod synth;
pub mod player;
pub mod window;

use std::sync::{Arc, Mutex, mpsc};
use std::thread;

use cpal::traits::{HostTrait, DeviceTrait, StreamTrait};

use song::*;
use synth::*;
use player::*;
use window::*;

pub fn main() -> anyhow::Result<()> {
    let sdl = sdl2::init()
        .expect("could not initialise SDL2");
    
    let video = sdl.video()
        .expect("video could not be initialised");
    
    let mut s = Song::new(1, 60, 4);
    
    s.add_note(0, Note::new(24, 0, 0, 90, 1.0));
    s.add_note(0, Note::new(29, 1, 0, 90, 1.0));
    s.add_note(0, Note::new(31, 2, 0, 90, 0.9));
    
    let (mut player, note_channel) = Player::new();
    player.volume = 0.5;
    player.instruments.push(make_organ());
    
    s.sequence(note_channel);
    
    let player_mutex = Arc::new(Mutex::new(player));
    
    let host = cpal::default_host();
    let device = host.default_output_device()
        .expect("No audio output device found");
    
    let tx = start_audio(device, player_mutex.clone())?;

    let mut win = Window::new(video, player_mutex.clone()).expect("could not create window");
    win.start(sdl).expect("could not start window");
    
    tx.send(())?;
    
    Ok(())
}

fn make_organ() -> Instrument {
    let mut instr = Instrument::new(8);
    
    for i in 0..4 {
        // carrier
        Operator
            ::new(WaveType::Sine, false, 2.0f32.powi(i as i32))
            .env(0.05, 0.2, 1.0, 0.1)
            .recv(i + 1, 4.0, ReceiveKind::Modulate)
            .send(0, 0.4 - i as f32 / 12.0)
            .add(&mut instr);
        
        // feedback
        Operator
            ::new(WaveType::Sine, false, 2.0f32.powi(i as i32))
            .env(0.05, 0.2, 1.0, 0.35)
            .recv(i + 1, 0.45 + i as f32 / 15.0, ReceiveKind::Normal)
            .send(i + 1, 2.0)
            .add(&mut instr);
    }
    
    instr
}

fn start_audio(device: cpal::Device, player: Arc<Mutex<Player>>)
  -> anyhow::Result<mpsc::Sender<()>> {
    let config = device.default_output_config()?;
    println!("Using {}", device.name()?);
    
    let (tx, rx) = mpsc::channel();
    
    thread::spawn(move || {
        let stream = match config.sample_format() {
            cpal::SampleFormat::F32 => make_stream::<f32>(&device, &config.into(), player),
            cpal::SampleFormat::I16 => make_stream::<i16>(&device, &config.into(), player),
            cpal::SampleFormat::U16 => make_stream::<u16>(&device, &config.into(), player),
        }.expect("could not make stream");
        
        stream.play().expect("could not play stream");
        rx.recv().expect("failed to receive stop message");
    });
    
    Ok(tx)
}

fn make_stream<T>(device: &cpal::Device, config: &cpal::StreamConfig, player: Arc<Mutex<Player>>)
  -> anyhow::Result<cpal::Stream>
where T : cpal::Sample {
    let sample_rate = config.sample_rate.0 as f32;
    let dt = 1.0 / sample_rate as f64;
    let channels = config.channels as usize;
        
    println!("Sample rate: {}Hz", sample_rate);
        
    let err_fn = |err| eprintln!("an error occurred: {}", err);
    
    let stream = device.build_output_stream(
        config,
        move |data: &mut [T], _: &cpal::OutputCallbackInfo| {
            write_data(data, channels, dt, player.clone());
        },
        err_fn)?;
    
    Ok(stream)
}

fn write_data<T>(output: &mut [T], channels: usize, dt: f64, player: Arc<Mutex<Player>>)
where
    T: cpal::Sample,
{
    let mut p = player.lock().unwrap();
    
    for frame in output.chunks_mut(channels) {
        let value: T = cpal::Sample::from::<f32>(&p.sample(dt));
        for sample in frame.iter_mut() {
            *sample = value;
        }
    }
}