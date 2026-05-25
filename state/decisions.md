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
