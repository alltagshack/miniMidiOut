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

```
./synth0815 midiDevice waveform
 - use 'amidi -l' to find midiDevice. example: hw:2,0,0
 - waveform: s=sin, a=saw, t=triangle, q=square
 - CTRL+C to exit
```
