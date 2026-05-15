# synth0815

A midi synthesizer. This code is a ugly combination of:

- idea and code from c#
- some modern c++ hacks
- the very ugly minimal endless loop from my first c version

## features

- 4 waveforms: sinus, sawtooth, square, triangle
- many voices
- velocity
- pitch
- sustain
- release volumen during key press
- midi device is selectable via CLI
- press 3x sustain on silence to switch through wavforms

## missing

- any error handling!
- unit tests
- comments or doc in code
- no setup or CI/CD routine
- missing tests for realtime: how many ms delay between key-press and audio out?

## build

```
cmake -B build
cmake --build build
```

## usage

Add YOURSELF to the input group, to have access to e.g. `/dev/midi2`

```
sudo usermod -aG input YOURSELF
```

After logout and relogin YOURSELF has the right for raw access to the midi device.

```
./build/synth0815 midiDevice waveform
 - midiDevice example: /dev/midi2
 - waveform: s=sin, a=saw, t=triangle, q=square
 - CTRL+C to exit
```
