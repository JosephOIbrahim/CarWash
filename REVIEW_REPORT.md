# CarWash Code Review — Synthesis Report

## 1. Executive Summary

CarWash is an ambitious and architecturally coherent Hydra render delegate: the rasterizer→G-buffer→ComfyUI/LTX-2 conditioning pipeline is the right design, the determinism instinct (sorted prim order, single-threaded face iteration, a frame hash) is sound, and the backend-capability abstraction shows where this is going. But the codebase is mid-Phase-1: **the headline AI feature is built but never wired** — `_Execute` only ever calls the CPU rasterizer, and the entire `comfyClient` is not even compiled into the delegate (CMake references a non-existent `comfyuiClient.cpp` under `if(FALSE)`). The single most important thing to address is the **gap between what is documented and what ships**: the docs (INDEX.md, README) describe roughly 20 files and 6 directories that don't exist, the deployment script never installs `plugInfo.json` (so the plugin can't be discovered at all), and the determinism guarantee is contradicted by per-frame seed jitter. Behind that, there is a cluster of genuine memory-safety bugs in the AOV copy and ComfyUI codec paths that will bite the moment those paths run.

A note on confidence: 22 of 65 findings were verified line-by-line (high confidence). The remaining ~43 hit the verification budget cap and are marked **unverified** below — they are plausible and well-cited but should be spot-checked before acting. I've grouped accordingly.

---

## 2. Critical & High

### Verified (high confidence)

**[HIGH] AOV copy casts by name, ignoring the buffer's real `HdFormat` → heap overflow**
`renderPass.cpp` `_CopyFramebufferToAOVs` (221–283). The copy picks the destination pointer type from the AOV token (`color`→`GfVec4f*`, etc.) and `std::copy`s the full source vector in, never consulting `buffer->GetFormat()`. Clients (Solaris/Husk/usdview/`HdxColorizeTask`) routinely allocate color as `UNorm8Vec4` (4 B/px) or `Float16Vec4` (8 B/px), not the `Float32Vec4` (16 B/px) you advertise as *default*. When the allocation is smaller, you write up to 4× past the heap buffer.
*Fix:* read `fmt = buffer->GetFormat()`, branch on it, and convert (float32→float16/unorm8); or verify `HdDataSizeOfFormat(fmt)*w*h` matches source bytes and `TF_WARN`-skip on mismatch. This is the most dangerous live bug in the tree.

**[HIGH] No size/bounds validation between source bytes and mapped dest** *(same root as above)*
`renderPass.cpp` 229–279. The only guard is a width/height check (232–235); `numPixels` is computed (244) but never bounds the copy, and destination capacity is never compared. Merge the fix with the format check above: compute expected dest bytes, compare, then `memcpy` a checked count.

**[HIGH] Use-after-close race on the WebSocket socket**
`comfyClient.cpp`. `_WebSocketConnect/Disconnect/Send` all take `_wsMutex`; `_WebSocketReceive` (2145–2251) does **not** — it reads `_wsSocket`, runs `select`/`recv`, and writes the non-atomic `_wsConnected` with no lock. `ProcessFrameAsync` runs the receive loop on a `std::async` thread; the destructor (336–342) calls `_WebSocketDisconnect` → `closesocket` under the mutex while that thread is parked in a 500 ms `select`/`recv`. Classic use-after-close (handle can be reused mid-call) plus an unconditional data race on `_wsConnected`. Triggered by delegate teardown while a frame is still in flight.
*Fix:* atomic flag for the fast-path check; signal the receive loop to stop and join before `closesocket`; never close from a different thread than the one in `recv`.

**[HIGH] Four `Encode*` helpers: allocation sized by `width*height`, loop bounded by `vector.size()`**
`comfyClient.cpp` `EncodeDepthBuffer` (442–465), `EncodeNormalBuffer` (477–485), `EncodeColorBuffer` (497–505), `EncodeIdBuffer` (517–540). Each allocates from the dimension args but loops over the input vector — a caller-controlled heap overflow on these *public static* API entry points. Latent today (no in-repo caller passes a mismatched size), but it's a footgun on a public surface.
*Fix:* early-out on `vector.size() != width*height*channels`, or clamp the loop to `min`. Apply to all four.

### Unverified (cited, plausible — spot-check before acting)

These hit the verification cap. Several are high-severity *if accurate*; I'd verify the determinism cluster and the deploy bug first since they undercut the project's two core promises.

**[CRITICAL?] `rebuild_and_install.bat` never installs `plugInfo.json`** (64–75). It hand-copies only `hdCarWash.dll` and bypasses `cmake --install`. Without the manifest beside the lib, Hydra never registers `HdCarWashRendererPlugin` and **"CarWash" never appears as a renderer**. *Fix:* deploy `plugInfo.json` (and `resources/`) per its `Root`/`LibraryPath`, or just use `cmake --install`. — *If true, this is the #1 blocker for a new user and arguably the single most important fix in the report.*

**[CRITICAL?] INDEX.md and README document ~20 files / 6 dirs that don't exist** (`workflows/`, `python/`, `schema/`, `comfyui/`, `houdini/`, `scripts/`, `download_models.ps1`, the pytest suites, etc.). A new user's map points at nothing. *Fix:* rewrite the trees to the actual layout; mark planned items as planned.

**[HIGH?] Determinism cluster — the project's headline claim vs. the code:**
- *Lossy hash:* `fnv1a_float` quantizes `(int32_t)(value*10000)` before hashing, so 5th-decimal FP drift (exactly the accumulation-order/reassociation nondeterminism you most need to catch) hashes **identical** and reports "VERIFIED." (`rasterizer.cpp` 70–166)
- *Sparse sample:* `ComputeHash` samples 1/256 of pixels + 5 corners; differences in unsampled pixels are invisible. Doc specifies full-buffer SHA256.
- *Seed jitter:* every workflow builder adds `params.seed + ms%1000000` and stamps a wall-clock cache-buster (`comfyClient.cpp` 827, 1124, 1354) — same scene + same seed → different noise every submission, directly contradicting README line 15 and `DETERMINISM_CROSS_REFERENCE.md` (`"seed": 42 // FIXED`).
*Fix:* hash raw IEEE-754 bits (canonicalize NaN/−0), `sampleRate=1` for the authoritative check, and gate seed jitter / timestamp cache-busting behind a "force re-render" flag using a content-derived cache key. Scope the guarantee to same-binary/same-machine.

**[HIGH?] Rasterizer correctness** (all unverified): screen-space-linear depth interpolation (not perspective-correct, and not the "camera-space Z" the header claims), and no near-plane clipping (triangles straddling `w<=0` produce garbage NDC). Both distort the depth/geometry conditioning fed to LTX-2.

**[HIGH?] Install-layout mismatch** — CMake puts `plugInfo.json` in `resources/` while its `LibraryPath: "lib/hdCarWash.dll"` resolves relative to the manifest dir, yielding a non-loadable layout; the `.bat` targets a third path entirely.

**[HIGH?] Docs/test gaps:** README points at non-existent `test_determinism.py`/`test_comfyui_nodes.py`; no automated determinism/ComfyUI tests exist; `test_carwash_render.py` can't fail (bare `except`, `return True` on missing output); README documents render-setting tokens (`positive_prompt`, `video_length`, `frame_rate`) the delegate never registers; root CMake `install()` globs absent `schema/`/`houdini/` dirs; hardcoded `Houdini 21.0.607` path breaks on upgrade.

---

## 3. Medium & Low

### Verified

**Medium**
- **`std::stoi` on URL port can crash the render thread** — `comfyClient.cpp` 1690/1766/1845/1978, no try/catch; throws on empty/IPv6/non-numeric port and propagates through `ProcessFrame`. Use `from_chars`/try-catch + strip trailing path. *(Note: the `http://host:8188/` example doesn't actually throw — stoi stops at `/`.)*
- **No socket timeouts on HTTP connect/recv** — `_HttpGet/_HttpPost/_UploadImageToComfyUI`. A server that accepts-then-stalls blocks `recv` forever; `_WaitForCompletion`'s timeout can't interrupt a call already inside `recv`. Set `SO_RCVTIMEO/SO_SNDTIMEO` + non-blocking connect.
- **All ComfyUI JSON parsing is substring-based** — `comfyClient.cpp` 1421–1449, 1484–1496, 1529–1578, 2312–2357. Real bugs: compact-JSON `"outputs":{}` defeats the empty-output guard (failed job read as success); `_DownloadResult` grabs the *first* `filename` (wrong frame for multi-image LTX2); WS completion matches the substring `SaveImage` anywhere. Use `pxr/base/js` (already transitively available); navigate `outputs[id].outputs.<node>.images[0].filename`.
- **LTX2 workflow hardcodes model names / node classes / magic constants** — incl. an SD1.5 normal ControlNet (`control_v11p_sd15_normalbae.pth`) wired into an SDXL graph (dormant; `useNormalControl` defaults false). The unused `_workflowPath` template hook already exists. Externalize names/constants; surface the `/prompt` `node_errors` body instead of a generic timeout.
- **Render-buffer dimension getters read `_width/_height/_format` without the mutex** that `Allocate`/`_Deallocate` hold (`renderBuffer.h` 44–52), despite the "thread-safe concurrent read/write" doc and `IsMapped`/`IsConverged` correctly locking. Lock the getters or make the scalars `std::atomic`.
- **`CreateInstancer` returns `nullptr` + `TF_CODING_ERROR`** while `mesh` is a supported Rprim (`renderDelegate.cpp` 271–279). Any point/native-instanced scene → Hydra deref of null → crash. Return a minimal concrete `HdInstancer` stub. *(Bonus: CMake references `instancer.cpp/.h` that don't exist on disk.)*

**Low**
- Untrusted-PNG over-reads in deprecated `DecodeColorImage` (546–710) — read-only, off the live path (live uses stb_image). Delete it.
- `_cancelRequested` non-atomic + never reset → data race **and** permanent client disablement after first cancel. Make atomic, reset at `ProcessFrame` start.
- Unbounded recursion on WS ping/pong floods (2237–2245) → stack overflow. Convert to a loop.
- Fixed `Sec-WebSocket-Key` + constant frame mask + substring-`"101"` handshake check — RFC 6455 violations, latent breakage behind any proxy. Local-only today.
- Per-instance `WSAStartup/WSACleanup` with unchecked return (325–342). Only the unchecked return is a real (minor) defect; refcounting makes the "cross-instance teardown" framing largely moot. Init once + check.
- `send()` return ignored in all HTTP paths (partial sends truncate body) — low probability over loopback; loop on bytes written.
- `carwashUV` declared in tokens but has no `GetDefaultAovDescriptor` entry → `HdFormatInvalid`; `carwashMotionVector/Edges/StabilityMask` are advertised but never written (render as clear value). *(Finding's "framebuffer carries uv" is wrong — there is no uv plane.)*
- Pass reports converged unconditionally; unwritten AOVs marked converged (cosmetic for single-pass).
- Dead frustum-cull block (`rasterizer.cpp` 342–352) — comment promises culling the empty body doesn't do.

### Unverified (medium/low, cited)
- Projection ignores vertical aperture; FOV from horizontal only (`camera.cpp` 104–134); `_verticalAperture` stored, never used.
- `GetViewProjectionMatrix()` multiplies an uninitialized identity `_projectionMatrix` (dormant; pass computes its own).
- No bounds checks on face-vertex indices (`rasterizer.cpp` 254–269, `mesh.cpp` 225–241) → OOB read on malformed topology.
- `comfyClient.cpp/.h` exist on disk but excluded from build; CMake's only ref is `if(FALSE)` and names a non-existent `comfyuiClient.cpp`.
- Tests enabled by default but `tests/cpp` is an empty stub → zero C++ coverage.
- Auto-`pip install` via `subprocess` on import (×5 synapse scripts); `[sys.executable, '-m', 'pip']` at minimum, or just declare the dep.
- `synapse_reload.py --install` double-copies the DLL via a redundant `asyncio.to_thread` dance + dead `isinstance` fallback; 3 scripts locate the DLL 3 different ways.
- Hardcoded paths/ports throughout automation; debug-log read without `encoding=` (cp1252 → `UnicodeDecodeError`); bare-`except` hiding connection failures inconsistently across synapse scripts.
- Per-corner n-gon normals from only the first triangle's face normal (`mesh.cpp` 224–242) — inconsistent with the rasterizer's fan triangulation.
- Per-pixel `std::abs` on barycentrics makes everything double-sided regardless of `IsDoubleSided()`; back-face normals not flipped.
- `houdini_configure_target(hdCarWash)` never called despite the root-CMake comment claiming it is → possible USD ABI/namespace mismatch at load.
- Project-name casing chaos (`CarWash`/`hdCarWash`/`HdCarWash`, plus the `HDCARWAASH` typo path); 3 different user-facing renderer labels.

---

## 4. Quick Wins (high value, low effort)

1. **Install `plugInfo.json` in the `.bat`** (or switch to `cmake --install`). One-line-ish change that is plausibly the difference between the plugin loading at all and not. *(verify first)*
2. **Make `_cancelRequested` `std::atomic<bool>` + reset it** — two lines, removes UB and a permanent-disable bug.
3. **`try/catch` (or `from_chars`) around the four `std::stoi` port parses** — kills a hard render-thread crash from a trailing-slash/IPv6 URL.
4. **Add the `Encode*` size guard** (one early-out per helper) — closes four heap-overflow entry points.
5. **Replace `(value*10000)` quantization with raw IEEE-754 bit hashing** — small change that makes the determinism check actually able to detect what it claims. *(verify first)*
6. **Set `SO_RCVTIMEO/SO_SNDTIMEO`** on the HTTP sockets — removes the indefinite-hang failure mode.
7. **Gate the per-frame `C:/Temp/hdcarwash_debug.txt` writes behind the already-wired `TF_DEBUG(HD_CARWASH)`** — appears in `renderPass.cpp`, `renderDelegate.cpp`, `mesh.cpp`, and ~7 sites in `comfyClient.cpp`. Removes hot-path I/O, a hardcoded path, and a workflow-JSON leak.
8. **Default `HDCARWASH_BUILD_TESTS` OFF** until real tests exist (or add one rasterizer unit test) — stops `ctest` from implying coverage. *(verify first)*

---

## 5. Opportunities (by leverage)

1. **Wire the ComfyUI path into the render pass — the single highest-leverage step.** `_Execute` only calls `_ExecutePhase1`; `_ExecuteFullPipeline` is a `TF_CODING_ERROR` stub; `renderPass.h` doesn't even reference `ComfyClient`. The entire submit/wait/download pipeline is dead at the orchestration layer. Add a client member, branch in `_Execute` on the AI backends, feed the Phase-1 G-buffer as conditioning, write the AI result to the color AOV. *(verify)*
2. **Adopt a non-blocking convergence model for the AI call.** `ProcessFrame` is fully blocking with a hardcoded 60 s timeout — far too short for 25-frame LTX-2 at 1312×992, and it will stall the viewport when wired. `ProcessFrameAsync` is the right primitive but has zero callers. Kick it from `_Execute`, store the `future`, report `IsConverged()==false` until ready; make the timeout a resolution/frame-scaled setting; cache `IsServerAvailable()` instead of probing every frame. *(verify)*
3. **Feed the rich G-buffer into conditioning — the differentiating feature.** Only `depth.png` reaches LTX-2; motion vectors, semantic/object IDs, edges, and stability mask are computed/declared but never sent, even though `capTemporalCoherence` already exists to gate them. This is the product thesis (Houdini motion → temporal coherence, IDs → object permanence) and the plumbing is ~80% there. *(verify)*
4. **Finish or fence the backend abstraction.** Flux/Cosmos are capability-advertised and `CARWASH_BACKEND`-selectable but have no builders — they silently fall through to the SDXL txt2img path. Either add real builders behind a backend→builder registry, or restrict the exposed choices to what's implemented. *(verify)*
5. **Surface errors to the artist.** `HdCarWashRenderResult.errorMessage` is well-designed but never shown (no caller); diagnostics go to `TF_WARN` + the `C:/Temp` file. Route into render stats / `TF_RUNTIME_ERROR` so "server unreachable"/"timed out" reaches the Houdini render view. *(verify)*
6. **Split `comfyClient.cpp` (2360 lines, 5 subsystems).** base64 + a from-scratch PNG encoder + a partial zlib inflater + raw Winsock HTTP + hand-rolled WebSocket + 3 JSON-string builders in one TU. Split into `comfyHttp` / `comfyWorkflow` / `imageCodec`; delete dead `DecodeColorImage`; consider keep-alive instead of new-socket-per-request (matters once history polls every 250 ms). A small JSON object model also closes the prompt-injection-into-JSON surface. *(verify)*

---

## 6. Documentation (a new user hits these first)

The doc-vs-reality drift is the project's most user-facing weakness and overlaps the Critical/High section — consolidating here:

- **INDEX.md** lists 6 directories and ~20 files that don't exist. It's a map to nothing. **Rewrite to the actual tree** and mark planned items as planned. *(verify — flagged critical)*
- **README** install path is unfollowable end-to-end: `download_models.ps1`, `workflows/*.json`, and the pytest commands all reference missing artifacts; `plugin/lib/` doesn't exist. **Gate or remove** until the files land. *(verify — flagged critical)*
- **README render-settings table** documents tokens the delegate never registers (`positive_prompt`, `negative_prompt`, `video_length`, `frame_rate`). Replace with the real tokens: `backend, deterministicMode, comfyuiServerUrl, inferenceSteps, guidanceScale, substrateEnabled, substrateHistoryFrames, seed`. *(verify)*
- **Determinism docs vs. code:** README/`DETERMINISM_CROSS_REFERENCE.md` promise fixed-seed reproducibility and full-buffer SHA256; the code does per-frame seed jitter and a sampled+quantized FNV-1a, and references `sceneHash.h`/`frameVerifier.h`/OpenSSL that don't exist. Add a "forward-looking design doc" banner, correct the hashing reference, and scope the guarantee honestly. *(verify)*
- **Three divergent build invocations** (README vs. INDEX vs. CMakeLists header) and **three renderer labels** (`CarWash` / `HdCarWashRendererPlugin` / `CarWash (AI NPR)`). Pick one of each.
- **Naming convention:** settle `HdCarWash` (repo/dir) / `hdCarWash` (lib) / `CarWash` (display), state it once, fix the `HDCARWAASH` typo and stale `Downloads` path.
- **Branding doc** cites `rasterizer.cpp:350-354` (frustum code) for shading colors; the real tint is `474-480` `GfVec4f(0.2,0.6,0.9)` ≈ `#3399E6`, not the doc's `#3399EE`, and the secondary/accent/background colors appear nowhere in code. *(verify)*

---

*Confidence note: Section 2's "Verified" subsection and all of Section 3's "Verified" subsection are line-checked. Everything tagged **(verify)** / "Unverified" survived adversarial review on citations but exceeded the verification budget — treat as high-quality leads, not settled facts. The deploy bug (plugInfo.json) and the determinism cluster are the highest-value items to verify first, since they gate first-run success and the core product claim respectively.*