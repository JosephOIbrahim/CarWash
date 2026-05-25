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
- Status: PROPOSED — pending human confirmation.
- Date: 2026-05-25

## d003 — Real JSON parser over substring matching
- Decision: Parse ComfyUI responses with a header-only JSON library (specific lib TBD, recorded
  per Invariant 8 before install).
- Rejected: continue response.find(...) substring parsing (already carries dual-format hacks).
- Status: PROPOSED — pending human confirmation + dependency approval (q005).
- Date: 2026-05-25

## d004 — Wire the existing client into the build, don't rewrite
- Decision: Bring HdCarWashComfyClient into the build and delegate; replace only its
  networking/serialization internals.
- Rejected: greenfield rewrite (discards working LTX-2 graph knowledge); leave it detached
  (fails the core goal).
- Status: PROPOSED — pending human confirmation.
- Date: 2026-05-25

## d005 — Determinism cache-busters are a defect to remove
- Decision: Remove wall-clock seed perturbation from the default path; if anti-caching is ever
  needed, gate it behind an explicit opt-in flag.
- Rejected: keep wall-clock perturbation (contradicts the project's determinism thesis).
- Status: PROPOSED — pending human confirmation.
- Date: 2026-05-25

## d006 — Bound EXIT to "structurally correct + aligned with verified practice"
- Decision: EXIT does not require validated video OUTPUT (no live GPU/ComfyUI here). Output-
  quality validation deferred to a downstream externally-resourced effort.
- Rejected: require validated video output (impossible in this environment).
- Status: PROPOSED — pending human confirmation.
- Date: 2026-05-25
