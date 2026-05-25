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
SUPERSEDED_BY:  c016
STATUS:         superseded
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
SUPERSEDED_BY:  c014
STATUS:         superseded
CREATED:        2026-05-25
UPDATED:        2026-05-25

# --- t010: current ComfyUI API (verified vs ComfyUI master, May 2026) ---

CLAIM_ID:       c007
CLAIM:          ComfyUI serves HTTP and WebSocket on the same default port 8188 (WS at /ws?clientId=<id>); CarWash's default ws://localhost:9999 (comfyClient.h:79, cpp:1960) is wrong and prevents WS connection, silently falling back to /history polling.
CONFIDENCE:     0.95
EVIDENCE:       t010 (ComfyUI master server.py:257-278; comfyClient.h:79, comfyClient.cpp:1960)
SUPERSEDES:     none
SUPERSEDED_BY:  c017
STATUS:         superseded
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c008
CLAIM:          Current ComfyUI emits a dedicated execution_success message as the authoritative whole-prompt completion signal, and progress_state as the primary progress channel; CarWash's WS parser (cpp:2313-2356) handles neither, and relies on the fragile executed+SaveImage assumption rather than the canonical executing:null sentinel.
CONFIDENCE:     0.9
EVIDENCE:       t010 (execution.py:805 execution_success, progress.py:162-186 progress_state, server.py:278; comfyClient.cpp:2313-2356)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c009
CLAIM:          The HTTP endpoints CarWash uses (/prompt, /history/{id}, /view, /upload/image, /system_stats) and their request/response shapes are still correct as of ComfyUI master; the client's HTTP surface is sound, the WS layer is the outdated part.
CONFIDENCE:     0.9
EVIDENCE:       t010 (server.py:450,502,647,905,918-972)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

# --- t011: current LTX-2 canonical graph ---

CLAIM_ID:       c010
CLAIM:          CarWash's hardcoded LTX-2 graph uses largely wrong/non-existent nodes: UNETLoader+transformer_only file (should be CheckpointLoaderSimple single checkpoint), LTXAVTextEncoderLoader (does not exist; should be LTXVGemmaCLIPModelLoader), plain LTXVImgToVideo (legacy 0.9.x; should be LTXVImgToVideoInplace/ConditionOnly), VAEDecode of taeltx (preview-only; should be LTXVSpatioTemporalTiledVAEDecode), SaveImage (should be CreateVideo->SaveVideo).
CONFIDENCE:     0.88
EVIDENCE:       t011 (Lightricks/ComfyUI-LTXVideo master example_workflows 2.0/2.3, __init__.py, README)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c011
CLAIM:          The text encoder family Gemma 3 12B is correct for LTX-2 (not T5/T5-XXL); CarWash's error is the loader node name, not the model choice.
CONFIDENCE:     0.9
EVIDENCE:       t011 (Lightricks README: google/gemma-3-12b-it-qat to models/text_encoders/; blog.comfy.org)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c012
CLAIM:          The current Lightricks flagship is LTX-2.3 (22B); LTX-2.0 19B (what CarWash references) is real but now labeled an older workflow — so "modernize" requires a target-version decision (2.0 19B vs 2.3 22B).
CONFIDENCE:     0.85
EVIDENCE:       t011 (Lightricks/ComfyUI-LTXVideo README; HF Lightricks/LTX-2.3)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

# --- t012: output contract / conditioning ---

CLAIM_ID:       c013
CLAIM:          A single depth image fed to LTXVImgToVideo conditions only the first frame; the remaining frames are unguided, so CarWash's current path cannot produce renderer-controllable, temporally coherent video.
CONFIDENCE:     0.9
EVIDENCE:       t012 (ComfyUI/LTX img2vid = first-frame conditioning; comfyClient.cpp:1291-1315)
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c014
CLAIM:          The recommended output contract is clip-native + depth-video IC-LoRA v2v: render the Hydra frame range to a per-frame depth sequence, submit ONE LTX-2 IC-LoRA Depth job (length quantized to the 8n+1 latent stride), cache the decoded clip keyed by (range, scene hash, seed), and serve per-frame Hydra requests from the cache. (Supersedes the open framing in c006.) PENDING human confirmation.
CONFIDENCE:     0.8
EVIDENCE:       t012 (LTX IC-LoRA blog, RunComfy depth-controlled video, docs.comfy.org LTX-2; arXiv 2601.03233)
SUPERSEDES:     c006
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

# --- build blocker found during t020 ---

CLAIM_ID:       c015
CLAIM:          comfyClient.cpp (lines 22-27) #includes "stb_image.h" with STB_IMAGE_IMPLEMENTATION, but no stb_image.h exists anywhere in the repo and there is no third_party/ dir — so the file cannot compile until the header is vendored. This blocks EXIT criterion 6 (functional AI path) even though CMake configure itself succeeds.
CONFIDENCE:     0.95
EVIDENCE:       t020 (comfyClient.cpp:24-27; find -iname stb_image* returns nothing; INDEX.md:68 references a nonexistent third_party/stb_image.h)
SUPERSEDES:     none
SUPERSEDED_BY:  c018
STATUS:         superseded
CREATED:        2026-05-25
UPDATED:        2026-05-25

# --- structural-layer fixes (t021/t023/t025/t050), review-only (not compiled here) ---

CLAIM_ID:       c016
CLAIM:          The wall-clock seed perturbation (params.seed + ms%1000000) has been removed from all three workflow builders so the generation seed is now the fixed params.seed; determinism (seed) is consistent with the project's thesis. (Output filenames still carry a ms cache-buster, which does not affect generation determinism.)
CONFIDENCE:     0.9
EVIDENCE:       t023 (comfyClient.cpp seed lines now `params.seed`; d005)
SUPERSEDES:     c004
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c017
CLAIM:          The ComfyUI WebSocket default is corrected to ws://127.0.0.1:8188/ws (constructor default in comfyClient.h and the port fallback in _WebSocketConnect), so progress WS now targets the correct port instead of :9999.
CONFIDENCE:     0.9
EVIDENCE:       t023 (comfyClient.h ctor default; comfyClient.cpp _WebSocketConnect port=8188; c007)
SUPERSEDES:     c007
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c018
CLAIM:          stb_image.h (v2.30) and nlohmann/json (v3.11.3) are vendored into plugin/hdCarWash/third_party/ and that dir is on the include path; the previously-missing stb_image.h is resolved, so comfyClient.cpp's includes are satisfiable.
CONFIDENCE:     0.85
EVIDENCE:       t025/t021 (third_party/stb_image.h, third_party/json.hpp; CMakeLists include dir added). NOTE: not compiled here — verification is review-only.
SUPERSEDES:     c015
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25

CLAIM_ID:       c019
CLAIM:          The fragile substring JSON handling in _SubmitWorkflow, _WaitForCompletion (polling), _DownloadResult, and _ParseWebSocketMessage is replaced with nlohmann::json parsing; the WS parser now recognizes execution_success and the executing:null sentinel (per t010).
CONFIDENCE:     0.8
EVIDENCE:       t021 (comfyClient.cpp parse rewrites; c008). NOTE: review-only, not compiled here.
SUPERSEDES:     none
SUPERSEDED_BY:  none
STATUS:         active
CREATED:        2026-05-25
UPDATED:        2026-05-25
