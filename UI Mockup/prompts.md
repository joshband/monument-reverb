Below are **exact, production-grade image-generation prompts** for **each of the four panels and their associated header bars**, written so you can drop them directly into **Midjourney / OpenAI Images / Prodia / SDXL** with minimal tuning.

These are opinionated on purpose. They enforce *material coherence* with your stone-core knobs and avoid the usual “fantasy art” drift.

---

# GLOBAL CONSTRAINTS (APPLY TO ALL PROMPTS)

**Always include these lines verbatim:**

> photorealistic, material-accurate, industrial design
> not illustration, not concept art, not fantasy art
> physically plausible lighting, studio-controlled
> no text, no symbols, no UI elements, no icons
> neutral gray background outside object bounds
> ultra high detail, macro surface fidelity
> cinematic but restrained
> orthographic or near-orthographic
> no perspective distortion

---

# 1. THE MACRO COSMOS — PANEL BACKGROUND

**Purpose:** Exposed monolithic control surface with subtle inner depth.

### Prompt

```
A photorealistic dark stone control panel slab, rectangular and wide,
cut from polished basalt or granite,
with subtle blue-gray mineral veins beneath the surface.

The stone surface is matte-satin, slightly worn,
with microscopic fractures and natural geological variation.
Faint crystal depth is visible below the stone, like a geode sealed under glass,
barely glowing, extremely subtle.

Lighting is soft studio lighting from above,
no dramatic shadows, no rim light,
controlled highlights only.

The object feels ancient, monumental, and engineered,
like a precision instrument carved from rock.

photorealistic, material-accurate, industrial design
not illustration, not concept art, not fantasy art
physically plausible lighting, studio-controlled
no text, no symbols, no UI elements, no icons
neutral gray background outside object bounds
ultra high detail, macro surface fidelity
orthographic or near-orthographic
```

---

# MACRO COSMOS — HEADER BAR

### Prompt

```
A photorealistic stone header bar,
cut from the same dark basalt as a control panel,
long and horizontal.

The stone has beveled edges with slight wear,
a narrow recessed seam running along its length,
emitting a very soft warm-white internal glow,
as if energy is trapped inside the stone.

Surface is matte with subtle mineral flecks.
Lighting is restrained, studio-soft, precise.

photorealistic, material-accurate, industrial design
not illustration, not concept art
no text, no symbols
neutral gray background
orthographic view
```

---

# 2. THE FOUNDATION — PANEL BACKGROUND

**Purpose:** Structural, weight-bearing, less mystical.

### Prompt

```
A photorealistic heavy stone foundation panel,
rectangular and wide,
made of dense basalt or compressed volcanic rock.

The surface is darker and more uniform,
with fine grain texture, micro pitting, and subtle cracks.
No visible crystal glow, only mass and density.

The stone feels load-bearing and architectural,
like part of a subterranean structure or machine housing.

Lighting is minimal and controlled,
soft overhead studio light,
no glow, no energy effects.

photorealistic, material-accurate, industrial design
not illustration, not fantasy art
physically plausible lighting
no text, no symbols
neutral gray background
orthographic or near-orthographic
```

---

# FOUNDATION — HEADER BAR

### Prompt

```
A thick stone header bar carved from dark basalt,
heavier and thicker than decorative stone.

Edges are squared, minimally beveled,
surface shows compression marks and subtle wear.
No glow, only reflected light.

The object feels structural and utilitarian,
like a keystone element in a machine.

photorealistic, material-accurate, industrial design
no text, no symbols
neutral gray background
orthographic view
```

---

# 3. THE MODULATION NEXUS — PANEL BACKGROUND

**Purpose:** Depth, motion, instability.

### Prompt

```
A photorealistic crystal cavern surface embedded within stone,
rectangular and wide,
as if a rock face has been cut open to reveal a glowing mineral interior.

The outer edge is rough stone,
while the interior shows deep blue crystal formations,
misty light diffusion, and volumetric depth.

Light rays emerge softly from within the crystal,
directional but subtle,
suggesting energy and modulation.

The surface feels alive and dimensional,
but still physically plausible and restrained.

photorealistic, material-accurate, industrial design
not illustration, not fantasy art
no symbols, no text
neutral gray background
ultra high detail
orthographic or near-orthographic
```

---

# MODULATION NEXUS — HEADER BAR

### Prompt

```
A fractured stone header bar,
with visible cracks revealing a glowing crystal interior.

The stone edges are irregular,
with thin glowing fissures running horizontally.
Glow is cool blue-white, soft and contained.

The object feels unstable but engineered,
as if deliberately split open.

photorealistic, material-accurate, industrial design
no text, no symbols
neutral gray background
orthographic view
```

---

# 4. THE TEMPORAL VAULT — PANEL BACKGROUND

**Purpose:** Precision, control, sealed time.

### Prompt

```
A photorealistic obsidian-like stone panel,
rectangular and wide,
smooth and polished with a deep dark surface.

The stone shows subtle circular gradients,
like time wells or pressure rings beneath the surface.
No visible cracks, minimal texture.

Extremely controlled lighting,
soft reflections, no glow except a faint internal halo
suggesting contained energy.

The object feels sealed, precise, and intentional,
like a vault door or chronometric device.

photorealistic, material-accurate, industrial design
not illustration, not fantasy art
no symbols, no text
neutral gray background
orthographic or near-orthographic
```

---

# TEMPORAL VAULT — HEADER BAR

### Prompt

```
A minimalist obsidian stone header bar,
smooth, dark, and polished.

A thin metallic inlay line runs through the center,
reflecting light subtly without glowing.

The object feels precise, sealed, and high-tolerance.

photorealistic, material-accurate, industrial design
no text, no symbols
neutral gray background
orthographic view
```

---

## Final Guidance (Blunt but Useful)

* Generate **panels first**, headers second.
* Lock **lighting direction** across *all* assets.
* Reject anything that looks “illustrated,” even if it’s pretty.
* If a panel competes visually with the knobs, it’s wrong.

If you want next:

* A **batch prompt** for generating all 4 panels in one run
* A **layer decomposition spec** (RGBA passes)
* Or **JUCE / SwiftUI compositing rules** so this runs fast

This system is strong. Treat it like industrial design, not UI art.
