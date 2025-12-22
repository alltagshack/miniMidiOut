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

## buildroot mods

copy `pkg/` to buildroot packages:
```
mkdir -p buildroot-2025.02.9/package/midisynth
cp midisynth-src/pkg/* buildroot-2025.02.9/package/midisynth/
cd buildroot-2025.02.9/
```

add this line into `package/Config.in` e.g. in the menu *Audio and video applications*:
```
	source "package/midisynth/Config.in"
```

for an autostart add a file `board/raspberrypi/rootfs-overlay/etc/init.d/S99midisynth`:
```
#!/bin/sh
modprobe snd-bcm2835
modprobe snd-usb-audio
modprobe snd-seq-midi
midiSynth hw:2,0,0 saw &
```

make it executeable:
```
chmod +x board/raspberrypi/rootfs-overlay/etc/init.d/S99midisynth
```

make a default pi1 config and start menuconfig:
```
make raspberrypi_defconfig
make menuconfig
```

Select/set this:

- alsa-utils
  - amidi
  - alsamixer
- midisynth
- BR2_ROOTFS_OVERLAY="board/raspberrypi/rootfs-overlay"

save as `.config` and than do:
```
make
```

write image to sd-card on e.g. `/dev/mmcblk0`:
```
sudo dd status=progress if=output/images/sdcard.img of=/dev/mmcblk0
```

### hint

if code changed:
```
make midisynth-rebuild
```
or
```
make midisynth-dirclean
```
and then do a complete `make` again.


## sd-card mods

add into `config.txt`:
```
disable_overscan=1
dtparam=audio=on
hdmi_group=1
hdmi_drive=2
```

change video in `cmdline.txt`:
```
root=/dev/mmcblk0p2 rootwait console=tty1 console=ttyAMA0,115200 video=800x480
```
