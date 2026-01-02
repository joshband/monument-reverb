#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLUGIN_NAME="Monument"

BUILD_DIR="${BUILD_DIR:-}"
if [[ -z "${BUILD_DIR}" ]]; then
    if [[ -d "${ROOT_DIR}/build-ninja" ]]; then
        BUILD_DIR="${ROOT_DIR}/build-ninja"
    elif [[ -d "${ROOT_DIR}/build" ]]; then
        BUILD_DIR="${ROOT_DIR}/build"
    else
        BUILD_DIR="${ROOT_DIR}/build-ninja"
    fi
fi

BUILD_TYPE="${BUILD_TYPE:-Debug}"
ARTEFACTS_BASE="${BUILD_DIR}/${PLUGIN_NAME}_artefacts"

CONFIG_DIR="${ARTEFACTS_BASE}/${BUILD_TYPE}"
if [[ ! -d "${CONFIG_DIR}" ]]; then
    CONFIG_DIR="$(ls -1d "${ARTEFACTS_BASE}"/* 2>/dev/null | head -n 1 || true)"
fi

if [[ -z "${CONFIG_DIR}" || ! -d "${CONFIG_DIR}" ]]; then
    echo "No artefacts found in ${ARTEFACTS_BASE}. Build first."
    exit 1
fi

AU_SRC="${CONFIG_DIR}/AU/${PLUGIN_NAME}.component"
VST3_SRC="${CONFIG_DIR}/VST3/${PLUGIN_NAME}.vst3"

AU_DEST="${HOME}/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component"
VST3_DEST="${HOME}/Library/Audio/Plug-Ins/VST3/${PLUGIN_NAME}.vst3"

copy_bundle() {
    local src="$1"
    local dest="$2"

    if [[ ! -d "${src}" ]]; then
        echo "Missing: ${src}"
        return
    fi

    mkdir -p "$(dirname "${dest}")"
    rm -rf "${dest}"
    ditto "${src}" "${dest}"

    if [[ "${CLEAR_QUARANTINE:-1}" == "1" ]]; then
        xattr -dr com.apple.quarantine "${dest}" || true
    fi

    echo "Installed: ${dest}"
}

copy_bundle "${AU_SRC}" "${AU_DEST}"
copy_bundle "${VST3_SRC}" "${VST3_DEST}"

echo "Done. If the host doesn't see updates, rescan plugins."
