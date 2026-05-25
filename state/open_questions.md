# Open Questions (leverage-ranked)

A question closes when a claim with confidence >= CONFIDENCE_THRESHOLD (0.8) answers it.

QUESTION_ID:    q001
QUESTION:       What are the current ComfyUI HTTP + WebSocket API contracts (endpoints, request/response shapes, WS message schema) the client must target?
LEVERAGE:       high
STATUS:         closed
CLOSED_BY:      c007,c008,c009 (see state/tasks/t010/findings.md)
CREATED:        2026-05-25

QUESTION_ID:    q002
QUESTION:       What is the canonical current LTX-2 19B ComfyUI workflow (exact node names/signatures, loaders), and is Gemma 3 12B / 3840-dim the correct text encoder vs T5?
LEVERAGE:       high
STATUS:         closed
CLOSED_BY:      c010,c011 (Gemma correct; node graph mostly wrong — see state/tasks/t011/findings.md). Residual: exact embedding dim + taeltx existence (q008).
CREATED:        2026-05-25

QUESTION_ID:    q003
QUESTION:       For a Hydra delegate emitting per-frame depth/normal, what is the right modern conditioning + output contract for a clip-based video diffusion model (img2vid vs depth-ControlNet vs v2v; per-frame vs per-clip)?
LEVERAGE:       high
STATUS:         answered-pending-decision
CLOSED_BY:      c013,c014 (recommendation: clip-native depth-video IC-LoRA v2v + clip cache). Requires human confirmation before t022/t032 implement.
CREATED:        2026-05-25

QUESTION_ID:    q004
QUESTION:       Is "single frame from a 25-frame clip" the intended product, or should the engine produce video sequences natively?
LEVERAGE:       medium
STATUS:         answered-pending-decision
CLOSED_BY:      c014 (recommend clip-native with a per-frame cache serving Hydra). Human decision pending.
CREATED:        2026-05-25

QUESTION_ID:    q005
QUESTION:       Which third-party libraries are acceptable to add (JSON, HTTP/WS, zlib/libpng) given Houdini's bundled-dependency constraints and Invariant 8?
LEVERAGE:       medium
STATUS:         closed
CLOSED_BY:      human decision 2026-05-25 (d003): header-only JSON approved; networking lib deferred to a separate decision (t024).
CREATED:        2026-05-25

QUESTION_ID:    q006
QUESTION:       Is the large "cognitive substrate" token hierarchy in tokens.h in scope for this scaffolding effort, or parked?
LEVERAGE:       low
STATUS:         open
CLOSED_BY:      none
CREATED:        2026-05-25

QUESTION_ID:    q007
QUESTION:       Target LTX version: scaffold against LTX-2.0 19B (what CarWash references) or current flagship LTX-2.3 22B? Drives the t022 template graph and model paths.
LEVERAGE:       high
STATUS:         open
CLOSED_BY:      none
CREATED:        2026-05-25

QUESTION_ID:    q008
QUESTION:       Exact Gemma-3-12B projection dim into LTX-2 cross-attention (is README "3840-dim" right?), and does an official taeltx_2.safetensors ship for LTX-2 at all?
LEVERAGE:       low
STATUS:         open
CLOSED_BY:      none
CREATED:        2026-05-25

QUESTION_ID:    q009
QUESTION:       Does LTX-2 IC-LoRA depth v2v accept a pre-rendered depth sequence directly, or does it insist on running its own depth estimator (Lotus) on an RGB reference? (Determines whether CarWash's ground-truth depth AOV is consumed directly.) Also: exact valid clip lengths (strictly 8n+1? max?) for the chosen checkpoint.
LEVERAGE:       high
STATUS:         open
CLOSED_BY:      none
CREATED:        2026-05-25
