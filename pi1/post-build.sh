#!/bin/sh

set -u
set -e

ln -sf /dev/null "${TARGET_DIR}/etc/systemd/system/getty@tty1.service"

mkdir -p "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants"

ln -sf ../autologin-root.service \
    "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/autologin-root.service"
