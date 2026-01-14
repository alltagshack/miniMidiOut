# miniMidiOut (handmade-polling branch)

A "fast" midi input to audio output synthesizer with a delay between 70 - 120ms on Pi1.
It is based on Buildroot and takes only 15sec to boot. The complete system
runs from RAM. You do not need any display! Just plugin your USB Midi keyboard
and power up the Pi1!

## Features

You can toggle through the waveforms **saw**, **square**, **triangle** and **sinus**:

- no sound should be played
- you press the sustain pedal 3 times during 1.5 seconds

## Build sd-card via buildroot package (Pi1)

Check out my code as `miniMidiOut-src` and get/uncompress `buildroot-2025.02.9.tar.gz`:

```
git clone --branch handmade_polling https://github.com/no-go/miniMidiOut.git miniMidiOut-src
tar -xzf buildroot-2025.02.9.tar.gz
```

use the `cmdline.txt` and `config.txt` from my code:
```
cp miniMidiOut-src/pi1/cmdline.txt buildroot-2025.02.9/board/raspberrypi/cmdline.txt
cp miniMidiOut-src/pi1/config.txt buildroot-2025.02.9/board/raspberrypi/config_default.txt
```

Copy `pkg/` to buildroot packages:
```
mkdir -p buildroot-2025.02.9/package/miniMidiOut
cp miniMidiOut-src/pkg/* buildroot-2025.02.9/package/miniMidiOut/
```

Change into buildroot:
```
cd buildroot-2025.02.9/
```

Add this line into `package/Config.in` e.g. in the menu *Audio and video applications*:
```
	source "package/miniMidiOut/Config.in"
```

Make this executeable:
```
chmod +x ../miniMidiOut-src/pi1/rootfs-overlay/etc/init.d/S99miniMidiOut
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
  - miniMidiOut
- Filesystem images:
  - cpio the root filesystem
    - compression method (gzip)
- Target packages: Hardware handling
  - evtest
  - firmware
    - (keep the pi0/1/2/3 pre selected untouched)
    - Install DTB overlays
- Target packages: Libraries: Audio/Sound
  - alsa-lib
    - everything!
    - especially *alsa-plugins*
  - portaudio (alsa + oss support)
- System configuration: () Root filesystem overlay directories
  - set the () empty to `../miniMidiOut-src/pi1/rootfs-overlay`
- Kernel
  - build devicetree with overlay support

Save as `.config`.

then do:
```
make
```

### Pi1 ramdisk

The upper description creates a `rootfs.cpio.gz` with the complete root filesystem.
You can use this file instead of an ext4 root filesystem on your sd-card.
The system will work in a copy of that filesystem, which is placed in
the RAM. The system will never write something (back) to the sd-card!

- make a single msdos/fat32 partition (minimum 53MB) on your sd-card
- copy to that partition...
  - the 4 `output/images/bcm2708*` files
  - the file `output/images/rootfs.cpio.gz`
  - the linux kernel `output/images/zImage`
  - the content of `output/images/rpi-firmware/`
- do not forget to copy the `overlays` folder inside `output/images/rpi-firmware/`

### Hints for buildroot

if code changed:
```
make miniMidiOut-dirclean
```
or
```
make miniMidiOut-rebuild
```
and then do just `make` again and copy the new
`output/images/rootfs.cpio.gz` to the sd-card.

## Handmade build

On debian or ubuntu you need these packages:

- gcc
- portaudio19-dev
- libasound-dev

```
gcc miniMidiOut.c -Wall -lm -lportaudio -lasound -pthread -o miniMidiOut
```

### cmake alternative build

```
cmake -B .
cmake --build .
```

## Usage

Install `alsa-utils` and `libportaudio2`. Use `amidi -l` to find your keyboard (midi input) device.

### Command line interface

For sinus wave:
```
miniMidiOut hw:2,0,0 sin
```

For saw sound effect:
```
miniMidiOut hw:2,0,0 saw
```

For square sound effect:
```
miniMidiOut hw:2,0,0 sqr
```

For triangle sound effect:
```
miniMidiOut hw:2,0,0 tri
```

`miniMidiOut` uses the default audio device for output. You can
change this e.g. `hw:1` with an optional 3rd `miniMidiOut hw:2,0,0 sin 1`.
The default is used with `miniMidiOut hw:2,0,0 tri -1`.

`miniMidiOut` has an optional 4th parameter for the buffer size. The default
is 8. You can change this e.g. to 512 with `miniMidiOut hw:2,0,0 sin -1 512`.

Quit the application with `CRTL + c`.
