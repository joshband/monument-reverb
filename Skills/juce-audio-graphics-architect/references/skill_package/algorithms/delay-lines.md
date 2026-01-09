## Delay Lines and Physical Modelling

A **delay line** is one of the most fundamental building blocks in digital signal processing.  It stores a signal and then replays it a fixed number of samples later.  By combining and feeding back delayed copies of a signal, delay lines form the basis of reverbs, filters and time‑domain effects.

### Uses of delay lines

- **Reverberation and time‑based effects:** Delay lines are at the heart of algorithmic reverbs and classic effects such as chorus, phaser, flanger and vibrato【139914273116376†L80-L90】.
- **Sound synthesis and filtering:** They are essential for physical modelling, waveguide synthesis and various digital filters【139914273116376†L80-L90】.

### Implementation

In the digital domain, delay lines are typically implemented as *circular buffers*.  A circular buffer wraps around on itself so that samples from previous blocks can be reused in the current block【139914273116376†L94-L100】.  This allows efficient storage and retrieval of past samples without expensive memory copying.

### Physical modelling

In **digital waveguide** models of strings and air columns, two delay lines—one for forward‑traveling waves and one for backward‑traveling waves—simulate the propagation of vibrations.  Reflection and inversion at the boundaries, combined with damping filters, produce realistic decays【139914273116376†L120-L126】.