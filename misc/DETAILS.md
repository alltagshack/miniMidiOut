# Details

## Non SD-Card usage

Install `alsa-utils` and `libportaudio2`. Use `amidi -l` to find your keyboard (midi input) device.

Add YOURSELF to the input group. The code uses a letter keyboard or usb numpad, if it exists.

```
sudo usermod -aG input YOURSELF
```

After logout and relogin YOURSELF you and `miniMidiOut` have the right to access keypad events (`/dev/input/event0`).

## Start it

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

# Build

On debian or ubuntu you need these packages:

- gcc
- portaudio19-dev
- libasound-dev

```
gcc main.c \
    modus.c pseudo_random.c time_tools.c voice.c noise.c keyboard.c globals.c \
    -Wall -Iinclude/ -lm -lportaudio -lasound -pthread -o miniMidiOut
```

## Cmake alternative build

```
cmake -B .
cmake --build .
```

## Build sd-card via buildroot package (Pi1)

Check out my code as `miniMidiOut-src` and get/uncompress `buildroot-2025.02.9.tar.gz`:

```
git clone https://github.com/no-go/miniMidiOut.git miniMidiOut-src
tar -xzf buildroot-2025.02.9.tar.gz
```

Use the `cmdline.txt` and `config.txt` from my code:
```
cp miniMidiOut-src/pi1/cmdline.txt buildroot-2025.02.9/board/raspberrypi/cmdline.txt
cp miniMidiOut-src/pi1/config.txt buildroot-2025.02.9/board/raspberrypi/config_default.txt
```

Look into `buildroot-2025.02.9/board/raspberrypi/genimage.cfg.in` and
change `size = 32M` into `size = 61M`.

Copy `pkg/` to buildroot packages:
```
mkdir -p buildroot-2025.02.9/package/minimidiout
cp miniMidiOut-src/pkg/* buildroot-2025.02.9/package/minimidiout/
```

Change into buildroot:
```
cd buildroot-2025.02.9/
```

Add this line into `package/Config.in` e.g. in the menu *Audio and video applications*:
```
	source "package/minimidiout/Config.in"
```

Make this executeable:
```
chmod +x ../miniMidiOut-src/pi1/rootfs-overlay/etc/init.d/S99minimidiout
```

Make a default pi1 config and start menuconfig:
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
  - deselct ext2/3/4
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
make minimidiout-dirclean
```
or
```
make minimidiout-rebuild
```
and then do just `make` again and copy the new
`output/images/rootfs.cpio.gz` to the sd-card.





## Build SD-Card via buildroot (eeepc)

```
unset LD_LIBRARY_PATH
make qemu_x86_defconfig
cp ../miniMidiOut-src/eeepc_4G_701/buildroot-2025.02.9-config.txt .config
make menuconfig
```

- Target options
  - Target Architecture: x86
  - Target Architecture Variant: i586   (or i486)
- System configuration
  - () Root filesystem overlay directories:
    set to `../miniMidiOut-src/eeepc_4G_701/rootfs-overlay`
  - Custom scripts to run before creating filesystem images: () empty
  - Custom scripts to run after creating filesystem images: () empty
- ost utilities
  - deselect qemu
- bootloaders
  - syslinux
    - mbr

```
make linux-menuconfig
```

- Device Drivers: Sound card support
  - <*> Advanced Linux Sound Architecture
    - HD Audio
      - <*> HD Audio PCI
      - [*] Build hwdep interface for HD-audio driver
      - <*> Build Realtek HD-audio codec support
    - [*] USB sound devices
      - <*> USB Audio/MIDI driver
        - [*] MIDI 2.0 support by USB Audio driver
    - <*> Sequencer support

or `cp ../miniMidiOut-src/eeepc_4G_701/kernel-config.txt output/build/linux-6.12.27/.config`

### Prepare sd-card

...

### Debugging

```
lsmod | grep snd
ls /dev/snd
dmesg | grep -i audio
```
