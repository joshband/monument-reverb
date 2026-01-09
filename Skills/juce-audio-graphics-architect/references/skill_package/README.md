# Audio Algorithms and Spatial/Temporal Audio Devices

This package collects concise reference notes for algorithms and devices relevant to spatial and temporal audio.  It is intended to help OpenAI Codex generate rich JUCE‑based plug‑ins by providing context on feedback delay networks, delay lines, reverb versus echo, loopers and a selection of modern effects pedals.  Each note includes citations back to the original sources so you can verify or expand upon the statements.

## Contents

### Algorithms (in `algorithms/`)

- **`fdn.md`** – Explains feedback delay networks (FDNs) and how they achieve high‑density artificial reverberation using a matrix of delay lines【301082280720046†L46-L63】.
- **`delay-lines.md`** – Outlines the role of delay lines in reverbs, synthesis, and physical modelling, and describes their implementation using circular buffers【139914273116376†L80-L100】.
- **`reverb-echo.md`** – Defines reverb and echo, contrasts them, and notes why the distinction matters in production【382356893620452†L63-L69】【382356893620452†L319-L327】.
- **`loopers-memory.md`** – Describes loop pedals (loopers) and the concept of audio memory, highlighting how they record, overdub and play back performances【397308062428236†L85-L97】.

### Devices (in `devices/`)

- **`chase-bliss-habit.md`** – Summarises the features of the Chase Bliss Habit “echo collector,” a delay with a long memory and advanced modifiers【756325403414970†screenshot】【794146278916888†screenshot】.
- **`soma-cosmos.md`** – Provides an overview of the Soma Laboratory Cosmos drifting memory station, which uses prime‑number delays and chaotic modulation to create evolving soundscapes【986922461742756†screenshot】.
- **`kinotone-ribbons.md`** – Describes the Kinotone Ribbons tape‑effects unit and its page‑based interface【875380392818289†screenshot】【795300359553208†screenshot】.
- **`cloudburst.md`** – Lists the features of the Strymon Cloudburst ambient reverb pedal, including its Ensemble effect, modulation and freeze/infinite modes【876625921092337†L771-L801】.
- **`bigsky.md`** – Presents the highlights of the Strymon BigSky reverb workstation, such as its 12 hand‑crafted algorithms and extensive preset management【357543303053795†L1483-L1503】.
- **`cloud-algorithm-pedals.md`** - Informal, uncited notes on ambient "cloud" pedals and related gear.

Each file is written in Markdown with citation markers that refer back to the sources used in this conversation, except `cloud-algorithm-pedals.md` which is an uncited, informal note. You can use these notes as reference material when building JUCE plug-ins or when writing detailed skill instructions.
