# fm

This is a barely functional [FM synthesizer](https://en.wikipedia.org/wiki/Frequency_modulation_synthesis).

The goal is to have a synth which can immitate lots of different instruments, and a sequencer to play it. MIDI input should be pretty straightforward too, since I'm using libsoundio.

## Installation

First, you'll need to install a few libraries:

 - [libsoundio](http://libsound.io)
 - [kissfft](https://github.com/mborgerding/kissfft)
 - [SDL](https://www.libsdl.org)
 
libsoundio and SDL are easy enough to install through package managers usually, but kissfft doesn't have very good instructions so this is what I'd suggest:

```sh
git clone https://github.com/mborgerding/kissfft
cd kissfft
make install

# if on macOS or BSD, the install stage will fail halfway through and you'll have to use this instead:
sudo install -m 644 kiss_fft.h kissfft.hh kiss_fftnd.h kiss_fftndr.h kiss_fftr.h /usr/local/include/kissfft
sudo install -m 644 libkissfft-float.dylib /usr/local/lib
```

Then, to build the project it's as easy as typing `make fm`.

`make run` will build it and then run the executable.
