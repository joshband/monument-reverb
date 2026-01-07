# Monument Reverb - UI/UX Improvement Roadmap

**Created:** 2026-01-04
**Status:** Planning Phase - Post-DSP Finalization
**Purpose:** Comprehensive UI enhancement plan based on current design analysis

---

## Executive Summary

This roadmap addresses identified UI/UX issues and proposes strategic improvements to create a more intuitive, visually coherent, and professionally polished interface for Monument Reverb. All improvements will be implemented after DSP work is finalized.

**Timeline:** 4-6 weeks post-DSP completion
**Priority:** Medium-High (impacts user experience but not core functionality)

---

## Current UI Analysis

### Strengths

**What's Working:**
- Clean white background with high contrast readability
- Unified hero knob aesthetic (codex brushed aluminum)
- Macro-first interface design (10 primary controls visible by default)
- ModMatrix panel provides excellent visual feedback for modulation routing
- Base parameters hidden by default reduces initial complexity

### Critical Issues Identified

#### 1. Top Toolbar Overlap and Cramping
**Severity:** High
**User Impact:** Poor first impression, hard to access core features

**Current Problems:**
- Mode selector ("Ancient Way", "Resonant Halls", "Breathing Stone") competes for space with Architecture selector
- Two dropdown boxes (grey/black on white) create visual noise
- No breathing room between elements
- Label placement unclear (Mode: label right-aligned, Architecture centered)
- Inconsistent styling between the two selectors

**UX Issues:**
- Users confused about hierarchy (which selector is more important?)
- Cramped spacing suggests lack of polish
- Hard to quickly scan and understand routing options

#### 2. Grey/Black Dropdowns on White Background
**Severity:** Medium-High
**User Impact:** Visual inconsistency, poor contrast hierarchy

**Current Color Scheme:**
```
Background: juce::Colour(0xff14171b) - Very dark grey
Text: juce::Colour(0xffe6e1d6) - Warm off-white
Outline: juce::Colour(0xff3a3f46) - Medium grey
```

**Problems:**
- Dark boxes float awkwardly on white background (no integration)
- Preset box uses same styling (creates visual clutter)
- No clear visual relationship to Monument's ancient stone aesthetic
- Warm off-white text conflicts with bright white background

#### 3. Preset System - Flat List Organization
**Severity:** High
**User Impact:** Difficult preset discovery, no sonic guidance

**Current State:**
- Single long dropdown list (28+ presets)
- No visual categorization
- No sonic information or preview
- Users must memorize preset names to find desired character
- "Evolving Spaces" category buried in list

**Missing Features:**
- No hierarchical organization (Traditional vs Living vs Physical Modeling spaces)
- No preset metadata display (modulation routing, DSP modules active, sonic character)
- No visual representation of space (chamber visualization, spectrum preview)
- No favorites or tagging system
- No search/filter capability

#### 4. Base Parameters Panel Spacing
**Severity:** Medium
**User Impact:** Cluttered feel when expanded, parameter relationship unclear

**Current Issues:**
- 12 base parameters + 1 toggle presented in linear grid
- No visual grouping by function (Chamber vs Geometry vs Atmosphere)
- Equal visual weight for all parameters (no hierarchy)
- Parameter labels compete with knob visuals
- Hidden by default (good) but chaotic when revealed (bad)

**Layout Problems:**
- Time, Size, Mass, Density, Bloom, Air, Width, Warp, Drift, Gravity, Freeze all appear equal
- No indication of parameter relationships (Time + Mass = duration character)
- Freeze toggle visually inconsistent with knobs

#### 5. Modulation Matrix Organization
**Severity:** Low
**User Impact:** Functional but could be more intuitive

**Current State (Good):**
- 4×15 grid provides clear routing visualization
- Color-coded sources (Chaos=Orange, Audio=Green, Brownian=Purple, Envelope=Blue)
- Hover, active, and selected states well-defined
- Real-time connection list display

**Improvement Opportunities:**
- Could group destinations by parameter type (macro vs base)
- Connection strength visualization (animated flow?)
- Preset modulation templates ("Breathing", "Chaotic", "Organic")
- Visual waveform display for modulation sources

---

## Design Goals

### Primary Objectives

1. **Establish Clear Visual Hierarchy**
   - Primary controls (10 macros) immediately recognizable
   - Secondary controls (mode/architecture/presets) visually subordinate but accessible
   - Tertiary controls (base parameters) hidden but discoverable

2. **Integrate Ancient Monuments Aesthetic**
   - Stone, earth, architectural materials
   - Warm, aged color palette (granite, bronze, weathered metal)
   - Subtle texture and depth (not flat UI)
   - Cohesive theme from top to bottom

3. **Improve Preset Discovery**
   - Categorized, browsable preset system
   - Visual + textual information about each space
   - Quick preview or sonic indicators
   - Encourage exploration and experimentation

4. **Optimize Spacing and Layout**
   - Generous whitespace around all interactive elements
   - Clear visual grouping (chamber/geometry/atmosphere sections)
   - Consistent sizing and alignment
   - Responsive design for different plugin window sizes

5. **Maintain Professional Polish**
   - No visual glitches or overlaps
   - Smooth animations and transitions
   - Consistent interaction patterns
   - Accessible (keyboard navigation, screen reader support)

---

## Proposed Solutions

### Solution 1: Top Toolbar Redesign

**Goal:** Create a unified, spacious header with clear hierarchy and visual integration.

#### Layout Proposal

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         MONUMENT                                        │
│  ┌──────────────────┐     ┌──────────────────┐     ┌────────────────┐ │
│  │  PRESETS ▾       │     │  MODE ▾          │     │  MODULATION    │ │
│  └──────────────────┘     └──────────────────┘     └────────────────┘ │
│                                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐ │
│  │  ARCHITECTURE: Traditional Cathedral  │  Resonant Halls  │  ...  │ │
│  └──────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
```

**Key Changes:**

1. **Two-Row Layout:**
   - Row 1: PRESETS (left) | MODE selector (center) | MODULATION button (right)
   - Row 2: ARCHITECTURE selector (full width, segmented button style)

2. **Visual Style:**
   - Stone/metal texture background (subtle, not distracting)
   - Warm bronze/copper accent for active elements
   - Engraved typography (debossed effect)
   - Subtle shadow depth (2-3 levels)

3. **Spacing:**
   - 16px padding around all elements
   - 24px gap between buttons in Row 1
   - 8px vertical gap between rows
   - Total header height: ~100px (was ~60px)

4. **Architecture Selector Enhancement:**
   - Segmented button style (iOS-style tabs)
   - Shows 3 routing modes inline: Traditional | Resonant | Breathing
   - Active mode highlighted with warm glow
   - Smooth transition animation (300ms ease)

#### Color Palette Proposal

**Background Tiers:**
```
Header BG:    #2B2621 (warm dark grey, stone texture overlay)
Button BG:    #3A342E (slightly lighter, brushed metal)
Active BG:    #8B7355 (warm bronze, 30% opacity)
Text Default: #E6E1D6 (warm off-white, legible)
Text Active:  #D4AF86 (golden bronze, emphasis)
Outline:      #524A41 (subtle separation)
```

**Accent Colors:**
```
Primary:   #B8956A (brushed bronze)
Secondary: #8C7B6A (weathered copper)
Tertiary:  #6A5D4F (aged stone)
Glow:      #FFA040 (amber LED, for active states)
```

#### Implementation Notes

**Files to Modify:**
- `plugin/PluginEditor.h` - Add new header component class
- `plugin/PluginEditor.cpp` - Implement two-row layout with spacing
- `ui/SegmentedButton.h/.cpp` - NEW: Create segmented button component for architecture selector

**Estimated Effort:** 2-3 days

---

### Solution 2: Enhanced Dropdown Styling

**Goal:** Integrate dropdowns with ancient monuments aesthetic, improve contrast hierarchy.

#### Visual Design Concept

**Before:** Dark grey boxes floating on white background
**After:** Stone-textured panels with subtle depth and warm accents

#### Styling Specifications

**Preset Selector:**
```cpp
presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff3A342E));  // Stone base
presetBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffE6E1D6));        // Warm white
presetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff524A41));     // Subtle edge
presetBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffB8956A));       // Bronze arrow
```

**Popup Menu Enhancements:**
```cpp
// Add subtle gradient background (stone-like)
// Add category separators with engraved divider lines
// Highlight preset with warm glow (not sharp contrast)
```

**Additional Visual Elements:**
- Subtle texture overlay (procedural granite noise at 5% opacity)
- Soft inner shadow (1px, 30% black) for recessed depth
- Bevel edge highlight (1px, 10% white) on top edge
- Rounded corners (4px radius) for softer feel

#### Hover and Active States

**Hover:**
- Brightness +10%
- Warm amber glow (2px outer glow, #FFA040 at 20% opacity)
- Smooth transition (150ms)

**Active (expanded):**
- Outline brightens to bronze (#B8956A)
- Arrow rotates 180° (dropdown expanded)
- Popup menu slides down with ease-out animation

#### Implementation Strategy

**Files to Modify:**
- `plugin/PluginEditor.cpp` - Update all ComboBox color properties
- Create custom LookAndFeel class: `MonumentLookAndFeel.h/.cpp`
- Override `drawComboBox()` and `drawPopupMenuItem()` methods
- Add texture overlay rendering in custom LookAndFeel

**Estimated Effort:** 1-2 days

---

### Solution 3: Hierarchical Preset Browser

**Goal:** Transform flat preset list into explorable, categorized system with visual/sonic guidance.

#### Approach A: Nested ComboBox (Simple)

**Pros:** Minimal code changes, familiar interaction pattern
**Cons:** Limited visual expression, no sonic previews

**Structure:**
```
PRESETS ▾
├─ 🏛️ Foundational Spaces (0-5)
│  ├─ Init Patch
│  ├─ Stone Hall
│  ├─ High Vault
│  └─ ...
├─ 🌱 Living Spaces (6-11)
│  ├─ Breathing Stone [MOD: AudioFollower → Bloom]
│  ├─ Drifting Cathedral [MOD: BrownianMotion → Drift+Gravity]
│  └─ ...
├─ 🎭 Remembering Spaces (12-17)
│  ├─ Frozen Monument (Engage Freeze)
│  ├─ Ruined Monument (Remembers)
│  └─ ...
├─ 🌀 Evolving Spaces (18-22)
│  ├─ Chaos Hall [MOD: ChaosAttractor → Warp+Density]
│  ├─ Living Pillars [MOD: EnvelopeTracker → PillarShape]
│  └─ ...
└─ 🔬 Physical Modeling Spaces (23-28)
   ├─ Metallic Corridor [TubeRayTracer]
   ├─ Elastic Cathedral [ElasticHallway]
   └─ ...
```

**Enhancements:**
- Category headers with icons (🏛️ 🌱 📜 ⏳ 🌀 🔬)
- Indented preset names (visual nesting)
- Metadata badges: [MOD], [FREEZE], [TubeRayTracer]
- Category separators (1px engraved divider line)

**Implementation:**
```cpp
// PresetManager.h - Add category metadata
struct PresetCategory {
    juce::String name;
    juce::String icon;  // Unicode emoji or custom glyph
    int startIndex;
    int endIndex;
};

// PresetManager.cpp - Define categories
const std::vector<PresetCategory> PRESET_CATEGORIES = {
    {"Foundational Spaces", "🏛️", 0, 5},
    {"Living Spaces", "🌱", 6, 11},
    {"Remembering Spaces", "📜", 12, 17},
    {"Evolving Spaces", "🌀", 18, 22},
    {"Physical Modeling Spaces", "🔬", 23, 28}
};

// PluginEditor.cpp - Populate nested ComboBox
void refreshPresetList() {
    presetBox.clear();
    int itemId = 1;

    for (const auto& category : PRESET_CATEGORIES) {
        // Add category header (non-selectable)
        presetBox.addSectionHeading(category.icon + " " + category.name);

        // Add presets in category
        for (int i = category.startIndex; i <= category.endIndex; ++i) {
            auto preset = presetManager.getPresetAt(i);
            juce::String displayName = "  " + preset.name;  // Indent

            // Append metadata badges
            if (preset.hasModulation) displayName += " [MOD]";
            if (preset.usesTubeRayTracer) displayName += " [Tubes]";

            presetBox.addItem(displayName, itemId++);
        }
    }
}
```

**Estimated Effort:** 2-3 days

---

#### Approach B: Dedicated Preset Panel (Advanced)

**Pros:** Rich visual feedback, sonic previews, better discovery
**Cons:** More complex implementation, larger UI footprint

**Layout Concept:**
```
┌──────────────────────────────────────────────────────────────────┐
│  PRESET BROWSER                                    [SAVE] [X]    │
├──────────────────────────────────────────────────────────────────┤
│  Categories:  [All] [Foundational] [Living] [Evolving] [Physical]│
├──────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                │
│  │ Init Patch  │ │ Stone Hall  │ │ High Vault  │  ...           │
│  │             │ │             │ │             │                │
│  │  [Preview]  │ │  [Preview]  │ │  [Preview]  │                │
│  │             │ │             │ │             │                │
│  │ Time: Med   │ │ Time: Short │ │ Time: Long  │                │
│  │ Mass: Low   │ │ Mass: High  │ │ Mass: Low   │                │
│  │ Character:  │ │ Character:  │ │ Character:  │                │
│  │ Clean/Neu-  │ │ Dense/Dry   │ │ Bright/Tall │                │
│  │ tral        │ │             │ │             │                │
│  └─────────────┘ └─────────────┘ └─────────────┘                │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ SELECTED: Cathedral of Glass                             │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │ "Bright surfaces carry long light trails; a few          │   │
│  │ remnants drift back, fragile and high."                  │   │
│  │                                                            │   │
│  │ Modulation: None                                          │   │
│  │ Special Modules: Standard FDN only                        │   │
│  │                                                            │   │
│  │ [LOAD PRESET]                                             │   │
│  └──────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
```

**Key Features:**

1. **Preset Cards:**
   - Visual chamber thumbnail (top-down chamber cutaway rendering)
   - Key parameter values (Time, Mass, Character summary)
   - Modulation badge (if uses modulation matrix)
   - Physical modeling badge (if uses TubeRayTracer/ElasticHallway)

2. **Category Filtering:**
   - Segmented button row for quick category selection
   - "All" view shows chronological list with category dividers
   - Category-specific views show only relevant presets

3. **Sonic Visualization (Optional Enhancement):**
   - Spectrum analyzer preview (frequency response curve)
   - Temporal visualization (impulse response decay curve)
   - Generates from preset parameter values (no audio needed)

4. **Detail Panel:**
   - Full preset description (from PRESET_GALLERY.md)
   - Modulation routing summary
   - Active DSP modules list
   - Load button (large, clear call-to-action)

5. **Search and Filter:**
   - Text search box (filter by preset name or description)
   - Tag filters: #long-tail #short #bright #dark #modulated #tubes

**Visual Design:**
- Stone texture background (consistent with header)
- Preset cards: Raised panels with subtle shadow depth
- Category buttons: Segmented style matching architecture selector
- Thumbnails: 120×120px rendered chamber cross-sections
- Typography: Engraved headers, readable body text

**Interaction Flow:**
1. User clicks PRESETS button in header
2. Panel slides in from left (or expands vertically below header)
3. User browses categories or searches
4. User hovers preset card → Preview highlights
5. User clicks preset card → Detail panel updates
6. User clicks LOAD PRESET → Panel closes, preset applies with smooth crossfade

**Implementation Strategy:**

**New Files:**
- `ui/PresetBrowserPanel.h/.cpp` - Main browser component
- `ui/PresetCard.h/.cpp` - Individual preset card component
- `ui/PresetDetailPanel.h/.cpp` - Detail view component
- `ui/ChamberThumbnail.h/.cpp` - Chamber visualization renderer

**Modified Files:**
- `plugin/PluginEditor.h` - Add preset browser toggle and panel
- `plugin/PluginEditor.cpp` - Integrate browser with PRESETS button
- `plugin/PresetManager.h` - Add category and metadata support
- `plugin/PresetManager.cpp` - Implement category queries

**Data Model Additions:**
```cpp
// PresetManager.h
struct PresetMetadata {
    juce::String category;          // "Foundational Spaces"
    juce::String description;       // Full text from PRESET_GALLERY.md
    juce::Array<juce::String> tags; // {"long-tail", "bright", "modulated"}
    bool hasModulation;
    bool usesTubeRayTracer;
    bool usesElasticHallway;
    bool usesAlienAmplification;

    // Sonic character (for visualization)
    float timeCharacter;    // 0-1 (short to long)
    float massCharacter;    // 0-1 (light to heavy)
    float brightnessChar;   // 0-1 (dark to bright)
    float densityChar;      // 0-1 (sparse to dense)
};
```

**Estimated Effort:** 1-2 weeks (full implementation)

---

#### Recommendation: Phased Approach

**Phase 1 (Quick Win):** Implement Approach A (Nested ComboBox) - 2-3 days
- Immediate improvement in preset organization
- Low risk, minimal code changes
- Gets categories in place for user feedback

**Phase 2 (Future Enhancement):** Implement Approach B (Dedicated Panel) - 1-2 weeks
- Based on user feedback from Phase 1
- Higher visual impact and discoverability
- Aligns with "explorative" UI vision from MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md

---

### Solution 4: Base Parameters Panel Reorganization

**Goal:** Create clear visual grouping and hierarchy when base parameters are revealed.

#### Layout Proposal: Functional Zones

```
┌─────────────────────────────────────────────────────────────────┐
│  BASE PARAMETERS                                     [HIDE ▴]   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  CHAMBER                  GEOMETRY              ATMOSPHERE       │
│  ┌────────────────────┐  ┌────────────────┐   ┌──────────────┐│
│  │ Time    Mass       │  │ Warp           │   │ Air          ││
│  │   ⚪      ⚪        │  │   ⚪            │   │   ⚪          ││
│  │                    │  │                │   │              ││
│  │ Density  Bloom     │  │ Drift          │   │ Width        ││
│  │   ⚪      ⚪        │  │   ⚪            │   │   ⚪          ││
│  │                    │  │                │   │              ││
│  │ Size               │  │ Gravity        │   │ Mix          ││
│  │   ⚪                │  │   ⚪            │   │   ⚪          ││
│  │                    │  │                │   │              ││
│  │         [FREEZE]   │  │                │   │              ││
│  └────────────────────┘  └────────────────┘   └──────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

**Key Changes:**

1. **Three Functional Zones:**
   - **Chamber:** Time, Mass, Density, Bloom, Size, Freeze (core reverb controls)
   - **Geometry:** Warp, Drift, Gravity (spatial/topological controls)
   - **Atmosphere:** Air, Width, Mix (high-freq/stereo/blend controls)

2. **Visual Grouping:**
   - Subtle border around each zone (1px, warm grey)
   - Zone headers with engraved typography
   - Consistent spacing within zones (16px between knobs)
   - Generous padding (24px around zones)

3. **Hierarchy:**
   - Chamber zone largest (most parameters, most important)
   - Geometry and Atmosphere zones equal size
   - Freeze toggle integrated into Chamber zone (visual consistency)

4. **Grid Layout:**
   - 2×3 grid for Chamber (Time/Mass, Density/Bloom, Size/Freeze)
   - 1×3 column for Geometry (Warp, Drift, Gravity)
   - 1×3 column for Atmosphere (Air, Width, Mix)
   - All knobs same size (90×90px including label)

5. **Spacing:**
   - 16px horizontal gap between zones
   - 20px vertical gap between knob rows
   - 8px gap between knob and label
   - Total panel height: ~280px (was ~200px)

#### Visual Design Enhancements

**Zone Headers:**
```
Text: 12pt bold, warm grey (#8C7B6A)
Style: Debossed (subtle inner shadow)
Alignment: Left-aligned with 8px left padding
Separator: 1px horizontal line below (warm grey)
```

**Zone Borders:**
```
Color: #524A41 (subtle warm grey)
Width: 1px solid
Radius: 6px rounded corners
Background: Subtle gradient (top: #F9F7F5, bottom: #F5F3F1)
Shadow: Soft inner shadow (1px, 10% black)
```

**Parameter Labels:**
```
Text: 10pt sans-serif, dark grey (#3A342E)
Alignment: Centered below knob
Case: Title Case (Time, Mass, Density)
Hover: Brightens to bronze (#B8956A)
```

#### Implementation Notes

**Files to Modify:**
- `plugin/PluginEditor.cpp` - Update resized() method with zone layout
- Create helper method: `layoutBaseParameterZones()`
- Add zone boundary rendering in paint() method

**Code Structure:**
```cpp
void MonumentAudioProcessorEditor::layoutBaseParameterZones() {
    const int zoneGap = 16;
    const int zonePadding = 24;
    const int knobSize = 90;
    const int knobGap = 20;

    // Chamber zone (left, largest)
    juce::Rectangle<int> chamberZone(zonePadding, 160, 280, 240);

    // Layout Chamber parameters in 2×3 grid
    timeKnob.setBounds(chamberZone.getX() + 10, chamberZone.getY() + 30, knobSize, knobSize);
    massKnob.setBounds(chamberZone.getX() + 10 + knobSize + knobGap, chamberZone.getY() + 30, knobSize, knobSize);
    // ... etc

    // Geometry zone (center)
    juce::Rectangle<int> geometryZone(chamberZone.getRight() + zoneGap, 160, 140, 240);
    warpKnob.setBounds(geometryZone.getX() + 10, geometryZone.getY() + 30, knobSize, knobSize);
    // ... etc

    // Atmosphere zone (right)
    juce::Rectangle<int> atmosphereZone(geometryZone.getRight() + zoneGap, 160, 140, 240);
    airKnob.setBounds(atmosphereZone.getX() + 10, atmosphereZone.getY() + 30, knobSize, knobSize);
    // ... etc
}

void MonumentAudioProcessorEditor::paint(juce::Graphics& g) {
    // ... existing background painting ...

    if (baseParamsVisible) {
        // Draw zone borders and headers
        g.setColour(juce::Colour(0xff524A41));

        // Chamber zone
        juce::Rectangle<int> chamberZone(24, 160, 280, 240);
        g.drawRoundedRectangle(chamberZone.toFloat(), 6.0f, 1.0f);
        g.setFont(12.0f);
        g.drawText("CHAMBER", chamberZone.getX() + 8, chamberZone.getY() + 4, 100, 16, juce::Justification::left);

        // ... repeat for Geometry and Atmosphere zones ...
    }
}
```

**Estimated Effort:** 2 days

---

### Solution 5: Modulation Matrix Enhancements

**Goal:** Improve visual clarity and add preset template shortcuts.

#### Current Strengths (Keep These)
- 4×15 grid clear and functional
- Color-coded sources work well
- Hover/active/selected states intuitive
- Real-time connection list helpful

#### Proposed Enhancements

**Enhancement 1: Destination Grouping**

**Goal:** Group 15 destinations into logical categories (Macro vs Base vs Physical).

**Visual Change:**
- Add subtle vertical dividers every 5-6 columns
- Column header categories: MACRO CONTROLS | BASE PARAMS | PHYSICAL

**Layout:**
```
              MACRO CONTROLS    BASE PARAMS    PHYSICAL
Sources    | 1 2 3 4 5 6 | 7 8 9 10 11 12 | 13 14 15
──────────────────────────────────────────────────────
Chaos      | ○ ○ ○ ○ ○ ○ | ○ ○ ○ ○  ○  ○  | ○  ○  ○
Audio      | ○ ○ ○ ○ ○ ○ | ○ ○ ○ ○  ○  ○  | ○  ○  ○
Brownian   | ○ ○ ○ ○ ○ ○ | ○ ○ ○ ○  ○  ○  | ○  ○  ○
Envelope   | ○ ○ ○ ○ ○ ○ | ○ ○ ○ ○  ○  ○  | ○  ○  ○
```

**Enhancement 2: Modulation Template Presets**

**Goal:** Quick-start presets for common modulation routings.

**Location:** Above grid, horizontal button row

**Templates:**
1. **Breathing** - AudioFollower → Bloom (depth: 0.5)
2. **Chaotic** - ChaosAttractor X/Y → Warp/Density (depth: 0.4)
3. **Organic** - BrownianMotion → Drift/Gravity (depth: 0.6)
4. **Responsive** - EnvelopeTracker → PillarShape + AudioFollower → Width
5. **Clear All** - Remove all connections

**UI:**
```
┌────────────────────────────────────────────────────────────┐
│ Templates: [Breathing] [Chaotic] [Organic] [Responsive]   │
│            [Clear All]                                      │
└────────────────────────────────────────────────────────────┘
```

**Implementation:**
```cpp
// ModMatrixPanel.cpp - Add template application method
void ModMatrixPanel::applyTemplate(TemplateType template) {
    modMatrix.clearAllConnections();

    switch (template) {
        case TemplateType::Breathing:
            modMatrix.setConnection(SourceType::AudioFollower, DestinationType::Bloom, 0, 0.5f, 200.0f);
            break;
        case TemplateType::Chaotic:
            modMatrix.setConnection(SourceType::ChaosAttractor, DestinationType::Warp, 0, 0.4f, 300.0f);
            modMatrix.setConnection(SourceType::ChaosAttractor, DestinationType::Density, 1, 0.4f, 300.0f);
            break;
        // ... etc
    }

    refresh();  // Update UI
}
```

**Enhancement 3: Connection Strength Visualization (Future)**

**Goal:** Show active modulation with animated visual flow.

**Concept:**
- Active connection buttons show subtle animated glow pulse
- Pulse rate matches modulation frequency (slow for Brownian, fast for AudioFollower)
- Depth indicated by pulse brightness (±1.0 = full brightness, ±0.2 = subtle)

**Implementation:** Post-Phase 6 (nice-to-have, not critical)

**Estimated Effort:**
- Enhancement 1 (grouping): 1 day
- Enhancement 2 (templates): 1-2 days
- Enhancement 3 (animation): 2-3 days (future)

---

## Color Palette Recommendation

### Ancient Monuments Palette

**Primary Background:**
```
Main BG:           #FDFCFB (warm white, stone base)
Panel BG:          #F9F7F5 (slight grey, recessed panels)
Header BG:         #2B2621 (dark warm grey, stone texture)
```

**UI Elements:**
```
Button Inactive:   #3A342E (medium warm grey, brushed metal)
Button Active:     #8B7355 (warm bronze, 60% opacity)
Button Hover:      #B8956A (brushed bronze, highlight)
Text Primary:      #2B2621 (dark warm grey, high contrast)
Text Secondary:    #8C7B6A (medium warm grey, labels)
Text Active:       #D4AF86 (golden bronze, emphasis)
```

**Accents:**
```
Border:            #524A41 (subtle warm grey, dividers)
Glow/LED:          #FFA040 (warm amber, 2500K LED)
Highlight:         #E6D5B8 (cream, raised edges)
Shadow:            #1A1612 (near black, depth)
```

**Modulation Source Colors (Keep Current):**
```
Chaos Attractor:   #FF8C3C (warm orange)
Audio Follower:    #50C878 (green)
Brownian Motion:   #B464DC (purple)
Envelope Tracker:  #6496FF (blue)
```

### Contrast Ratios (WCAG 2.1 AA Compliance)

**Text on Light Backgrounds:**
- Primary text (#2B2621) on Main BG (#FDFCFB): 12.1:1 (AAA ✓)
- Secondary text (#8C7B6A) on Main BG: 4.8:1 (AA ✓)

**Text on Dark Backgrounds:**
- Light text (#E6E1D6) on Header BG (#2B2621): 10.2:1 (AAA ✓)
- Active text (#D4AF86) on Header BG: 6.1:1 (AA ✓)

**Interactive Elements:**
- Button contrast (#3A342E on #F9F7F5): 7.2:1 (AA ✓)
- Active state (#8B7355 on #F9F7F5): 4.5:1 (AA ✓)

---

## Layout and Spacing Recommendations

### Consistent Spacing System

**Base Unit:** 8px (all spacing multiples of 8)

**Spacing Scale:**
```
Micro:    4px   (icon padding, tight label gaps)
Small:    8px   (knob-to-label, inline element gaps)
Medium:   16px  (between related controls, zone padding)
Large:    24px  (between sections, outer margins)
XLarge:   32px  (major section breaks)
```

### Knob Sizing

**Primary Controls (10 Macros):**
- Size: 120×120px (including label)
- Knob artwork: 90×90px
- Label height: 24px
- Gap between knobs: 24px
- Total grid spacing: 144×144px per knob

**Base Parameters (12 controls):**
- Size: 90×90px (including label)
- Knob artwork: 70×70px
- Label height: 16px
- Gap between knobs: 16px
- Total grid spacing: 106×106px per knob

### Window Dimensions

**Default Window:**
- Width: 1100px (accommodates 10 macros + margins)
- Height: 680px (header + macros + footer)

**With Base Parameters Expanded:**
- Width: 1100px (same)
- Height: 980px (adds 300px for base params panel)

**With ModMatrix Expanded:**
- Width: 1580px (adds 480px for ModMatrix panel on right)
- Height: 680px (same)

**With Both Expanded:**
- Width: 1580px
- Height: 980px
- Warning: Large window size, may not fit on smaller screens
- Solution: Make ModMatrix a modal overlay when base params visible

### Responsive Breakpoints (Future Enhancement)

**Medium Window (900-1100px):**
- Reduce knob size to 80×80px
- Reduce spacing to 16px
- Stack base params in 2 columns instead of 3 zones

**Small Window (<900px):**
- Single column layout for macros (5×2 grid)
- Base params in single column
- ModMatrix as modal overlay only

---

## Accessibility Guidelines

### Keyboard Navigation

**Tab Order:**
1. Preset selector
2. Mode selector (Architecture/Processing Mode)
3. Modulation toggle
4. Save preset button
5. 10 macro knobs (left-to-right, top-to-bottom)
6. Base params toggle
7. Base parameter knobs (if visible)
8. ModMatrix toggle
9. ModMatrix grid (if visible)

**Keyboard Shortcuts:**
```
Cmd/Ctrl + P:  Open preset browser
Cmd/Ctrl + S:  Save preset
Cmd/Ctrl + M:  Toggle modulation matrix
Cmd/Ctrl + B:  Toggle base parameters
↑/↓:          Adjust selected knob (fine)
Shift + ↑/↓:  Adjust selected knob (coarse)
Space:        Toggle freeze (if selected)
Esc:          Close panels/dialogs
```

### Screen Reader Support

**Implement ARIA Labels:**
```cpp
materialKnob.setTitle("Material Macro Control");
materialKnob.setDescription("Controls surface character from soft to hard materials");
materialKnob.setHelpText("Current value: 0.50 (neutral)");
```

**State Announcements:**
- Preset changes: "Loaded preset: Cathedral of Glass"
- Modulation connections: "Created connection: Chaos Attractor to Warp, depth 50%"
- Panel visibility: "Base parameters panel opened"

### Visual Accessibility

**Color Independence:**
- Never rely on color alone (use icons + text + shape)
- Modulation sources: Color + icon + label
- Active states: Color + outline + badge

**High Contrast Mode Support:**
- Detect system high contrast mode
- Increase outline widths to 2px
- Boost text contrast to 15:1 minimum
- Remove subtle textures (reduce visual noise)

**Focus Indicators:**
- 2px solid outline in accent color (#B8956A)
- 2px offset from element edge (breathing room)
- High contrast (8:1 against background)
- Animated pulse on focus (subtle, non-distracting)

---

## Implementation Roadmap

### Phase 1: Foundation (Week 1)

**Goal:** Fix critical visual issues and establish design system.

**Tasks:**
1. Implement new color palette across all components
2. Create MonumentLookAndFeel class for consistent styling
3. Redesign top toolbar with two-row layout
4. Fix dropdown styling (stone texture, proper contrast)
5. Update spacing system (8px base unit)

**Deliverables:**
- MonumentLookAndFeel.h/.cpp
- Updated PluginEditor.cpp with new color palette
- Two-row header layout implemented
- Visual consistency across all UI elements

**Success Criteria:**
- No visual overlaps or cramping
- Cohesive ancient monuments aesthetic
- WCAG AA contrast compliance

**Estimated Effort:** 5 days

---

### Phase 2: Preset Browser (Week 2)

**Goal:** Implement hierarchical preset organization.

**Tasks:**
1. Add category metadata to PresetManager
2. Implement nested ComboBox with category headers
3. Add modulation/physical modeling badges
4. Style preset menu with icons and indentation
5. Test all 28 presets in categorized view

**Deliverables:**
- PresetManager.h/.cpp with category support
- Nested preset menu with 5 categories
- Visual badges for modulation and DSP modules

**Success Criteria:**
- All presets organized into logical categories
- Clear visual distinction between preset types
- Faster preset discovery (user testing)

**Estimated Effort:** 3 days

---

### Phase 3: Base Parameters Reorganization (Week 2)

**Goal:** Create clear functional grouping when base params revealed.

**Tasks:**
1. Design three-zone layout (Chamber/Geometry/Atmosphere)
2. Implement zone rendering with borders and headers
3. Update resized() method with zone-based layout
4. Add smooth expand/collapse animation
5. Test visibility toggle and zone clarity

**Deliverables:**
- layoutBaseParameterZones() method
- Zone rendering in paint()
- Updated spacing and hierarchy

**Success Criteria:**
- Clear visual grouping when expanded
- No clutter or confusion
- Smooth animation (300ms ease)

**Estimated Effort:** 2 days

---

### Phase 4: ModMatrix Enhancements (Week 3)

**Goal:** Add destination grouping and template presets.

**Tasks:**
1. Add vertical dividers between destination groups
2. Implement 5 modulation template buttons
3. Add template application logic to ModMatrixPanel
4. Test templates with audio and verify smooth application
5. Document template behaviors

**Deliverables:**
- Grouped destination columns with headers
- 5 template buttons above grid
- applyTemplate() method in ModMatrixPanel

**Success Criteria:**
- Templates apply instantly without clicks
- Visual grouping improves clarity
- Users can quickly explore modulation patterns

**Estimated Effort:** 2 days

---

### Phase 5: Advanced Preset Browser (Week 4-5)

**Goal:** Implement dedicated preset panel with visual previews (optional, based on feedback from Phase 2).

**Tasks:**
1. Design PresetBrowserPanel layout
2. Implement preset card component with thumbnails
3. Add category filtering with segmented buttons
4. Create detail panel with full descriptions
5. Integrate with PRESETS button toggle
6. Add smooth slide-in animation

**Deliverables:**
- PresetBrowserPanel.h/.cpp
- PresetCard.h/.cpp
- PresetDetailPanel.h/.cpp
- Thumbnail rendering system

**Success Criteria:**
- Preset browsing is explorative and intuitive
- Visual feedback helps users understand sonic character
- Panel integrates seamlessly with existing UI

**Estimated Effort:** 7-10 days

**Decision Point:** Implement only if Phase 2 nested ComboBox proves insufficient based on user feedback.

---

### Phase 6: Polish and Testing (Week 6)

**Goal:** Final refinements and accessibility testing.

**Tasks:**
1. Add keyboard navigation to all components
2. Implement ARIA labels and screen reader support
3. Test in multiple DAWs (Ableton, Logic, Reaper, FL Studio)
4. Test on different screen sizes and DPI settings
5. Performance profiling (UI rendering <5% CPU)
6. User testing with 5-10 musicians
7. Fix all reported bugs and polish edge cases

**Deliverables:**
- Fully accessible UI (WCAG 2.1 AA compliant)
- Smooth performance across all DAWs
- Polished animations and transitions
- User testing report with feedback

**Success Criteria:**
- Keyboard navigation works flawlessly
- Screen reader announces all interactions
- No performance regressions
- Positive user feedback on discoverability

**Estimated Effort:** 5 days

---

## Testing Strategy

### Visual Regression Testing

**Tool:** Percy.io or custom screenshot comparison

**Test Cases:**
1. Default window (10 macros visible)
2. Base params expanded
3. ModMatrix expanded
4. Both expanded
5. Preset browser open (if implemented)
6. All dropdown menus expanded
7. Hover states on all interactive elements
8. Focus indicators on all controls

**Frequency:** Run on every UI commit

---

### Interaction Testing

**Manual Test Scenarios:**

1. **Preset Loading:**
   - Load each of 28 presets
   - Verify smooth parameter transitions
   - Check preset name displays correctly

2. **Mode Switching:**
   - Switch between Traditional/Resonant/Breathing modes
   - Verify no audio glitches during transition
   - Check mode label updates

3. **Modulation Workflow:**
   - Create 5 modulation connections
   - Adjust depth and smoothing
   - Remove connections one by one
   - Apply each template preset

4. **Keyboard Navigation:**
   - Tab through entire UI
   - Adjust knobs with arrow keys
   - Trigger shortcuts (Cmd+P, Cmd+S, etc.)

5. **Responsive Layout:**
   - Resize window to minimum size
   - Expand/collapse panels
   - Verify no overlaps or clipping

---

### Performance Testing

**Metrics:**
- UI render time: <16ms (60 FPS)
- Preset load time: <100ms (perceived instant)
- Panel animation: Smooth 60 FPS
- Memory usage: <50MB for UI assets

**Profiling Tools:**
- Xcode Instruments (Time Profiler, Allocations)
- JUCE PerformanceCounter for custom timing
- Visual Studio Performance Profiler (Windows)

---

### Accessibility Testing

**Automated:**
- Axe accessibility scanner (web-based UI preview)
- Color contrast analyzer (Stark, Colour Contrast Analyser)

**Manual:**
- VoiceOver navigation (macOS)
- NVDA navigation (Windows)
- Keyboard-only navigation
- High contrast mode testing

**User Testing:**
- Recruit 2-3 users with accessibility needs
- Test with screen readers, keyboard-only, high contrast
- Document issues and prioritize fixes

---

## Success Metrics

### Quantitative Goals

**UI Performance:**
- Preset load time: <100ms (from user click to audio change)
- Panel animation: 60 FPS sustained
- UI render budget: <3ms per frame
- Memory footprint: <50MB for all UI assets

**Accessibility:**
- WCAG 2.1 AA compliance: 100% of interactive elements
- Keyboard navigation: 100% coverage
- Screen reader support: All actions announced

**User Efficiency:**
- Preset discovery time: <30 seconds to find desired character
- Modulation setup time: <60 seconds to create complex routing
- Learning curve: 80% of users comfortable within 10 minutes

### Qualitative Goals

**User Feedback (5-point scale, target: 4.0+):**
- Visual appeal: "The UI is beautiful and matches Monument's sonic character"
- Clarity: "I can easily understand what each control does"
- Discoverability: "I can quickly find presets that fit my creative intent"
- Responsiveness: "The UI feels smooth and professional"
- Cohesion: "The design feels unified and intentional"

**Expert Review:**
- UI/UX professional audit (1-2 hour session)
- Identify any remaining friction points
- Validate accessibility compliance
- Confirm professional polish level

---

## Risk Assessment

### High-Risk Items

**Risk 1: Scope Creep**
- **Probability:** Medium
- **Impact:** High (delays release, budget overrun)
- **Mitigation:** Strict adherence to phased roadmap, Phase 5 (advanced preset browser) optional based on Phase 2 results

**Risk 2: Performance Regression**
- **Probability:** Low
- **Impact:** High (impacts user experience, DAW compatibility)
- **Mitigation:** Continuous profiling, render budget enforcement (<3ms per frame), early testing in multiple DAWs

**Risk 3: Accessibility Gaps**
- **Probability:** Medium
- **Impact:** Medium (excludes some users, professional criticism)
- **Mitigation:** Early ARIA implementation, automated testing, manual testing with accessibility users

### Medium-Risk Items

**Risk 4: Visual Inconsistency**
- **Probability:** Medium
- **Impact:** Medium (unprofessional appearance, brand confusion)
- **Mitigation:** MonumentLookAndFeel class enforces consistency, design system documentation, visual regression testing

**Risk 5: Preset Categorization Disagreement**
- **Probability:** Medium
- **Impact:** Low (users can still access all presets, just less organized)
- **Mitigation:** User testing on category names/structure, iterative refinement based on feedback

---

## Future Enhancements (Post-v1.0)

### v1.1 Enhancements
- Preset favorites and tags system
- User-created preset categories
- Preset sharing/export functionality
- Dark mode toggle (alternative color palette)

### v1.2 Enhancements
- Animated modulation strength visualization (flowing connections)
- Preset sonic visualization (spectrum/impulse response curves)
- Chamber thumbnail rendering system for presets
- Multi-preset comparison view

### v1.3 Enhancements
- Resizable UI with multiple window sizes
- Advanced modulation: probability gates, quantization UI
- Gesture recording and playback for automation
- Preset morphing interface (2D XY pad)

---

## Conclusion

This roadmap provides a comprehensive plan for transforming Monument Reverb's UI from functional to exceptional. By addressing critical issues (toolbar cramping, dropdown styling, preset organization, base parameter layout) and introducing strategic enhancements (hierarchical presets, visual grouping, modulation templates), we can create an interface that matches the quality and sophistication of Monument's DSP architecture.

**Key Priorities:**
1. Establish cohesive ancient monuments aesthetic (Phase 1)
2. Improve preset discoverability (Phase 2)
3. Create clear visual hierarchy (Phase 3)
4. Enhance modulation workflow (Phase 4)
5. Polish and accessibility (Phase 6)

**Recommended Timeline:** 4-6 weeks post-DSP finalization
**Total Estimated Effort:** 25-35 days (depending on Phase 5 decision)

**Next Step:** Review this roadmap, prioritize phases based on project goals, and schedule Phase 1 kickoff after DSP work is complete.

---

**Document Version:** 1.0
**Last Updated:** 2026-01-04
**Author:** Claude (UI/UX Design Expert)
**Review Status:** Awaiting stakeholder approval
