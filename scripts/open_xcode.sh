#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
open "${BUILD_DIR}/Monument.xcodeproj"
