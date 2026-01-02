#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-ninja"

if [[ -z "${JUCE_SOURCE_DIR:-}" ]]; then
    echo "JUCE_SOURCE_DIR is required (local JUCE checkout)."
    echo "Example: JUCE_SOURCE_DIR=/path/to/JUCE ./scripts/build_macos.sh"
    exit 1
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DMONUMENT_USE_LOCAL_JUCE=ON \
    -DJUCE_SOURCE_DIR="${JUCE_SOURCE_DIR}"

cmake --build "${BUILD_DIR}"
