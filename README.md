# 6bit PWM magic

Code Works with 5x 884 Ohm (10x 1800) and 7x 1800 Ohm R2R bridge based on D2 (till D7). No final RC needed for good retro sound!

Do you want to upload it normal and not via usbasp? You have to comment out the "upload_" parts in `platformio.ini`.

## Features

- velocity (64 steps)
- pitchbend (2 half tones)
- sustain
- hold and release
- 12 voices
- A0 to GND and all is 1 octave deeper.
- A1 to GND for toggle into aerodyn mode (A2 to A5 with LEDs)
- A5 is high for 300ms, if MIDI device is found