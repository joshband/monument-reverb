#!/bin/bash
# Download high-quality PBR textures for hero knob rendering
# Resources: Poly Haven, FreePBR, Lightmap

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEXTURE_DIR="$SCRIPT_DIR/../assets/textures"

echo "=== Texture Asset Downloader ==="
echo "Target: $TEXTURE_DIR"
echo ""

mkdir -p "$TEXTURE_DIR"/{stone,brass,hdri}

# Function to download if not exists
download_if_missing() {
    local url="$1"
    local output="$2"

    if [ -f "$output" ]; then
        echo "✓ Already exists: $(basename "$output")"
    else
        echo "Downloading: $(basename "$output")..."
        curl -L "$url" -o "$output"
        echo "✓ Downloaded: $(basename "$output")"
    fi
}

echo "📦 Downloading stone textures (dark basalt)..."
# Poly Haven - Free basalt rock texture
# Note: Replace with actual direct download URL after browsing polyhaven.com/textures/rock
echo "  Manual step required:"
echo "  1. Visit: https://polyhaven.com/textures/rock"
echo "  2. Find dark basalt/stone texture (e.g., 'rocky_terrain_02')"
echo "  3. Download 4K JPG set (Color, Normal, Roughness, Displacement)"
echo "  4. Place in: $TEXTURE_DIR/stone/"
echo ""

echo "📦 Downloading brass textures (aged/weathered)..."
# FreePBR - Dull Brass PBR Material (2K)
echo "  Manual step required:"
echo "  1. Visit: https://freepbr.com/product/dull-brass-pbr/"
echo "  2. Download 2K texture set (includes all maps)"
echo "  3. Place in: $TEXTURE_DIR/brass/"
echo ""

echo "📦 Downloading studio HDRI..."
# Lightmap - 25 Free Studio HDRIs
echo "  Manual step required:"
echo "  1. Visit: https://www.lightmap.co.uk/hdrlightstudio/freestudiohdrimaps/"
echo "  2. Download pack (25 studio HDRIs, 3000x1500 EXR)"
echo "  3. Extract one suitable HDRI to: $TEXTURE_DIR/hdri/studio.exr"
echo ""

echo "🚀 Attempting automated downloads from Poly Haven..."
echo ""

# Download studio HDRI (2K is sufficient, faster)
download_if_missing \
    "https://dl.polyhaven.org/file/ph-assets/HDRIs/hdr/2k/studio_small_03_2k.hdr" \
    "$TEXTURE_DIR/hdri/studio.hdr"

# Download dark stone texture (2K for speed)
echo ""
echo "Downloading rocky terrain texture (dark stone)..."
if [ ! -f "$TEXTURE_DIR/stone/rocky_terrain_02_diff_2k.jpg" ]; then
    curl -L "https://dl.polyhaven.org/file/ph-assets/Textures/zip/2k/rocky_terrain_02_2k.zip" \
        -o "$TEXTURE_DIR/stone/rocky_terrain_02_2k.zip"

    echo "Extracting textures..."
    cd "$TEXTURE_DIR/stone"
    unzip -o rocky_terrain_02_2k.zip
    rm rocky_terrain_02_2k.zip
    cd - > /dev/null
    echo "✓ Extracted stone textures"
else
    echo "✓ Stone textures already exist"
fi

echo ""
echo "=== Automated download complete ==="
echo ""
echo "Next steps:"
echo "1. Download textures manually from links above"
echo "2. Place in $TEXTURE_DIR/{stone,brass,hdri}/"
echo "3. Run: ./scripts/run_hero_knob.sh"
