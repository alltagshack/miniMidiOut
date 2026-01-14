# miniMidiOut (epoll version)

A "fast" midi input to audio output synthesizer with 5 popular waveforms.

Messurement on **modern Laptop**: delay between key press and audio out is *20ms*

## Usage

You can use (and build) the command line interface (CLI) `miniMidiOut` on many Linux systems. It does not use a realtime linux-kernel.

## Build

On debian or ubuntu you need these packages:

- gcc
- portaudio19-dev
- libasound-dev

```
cmake -B .
cmake --build .
```

## Features (alphanumeric keyboard)

- press `1` for sinus
- press `2` for saw
- press `3` for square
- press `4` for triangle
- press `5` for noise
- press `0` for sustain

Change fading out the tone (release):

- press `6` for default
- press `7` for a long fade out
- press `8` for no fade out
- press `9` toggles automatic fade out on/off

Change octave:

- press `-` to set down
- press `.` to set up

## Feature (MIDI keyboard)

Switch through the waveforms **saw**, **square**, **triangle**, **noise** and **sinus**:

- no sound should be played
- you press the sustain pedal 3 times in 1.5 seconds

## Start it

Install `alsa-utils` and `libportaudio2`. Use `amidi -l` to find your keyboard (midi input) device.

Add YOURSELF to the input group. The code uses a letter keyboard or usb numpad, if it exists.

```
sudo add usermod -aG input YOURSELF
```

After logout and relogin YOURSELF you and `miniMidiOut` have the right to access keypad events (`/dev/input/event0`).

For sinus wave similar to a bell or flute:
```
miniMidiOut hw:2,0,0 sin
```

For saw sound effect. It is the synth-sound from the 80th and similar to a spinet:
```
miniMidiOut hw:2,0,0 saw
```

For square sound effect similar to 8bit arcade game from the 80th:
```
miniMidiOut hw:2,0,0 sqr
```

For triangle sound effect similar to a cheap electric doorbell or gong:
```
miniMidiOut hw:2,0,0 tri
```

For noise sound effect similar to wind, ocean waves or percussion:
```
miniMidiOut hw:2,0,0 noi
```

After starting `miniMidiOut` with success, it plays 3 tones as startup theme.

`miniMidiOut` uses the default audio device for output. You can
change this e.g. `hw:1` with an optional 3rd `miniMidiOut hw:2,0,0 sin 1`.
The default is used with `miniMidiOut hw:2,0,0 tri -1`.

`miniMidiOut` has an optional 4th parameter for the buffer size. The default
is `16` and works well on Pi1. You can change the buffer e.g. to `512`
with `miniMidiOut hw:2,0,0 sin -1 512` if you want.

There is an optional 5th parameter form `-20` to `100` (fade). A negative value
fades the sound of a key out WHILE it is pressed. With a value of `-20` this
sound is extreme short! Values form `0` to `100` changes the fade out AFTER the
key is released. The default is `10`.

You can set the envelope with a 6th optional parameter. The default value is `16000` and
results in tone, where the first 16000 samples are a bit louder.

An optional 7th parameter is `/dev/input/event0` as default. It is used to set the event
device, which detects a pressed key on the Numpad.

You can set the samplerate with a 8th optional parameter. The default value is `44100`.
Some values may not work on your sound system an it will fall back to its own defaults.

Quit the application with `CRTL + c` or `ESC`.