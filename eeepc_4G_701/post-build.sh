#!/bin/sh

set -e

BOARD_DIR=$(dirname "$0")

cp -f ${BOARD_DIR}/syslinux.cfg ${BINARIES_DIR}/
#support/scripts/genimage.sh -c "${BOARD_DIR}/genimage.cfg"
