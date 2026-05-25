# Beliefs (single-writer: orchestrator only)

Claims below are grounded in the t000 planner survey (direct repo inspection). Codebase-
verifiable claims carry high confidence; claims requiring external (ComfyUI/model) knowledge
are deliberately NOT asserted here — they are tracked as open questions until critic[evaluate]
resolves them.

CLAIM_ID:       c001
CLAIM:          The ComfyUI/LTX-2 client (plugin/hdCarWash/comfyClient.cpp, ~2,359 LOC) is fully written but orphaned — it is not in the plugin CMake sources, is never instantiated by the delegate, and the delegate's client member is commented out.
CONFIDENCE:     0.9
EVIDENCE:       t000-planner-survey (grep: only self-references + commented member at renderDelegate.h:159; comfyClient.cpp absent from HDCARWASH_SOURCES)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c002
CLAIM:          The AI render path is not connected: renderPass _ExecuteFullPipeline is a TF_CODING_ERROR stub; only _ExecutePhase1 (CPU rasterized depth/normal AOVs) actually runs.
CONFIDENCE:     0.9
EVIDENCE:       t000-planner-survey (renderPass.cpp _Execute -> _ExecutePhase1; _ExecuteFullPipeline stub)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c003
CLAIM:          The ComfyUI client hand-rolls its entire network/serialization stack (raw Winsock HTTP, manual RFC6455 WebSocket with a fixed Sec-WebSocket-Key, hand-written PNG/zlib-store/base64, and JSON via string concatenation + substring parsing), all Windows-only (#ifdef _WIN32 else returns "").
CONFIDENCE:     0.88
EVIDENCE:       t000-planner-survey (comfyClient.cpp _HttpGet/_HttpPost, _BuildWorkflow* ostringstream builders, escapeJson lambda, response.find(...) parsing)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c004
CLAIM:          Determinism claims conflict with the code: every workflow builder applies a wall-clock seed perturbation (seed + ms % 1000000) and millisecond-based filenames, defeating the fixed-seed reproducibility the README/DETERMINISM_CROSS_REFERENCE.md assert.
CONFIDENCE:     0.85
EVIDENCE:       t000-planner-survey (seed perturbation in _BuildWorkflow*, ms-based filenames; doc claims of reproducibility)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c005
CLAIM:          Documentation overstates reality: README/INDEX describe workflows/, python/, comfyui/, schema/, houdini/, scripts/ dirs and pytest files that do not exist in the tree (only automation/ exists); tests/cpp/ has no actual tests.
CONFIDENCE:     0.85
EVIDENCE:       t000-planner-survey (tree listing vs README/INDEX references)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c006
CLAIM:          There is a fundamental design tension: a Hydra delegate renders per-frame, but LTX-2 is a clip-based video diffusion model emitting 25-frame clips (hard-coded). Whether the product is single-frame extraction or native video sequences is unresolved and is the deepest design risk.
CONFIDENCE:     0.7
EVIDENCE:       t000-planner-survey (_BuildWorkflowLTX2 length=25; Hydra per-frame model)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25
