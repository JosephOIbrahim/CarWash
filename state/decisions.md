# Decisions (append-only)

## d000 — Orchestrator framework installed
- Decision: Adopt the Orchestrator CLAUDE.md + four subagents (planner/worker/critic/integrator).
- Rejected: CLAUDE.md only (non-functional — agents must exist); inherit-all-tools for agents
  (chose role-appropriate least privilege instead).
- Date: 2026-05-25

## d001 — New goal accepted; EXIT_CRITERIA to be drafted by planner
- Decision: Accept GOAL "scaffold the current render engine with awareness of current ComfyUI
  and generative AI models and practices." No EXIT_CRITERIA supplied. Per CLAUDE.md Invocation,
  the planner drafts a candidate set as its first act; no work begins until they are confirmed
  by the human.
- Rejected: Jumping straight to implementation (violates Invariant 1 and the Invocation rule).
- Date: 2026-05-25

## d002 — Template-JSON workflow construction over string concatenation
- Decision: Build ComfyUI graphs by loading versioned .json template workflows and mutating
  node inputs by id/title.
- Rejected: keep ostringstream builders (brittle, no schema validation); full C++ node-graph DSL
  (over-engineered for the node count).
- Status: CONFIRMED 2026-05-25.
- Date: 2026-05-25

## d003 — Real JSON parser over substring matching (header-only, JSON only)
- Decision: Parse ComfyUI responses with a header-only JSON library (nlohmann/json unless a
  better fit surfaces). Header-only JSON is APPROVED by human. A networking/HTTP/WS library is
  NOT yet approved — t024 must bring a separate dependency decision before adding one.
- Rejected: continue response.find(...) substring parsing (carries dual-format hacks);
  approving a networking lib up front (deferred per human).
- Status: CONFIRMED 2026-05-25 (JSON only); networking lib DEFERRED.
- Date: 2026-05-25

## d004 — Wire the existing client into the build, don't rewrite
- Decision: Bring HdCarWashComfyClient into the build and delegate; replace only its
  networking/serialization internals.
- Rejected: greenfield rewrite (discards working LTX-2 graph knowledge); leave it detached
  (fails the core goal).
- Status: CONFIRMED 2026-05-25.
- Date: 2026-05-25

## d005 — Determinism cache-busters are a defect to remove
- Decision: Remove wall-clock seed perturbation from the default path; if anti-caching is ever
  needed, gate it behind an explicit opt-in flag.
- Rejected: keep wall-clock perturbation (contradicts the project's determinism thesis).
- Status: CONFIRMED 2026-05-25.
- Date: 2026-05-25

## d006 — Bound EXIT to "structurally correct + aligned with verified practice"
- Decision: EXIT does not require validated video OUTPUT (no live GPU/ComfyUI here). Output-
  quality validation deferred to a downstream externally-resourced effort.
- Rejected: require validated video output (impossible in this environment).
- Status: CONFIRMED 2026-05-25.
- Date: 2026-05-25

## d007 — Vendor stb_image.h (already-referenced single-header dependency)
- Decision: comfyClient.cpp already #includes "stb_image.h" (public-domain, single-header).
  Vendor a pinned copy into plugin/hdCarWash/third_party/ and add the include dir, so the file
  the build now references actually compiles. This restores an intended dependency, not a new one.
- Rejected: rip out stb and re-hand-roll PNG decode (regressive); leave the build broken.
- Status: PROPOSED — pending human approval (per Invariant 8, even for vendored single-header).
- Date: 2026-05-25

## d008 — LTX target version (OPEN — needs human decision, q007)
- Question: scaffold the workflow template against LTX-2.0 19B (what CarWash references today) or
  the current flagship LTX-2.3 22B? Affects t022 node graph and model paths.
- Status: OPEN — awaiting human decision.
- Date: 2026-05-25

## d009 — Output contract (RECOMMENDED, needs human confirmation, q003/q004)
- Recommendation (t012): clip-native + depth-video IC-LoRA v2v + per-(range,scene,seed) clip cache
  serving Hydra per-frame requests. Rejected: single-image img2vid (frame-0 only), one-clip-per-frame
  (flicker + 25x cost), native sequence renderer bypassing Hydra (larger refactor, deferred).
- Status: PROPOSED — awaiting human confirmation before t022/t032 implement.
- Date: 2026-05-25
