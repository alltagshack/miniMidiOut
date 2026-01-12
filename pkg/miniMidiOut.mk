MINIMIDIOUT_VERSION = 1.0
MINIMIDIOUT_SITE = $(TOPDIR)/../miniMidiOut-src
MINIMIDIOUT_SITE_METHOD = local

MINIMIDIOUT_DEPENDENCIES = portaudio

define MINIMIDIOUT_BUILD_CMDS
    mkdir -p build && \
    $(TARGET_CONFIGURE_OPTS) cmake -B build -S $(MINIMIDIOUT_SITE) \
        -DCMAKE_INSTALL_PREFIX=$(TARGET_DIR) \
        -DPORTAUDIO_INCLUDE_DIR=$(STAGING_DIR)/usr/include \
        -DPORTAUDIO_LIBRARY=$(STAGING_DIR)/usr/lib/libportaudio.so && \
    cmake --build build
endef

define MINIMIDIOUT_INSTALL_TARGET_CMDS
    cmake --build build --target install
endef

$(eval $(generic-package))
