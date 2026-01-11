MINIMIDIOUT_VERSION = 1.0
MINIMIDIOUT_SITE = $(TOPDIR)/../miniMidiOut-src
MINIMIDIOUT_SITE_METHOD = local

MINIMIDIOUT_DEPENDENCIES = portaudio

define MINIMIDIOUT_BUILD_CMDS
    $(TARGET_CC) \
        -Wall \
        $(@D)/miniMidiOut.c \
        -o $(@D)/miniMidiOut \
        $(TARGET_CFLAGS) \
        $(TARGET_LDFLAGS) \
        -lm -lportaudio -lasound -pthread
endef

define MINIMIDIOUT_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/miniMidiOut $(TARGET_DIR)/usr/bin/miniMidiOut
endef

$(eval $(generic-package))
