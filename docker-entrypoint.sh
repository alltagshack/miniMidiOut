#!/usr/bin/env bash
set -e

USER=developer
HOME_DIR="/home/${USER}"

# If env vars passed, adjust uid/gid of the user
if [ -n "${CONTAINER_UID:-}" ] && [ -n "${CONTAINER_GID:-}" ]; then
  EXISTING_UID=$(id -u $USER)
  EXISTING_GID=$(id -g $USER)
  if [ "$EXISTING_GID" != "$CONTAINER_GID" ]; then
    groupmod -g "$CONTAINER_GID" $USER || true
  fi
  if [ "$EXISTING_UID" != "$CONTAINER_UID" ]; then
    usermod -u "$CONTAINER_UID" $USER || true
  fi
  # Fix ownership for home and project mounts
  chown -R ${CONTAINER_UID}:${CONTAINER_GID} ${HOME_DIR} || true
fi

# Ensure SSH dir/authorized_keys exist with correct perms
mkdir -p "${HOME_DIR}/.ssh"
chown -R ${USER}:${USER} "${HOME_DIR}/.ssh"
chmod 700 "${HOME_DIR}/.ssh"

# Start sshd in foreground
/usr/sbin/sshd -D
