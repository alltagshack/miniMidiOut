# miniMidiOut

A "fast" (booting) usb-midi input to audio output synthesizer with 5 popular waveforms. It
supports velocity, pitch bend and sustain.

## Specials

- Pi1 and eeepc is NOT a MUST-HAVE
- Mainstream CIP RT kernel (6.1.157) for Pi1 and eeepc
- system runs from RAM, not from SD
- boots up in 15s on Pi1 and 8s on eeepc

## It is not realy realtime

Messure delay between key press and audio out:

- Pi1: *64ms*
- eeepc: *48ms*

## Usage

You can use (and build) the command line interface (CLI) `miniMidiOut` on many Linux systems. It
does not depend on a realtime linux-kernel.

For Pi1 or eeepc use ready to use [Release Files](https://github.com/no-go/miniMidiOut/releases):

- use my Pi1 sd-card content (just a single FAT32 partition is needed)
- use my sd-card image for the eeepc 4G 701 (32bit Pentium, BIOS boot)

With these files/images the system boots a minimal Linux and autostarts `miniMidiOut`.

![use-case](misc/usecase.png)

1. plugin sd-card
2. plugin MIDI USB keyboard
3. if the MIDI USB keyboard has its own power button: power on!
4. optional: plugin an USB alphanumeric keyboard
5. plugin your headphones to the analog audio
   - pi1: If you use the HDMI plug, the audio signal is there. (not recommended!)
   - pi1: Attention! There is a 4th connection for analog video in the 3.5mm hole :-S
6. power on the Pi1 (or eeepc with ESC-key and boot menu)
7. wait until 3 tones comes up
8. **enjoy classic synthesizer sounds from the 80th !**
9. power off the Pi1 (or eeepc)

## Features (alphanumeric keyboard)

- press `1` for sinus
- press `2` for saw
- press `3` for square
- press `4` for triangle
- press `5` for noise
- press `0` for sustain (do not use together with sustain pedal)

Change fading out the tone (release):

- press `6` for default
- press `7` for a long fade out
- press `8` for no fade out
- press `9` toggles
  - automatic fade out on
  - automatic fade out off
  - if release the key, the freqency goes up
  - if release the key, the freqency goes down

Change octave:

- press `-` to set down
- press `.` to set up

## Feature (MIDI keyboard)

- velocity
- pitch bend (+/- 4 half tones)
- sustain pedal

Switch through the waveforms **saw**, **square**, **triangle**, **noise** and **sinus**:

- no sound should be played
- you press the sustain pedal 3 times in 1.5 seconds

[Listen to the Audio File](https://raw.githubusercontent.com/no-go/miniMidiOut/refs/heads/main/misc/example.mp3)

## Bugs

Hotplug does not work.

## Details

See [this file](misc/DETAILS.md) for more details to build and use `miniMidiOut`.
