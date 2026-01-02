#!/bin/bash
set -euo pipefail

# Runs pluginval against a built Monument plugin. Set PLUGINVAL to the binary path.
# Example:
#   PLUGINVAL=/Applications/pluginval.app/Contents/MacOS/pluginval \
#   BUILD_DIR=build CONFIG=Release \
#   ./scripts/run_pluginval.sh

PLUGINVAL="${PLUGINVAL:-}"
if [[ -z "${PLUGINVAL}" ]]; then
    echo "Set PLUGINVAL to the pluginval binary path."
    exit 1
fi

PLUGIN_NAME="Monument"
BUILD_DIR="${BUILD_DIR:-build}"
CONFIG="${CONFIG:-Release}"
OUTPUT_DIR="${OUTPUT_DIR:-${BUILD_DIR}/pluginval-report}"

PLUGIN_PATH="${PLUGIN_PATH:-${BUILD_DIR}/${PLUGIN_NAME}_artefacts/${CONFIG}/VST3/${PLUGIN_NAME}.vst3}"
if [[ ! -e "${PLUGIN_PATH}" ]]; then
    echo "Plugin not found: ${PLUGIN_PATH}"
    exit 1
fi

mkdir -p "${OUTPUT_DIR}"
"${PLUGINVAL}" --strictness-level 10 --validate "${PLUGIN_PATH}" --output-dir "${OUTPUT_DIR}"

echo "pluginval report: ${OUTPUT_DIR}"
