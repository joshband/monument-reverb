# Archived Reviews & Audits — Timeline Index

This directory is a **historical record**, not living documentation. Every file
here is a standalone review, audit, or plan captured at a point in time —
useful for understanding *why* a decision was made or *what a past state of
the codebase looked like*, but not maintained afterward. Facts inside them
(test counts, preset counts, CPU numbers, "critical issue" lists) can and do
drift out of date the moment the code moves on.

**For the current state of the project, trust [`ARCHITECTURE.md`](../../../ARCHITECTURE.md)
and [`TESTING.md`](../../../TESTING.md) over anything in this directory.** Both
files say the same thing about themselves, and it applies doubly here: when a
file below disagrees with those two, the two win.

Each entry gives the file's own self-declared date, a one/two-line summary of
what it covered, and — where this project's later history makes it clear —
whether its findings were fixed, superseded by more rigorous verification, or
are still a real, open concern worth reading the original for.

---

## 2026-01-03

**[01032026-DocumentationHygieneReview.md](01032026-DocumentationHygieneReview.md)**
Automated pass grading doc accuracy across the repo (~75% accurate at the time), flagging UI docs that still described the old MVP system alongside current ones. Purely a documentation-health snapshot; superseded by this project's later, more thorough doc-cleanup passes (this one included).

**[01032026-DspClickAnalysisReport.md](01032026-DspClickAnalysisReport.md)**
Root-caused audible clicks/pops on Viscosity/Topology macro changes to unsmoothed coefficients and tap-delay discontinuities. **Fixed at the time**: the report documents Priority 1–2 smoothing fixes and deferred tap-layout updates as implemented, with only a lower-severity Priority 3 (mode filter coefficients) left open.

## 2026-01-04

**[01042026-ArchitectureReview.md](01042026-ArchitectureReview.md)**
"Comprehensive Architecture Review" focused on the Ancient Monuments 10-macro system, 3 physical-modeling modules, and 3 routing modes; claims **28 factory presets**. That count is now stale — `ARCHITECTURE.md` confirms 37 factory presets today — and its "Routing Modes" framing predates the later, more precise "routing preset vs. routing graph" distinction now in `ARCHITECTURE.md`.

**[01042026-ParallelRoutingGuide.md](01042026-ParallelRoutingGuide.md)**
Describes `DspRoutingGraph`'s generic series/parallel/feedback/crossfeed executor as "fully implemented," with line-numbered code walkthroughs of each mode. **True but misleading without context**: `ARCHITECTURE.md`'s "Routing preset vs. routing graph" section clarifies the processor never actually calls this generic executor in ordinary builds — the mechanism exists and works in isolation, but a routing preset's name does not select the topology this guide describes. Read `ARCHITECTURE.md` first.

**[01042026-DspRealtimeSafetyAudit.md](01042026-DspRealtimeSafetyAudit.md)**
Systematic file-by-file RT-safety audit of 27 DSP files; found 11 violations (heap allocations in `TubeRayTracer`, `ElasticHallway`, `AlienAmplification`, `DspModules`) and 12 warnings (including `ModulationMatrix` lock usage). **Largely fixed same day** — see the fixes summary below — though `TubeRayTracer` allocation on tube-count changes was later re-confirmed as still present (see `TESTING.md`'s `monument_realtime_allocation_characterization_test`).

**[01042026-DspRealtimeSafetyFixPlan.md](01042026-DspRealtimeSafetyFixPlan.md)**
The implementation plan (9–13 hours, 3 priority tiers) for the violations found in the audit above, with before/after code for each fix. Historical implementation record; superseded by the actual fixes (next entry) and by current source.

**[01042026-DspRealtimeFixesSummary.md](01042026-DspRealtimeFixesSummary.md)**
Claims completion of all 11 critical RT-safety violations from the audit, plus a 2026-01-11 addendum on parameter sanitization and routing-graph guard fixes. **Partially superseded by more rigorous verification**: this session's `RealtimeAllocationCharacterizationTest` (a deliberately-red CTest target, see `TESTING.md`) later proved a `TubeRayTracer` tube-count-boundary allocation still reaches the audio thread — the 2026-01-04 fix did not fully close that path. A separate timeline-preset allocation (found later, not by this report) was fixed for real in commit `e8b48f6`.

## 2026-01-07

**[01072026-ArchitectureReview.md](01072026-ArchitectureReview.md)**
Flagged architectural ambiguity between the plugin UI and the standalone `playground/` app, and a split asset pipeline. Documentation-scope findings; largely a "declare a canonical path" recommendation rather than a code defect.

**[01072026-CodeReview.md](01072026-CodeReview.md)**
Found a critical possible null-dereference on `getPlayHead()` in `PluginProcessor.cpp`, plus RT-unsafe routing-preset mutation on the audio thread. **Both fixed**: `PluginProcessor.cpp`'s `processBlock()` now null-checks `getPlayHead()` before use, and routing-preset swaps were made RT-safe in a later `DspRoutingGraph` rewrite (commit `ed10a59`, "make routing preset swaps RT-safe").

**[01072026-Performance.md](01072026-Performance.md)**
Companion performance pass: same routing-preset audio-thread allocation finding as the code review, plus a `ModulationMatrix` `SpinLock`-contention risk. Routing-preset allocation fixed (see above); the `ModulationMatrix` concern's status is nuanced — see the 2026-01-09 findings entry below and `ARCHITECTURE.md`'s realtime-safety-limitations section.

## 2026-01-08

**[01082026-ArchitectureReview.md](01082026-ArchitectureReview.md)**
A more polished architecture review: praises the DSP/UI separation and APVTS usage, flags real-time-safety and modular-architecture opportunities. General/strategic; superseded by `ARCHITECTURE.md`'s current, more precise module-by-module description.

**[01082026-CodeReview.md](01082026-CodeReview.md)**
Graded the codebase A- with two critical RT-safety issues called out explicitly: routing-preset allocation in `processBlock()` and `ModulationMatrix` `SpinLock` use on the audio thread. Routing-preset allocation fixed (`ed10a59`); `ModulationMatrix` see below.

**[01082026-Performance.md](01082026-Performance.md)**
Deep CPU/memory profiling pass (SIMD opportunities, cache efficiency) reaching the same two critical RT findings as the code review that day. Same fix status as above.

**[01082026-TestingAudit.md](01082026-TestingAudit.md)**
Inventoried the test infrastructure (3,984 lines of test/tooling code, 7 C++ executables, 6 Python analysis tools) and rated coverage ~85%. Descriptive snapshot; the actual test inventory has grown substantially since (see `TESTING.md`'s CTest Inventory, ~26 registered targets including the newer allocation-characterization and concurrency-stress tests this session added).

## 2026-01-09

**[01092026-ArchitectureReviewDspAnalysis.md](01092026-ArchitectureReviewDspAnalysis.md)**
"Architecture Review & DSP Analysis" — the most polished/executive-summary-style of the reviews, claiming production-ready status and **"155/155 tests passing (100% coverage)."** That figure is an unverifiable historical snapshot (no CI artifact backs it, and the test suite's shape has since changed materially, including a deliberately-failing characterization test) — do not cite it as current.

**[01092026-DspArchitectureComprehensiveReview.md](01092026-DspArchitectureComprehensiveReview.md)**
The most detailed module-by-module inventory of the set (9 core modules, 3 physical-modeling modules, full parameter/routing/modulation breakdown); correctly states **37 factory presets** and 12.89% p99 CPU. The most internally-accurate of the January reviews at the time it was written, but still a snapshot — defer to `ARCHITECTURE.md` for current module wiring, especially the routing-preset-vs-routing-graph and Memory-Echoes-not-reached caveats it predates.

**[01092026-ArchitectureReviewFindings.md](01092026-ArchitectureReviewFindings.md)**
Root-level code-level review (AR-01 through AR-10) of the DSP/processor path: `ParameterBuffer` dangling-reference risk, a routing-graph semantics mismatch between preset names and actual topology, `ProcessingMode` crossfade not fading the previous mode, physical-modeling audio-thread allocations, `SequenceScheduler` live-edit thread-safety, and more, with a 2026-01-10 update appended. Several of its concerns (routing-preset-vs-graph mismatch, physical-modeling allocations) are exactly the issues `ARCHITECTURE.md` and this session's characterization tests independently re-confirm as real and still relevant — this is probably the single most worth-reading file in this archive if you want the detailed technical reasoning behind those current caveats.

**[01092026-ArchiveVerificationReport.md](01092026-ArchiveVerificationReport.md)**
A meta-review checking whether the content of the (then newly-archived) 2026-01-07 review trio was still preserved elsewhere in living docs, and what would be lost by archiving it. Itself now historical — its own subject matter (the 01-07 files) lives in this same directory today.

**[01092026-DocumentationReorganizationPlan.md](01092026-DocumentationReorganizationPlan.md)**
An earlier, apparently-unfinished attempt at exactly the documentation cleanup this archive is now part of — an inventory of root/`docs/` markdown files with proposed archive destinations. Superseded by the actual cleanup that produced this directory and its siblings; kept for the historical record of that earlier attempt.

---

*Six files above (`01072026-*`, `01082026-*`) were archived before this index was written; the remaining thirteen were moved here together as part of a broader documentation audit that also rewrote `ARCHITECTURE.md` and `TESTING.md` to reflect verified, current behavior rather than point-in-time review claims.*
