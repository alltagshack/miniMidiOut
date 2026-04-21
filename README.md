# 6bit PWM magic

Code Works with 5x 884 Ohm (10x 1800) and 7x 1800 Ohm R2R bridge
based on D2 (till D7). No final RC needed for good retro sound!

Do you want to upload it normal and not via usbasp? You have
to comment out the "upload_" parts in `platformio.ini`.

## Features

midi

- velocity (64 steps = 6bit)
- pitchbend (2 half tones)
- sustain
- hold and release
- multiple voices

hardware

- octave switch down (A3 to GND)
- additional sustain (A2 to GND)
- additional pitchbend (5V..A0..GND)
- A1 is HIGH, if pitchbend is neutral

## new ideas

- 7bit velocity
- hall button (NO)
- modulation control
  - change in amplitude or frequency
  - change its speed (?)

## bugs

- very 5V sensitive: sometimes MAX USB chip needs minutes or do not match to find device
- inital find USB-Client takes up to 3 minutes
