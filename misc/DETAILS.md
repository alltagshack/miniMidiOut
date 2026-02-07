# Details

## Non SD-Card usage

Install `alsa-utils`, OSS (?) and `libportaudio2`.

Add YOURSELF to the input group. The code uses a letter keyboard or usb numpad, if it exists.

```
sudo usermod -aG input YOURSELF
```

After logout and relogin YOURSELF you and `miniMidiOut` have the right to access keypad events (`/dev/input/event0`).

## Start it

For sinus wave similar to a bell or flute:
```
miniMidiOut /dev/midi1 sin
```

For saw sound effect. It is the synth-sound from the 80th and similar to a spinet:
```
miniMidiOut /dev/midi1 saw
```

For square sound effect similar to 8bit arcade game from the 80th:
```
miniMidiOut /dev/midi1 sqr
```

For triangle sound effect similar to a cheap electric doorbell or gong:
```
miniMidiOut /dev/midi1 tri
```

For noise sound effect similar to wind, ocean waves or percussion:
```
miniMidiOut /dev/midi1 noi
```

After starting `miniMidiOut` with success, it plays 3 tones as startup theme.

`miniMidiOut` uses the default audio device for output. You can
change this e.g. `hw:1` with an optional 3rd `miniMidiOut /dev/midi1 sin 1`.
The default is used with `miniMidiOut /dev/midi1 tri -1`.

`miniMidiOut` has an optional 4th parameter for the buffer size. The default
is `16` and works well on Pi1. You can change the buffer e.g. to `512`
with `miniMidiOut /dev/midi1 sin -1 512` if you want.

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

```
gcc main.c \
    modus.c pseudo_random.c time_tools.c voice.c noise.c keyboard.c globals.c \
    -Wall -Iinclude/ -lm -lportaudio -pthread -o miniMidiOut
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

Make this executeable:
```
chmod +x miniMidiOut-src/pi1/rootfs-overlay/etc/init.d/S99minimidiout
```

Add my `pkg` to buildroot packages and add my `pi1_defconfig` file to `configs`:
```
cp -r miniMidiOut-src/pkg buildroot-2025.02.9/package/minimidiout
cp miniMidiOut-src/pi1/pi1_defconfig buildroot-2025.02.9/configs/
```

Change into buildroot:
```
cd buildroot-2025.02.9/
```

Add this line into `package/Config.in` e.g. in the menu *Audio and video applications*:
```
	source "package/minimidiout/Config.in"
```

Make a default pi1 config and start menuconfig:
```
unset LD_LIBRARY_PATH
make pi1_defconfig
make menuconfig
```

Save as `.config`.

then do:
```
make linux-menuconfig
```

- general setup
  - Preemption Model (Fully Preemptible Kernel (Real-Time))
  - boot config support
- Kernel Features
  - Timer frequency (1000 Hz)
- Device Drivers
  - Sound card support
    - <*> Advanced Linux Sound Architecture
      - [*] Enable OSS Emulation
        - <*> OSS Mixer API
        - <*> OSS PCM (digital audio) API
      - <*> Sequencer support
        - <*> OSS Sequencer API 
      - [*] USB sound devices
        - <*> USB Audio/MIDI driver

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
  - the 4 `output/images/bcm2835-rpi-*` files
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


## Build SD-Card with buildroot (eeepc)

```
tar -xzf buildroot-2025.02.9.tar.gz
git clone https://github.com/no-go/miniMidiOut.git miniMidiOut-src
chmod +x miniMidiOut-src/eeepc_4G_701/rootfs-overlay/etc/init.d/S99minimidiout
cd buildroot-2025.02.9
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
- host utilities
  - deselect qemu
- bootloaders
  - syslinux
    - mbr

```
make linux-menuconfig
```

same like pi1, but add this:

- Device Drivers: Sound card support
  - <*> Advanced Linux Sound Architecture
    - HD Audio
      - <*> HD Audio PCI
      - [*] Build hwdep interface for HD-audio driver
      - <*> Build Realtek HD-audio codec support

or `cp ../miniMidiOut-src/eeepc_4G_701/kernel-config.txt output/build/linux-5.10.162-cip24-rt10/.config`

Remove both patches inside qemu, because they make not sense on eeepc:
```
rm board/qemu/patches/linux/*.patch
```

Do the build, now:

```
make
```

If something went wrong, this build makes sense, too:
```
make host-fakeroot-rebuild
make host-genimage-rebuild
```

Build bootable sd-card in `/dev/mmcblk0` as `root` user:
```
parted /dev/mmcblk0 mklabel msdos
parted -a optimal /dev/mmcblk0 mkpart primary fat32 1MiB 53MiB set 1 boot on
mkfs.vfat -F 32 /dev/mmcblk0p1

syslinux --install /dev/mmcblk0p1
dd if=output/images/syslinux/mbr.bin of=/dev/mmcblk0 bs=440 count=1

mkdir -p /tmp/mmc
mount /dev/mmcblk0p1 /tmp/mmc

cp ../miniMidiOut-src/eeepc_4G_701/syslinux.cfg /tmp/mmc/
cp output/images/bzImage /tmp/mmc/
cp output/images/rootfs.cpio.gz /tmp/mmc/

umount /tmp/mmc
```

### Debugging

```
lsmod | grep snd
ls /dev/snd
dmesg | grep -i audio
```
