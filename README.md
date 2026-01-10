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

Add YOURSELF to the input group. The code uses as letter keyboard, if it exists.

```
sudo add usermod -aG input YOURSELF
```

After logout and relogin YOURSELF you and `midiSynth` have the right to access keypad events (`/dev/input/event0`).

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

After starting `midiSynth` with success, it plays 3 tones as startup theme.

`midiSynth` uses the default audio device for output. You can
change this e.g. `hw:1` with an optional 3rd `midiSynth hw:2,0,0 sin 1`.
The default is used with `midiSynth hw:2,0,0 tri -1`.

`midiSynth` has an optional 4th parameter for the buffer size. The default
is `16` and works well on Pi1. You can change the buffer e.g. to `512`
with `midiSynth hw:2,0,0 sin -1 512` if you want.

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

### Features via Numpad

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

### MIDI device

Switch through the waveform s**sinus**, **saw**, **square**, **triangle** and **noise**:

- no sound should be played
- you press the sustain pedal 3 times in 1.5 seconds

## todo (primary buildroot)

- ram image
- pi1
  - there is a issue with hotplug the midi keyboard
  - add pi's vc4-kms-v3d.dtbo to overlay (must be open source)
  - create additional linux config file (driver for snd-bcm2835 and midi missing)
- eeepc
  - better description for eeepc and configs
  - an init.d script
  - unmute sound device after boot
  - handle issue with keypad, ctrl+c and ESC (does not work)

# buildroot mods

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

save as `.config`. Maybe you have to select `Device Drivers: Sound card support`
in the kernel via `make linux-menuconfig` and then do:
```
make
```

write image to sd-card on e.g. `/dev/mmcblk0`:
```
sudo dd status=progress if=output/images/sdcard.img of=/dev/mmcblk0
```

## hint

if code changed:
```
make midisynth-rebuild
```
or
```
make midisynth-dirclean
```
and then do a complete `make` again.

## notes eeepc

Dass der Kernel bootet, ist ein großer Erfolg! Dass der Sound nicht funktioniert,
liegt bei alten EeePCs (meist mit Intel High Definition Audio oder Realtek ALC 
Chipsätzen) fast immer daran, dass entweder die Treiber nicht im Kernel aktiviert
sind oder die benötigte Firmware/Alsa-Tools im Rootfs fehlen.
Hier ist die Checkliste, um den Sound auf dem EeePC zum Laufen zu bringen:

### Kernel-Konfiguration (Treiber)

Du musst sicherstellen, dass der passende Sound-Treiber fest im Kernel eingebaut ist (oder als Modul geladen wird).

Führe `make linux-menuconfig` aus.

Navigiere zu: `Device Drivers: Sound card support`.

Aktiviere Advanced Linux Sound Architecture (ALSA).

Gehe in den Unterpunkt PCI sound devices.

Aktiviere Intel HD Audio (CONFIG_SND_HDA_INTEL). Dies ist der Standard-Treiber
für fast alle EeePC-Modelle (701, 900, 1000).

Device Drivers: Sound card support: Advanced Linux Sound Architecture:

- <*> HD Audio PCI
- [*] Build hwdep interface for HD-audio driver
- <*> Build Realtek HD-audio codec support

Device Drivers: Sound card support: Advanced Linux Sound Architecture: USB sound devices:

- <*> USB Audio/MIDI driver
- [*] MIDI 2.0 support by USB Audio driver

Achte darauf, dass auch die Codecs aktiviert sind (meist Realtek oder Generic).

Speichere und verlasse das Menü.

### Buildroot-Pakete (Software)

Ohne die passenden Bibliotheken und Tools kann Linux den Soundchip
nicht ansprechen.

Führe `make menuconfig` aus.

Gehe zu `Target packages -> Audio and video applications`.
Aktiviere `alsa-utils`. Das gibt dir das Tool `amixer` und `alsamixer`.
(Optional) Aktiviere `alsa-plugins`, falls du komplexere Audio-Ausgaben planst.

### Sound aktivieren (Stummschaltung aufheben)

Unter Linux ist der Sound nach dem Booten standardmäßig oft gemuted (stummgeschaltet).
Nachdem du dein neues System gebootet hast, versuche Folgendes auf der Konsole:

```
aplay -l
# Alle Kanäle entstummen und Lautstärke auf 100% setzen
amixer sset Master unmute
amixer sset Master 100%
amixer sset Speaker unmute
```

### Firmware (Wichtig für spätere Modelle)

Manche EeePC-Modelle benötigen spezielle Firmware-Dateien für den Soundchip.
Gehe in make menuconfig zu Target packages -> Hardware handling -> Firmware.
Prüfe, ob dort Pakete wie alsa-firmware verfügbar sind
(oft für die i386-EeePCs nicht nötig, aber ein guter Check).

### Fehlersuche auf dem EeePC

Wenn du auf dem laufenden System bist, prüfe mit diesem Befehl, ob
der Treiber geladen wurde:

```
lsmod | grep snd
ls /dev/snd
dmesg | grep -i audio
```
Wenn die Liste leer ist, wurde der Treiber nicht geladen
(oder nicht fest in den Kernel eingebaut). Wenn `dmesg | grep -i audio` Fehlermeldungen
zeigt, fehlt wahrscheinlich die Firmware.
Tipp: Da du `isolinux` nutzt, stelle sicher, dass du nach den
Kernel-Änderungen `make linux-update-savedefconfig` und `make` das
neue `bzImage` auf deine SD-Karte kopierst!
