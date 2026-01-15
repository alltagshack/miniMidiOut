#!/bin/sh
set -e

BOARD_DIR="$(dirname "$0")"
GENIMAGE_CFG="${BOARD_DIR}/genimage.cfg"

GENIMAGE_TMP="${BINARIES_DIR}/genimage.tmp"
GENIMAGE_INPUT="${BINARIES_DIR}/genimage.input"

rm -rf "${GENIMAGE_TMP}"

rm -rf "${GENIMAGE_INPUT}"
mkdir -p "${GENIMAGE_INPUT}"

cp "${BINARIES_DIR}/bzImage" "${GENIMAGE_INPUT}/"
cp "${BINARIES_DIR}/rootfs.cpio.gz" "${GENIMAGE_INPUT}/"
cp "${BOARD_DIR}/syslinux.cfg" "${GENIMAGE_INPUT}/"

genimage \
    --rootpath "${TARGET_DIR}" \
    --tmppath "${GENIMAGE_TMP}" \
    --inputpath "${GENIMAGE_INPUT}" \
    --outputpath "${BINARIES_DIR}" \
    --config "${GENIMAGE_CFG}"
