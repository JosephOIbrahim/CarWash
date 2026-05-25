# Parked (append-only) — out-of-scope / deferred findings

## p001 — Hardcoded Sec-WebSocket-Key (MINOR, promote: false)
comfyClient.cpp:2022 uses a fixed non-random Sec-WebSocket-Key. Works (ComfyUI/aiohttp doesn't
validate it) but non-spec; could fail behind a strict proxy. Fix opportunistically in t024.
Source: t010.

## p002 — WS binary preview frames unhandled (MINOR, promote: false)
ComfyUI sends binary (opcode 0x2) preview-image frames; CarWash's receive loop is text-only and
ignores them. Not fatal. Address in t024 if convenient. Source: t010.

## p003 — taeltx_2 / model file paths look like local conventions (MINOR, promote: true)
The "LTX2/..." safetensors paths in CarWash appear to be local naming, not official HF paths, and
taeltx is preview-only. Reconcile model paths when t022 rebuilds the workflow template. Source: t011.
