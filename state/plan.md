GOAL: Scaffold the current CarWash render engine (Houdini USD/Hydra render delegate that
drives ComfyUI/LTX-2 for AI video generation) with awareness of current ComfyUI APIs and
current generative-AI model practices — so the engine's structure, integration points, and
conventions reflect how ComfyUI and modern gen-AI video pipelines are built today.

CONFIDENCE_THRESHOLD: 0.8

STATUS: EXECUTING — EXIT_CRITERIA CONFIRMED by human 2026-05-25. Scope bound d006 confirmed
(structural + verified practice; no validated-video requirement). Output contract (per-frame
vs per-clip) to be DECIDED VIA RESEARCH (t012) then human-confirmed before t032 implements.
Dependencies: header-only JSON approved; networking lib deferred to a separate decision.
Verification: code-review + standalone structural checks here (no Houdini/ComfyUI in container);
full build/run deferred to a Houdini machine. Phase A research wave launched first.

-----------------------------------------------------------------------------------------------
KEY SURVEY FINDING (see beliefs c001–c006):
The ComfyUI/LTX-2 AI path is fully written (comfyClient.cpp, ~2,359 LOC) but ORPHANED:
not in the plugin build, never instantiated by the delegate, and _ExecuteFullPipeline is a
TF_CODING_ERROR stub. Today the engine only rasterizes depth/normal AOVs on CPU. Docs
(README/INDEX) describe many dirs/tests/workflows that do not exist. So "scaffold" = connect +
modernize an orphaned integration, not greenfield.
-----------------------------------------------------------------------------------------------

EXIT_CRITERIA (CONFIRMED 2026-05-25):
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

Phase A — Ground truth & alignment (parallelizable; mostly research)  [COMPLETE]
  t010 [V] DONE  ComfyUI HTTP/WS API verified -> beliefs c007-c009, state/tasks/t010/findings.md. Closes q001.
  t011 [V] DONE  LTX-2 graph verified -> beliefs c010-c012, state/tasks/t011/findings.md. Closes q002.
  t012 [V] DONE  Conditioning/output contract -> beliefs c013-c014, state/tasks/t012/findings.md.
                 RECOMMENDATION (clip-native depth-video IC-LoRA v2v + clip cache) pending human decision (q003/q004).
  t013 [R] DONE  Doc-vs-reality delta -> beliefs c005,c015. INDEX/README reference many nonexistent
                 dirs/files (workflows/ python/ schema/ comfyui/ houdini/ scripts/, light.*, third_party/stb_image.h,
                 pytest files). Resolved in t050 (doc fix).

Phase B — Build & structural scaffolding (depends on A for naming only)
  t020 [R] DONE  Build graph fixed: comfyClient.cpp/.h added to sources; 3 dead if(FALSE) blocks removed.
                 BLOCKED on compile by t025 (missing stb_image.h, belief c015).
  t025 [R] TODO  Vendor the stb_image.h header comfyClient.cpp already #includes (belief c015). (dep: decision d007)
  t021 [R] TODO  Add header-only JSON lib (d003); replace substring response parsing.
  t022 [R] TODO  Template-based workflow construction from versioned .json graphs, using the
                 corrected LTX-2 node names from t011.                            (dep: t021, t011, q007 version, q003 contract)
  t023 [R] TODO  Remove determinism-defeating cache-busters; un-hardcode paths; fix WS default to :8188/ws (c007).
  t024 [R] TODO  Cross-platform networking layer; add execution_success/progress_state handling (c008). (dep: networking-lib decision)
  t050 [R] TODO  Reconcile README/INDEX with reality (t013 delta); remove fictional dirs/files/tests.

Phase C — Wire the engine together (depends on B)
  t030 [R] TODO  Instantiate + feed ComfyUI client from delegate render settings.
  t031 [R] TODO  Implement _ExecuteFullPipeline (AOVs -> client -> color AOV; graceful fallback).
  t032 [V/R] TODO Resolve & implement per-frame vs per-clip output contract.        (dep: t012)

Phase D — Verification scaffolding (depends on B)
  t040 [R] TODO  C++ unit tests in tests/cpp/ (JSON parse, template mutation, codec, frame hash).
  t041 [R] TODO  Determinism regression test (render fixed scene twice, assert equal hash).

Ordering: t013 + Phase A parallel immediately. B mostly parallel except t022<-t021. C after B.
D after B. The deepest design risk is t012/t032 (per-frame vs per-clip output contract).
