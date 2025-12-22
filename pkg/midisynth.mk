MIDISYNTH_VERSION = 1.0
MIDISYNTH_SITE = $(TOPDIR)/../midisynth-src
MIDISYNTH_SITE_METHOD = local

MIDISYNTH_DEPENDENCIES = portaudio

define MIDISYNTH_BUILD_CMDS
    $(TARGET_CC) \
        -Wall \
        $(@D)/midiSynth.c \
        -o $(@D)/midiSynth \
        $(TARGET_CFLAGS) \
        $(TARGET_LDFLAGS) \
        -lm -lportaudio -lasound -pthread
endef

define MIDISYNTH_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/midiSynth $(TARGET_DIR)/usr/bin/midiSynth
endef

$(eval $(generic-package))
