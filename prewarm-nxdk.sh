#!/usr/bin/env bash

function print_help_and_exit() {
  echo "Usage: $0 [<option> ...]"
  echo ""
  echo "Options:"
  echo "  --help          - Print this message"
  echo "  --debug         - Build the nxdk libraries with debug symbols"
  echo "  --lto           - Build the nxdk libraries with link time optimization"
  echo "  --clean         - Clean the nxdk libraries before building"
  exit 1
}

DEBUG=n
LTO=n
CLEAN=n

set +u
while [ ! -z "${1}" ]; do
  case "${1}" in
  '--debug'*)
    DEBUG=y
    shift
    ;;
  '--lto'*)
    LTO=y
    shift
    ;;
  '--clean'*)
    CLEAN=y
    shift
    ;;
  '-h'*)
    print_help_and_exit
    ;;
  '-?'*)
    print_help_and_exit
    ;;
  '--help'*)
    print_help_and_exit
    ;;
  *)
    echo "Ignoring unknown option '${1}'"
    break
    ;;
  esac
done
set -u


if [[ "$(NXDK_DIR:+x)" == "x" ]]; then
  eval "$("${NXDK_DIR}/bin/activate" -s)"
else
  eval "$(./third_party/nxdk/bin/activate -s)"
fi

set -eu
set -o pipefail


if [[ "${CLEAN}" == "y" ]]; then
  cd "${NXDK_DIR}"
  make clean
fi

cd "${NXDK_DIR}/samples"
set -x
for x in *; do
  pushd "${x}"
  DEBUG="${DEBUG}" LTO="${LTO}" make -j12
  popd
done
