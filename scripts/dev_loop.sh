#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-dev}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"

CMAKE_ARGS=(
    -S "${ROOT_DIR}"
    -B "${BUILD_DIR}"
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    -DCMAKE_OSX_ARCHITECTURES=arm64
)

if command -v ninja >/dev/null 2>&1; then
    CMAKE_ARGS+=( -G Ninja )
fi

if [[ -n "${JUCE_SOURCE_DIR:-}" ]]; then
    CMAKE_ARGS+=( -DMONUMENT_USE_LOCAL_JUCE=ON -DJUCE_SOURCE_DIR="${JUCE_SOURCE_DIR}" )
fi

cmake "${CMAKE_ARGS[@]}"

build_install() {
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}"
    BUILD_DIR="${BUILD_DIR}" BUILD_TYPE="${BUILD_TYPE}" "${ROOT_DIR}/scripts/install_macos.sh"
}

find_audio_plugin_host() {
    if [[ -n "${AUDIO_PLUGIN_HOST:-}" && -d "${AUDIO_PLUGIN_HOST}" ]]; then
        echo "${AUDIO_PLUGIN_HOST}"
        return
    fi

    local candidates=()
    if [[ -n "${JUCE_SOURCE_DIR:-}" ]]; then
        candidates+=("${JUCE_SOURCE_DIR}/extras/AudioPluginHost/Builds/MacOSX/build/${BUILD_TYPE}/AudioPluginHost.app")
        candidates+=("${JUCE_SOURCE_DIR}/extras/AudioPluginHost/Builds/MacOSX/build/Debug/AudioPluginHost.app")
    fi
    candidates+=("/Applications/AudioPluginHost.app")

    local candidate
    for candidate in "${candidates[@]}"; do
        if [[ -d "${candidate}" ]]; then
            echo "${candidate}"
            return
        fi
    done
}

launch_audio_plugin_host() {
    local host_app
    host_app="$(find_audio_plugin_host || true)"

    if [[ -z "${host_app}" ]]; then
        echo "AudioPluginHost not found. Set AUDIO_PLUGIN_HOST=/path/to/AudioPluginHost.app"
        return
    fi

    if ! pgrep -x "AudioPluginHost" >/dev/null 2>&1; then
        open -a "${host_app}"
    fi
}

build_install
launch_audio_plugin_host

WATCH_PATHS=(
    "${ROOT_DIR}/plugin"
    "${ROOT_DIR}/ui"
    "${ROOT_DIR}/dsp"
    "${ROOT_DIR}/CMakeLists.txt"
)

if command -v fswatch >/dev/null 2>&1; then
    echo "Watching for changes (fswatch)."
    fswatch -o "${WATCH_PATHS[@]}" | while read -r _; do
        build_install
        launch_audio_plugin_host
    done
else
    echo "fswatch not found. Install with: brew install fswatch"
    echo "Re-run this script for auto rebuilds."
fi
