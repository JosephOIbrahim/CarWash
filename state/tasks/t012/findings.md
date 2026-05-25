# t012 — Output contract & conditioning practice (the deepest design decision)

Source: LTX-2 arXiv (2601.03233), Lightricks/LTX-2 README, docs.comfy.org LTX-2 tutorials,
RunComfy LTX-2 depth-controlled video, LTX IC-LoRA blog, Next Diffusion / MindStudio v2v guides.

## The core problem
A single depth image -> LTXVImgToVideo conditions ONLY frame 0; the other 24 frames are
hallucinated with no structural guidance. That is the opposite of what a renderer wants.
Temporal coherence is a property of ONE clip denoise pass — it cannot be obtained by stitching
independently-sampled single frames.

## Best practice (2026)
Depth-based VIDEO-to-video structural control via LTX-2 IC-LoRA (modes: Depth / Pose / Canny).
The reference video supplies per-frame structure/motion; the prompt is reduced to styling.
This conditions EVERY output frame, not just the first.

## RECOMMENDED OUTPUT CONTRACT — clip-native, depth-video driven
Per render of a frame range [f0..f1]:
  1. Rasterize and emit a per-frame DEPTH SEQUENCE (one normalized depth PNG per frame; optionally
     normals), not a single depth.png. (CarWash already normalizes depth correctly, cpp:459-463.)
  2. Submit ONE LTX-2 IC-LoRA Depth video-to-video job over the whole range; length quantized to
     the model's 8n+1 latent stride (replace hard-coded 25 with quantize(f1-f0+1)).
  3. CACHE the decoded clip keyed by (frame-range, camera/scene hash, seed).
  4. Serve each Hydra per-frame ProcessFrame request from the cache (map output-frame -> Hydra frame).

## AOV priority
depth (linear, native IC-LoRA mode) > normal (edge/structure analogue) > motion vectors
(desirable for coherence but NO standard LTX-2 control path today; AOV currently unused) > color
(keyframe/low-strength v2v reference, not the structural driver).

## Rejected alternatives
- Keep single-image img2vid: only conditions frame 0; incoherent. REJECT.
- One clip per Hydra frame, keep one frame: ~25x compute + flicker (independent samples). REJECT.
- Native sequence renderer bypassing Hydra per-frame API: cleanest but larger refactor, fights
  Houdini's driver model. DEFER (the clip cache gets the same coherence with compatibility).

## New open questions (high leverage)
- Exact valid clip lengths for the specific LTX-2 19B distilled checkpoint (strictly 8n+1? max 257?).
- Any LTX-2 control mode ingesting optical flow / motion vectors directly?
- Does IC-LoRA depth v2v accept a pre-rendered depth sequence directly, or insist on running its
  own depth estimator (Lotus) on RGB — i.e. can CarWash supply ground-truth depth and bypass estimation?

## REQUIRES HUMAN DECISION
This output contract is the deepest design choice. Per the user's "decide via research" answer,
bring this recommendation back for confirmation before t022/t032 implement it.
