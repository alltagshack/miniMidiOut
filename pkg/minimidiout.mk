MINIMIDIOUT_VERSION = 1.5.4
MINIMIDIOUT_SITE = $(TOPDIR)/../miniMidiOut-src
MINIMIDIOUT_SITE_METHOD = local
MINIMIDIOUT_LICENSE = BSD 3-Clause License
MINIMIDIOUT_LICENSE_FILES = LICENSE

MINIMIDIOUT_DEPENDENCIES = portaudio host-cmake

MINIMIDIOUT_CONF_OPTS = \
    -DPORTAUDIO_INCLUDE_DIR=$(STAGING_DIR)/usr/include \
    -DPORTAUDIO_LIBRARY=$(STAGING_DIR)/usr/lib/libportaudio.so

define MINIMIDIOUT_INSTALL_TARGET_CMDS
    $(INSTALL) -m 0755 -D $(@D)/miniMidiOut $(TARGET_DIR)/usr/bin/miniMidiOut
endef

#define MINIMIDIOUT_INSTALL_INIT_SYSV
#    $(INSTALL) -m 0755 -D package/minimidiout/S99minimidiout $(TARGET_DIR)/etc/init.d/S99minimidiout
#endef

$(eval $(cmake-package))
