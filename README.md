# midiSynth

A fast midi input to audio output synthesizer.

## build

On debian or ubuntu you need these packages:

- gcc
- portaudio19-dev
- libasound-dev

On Alpine Linux:

- gcc
- libc-dev
- alsa-utils
- portaudio-dev

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

## Notes

add into `config.txt`

```
disable_overscan=1
dtparam=audio=on
hdmi_group=1
hdmi_drive=2
```

### cmdline.txt
```
root=/dev/mmcblk0p2 rootwait console=tty1 console=ttyAMA0,115200 video=800x480
```

### sound.conf (not working)

board/raspberrypi/rootfs-overlay/etc/modules-load.d/sound.conf
```
snd-usb-audio
snd-bcm2835
snd-seq-midi
```

### for a autostart (not working)

board/raspberrypi/rootfs-overlay/etc/init.d/S99midisynth
```
#!/bin/sh
midiSynth hw:0,0,0 saw &
```
