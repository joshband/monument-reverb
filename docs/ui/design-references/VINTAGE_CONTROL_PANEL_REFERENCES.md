# Vintage Industrial Control Panel Design References

## Overview
This document catalogs design inspiration from vintage industrial control panels featuring brutalist geometric aesthetics and premium tactile elements.

**Reference Images Location:** `docs/ui/design-references/` (save uploaded images here)
- `vintage-panel-01.jpg` - Complex mixed-geometry panel
- `vintage-panel-02.jpg` - Organized grid layout with varied controls
- `vintage-panel-03.jpg` - Individual component showcase (15 units)
- `vintage-panel-04.jpg` - LED-lit knurled knobs with amber indicators

## Key Design Elements Observed

### 1. Knob Cap Geometries
- **Domed Caps:** Smooth hemispheres with varied radii (15mm - 40mm)
- **Flat/Low Profile:** Minimal height, emphasis on knurling texture
- **Organic Shapes:** Rounded rectangles, elliptical caps
- **Faceted/Architectural:** Angular multi-level constructions

### 2. Material & Finish Treatments
- **Primary:** Brushed aluminum/steel (pewter gray tones)
- **Accent Finishes:** Warm bronze/copper patina
- **Surface Contrast:** Mix of smooth, knurled, and cast textures
- **Weathering:** Subtle oxidation, aged industrial aesthetic

### 3. Embedded LED Indicators
**Reference:** Image 4 - Knurled knobs with backlit amber LEDs
- **Placement:** Recessed in panel behind/beside knobs
- **Color Temperature:** Warm amber (2200K-2800K)
- **Intensity:** Soft glow, not glare
- **Function:** Active parameter indication, visual feedback

### 4. Depth Architecture
- **2-Level (Current):** Base panel + raised knob assembly
- **3-Level (Observed):** Panel → recess frame → knob housing → cap
- **4-Level (Premium):** Deep shadow boxes with multiple mounting planes

### 5. Detail Elements

#### Tick Marks & Indicators
- Variable height engraved lines (0.3mm - 1.5mm deep)
- Radial pattern around knob perimeter (12-60 divisions)
- Some with contrasting fill (white, silver)

#### Mounting Hardware
- Visible Phillips/hex screws (authentic industrial feel)
- Recessed bezels and trim rings
- Corner fasteners on module panels

#### Panel Integration
- Flush mounting in recessed frames
- Raised borders creating shadow lines
- Mix of square and circular apertures

## Implementation Priority for Monument Reverb

### Phase 1: LED Ring Layer (RECOMMENDED)
**Why:** Most impactful, single new layer, works with current geometry

**Technical Approach:**
```python
def create_led_ring_layer():
    """
    Add glowing LED ring beneath knob cap
    - Radius: knob_radius * 1.15
    - Thickness: 1.0mm
    - Emissive shader: warm amber (2500K)
    - Opacity: 0.7 (subtle glow)
    """
```

**Blender Implementation:**
- New layer: `Layer_11_LED_Ring`
- Material: Emission shader + transparent BSDF mix
- Color: (1.0, 0.6, 0.2) RGB warm amber
- Strength: 3.0 (visible but not overwhelming)

### Phase 2: Concave Cap Geometry
**Ergonomics-focused** - Better finger grip, vintage feel

**Modification:**
- Replace dome with inverted curve
- Depth: 2mm concave
- Radius: Same as current (22.5mm)
- Benefits: Better tactile feedback, unique silhouette

### Phase 3: Enhanced Tick Marks
**Visual detail** - Variable height, engraved depth

**Layers:**
- Deeper grooves (0.5mm → 1.0mm)
- Variable lengths (major/minor divisions)
- Optional: Contrasting fill material

### Phase 4: 3-Level Depth Architecture
**Premium look** - More pronounced shadows

**Structure:**
- Level 1: Base panel (0mm)
- Level 2: Recess frame (-2mm)
- Level 3: Knob housing (+2mm)
- Level 4: Cap dome (+6mm)

## Material Palette (From References)

### Metals
- **Primary Base:** Brushed aluminum #8C8C8C
- **Dark Accent:** Gun metal #4A4A4A
- **Warm Accent:** Brushed bronze #B8956A
- **Highlight:** Polished chrome #D4D4D4

### LED Colors
- **Active Warm:** Amber 2500K (#FFA040)
- **Active Cool:** Soft white 4000K (#FFF4E0)
- **Standby Dim:** Orange 2000K (#FF7820, 30% strength)

## Composite Assembly Ideas

### "Sandwich" Design (Image 3, bottom-right)
- Multiple stacked layers visible from side view
- Each layer different diameter (largest at base)
- Creates architectural "stepped pyramid" look

### Split-Ring Dual Control (Image 1, center area)
- Outer ring + inner knob (concentric)
- Could represent modulation depth + rate
- Complex but highly functional

## Next Steps

1. **Save reference images** to `docs/ui/design-references/` with filenames above
2. **Implement LED ring layer** (scripts/generate_knob_blender_enhanced.py)
3. **Test composite rendering** with new layer
4. **Generate preview** (scripts/preview_knob_composite_enhanced.py)
5. **Iterate** based on visual results

## Notes for Future Sessions

**Quick Context Recap:**
- Current knobs: 2-level domed caps with tick marks (10 layers)
- Enhanced script: `scripts/generate_knob_blender_enhanced.py`
- Preview script: `scripts/preview_knob_composite_enhanced.py`
- Output: `assets/ui/knobs_enhanced/`
- Documentation: `docs/ui/ENHANCED_UI_SUMMARY.md`

**Recommended First Task:** Add LED ring layer (emission shader, warm amber)
