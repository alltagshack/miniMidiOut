# midiSynth

A fast midi input to audio output synthesizer with 5 popular waveforms.

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

## Usage

Install `alsa-utils` and `libportaudio2`. Use `amidi -l` to find your keyboard (midi input) device.

### CLI

For sinus wave similar to a bell or flute:
```
midiSynth hw:2,0,0 sin
```

For saw sound effect. It is the synth-sound from the 80th and similar to a spinet:
```
midiSynth hw:2,0,0 saw
```

For square sound effect similar to 8bit arcade game from the 80th:
```
midiSynth hw:2,0,0 sqr
```

For triangle sound effect similar to a cheap electric doorbell or gong:
```
midiSynth hw:2,0,0 tri
```

For noise sound effect similar to wind, ocean waves or percussion:
```
midiSynth hw:2,0,0 noi
```

`midiSynth` uses the default audio device for output. You can
change this e.g. `hw:1` with an optional 3rd `midiSynth hw:2,0,0 sin 1`.
The default is used with `midiSynth hw:2,0,0 tri -1`.

`midiSynth` has an optional 4th parameter for the buffer size. The default
is `32`. You can change this e.g. to `512` with `midiSynth hw:2,0,0 sin -1 512`.

There is an optional 5th parameter form `-20` to `100` (fade). A negative value
fades the sound of a key out WHILE it is pressed. With a value of `-20` this
sound is extreme short! Values form `0` to `100` changes the fade out AFTER the
key is released. The default is `10`.

You can set the envelope with a 6th optional parameter. The default value is `16000` and
results in tone, where the first 16000 samples are a bit louder.

Quit the application with `CRTL + c`.

### optional letter keyboard

- press `1` for sinus
- press `2` for saw
- press `3` for square
- press `4` for triangle
- press `5` for noise
- press on Keypad `+` or `SPACE` for sustain

### MIDI device

The *change instrument* buttons on your keyboard
should toggle through
the modes **sinus**, **saw**, **square**, **triangle** and **noise**.

An alternative way to switch the mode:

- no sound should be played
- you press the sustain pedal 3 times in 1.5 seconds

## buildroot mods

copy `pkg/` to buildroot packages:
```
mkdir -p buildroot-2025.02.9/package/midisynth
cp midisynth-src/pkg/* buildroot-2025.02.9/package/midisynth/
```

change to buildroot:
```
cd buildroot-2025.02.9/
```

add this line into `package/Config.in` e.g. in the menu *Audio and video applications*:
```
	source "package/midisynth/Config.in"
```

make a `init.d` dir as overlay:
```
mkdir -p board/raspberrypi/rootfs-overlay/etc/init.d
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

add 2 lines into `board/raspberrypi/config_default.txt`:
```
echo "dtparam=audio=on" >>board/raspberrypi/config_default.txt
echo "dtoverlay=vc4-kms-v3d" >>board/raspberrypi/config_default.txt
```

make a default pi1 config and start menuconfig:
```
unset LD_LIBRARY_PATH
make raspberrypi_defconfig
make menuconfig
```

Select/set this:

- Target packages: Audio and video applications
  - alsa-utils
    - alsamixer
    - alsactl
    - amidi
    - amixer
    - aplay
    - aseqdump
  - midisynth
- Target packages: Libraries: Audio/Sound
  - alsa-lib
    - everything!
    - especially *alsa-plugins*
  - portaudio (alsa + oss support)
- System configuration: () Root filesystem overlay directories
  - set the () empty to `board/raspberrypi/rootfs-overlay`
- Kernel
  - build devicetree with overlay support

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

## todo

- buildroot: add usermod -aG input USERNAME
- buildroot: ram image
- buildroot: add pi's vc4-kms-v3d.dtbo to overlay (must be open source)
- buildroot: 386er eeepc image?