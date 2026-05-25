# t011 — Current canonical LTX-2 ComfyUI graph vs CarWash hardcoded graph

Source: Lightricks/ComfyUI-LTXVideo master (example_workflows 2.0 & 2.3, __init__.py, README),
docs.comfy.org/tutorials/video/ltx/ltx-2, HF Lightricks/LTX-2.3, blog.comfy.org.

## Version landscape
- LTX-2 19B is real (Lightricks DiT audio-video, in ComfyUI core comfy/ldm/lightricks).
- Current flagship is LTX-2.3 (22B). LTX-2.0 (19B) is labeled "older workflows."
- Decision needed: target 2.0 19B (what CarWash references) or 2.3 22B (current).

## Node mapping  (CarWash hardcoded  ->  canonical LTX-2)
| CarWash (_BuildWorkflowLTX2)                        | Canonical LTX-2 |
|----------------------------------------------------|-----------------|
| UNETLoader(...transformer_only.safetensors)        | CheckpointLoaderSimple(single full checkpoint: model+VAE) |
| LTXAVTextEncoderLoader(gemma_3_12B_it_fp4_mixed)   | LTXVGemmaCLIPModelLoader (Gemma 3 12B dir)  [LTXAVTextEncoderLoader does NOT exist] |
| CLIPTextEncode x2                                  | CLIPTextEncode x2 (ok); 2.3 adds GemmaAPITextEncode |
| LTXVConditioning(frame_rate=25)                    | LTXVConditioning  (CORRECT — only node that matches) |
| LoadImage(depth.png) -> LTXVImgToVideo             | LTXVImgToVideoInplace (2.0) / LTXVImgToVideoConditionOnly (2.3); plain LTXVImgToVideo is legacy 0.9.x |
| LTXVScheduler                                      | ManualSigmas (+KSamplerSelect/RandomNoise/CFGGuider/SamplerCustomAdvanced) for 2.0 distilled; LTXVScheduler returns in 2.3 single-stage |
| VAELoader(taeltx_2) + VAEDecode                    | checkpoint VAE via LTXVSpatioTemporalTiledVAEDecode / LTXVTiledVAEDecode (TAE = preview-only, degraded final) |
| SaveImage (one frame)                              | CreateVideo -> SaveVideo (LTX-2 emits synced audio+video) |
| frames hard-coded 25, snap-to-32                   | EmptyLTXVLatentVideo constraints; temporal length wants 8n+1 |

## Text encoder
Gemma 3 12B is CORRECT (download google/gemma-3-12b-it-qat-q4_0-unquantized to models/text_encoders/).
NOT T5/T5-XXL (that was legacy 0.9.x). Loader node name is the error, not the model family.

## Open (unverified)
- Exact Gemma-3-12B projection dim into LTX-2 cross-attention (README "3840-dim" not primary-confirmed).
- Whether an official taeltx_2.safetensors ships for LTX-2 (CarWash "LTX2/..." paths look like local conventions, not official HF paths).
