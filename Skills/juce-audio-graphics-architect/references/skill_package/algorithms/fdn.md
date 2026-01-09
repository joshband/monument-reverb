## Feedback Delay Networks (FDNs)

A **feedback delay network** is a high‑density artificial reverberation topology composed of multiple delay lines whose outputs are mixed and fed back through a structured feedback matrix.  Each delay line introduces a short time delay; the network then scales, sums and feeds those delayed signals back into each other via a matrix that can be tuned for spectral and temporal decay.  The result is a recursive filter that can be configured to emulate realistic reverberation and spatial reflections.

### Key properties

- **Parametric filter topology:** FDNs are a flexible structure for artificial reverberation, spatial audio rendering, sound synthesis and physical modelling【301082280720046†L46-L63】.  By adjusting the delay lengths, feedback matrix and frequency‑dependent attenuation, designers can sculpt the reverberant tail and control its decay characteristics.
- **High echo density:** Because the delay lines feed each other through the feedback matrix, echoes rapidly build up and smear into a dense reverb tail【301082280720046†L46-L63】.
- **Frequency‑dependent attenuation:** Incorporating filters (such as parametric equalisers) inside the feedback loops allows the network to simulate the loss of high frequencies over time and shape the spectral content of the reverb【301082280720046†L46-L63】.

### Applications

FDNs are widely used in digital reverbs, 3‑D audio and physical models of instruments.  Modern research explores differentiable FDNs that can be optimised via machine‑learning techniques【301082280720046†L46-L63】.