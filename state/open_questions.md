# Open Questions (leverage-ranked)

A question closes when a claim with confidence >= CONFIDENCE_THRESHOLD (0.8) answers it.

QUESTION_ID:    q001
QUESTION:       What are the current ComfyUI HTTP + WebSocket API contracts (endpoints, request/response shapes, WS message schema) the client must target?
LEVERAGE:       high
STATUS:         open
CLOSED_BY:      none
CREATED:        2026-05-25

QUESTION_ID:    q002
QUESTION:       What is the canonical current LTX-2 19B ComfyUI workflow (exact node names/signatures, loaders), and is Gemma 3 12B / 3840-dim the correct text encoder vs T5?
LEVERAGE:       high
STATUS:         open
CLOSED_BY:      none
CREATED:        2026-05-25

QUESTION_ID:    q003
QUESTION:       For a Hydra delegate emitting per-frame depth/normal, what is the right modern conditioning + output contract for a clip-based video diffusion model (img2vid vs depth-ControlNet vs v2v; per-frame vs per-clip)?
LEVERAGE:       high
STATUS:         open
CLOSED_BY:      none
CREATED:        2026-05-25

QUESTION_ID:    q004
QUESTION:       Is "single frame from a 25-frame clip" the intended product, or should the engine produce video sequences natively?
LEVERAGE:       medium
STATUS:         open
CLOSED_BY:      none
CREATED:        2026-05-25

QUESTION_ID:    q005
QUESTION:       Which third-party libraries are acceptable to add (JSON, HTTP/WS, zlib/libpng) given Houdini's bundled-dependency constraints and Invariant 8 (no new dependency without a decision)?
LEVERAGE:       medium
STATUS:         open
CLOSED_BY:      none
CREATED:        2026-05-25

QUESTION_ID:    q006
QUESTION:       Is the large "cognitive substrate" token hierarchy in tokens.h in scope for this scaffolding effort, or parked?
LEVERAGE:       low
STATUS:         open
CLOSED_BY:      none
CREATED:        2026-05-25
