#!/usr/bin/env bash

set -x
eval "$(./third_party/nxdk/bin/activate -s)"

set -eu
set -o pipefail

cd third_party/nxdk/samples
for x in *; do
  pushd "${x}"
  make -j24
  popd
done
