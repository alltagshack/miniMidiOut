# midiSynth

A fast midi input to audio output synthesizer.

## build

On debian or ubuntu you need these packages:

- gcc
- portaudio19-dev
- libasound-dev

```
gcc midiSynth.c -Wall -lm -lportaudio -lasound -pthread -o midiSynth
```

### cmake alternative build

```
cmake -B .
cmake --build .
```

## example usage

Install `alsa-utils` and `libportaudio2`. Use `amidi -l` to find your keyboard (midi input) device.

For sinus wave:
```
./midiSynth hw:2,0,0 sin
```

For saw sound effect:
```
./midiSynth hw:2,0,0 saw
```

For square sound effect:
```
./midiSynth hw:2,0,0 sqr
```

For triangle sound effect:
```
./midiSynth hw:2,0,0 tri
```

The *change instrument* buttons on your keyboard should toggle trough
the modes **sinus**, **saw**, **square** and **triangle**.

Quit the application with `CRTL + c`.
