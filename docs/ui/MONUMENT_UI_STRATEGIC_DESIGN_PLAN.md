# Monument Reverb: Strategic UI Design & Implementation Plan
## Photorealistic, Explorative, Thematic Interface Design

**Created:** 2026-01-03
**Vision:** Elemental, explorative audio plugin UI with pseudo-3D photorealistic render assets
**Aesthetic:** Knolling arrangement, baked lighting, PBR materials, skeuomorphic controls

---

## PART 1: DESIGN PHILOSOPHY

### Core Principles

**Monument is not a utility reverb—it's an architectural experience.**

The UI should feel like:
- **Exploring a monument** - Discovering chambers, spaces, and hidden resonances
- **Material tactility** - Stone, metal, glass, air—each element feels real and responds to touch
- **Time and decay** - Visual feedback for evolution, bloom, weathering, and memory
- **No standard knobs** - Every control is a portal into the space, not a parameter dial

### Elemental Theme System

Each plugin in your series embodies an element. Monument = **STONE + TIME**

**Visual Motifs:**
- **Stone textures**: Granite, marble, limestone, basalt—each preset has material identity
- **Architectural geometry**: Pillars, chambers, vaults, alcoves—3D depth illusion
- **Light and shadow**: Baked ambient occlusion, soft shadows, edge lighting
- **Temporal decay**: Weathering effects, patina, moss growth for Memory Echoes
- **Spatial indicators**: Depth cues via parallax, scale, and atmospheric perspective

---

## PART 2: CONTROL PARADIGM REDESIGN

### Current Controls (Standard Plugin Approach)
```
❌ 16 traditional knobs arranged in rows
❌ Dropdown preset menu
❌ Generic slider aesthetics
❌ No visual connection between parameters and sonic architecture
```

### Proposed: Architectural Navigation Interface

**Concept:** The UI is a **top-down cutaway view of Monument's interior** where you manipulate the space itself, not abstract parameters.

#### Control Zones (Spatial Organization)

**ZONE 1: THE CHAMBER (Center, 50% of UI)**
- **Visual:** Cross-section of a reverberant stone chamber with FDN delay lines as architectural columns
- **Controls embedded in architecture:**
  - **Time** → Pull/push chamber walls (perspective depth change)
  - **Mass** → Material selector wheel (stone/metal/glass preset swatches)
  - **Density** → Particle field intensity (visible echoes as glowing motes)
  - **Bloom** → Ceiling height / vault arch expansion
  - **Gravity** → Floor angle tilt (low-frequency weight visualization)

**ZONE 2: THE GEOMETRY (Left side, 20% of UI)**
- **Visual:** Architectural blueprint overlay with deformable grid lines
- **Controls:**
  - **Warp** → Twist/fold the blueprint grid (non-Euclidean distortion)
  - **Drift** → Slow motion vectors (arrows showing wall movement)

**ZONE 3: THE ATMOSPHERE (Right side, 20% of UI)**
- **Visual:** Atmospheric particle system with light shafts and dust motes
- **Controls:**
  - **Air** → Fog density (high-freq damping as visible haze)
  - **Width** → Stereo field width as spatial echo spread

**ZONE 4: THE MEMORY VAULT (Bottom, 10% of UI)**
- **Visual:** Archaeological dig site with layered strata showing captured fragments
- **Controls:**
  - **Memory Echoes toggle** → Uncover/cover the vault entrance
  - **Memory Decay** → Depth of excavation (older = deeper)
  - **Memory Drift** → Wind erosion effect on fragments

**ZONE 5: THE MACRO COSMOS (Top ribbon, 5% of UI)**
- **Visual:** Constellation map with interconnected nodes
- **Controls:**
  - **Material** → Star constellation morphs (soft ↔ hard materials)
  - **Topology** → Grid curvature (Euclidean ↔ impossible space)
  - **Viscosity** → Nebula density (air ↔ thick medium)
  - **Evolution** → Time-lapse animation speed (static ↔ evolving)
  - **Chaos** → Fractal noise overlay (stable ↔ chaotic)
  - **Elasticity** → Membrane tension (rigid ↔ elastic walls)

---

## PART 3: ASSET CREATION PIPELINE

### Phase 1: Midjourney Render Generation (Week 1)

**Objective:** Generate photorealistic control sprites with multiple states

#### Prompt Engineering Strategy

**Base Prompt Template:**
```
Knolling arrangement, top-down view, deadpan, close-up of [CONTROL_TYPE],
[MATERIAL_THEME], [LIGHTING_CONDITION], photorealistic, studio photography,
soft shadows, ambient occlusion, [ELEMENT_THEME] aesthetic --ar 16:9 --v 6.1
--style raw --quality 2
```

**Control Types to Generate:**

1. **Chamber Wall Segments** (5 states: depth 0%, 25%, 50%, 75%, 100%)
   - Prompt: `stone chamber wall panel, push-pull depth, architectural cross-section, granite texture`
   - Output: 5 images × 4 material variants (granite, marble, basalt, glass) = **20 assets**

2. **Material Selector Wheel** (8 positions)
   - Prompt: `circular stone material swatch wheel, segmented, deadpan, knolling arrangement`
   - Output: 1 base wheel + 8 highlight states = **9 assets**

3. **Particle Field Overlays** (10 density levels)
   - Prompt: `floating dust motes, light shafts, atmospheric particles, cathedral light rays`
   - Output: 10 density variations × 2 color temperatures = **20 assets**

4. **Architectural Blueprint Grid** (12 warp states)
   - Prompt: `deformable architectural grid, blueprint lines, perspective distortion, Escher-like geometry`
   - Output: 12 distortion keyframes = **12 assets**

5. **Memory Vault Excavation** (6 depth layers)
   - Prompt: `archaeological strata, layered stone, weathered fragments, time-aged textures`
   - Output: 6 depth layers = **6 assets**

6. **Macro Constellation Nodes** (6 constellations × 11 morph states)
   - Prompt: `star constellation map, interconnected nodes, cosmic web, celestial navigation`
   - Output: 66 constellation morphs = **66 assets**

**Total Initial Asset Count:** ~133 base renders

#### Render Specifications
- **Resolution:** 4K (3840×2160) for high-DPI displays
- **Format:** PNG with alpha channel
- **Color Space:** sRGB for consistent rendering
- **Bit Depth:** 16-bit for gradient smoothness

---

### Phase 2: PBR Map Extraction (Week 1-2)

**Objective:** Run Materialize pipeline on all Midjourney outputs to generate PBR maps

#### Using Your Materialize Pipeline

```bash
# Batch process all Midjourney renders
materialize \
  --in ~/Desktop/midjourney_session \
  --out ~/Documents/monument-reverb-assets/pbr_extracted \
  --size 2048 \
  --workers 8 \
  --overwrite

# Output per asset:
# - canonical.png (normalized)
# - albedo.png (base color, no lighting)
# - normal.png (surface detail bumps)
# - roughness.png (surface micro-scatter)
# - metallic.png (conductor vs dielectric)
# - ao.png (ambient occlusion - contact shadows)
# - height.png (displacement for parallax)
# - material.tokens.json (W3C DTCG metadata)
```

#### PBR Map Usage in JUCE

**Map Type** | **JUCE Rendering Use Case**
--- | ---
**Albedo** | Base sprite texture (unlit color)
**Normal** | Fake 3D lighting response (optional)
**AO** | Multiply blend for contact shadows
**Roughness** | Specular highlight masking (gloss areas)
**Height** | Parallax scrolling for pseudo-3D depth

**Rendering Strategy in JUCE:**
1. **Static render:** Albedo × AO (baked look, fastest)
2. **Interactive states:** Blend between albedo variants (hover, active, disabled)
3. **Advanced (optional):** Real-time normal map lighting with mouse position as light source

---

### Phase 3: Asset Compositing & Slicing (Week 2)

**Objective:** Convert PBR maps into JUCE-compatible sprite sheets

#### Sprite Sheet Creation

**Tool:** Adobe Photoshop, Figma, or custom Python script

**Example: Chamber Wall Control (5 states)**
```
Layout: Horizontal sprite sheet 10240×2048 (5 frames × 2048px each)
Frame 0: Depth 0%  (wall closest)
Frame 1: Depth 25%
Frame 2: Depth 50%
Frame 3: Depth 75%
Frame 4: Depth 100% (wall farthest)
```

**Compositing Layers (per frame):**
```
Layer Stack (top to bottom):
├─ Specular highlights (roughness-based)
├─ Edge lighting (normal map-based)
├─ Albedo (base color)
├─ Ambient occlusion (multiply blend)
└─ Shadow layer (baked soft shadows)
```

**Export Settings:**
- Format: PNG-24 with alpha
- Compression: Maximum quality (large files okay for audio plugins)
- Naming: `chamber_wall_granite_states.png`

#### Automated Slicing Script

```python
# scripts/slice_sprites.py
from PIL import Image

def slice_horizontal_sprite(input_path, output_dir, frame_count):
    """Split horizontal sprite sheet into individual frames."""
    img = Image.open(input_path)
    width, height = img.size
    frame_width = width // frame_count

    for i in range(frame_count):
        left = i * frame_width
        frame = img.crop((left, 0, left + frame_width, height))
        frame.save(f"{output_dir}/frame_{i:03d}.png")

# Usage
slice_horizontal_sprite(
    "assets/chamber_wall_granite_states.png",
    "assets/sliced/chamber_wall",
    frame_count=5
)
```

---

### Phase 4: JUCE Integration (Week 3-4)

**Objective:** Implement custom JUCE components that render PBR-backed sprites

#### JUCE Component Architecture

**Custom Base Class:** `MonumentControl`

```cpp
// plugin/ui/MonumentControl.h
class MonumentControl : public juce::Component {
public:
    void setSpriteSheet(const juce::Image& sheet, int frameCount);
    void setState(float normalizedValue);  // 0.0-1.0 → frame interpolation

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

private:
    juce::Image spriteSheet;
    int frameCount = 1;
    float currentState = 0.0f;
    bool isHovered = false;

    // Interpolate between frames for smooth animation
    juce::Image getInterpolatedFrame() const;
};
```

**Example: Chamber Wall Control**

```cpp
// plugin/ui/ChamberWallControl.cpp
ChamberWallControl::ChamberWallControl(AudioProcessorValueTreeState& apvts)
    : timeAttachment(apvts, "time", [this](float value) { setState(value); })
{
    // Load pre-composited sprite sheet
    auto spriteData = juce::ImageFileFormat::loadFrom(
        BinaryData::chamber_wall_granite_states_png,
        BinaryData::chamber_wall_granite_states_pngSize
    );
    setSpriteSheet(spriteData, 5);  // 5 frames for 5 depth states
}

void ChamberWallControl::paint(juce::Graphics& g) {
    // Get interpolated frame based on current Time parameter value
    auto frame = getInterpolatedFrame();

    // Draw with subtle hover glow
    if (isHovered) {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
    }

    g.drawImage(frame, getLocalBounds().toFloat(),
                juce::RectanglePlacement::centred);
}

juce::Image ChamberWallControl::getInterpolatedFrame() const {
    // Determine which two frames to blend
    float scaledState = currentState * (frameCount - 1);
    int frameA = static_cast<int>(scaledState);
    int frameB = juce::jmin(frameA + 1, frameCount - 1);
    float blend = scaledState - frameA;

    // Extract frames from sprite sheet
    int frameWidth = spriteSheet.getWidth() / frameCount;
    juce::Image imgA = spriteSheet.getClippedImage(
        juce::Rectangle<int>(frameA * frameWidth, 0, frameWidth, spriteSheet.getHeight())
    );
    juce::Image imgB = spriteSheet.getClippedImage(
        juce::Rectangle<int>(frameB * frameWidth, 0, frameWidth, spriteSheet.getHeight())
    );

    // Blend using Graphics context
    juce::Image blended(juce::Image::ARGB, frameWidth, spriteSheet.getHeight(), true);
    juce::Graphics g(blended);
    g.setOpacity(1.0f - blend);
    g.drawImage(imgA, blended.getBounds().toFloat());
    g.setOpacity(blend);
    g.drawImage(imgB, blended.getBounds().toFloat());

    return blended;
}
```

#### BinaryData Integration

```cpp
// CMakeLists.txt - Add sprite sheets to binary resources
juce_add_binary_data(MonumentAssets SOURCES
    assets/chamber_wall_granite_states.png
    assets/chamber_wall_marble_states.png
    assets/material_wheel_selector.png
    assets/particle_field_density_00.png
    assets/particle_field_density_01.png
    # ... all 133 assets
)
target_link_libraries(Monument PRIVATE MonumentAssets)
```

**Asset Size Considerations:**
- Each sprite sheet: ~2-5 MB (PNG-24, 2048×2048)
- Total binary size: ~200-400 MB (high-quality photorealistic UI)
- **Optimization:** Use lossy WebP for non-critical assets (50% size reduction)

---

## PART 4: INTERACTION DESIGN

### Explorative, Non-Linear Parameter Control

**Traditional Approach (❌):**
- User adjusts Time knob from 0-100%
- Direct 1:1 mapping to reverb time

**Monument Approach (✅):**
- User **pushes chamber walls** inward/outward
- Visual feedback: Perspective depth changes
- Haptic feedback: Mouse cursor changes to push/pull icon
- Sonic feedback: Reverb time evolves smoothly
- **Discover relationships:** Pushing walls also subtly affects Mass (architectural coupling)

### Control Interaction Patterns

#### 1. **Drag-to-Morph (Chamber Wall, Material Wheel)**
- **Gesture:** Click + drag vertically or horizontally
- **Visual:** Control smoothly interpolates through sprite sheet frames
- **Parameter:** Maps gesture distance to normalized parameter value
- **Example:** Dragging chamber wall up/down pushes walls closer/farther

#### 2. **Click-to-Select (Material Wheel, Preset Categories)**
- **Gesture:** Click on wheel segment
- **Visual:** Wheel rotates to selected segment with spring physics
- **Parameter:** Discrete preset selection (8 material types)
- **Example:** Click marble segment → entire chamber changes to marble texture

#### 3. **Hover-to-Reveal (Memory Vault, Macro Constellation)**
- **Gesture:** Mouse hover over region
- **Visual:** Fade-in UI overlay with parameter tooltips
- **Parameter:** No immediate change (exploration mode)
- **Example:** Hover over Memory Vault → see captured fragment previews

#### 4. **Pinch-to-Zoom (Architectural Blueprint)**
- **Gesture:** Scroll wheel or two-finger trackpad pinch
- **Visual:** Blueprint grid zooms in/out with parallax effect
- **Parameter:** Controls Warp intensity (geometry distortion)
- **Example:** Zoom out → grid warps into Escher-like impossible geometry

#### 5. **Orbital Drag (Macro Constellation Nodes)**
- **Gesture:** Click + circular drag around constellation center
- **Visual:** Constellation rotates, nodes rearrange
- **Parameter:** Morphs between macro parameter presets
- **Example:** Rotate Material constellation → morph from soft to hard materials

---

## PART 5: MEMORY ECHOES VISUAL DESIGN

### Concept: Archaeological Time Layers

**Visual Metaphor:** Monument's memory system is a **dig site** where older echoes are buried deeper in strata.

#### UI Elements

**1. The Excavation Viewport (Bottom panel, 800×200px)**

**Visual Layers (composited PBR assets):**
```
Surface Layer (0s ago)   ─────────────────────────  [Recent captures, bright]
Weathered Layer (10s ago) ────────────────────────  [Slight patina, moss]
Deep Layer (30s ago)     ─────────────────────────  [Eroded, dark]
Ancient Layer (60s+ ago) ─────────────────────────  [Crumbling, faint]
```

**Fragment Representation:**
- Each captured memory = a glowing shard/crystal embedded in strata
- Brightness = signal intensity
- Color = frequency character (warm bass, cool treble)
- Position = temporal depth (vertical axis = age)

**Interaction:**
- **Hover over fragment** → Tooltip shows capture timestamp and decay state
- **Click fragment** → Manually trigger recall (surface that memory)
- **Drag decay slider** → Layers compress/expand (visual time dilation)

**2. Memory Echo Parameters**

Traditional controls reimagined:

**Parameter** | **Visual Control** | **Interaction**
--- | --- | ---
**Memory On/Off** | Vault door (open/closed) | Click to toggle
**Memory Depth** | Trowel tool (dig deeper) | Vertical drag
**Memory Decay** | Erosion wind effect | Horizontal drag (slow → fast)
**Memory Drift** | Geological shift vectors | Twist gesture (±15¢ pitch wobble)

---

## PART 6: PRESET SYSTEM REDESIGN

### Explorative Preset Browser

**Concept:** Presets are architectural **blueprints** you overlay onto the chamber, not dropdowns.

#### Visual Design: Blueprint Carousel

**Layout:**
```
┌─────────────────────────────────────────────┐
│  🏛️ FOUNDATIONAL SPACES (0-5)             │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐      │
│  │ Init │ │Cathedral│ Stone │ Event │ ...  │
│  └──────┘ └──────┘ └──────┘ └──────┘      │
│                                             │
│  🌱 LIVING SPACES (6-11)                   │
│  ┌──────┐ ┌──────┐ ┌──────┐               │
│  │Breathing│Drifting│Chaos │ ...          │
│  └──────┘ └──────┘ └──────┘               │
│                                             │
│  🌀 EVOLVING SPACES (18-22)                │
│  ┌──────┐ ┌──────┐ ┌──────┐               │
│  │Living │ Event │Pillars│ ...            │
│  │Pillars│Horizon│       │                │
│  └──────┘ └──────┘ └──────┘               │
└─────────────────────────────────────────────┘
```

**Each Preset Card:**
- **Visual:** Miniature chamber cross-section (PBR-rendered thumbnail)
- **Hover:** Card elevates with subtle shadow
- **Click:** Blueprint animates onto main chamber (1-second crossfade)
- **Metadata:** Preset name overlaid in engraved typography

#### Preset Categories as Architectural Zones

**Category** | **Visual Theme** | **Icon**
--- | --- | ---
Foundational Spaces | Brutalist concrete | 🏛️ Temple
Living Spaces | Organic growth (moss, vines) | 🌱 Seedling
Remembering Spaces | Archaeological ruins | 📜 Scroll
Time-Bent / Abstract | Escher geometry | ⏳ Hourglass
Evolving Spaces | Glowing modulation nodes | 🌀 Vortex

---

## PART 7: IMPLEMENTATION ROADMAP

### Week 1-2: Asset Generation & Extraction
- [ ] Generate 133 base Midjourney renders (Chamber, Material, Particles, Blueprint, Vault, Macros)
- [ ] Run Materialize pipeline to extract PBR maps (albedo, normal, AO, roughness, metallic, height)
- [ ] Review confidence scores in material.tokens.json
- [ ] Curate highest-quality renders (>0.7 confidence) for final UI

### Week 3: Asset Compositing & Slicing
- [ ] Composite PBR layers in Photoshop (albedo + AO + edge lighting)
- [ ] Create sprite sheets for animated controls (5-12 frames each)
- [ ] Export PNG-24 assets at 2048px resolution
- [ ] Organize assets into directory structure: `assets/ui/{chamber,materials,particles,blueprint,vault,macros}/`

### Week 4-5: JUCE Component Development
- [ ] Implement `MonumentControl` base class (sprite sheet rendering, frame interpolation)
- [ ] Build 6 custom control components:
  - `ChamberWallControl` (Time)
  - `MaterialWheelControl` (Mass)
  - `ParticleFieldControl` (Density)
  - `BlueprintGridControl` (Warp)
  - `MemoryVaultControl` (Memory parameters)
  - `MacroConstellationControl` (6 macro parameters)
- [ ] Integrate with APVTS using ParameterAttachment pattern
- [ ] Add BinaryData resources to CMakeLists.txt

### Week 6: Layout & Interaction Polish
- [ ] Design main editor layout (5 zones: Chamber, Geometry, Atmosphere, Memory, Macros)
- [ ] Implement custom mouse cursors (push/pull, rotate, dig, orbit)
- [ ] Add hover states and tooltips
- [ ] Implement smooth transitions (spring physics for wheel rotation, easing for wall depth)

### Week 7: Preset Browser Redesign
- [ ] Generate blueprint thumbnail renders for all 23 factory presets
- [ ] Implement horizontal carousel with smooth scrolling
- [ ] Add category tabs (5 architectural zones)
- [ ] Implement preset crossfade animation (1-second fade with parameter smoothing)

### Week 8: Testing & Optimization
- [ ] Performance profiling (target <5% CPU for UI rendering)
- [ ] High-DPI display testing (Retina, 4K monitors)
- [ ] DAW compatibility testing (Ableton, Logic, Reaper, FL Studio)
- [ ] Accessibility review (keyboard navigation, screen reader compatibility)

### Week 9-10: Documentation & Release
- [ ] Create visual user manual (annotated screenshots, not text)
- [ ] Record demo videos (preset walkthroughs, interaction gestures)
- [ ] Publish release notes with UI redesign highlights
- [ ] Collect user feedback on explorative UI paradigm

---

## PART 8: TECHNICAL CHALLENGES & SOLUTIONS

### Challenge 1: Large Binary Size (200-400 MB)

**Problem:** 133 high-res PBR sprite sheets = huge plugin binary

**Solutions:**
1. **WebP compression** for non-critical assets (50% size reduction, minimal quality loss)
2. **Separate asset pack** distributed via installer (not embedded in binary)
3. **LOD system** (Load high-res assets only on high-DPI displays)
4. **Lazy loading** (Load control sprites on-demand, not at plugin initialization)

**Recommended:** Use WebP for particle fields and blueprints (animated, subtle detail), keep PNG for hero controls (chamber, material wheel)

---

### Challenge 2: Smooth Frame Interpolation Performance

**Problem:** Blending two 2048×2048 images per paint() call = expensive

**Solutions:**
1. **Pre-render blended frames** (cache 100 interpolated frames at load time)
2. **GPU-accelerated blending** (use JUCE OpenGL context if available)
3. **Reduce resolution** (2048px may be overkill, 1024px often sufficient)
4. **Frame skipping** (interpolate every N frames instead of continuous)

**Recommended:** Pre-render 50 blended frames per control, cache in memory (~100 MB RAM for all controls)

---

### Challenge 3: Maintaining APVTS Parameter Sync

**Problem:** Visual controls (drag wall, rotate wheel) must sync with host automation

**Solutions:**
1. **Bidirectional binding:** Visual control listens to APVTS, APVTS listens to visual control
2. **ParameterAttachment:** Use JUCE's AudioProcessorValueTreeState::SliderAttachment pattern, but for custom components
3. **Gesture flags:** Set `ParameterAttachment::beginGesture()` on mouse down, `endGesture()` on mouse up for DAW automation recording

**Implementation:**
```cpp
class ChamberWallControl : public MonumentControl {
    using Attachment = AudioProcessorValueTreeState::ParameterAttachment;
    std::unique_ptr<Attachment> timeAttachment;

    ChamberWallControl(AudioProcessorValueTreeState& apvts) {
        timeAttachment = std::make_unique<Attachment>(
            apvts, "time",
            [this](float value) { setState(value); }  // APVTS → Visual
        );
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        float newValue = mapDragToNormalizedValue(e);
        timeAttachment->setValueAsPartOfGesture(newValue);  // Visual → APVTS
    }
};
```

---

### Challenge 4: Midjourney Render Consistency

**Problem:** Midjourney generates slight variations even with same prompt

**Solutions:**
1. **Use --seed flag** for reproducible renders
2. **Generate 4× renders per control state**, curate best one
3. **Post-process normalization** (histogram matching, color calibration)
4. **Material tokens validation** (use materialize confidence scores to filter low-quality extractions)

**Example:**
```bash
# Generate 4 variations of chamber wall depth 50%
/imagine prompt: stone chamber wall panel, 50% depth, architectural cross-section
             --seed 12345 --v 6.1 --quality 2

# Materialize extracts PBR maps
materialize --in chamber_wall_depth_50 --out pbr_extracted

# Filter by confidence (keep only >0.7)
python scripts/filter_high_confidence.py \
    --manifest pbr_extracted/manifest.json \
    --min-confidence 0.7 \
    --output final_assets/
```

---

## PART 9: MEMORY ECHOES REINTEGRATION

### Current Status (2026-01-03)

Memory Echoes was **extracted to standalone repository** for independent development (per CHANGELOG.md).

**Extraction rationale:**
- Monument v1.0-v1.5 release line is **intentionally memory-free**
- Planned reintegration: **v1.6+** after Memory Echoes stabilization

### Reintegration Plan for UI Design

**Objective:** Design Memory Echoes UI **now** (during Phase 2 UI work) even though DSP reintegration happens later

**Why design UI first:**
1. UI visual language influences DSP parameter design
2. Excavation viewport needs space reserved in layout
3. Memory fragment visualization affects preset design decisions
4. Earlier user feedback on explorative UI paradigm

**Implementation Strategy:**

#### Stage 1: UI-Only Prototype (Week 5)
- Design and implement Memory Vault UI components (excavation viewport, fragment crystals, erosion controls)
- **Stub DSP:** Memory toggle does nothing, but UI reacts to parameter changes
- **Static visualization:** Show pre-rendered fragment positions (no real captures)
- **Purpose:** Test UI/UX before committing to DSP reintegration

#### Stage 2: DSP Reintegration (v1.6, Month 3-4)
- Merge Memory Echoes DSP from standalone repo
- Wire Memory Vault UI to live capture/recall system
- Real-time fragment visualization (actual buffer captures)
- Performance testing with full UI + Memory DSP

#### Stage 3: Memory-Specific Presets (v1.7, Month 5)
- Create 5 new "Remembering Spaces" presets with complex Memory routing
- Example: "Echoes of Eternity" - 60s decay, deep strata, frequent recalls
- Document Memory Echoes workflow in visual user manual

### UI Design Specifications (For v1.6 Reintegration)

**Excavation Viewport:**
- **Position:** Bottom panel, 800×200px
- **Render:** 4-layer PBR composite (surface → ancient strata)
- **Fragment crystals:** 32 max simultaneous captures (FIFO)
- **Animation:** Fragments sink slowly from surface to deep layer (0.5px/sec)
- **Interaction:** Click fragment → manual recall (trigger surfaceMemory())

**Memory Decay Control:**
- **Visual:** Wind erosion effect (particle animation speed)
- **Parameter mapping:** 0.0 (instant decay) → 1.0 (eternal preservation)
- **Asset:** `assets/memory_vault/erosion_wind_states.png` (10 frames)

**Memory Drift Control:**
- **Visual:** Geological shift vectors (arrows showing pitch wobble)
- **Parameter mapping:** -15¢ ← 0.0 → +15¢
- **Asset:** `assets/memory_vault/drift_vectors.png` (24 frames, ±15 states)

---

## PART 10: NEXT STEPS & ACTION ITEMS

### Immediate Actions (This Week)

1. **Finalize visual direction:**
   - Review all 182 Midjourney renders in `~/Desktop/midjourney_session`
   - Select 3-5 "hero" renders that best represent Monument's aesthetic
   - Curate color palette (stone grays, warm ambers, cool blues)

2. **Generate missing control states:**
   - Chamber wall depth (5 states × 4 materials = 20 renders)
   - Material wheel (8 segments + 8 highlights = 16 renders)
   - Particle field (10 density levels = 10 renders)
   - Blueprint grid (12 warp states = 12 renders)
   - **Total new renders needed: ~58**

3. **Run Materialize pipeline:**
   ```bash
   materialize \
     --in ~/Desktop/midjourney_session \
     --out ~/Documents/monument-reverb-assets/pbr_extracted \
     --size 2048 \
     --workers 8 \
     --overwrite
   ```

4. **Set up asset directory structure:**
   ```
   monument-reverb/assets/ui/
   ├── chamber/
   │   ├── wall_granite_states.png
   │   ├── wall_marble_states.png
   │   └── ...
   ├── materials/
   │   ├── selector_wheel_base.png
   │   ├── selector_wheel_highlight_*.png
   │   └── ...
   ├── particles/
   ├── blueprint/
   ├── vault/
   └── macros/
   ```

5. **Update ARCHITECTURE_REVIEW.md:**
   - Remove ASCII diagrams
   - Add visual descriptions of UI zones
   - Include Memory Echoes reintegration plan
   - Document PBR asset pipeline

### Questions for You

1. **Material theme preference:** Do you want all 4 material variants (granite, marble, basalt, glass) for the chamber, or start with 1 hero material?
2. **UI complexity level:** Full 5-zone layout (Chamber + Geometry + Atmosphere + Memory + Macros) or simplified 3-zone version first?
3. **Memory Echoes priority:** Design full Memory Vault UI now (Week 5) or defer until DSP reintegration (v1.6)?
4. **Asset generation workload:** Should I help generate specific Midjourney prompts for the 58 missing control states?
5. **Alternative tools:** Do you want to explore Blender renders instead of Midjourney for more controllable asset generation?

### Long-term Strategic Questions

1. **Elemental plugin series:** What are the other elements in your series? (Fire, Water, Air, Light, Shadow?)
2. **Cross-plugin visual language:** Should Monument's UI aesthetic establish conventions for the entire series?
3. **Commercialization:** Is this series intended for commercial sale or open-source creative tool distribution?
4. **Modular asset library:** Should we build a shared asset library (`elemental-ui-kit`) that all plugins in the series use?

---

**Let's start with questions 1-5 to define scope, then dive into implementation!**
