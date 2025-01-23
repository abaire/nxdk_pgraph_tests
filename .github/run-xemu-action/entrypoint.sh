#!/bin/bash

set -eu
set -o pipefail

xemu_appimage="$1"
readonly xemu_appimage

if [[ ! -e "${xemu_appimage}" ]]; then
  echo "Xemu not found at ${xemu_appimage}"
  exit 1
fi

xemu_abspath="$(readlink -f "${xemu_appimage}")"
readonly xemu_abspath

chmod +x "${xemu_abspath}"

iso_path="$2"
readonly iso_path
if [[ ! -e "${iso_path}" ]]; then
  echo "xiso not found at ${iso_path}"
  exit 1
fi
iso_abspath="$(readlink -f "${iso_path}")"
readonly iso_abspath

hdd_path="$3"
readonly hdd_path
if [[ ! -e "${hdd_path}" ]]; then
  echo "HDD image not found at ${hdd_path}"
  exit 1
fi
hdd_abspath="$(readlink -f "${hdd_path}")"
readonly hdd_abspath
mkdir -p /xemu
cp "${hdd_path}" /xemu/


function create_sound_device() {
  rm -rf /var/run/pulse /var/lib/pulse "${HOME}/.config/pulse" 2> /dev/null

  export PULSE_RUNTIME_PATH="/run/pulse"
  export XDG_RUNTIME_DIR="/run/pulse"
  export XDG_CONFIG_HOME="/run/pulse/.config"

  pulseaudio \
      -D \
      -vvvvv \
      --exit-idle-time=-1 \
      --system \
      --disallow-exit \
      --log-level=debug

  chmod 0775 /run/pulse
  chmod 0775 /run/pulse/.config
  mkdir -p /run/pulse/.config/pulse
  chown pulse:pulse /run/pulse/.config/pulse
  chmod 0770 /run/pulse/.config/pulse
}


create_sound_device


xvfb-run \
    --server-args="-screen 0 1920x1080x24" \
    "${xemu_abspath}" --appimage-extract-and-run \
    -dvd_path "${iso_abspath}"

ls -la /github/home/.local/share/xemu/xemu/xemu.toml
cat /github/home/.local/share/xemu/xemu/xemu.toml
