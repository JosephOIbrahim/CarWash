GOAL: Scaffold the current CarWash render engine (Houdini USD/Hydra render delegate that
drives ComfyUI/LTX-2 for AI video generation) with awareness of current ComfyUI APIs and
current generative-AI model practices — so the engine's structure, integration points, and
conventions reflect how ComfyUI and modern gen-AI video pipelines are built today.

CONFIDENCE_THRESHOLD: 0.8

STATUS: planning — planner survey (t000) complete. DRAFT EXIT_CRITERIA below await human
confirmation. NO worker runs until criteria are confirmed (CLAUDE.md Invocation rule).

-----------------------------------------------------------------------------------------------
KEY SURVEY FINDING (see beliefs c001–c006):
The ComfyUI/LTX-2 AI path is fully written (comfyClient.cpp, ~2,359 LOC) but ORPHANED:
not in the plugin build, never instantiated by the delegate, and _ExecuteFullPipeline is a
TF_CODING_ERROR stub. Today the engine only rasterizes depth/normal AOVs on CPU. Docs
(README/INDEX) describe many dirs/tests/workflows that do not exist. So "scaffold" = connect +
modernize an orphaned integration, not greenfield.
-----------------------------------------------------------------------------------------------

DRAFT EXIT_CRITERIA (pending confirmation):
  1. Render engine mapped in beliefs.md at confidence >= 0.8: every plugin/hdCarWash/ file has a
     recorded role; "AI path orphaned/uncompiled" claim verified.
  2. All [V] open questions (current ComfyUI API, LTX-2 node graph, text-encoder dim,
     conditioning strategy, per-frame vs per-clip contract) closed by a claim >= 0.8 or parked
     with rationale.
  3. comfyClient.cpp is in the build; project configures without referencing nonexistent files.
  4. Workflow JSON produced from versioned template files + structured mutation; ComfyUI
     responses parsed with a real JSON parser — both covered by passing tests/cpp/ unit tests.
  5. Determinism internally consistent: no wall-clock seed perturbation in the default path; a
     passing determinism regression test exists; DETERMINISM_CROSS_REFERENCE.md matches code.
  6. Delegate instantiates the ComfyUI client from render settings and _ExecuteFullPipeline
     invokes it (no TF_CODING_ERROR on the AI path), with graceful fallback when server absent.
  7. README/INDEX no longer describe directories, tests, or capabilities that do not exist.

  Scope bound (decision D-5): EXIT means "structurally correct + aligned with verified
  practice", NOT "produces correct video" — output-quality validation needs a live GPU/ComfyUI
  not available here and is deferred.

-----------------------------------------------------------------------------------------------
TASK GRAPH  ([R]=refactor/locally-testable  [V]=research-shaped, needs external verification)
Status legend: TODO | DOING | DONE | PARKED

Phase A — Ground truth & alignment (parallelizable; mostly research)
  t010 [V] TODO  Verify current ComfyUI HTTP/WS API surface & message schema.
  t011 [V] TODO  Verify LTX-2 19B node graph + text-encoder (Gemma 3 12B / 3840-dim vs T5).
  t012 [V] TODO  Establish modern gen-AI video conditioning practice (img2vid vs depth-CN vs
                 v2v vs first/last-frame) and per-frame Hydra -> clip-model mapping.
  t013 [R] TODO  Reconcile docs (README/INDEX) with actual tree; produce delta list.

Phase B — Build & structural scaffolding (depends on A for naming only)
  t020 [R] TODO  Fix plugin build graph: add comfyClient.cpp to sources; remove dead/dup CMake.
  t021 [R] TODO  Add JSON library; replace substring response parsing.            (dep: decision)
  t022 [R] TODO  Template-based workflow construction from versioned .json graphs. (dep: t021,t011)
  t023 [R] TODO  Remove determinism-defeating cache-busters; un-hardcode paths/URLs.
  t024 [R] TODO  Cross-platform networking layer (replace hand-rolled Winsock HTTP/WS).

Phase C — Wire the engine together (depends on B)
  t030 [R] TODO  Instantiate + feed ComfyUI client from delegate render settings.
  t031 [R] TODO  Implement _ExecuteFullPipeline (AOVs -> client -> color AOV; graceful fallback).
  t032 [V/R] TODO Resolve & implement per-frame vs per-clip output contract.        (dep: t012)

Phase D — Verification scaffolding (depends on B)
  t040 [R] TODO  C++ unit tests in tests/cpp/ (JSON parse, template mutation, codec, frame hash).
  t041 [R] TODO  Determinism regression test (render fixed scene twice, assert equal hash).

Ordering: t013 + Phase A parallel immediately. B mostly parallel except t022<-t021. C after B.
D after B. The deepest design risk is t012/t032 (per-frame vs per-clip output contract).
