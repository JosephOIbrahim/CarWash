# CarWash Top-5 Findings — Fix Landing Report

Prepared for the codebase owner. Five experts each fixed one finding in their owned files; each fix was independently reviewed. All five reviews returned **approved-with-nits** — no rejections, no blockers. Two cross-team follow-ups are required before the determinism and discovery findings can be marked fully closed.

---

## 1. What landed

### Memory safety (lead)

**Finding #4a — AOV heap overflow** (`renderPass.cpp::_CopyFramebufferToAOVs`)
The original code cast the mapped destination pointer from the AOV token and `std::copy`'d the entire float32 source into it, never reading `buffer->GetFormat()`. A client allocating color as `UNorm8Vec4` (4 B/px) or `Float16Vec4` (8 B/px) — the standard Solaris/Husk/usdview viewport case — received a 16 B/px write, a 2x–4x heap overflow.
**Change:** Added two file-local helpers (`_WriteFloatComponent`, `_WriteColorAov`) in an anonymous namespace; the copy body now reads `fmt = buffer->GetFormat()`, computes `dstBytesPerPixel = HdDataSizeOfFormat(fmt)`, and routes every AOV through a format/capacity-bounded path. Color converts float32→dest component format (Float32/Float16/UNorm8/SNorm8); normal converts per-component for ≥3-component formats; depth handles Float32-exact + Float16-with-capacity-guard; int32 ids require exact format+size match. Incompatible formats `TF_WARN`+skip instead of overflowing. `renderPass.h` correctly untouched (helpers are file-local).
**Verdict:** approved-with-nits, high confidence. Reviewer traced every write path against source vector sizes and `renderBuffer.cpp` allocation — no path can write past capacity. A matching `Float32Vec4` dest still gets numerically identical full-precision data.

**Finding #4b — comfyClient Encode* heap overflow** (`comfyClient.cpp`, `EncodeDepth/Normal/Color/IdBuffer` ~524–620)
All four `Encode*` helpers allocated `width*height*channels` but looped over `vector.size()`; a mismatched caller vector wrote past the allocation.
**Change:** Each write loop now clamps to `min(vector.size(), (size_t)width*height)`. Reviewer confirmed `pixelCount` is `size_t` (no 32-bit overflow); matching-size inputs (all in-repo callers) are byte-identical.
**Verdict:** approved-with-nits, medium confidence.

### Deploy (lead)

**Finding #1 — "CarWash" never appears in Hydra** (`rebuild_and_install.bat`, root `CMakeLists.txt`, `plugin/hdCarWash/CMakeLists.txt`)
Root cause: `rebuild_and_install.bat:68` had exactly one copy command (`hdCarWash.dll` only) and never deployed `plugInfo.json`, so Hydra found no manifest and never ran the `TF_REGISTRY_FUNCTION` registration. Compounded by a three-way layout mismatch: root CMake put the manifest under `plugin/usd/hdCarWash/resources/`, the plugin CMake put the dll at `plugin/usd/hdCarWash/lib/`, and the .bat targeted a third path. `plugInfo.json` itself was internally correct (`Root="."`, `LibraryPath="lib/hdCarWash.dll"`, `ResourcePath="resources"`).
**Change:**
- Root `CMakeLists.txt` (~87–109): moved `plugInfo.json` install dest from `plugin/usd/hdCarWash/resources` to `usd/hdCarWash` (the manifest dir); wrapped `schema/` and `houdini/` `install(DIRECTORY)` in `if(EXISTS ...)` guards so `cmake --install` no longer aborts on absent source dirs.
- `plugin/hdCarWash/CMakeLists.txt` (~123–127): moved dll install dest to `usd/hdCarWash/lib` with RUNTIME-before-LIBRARY (correct for a Windows SHARED lib).
- `rebuild_and_install.bat` (15–28, 74–85): replaced the manual `mkdir` + single-dll `copy` with `cmake --install . --config Release --prefix "%INSTALL_PREFIX%"`; de-hardcoded the Houdini path via overridable `HFS` / `HOUDINI_USER_PREF_DIR`.

Now the manifest's `LibraryPath="lib/hdCarWash.dll"` resolves to the actual binary, and the .bat deploys both files together to `%USERPROFILE%\houdini21.0\dso\usd\hdCarWash`. `plugInfo.json` rightly left untouched.
**Verdict:** approved-with-nits, high confidence. Root cause correctly addressed; two caveats below keep it from clean approve.

### Determinism

**Finding #3a — frame hash falsely reports VERIFIED** (`rasterizer.cpp`, `rasterizer.h`)
Two defects: (1) `fnv1a_float` quantized via `(int32_t)(value*10000.0f)` before hashing, masking exactly the 5th-decimal accumulation/reassociation drift the check exists to detect; (2) `ComputeHash` defaulted to `sampleRate=16`, hashing ~1/256 of pixels, so differences in ~99.6% of pixels were invisible.
**Change:** `fnv1a_float` (~71–91) now hashes raw IEEE-754 bits via `memcpy` into a `uint32_t`, with NaN canonicalized to `0x7FC00000` and `-0.0`→`+0.0`. Added `ComputeAuthoritativeHash()` = `ComputeHash(1)`; the `sampleRate==1` path now does a clean full-buffer scan hashing every pixel once in index order (no corner/center double-count); added `sampleRate==0` guard. `rasterizer.h`: default arg changed 16→1, struct/method docs corrected to drop the false "lightweight strategic sampling" claim. Added `#include <cstring>`.
**Verdict:** approved-with-nits, high confidence. Canonicalization correct, includes present, no memory-safety bug. The hash-function half is sound; the call-site half is a cross-team follow-up (see §3).

### Networking / security

**Finding #5 + #3b — comfyClient quick wins + seed jitter** (`comfyClient.cpp`, `comfyClient.h`)
Multiple issues fixed in one pass:
- **#3b seed jitter / cache-busting:** all three builders + `_SaveControlImages` previously used `params.seed + ms%1000000` and a wall-clock cacheBuster, so identical scene+seed never reproduced or cache-hit. Now, when `_deterministic` (default true), `seed = params.seed` and the cache key is a 64-bit FNV-1a content hash (prompt+seed+steps+cfg+strengths+flags+dims+backend). Old time-jitter path survives only behind `SetDeterministic(false)`.
- `_cancelRequested`/`_wsConnected` → `std::atomic<bool>`; `_cancelRequested` reset at `ProcessFrame` entry (fixes the permanent-disable-after-first-cancel bug + data race).
- `ParsePort` via `std::from_chars` replaces `std::stoi` at all four sites (no longer throws out of the render thread on malformed/IPv6 ports; falls back to default).
- `SO_RCVTIMEO`/`SO_SNDTIMEO`=30s on all three HTTP sockets (stalled server no longer blocks forever).
- Unconditional `C:/Temp/hdcarwash_debug.txt` writes + workflow-JSON leak gated behind `TfDebug::IsEnabled(HD_CARWASH)` via drop-in `_DebugLog`.
- WS receive rewritten loop+`continue` (no more unbounded ping/pong recursion); receive/close barrier (`_wsStop`+`_wsConnected=false`, wait for `_wsReceiving`, then `closesocket`) fixes the use-after-close race.
- Dead `DecodeColorImage` (hand-rolled PNG/inflate parser, no live caller) removed; `Base64Decode` tagged `[[maybe_unused]]`.

`SetDeterministic` wiring correctly left to the render-delegate owner (default-on means safe out of the box).
**Verdict:** approved-with-nits, **medium** confidence (lowest of the five — see §3).

### Docs

**Finding #2 — docs describe a fictional codebase** (`INDEX.md`, `README.md`, `DETERMINISM_CROSS_REFERENCE.md`, `branding/CARWASH_IDENTITY.md`)
**Change:** Removed/relabeled ~20 phantom files and 6 phantom directories (`workflows/`, `python/`, `schema/`, `comfyui/`, `houdini/`, `scripts/`, `plugin/lib/`, `download_models.ps1`, pytest suites) into clearly-labeled "Planned" tables matching the real layout. Render-settings table rewritten to the actual registered tokens from `GetRenderSettingDescriptors()` (`carwash:backend`, `:deterministicMode`, `:comfyui:serverUrl`, `:inferenceSteps`, `:guidanceScale`, `:substrate:enabled`, `:substrate:historyFrames`, `:seed`) — the old `positive_prompt`/`video_length`/`frame_rate` tokens are not registered. Brand color corrected `#3399EE`→`#3399E6` (0.2/0.6/0.9 × 255 = 51/153/230, mathematically verified). Naming convention defined once (HdCarWash / hdCarWash / CarWash). `DETERMINISM_CROSS_REFERENCE.md` got a "Forward-Looking Design Intent" banner; SHA-256/sceneHash.h/frameVerifier.h/OpenSSL all annotated as PLANNED (none exist). Stale `HDCARWAASH` path corrected.
**Verdict:** approved-with-nits, high confidence. Every factual claim cross-checked against source.

---

## 2. Cross-cutting consistency

### (a) Determinism story — coherent in design, NOT yet wired end-to-end

- **Hash side (`rasterizer.cpp`):** now correctly drift-sensitive (raw IEEE-754 bits) with an authoritative full-buffer path. Sound.
- **Seed side (`comfyClient.cpp`):** seed jitter is gated behind `_deterministic` (default on), cache key is content-hashed. Sound.
- **The gap — two unwired call sites, both teammate-owned:**
  1. `renderPass.cpp:188` still calls `_framebuffer.ComputeHash(16)` — the explicitly *non-authoritative* sampled preview. The live "VERIFIED" signal is still based on ~1/256 of pixels. The hash *function* is fixed; the *call site* is not. **Needs a one-line change** to `ComputeAuthoritativeHash()` / `ComputeHash(1)`.
  2. `SetDeterministic` is never called by the render delegate. Determinism currently relies on the default `true`. The `deterministicMode` render setting is not yet wired to `SetDeterministic()`.

  Until both land, the determinism story is **coherent by construction but not provably authoritative at runtime.** This is the single most important cross-cutting item.

### (b) Deploy layout — now coherent across all three sources

| Source | Path | Agrees? |
|---|---|---|
| `plugInfo.json` `LibraryPath` | `lib/hdCarWash.dll` (relative to manifest dir) | reference |
| Root `CMakeLists.txt` install | manifest → `usd/hdCarWash/plugInfo.json` | yes |
| `plugin/hdCarWash/CMakeLists.txt` install | dll → `usd/hdCarWash/lib/hdCarWash.dll` | yes |
| `rebuild_and_install.bat` | `cmake --install --prefix …/dso` → same layout | yes |

The three-way mismatch is resolved: manifest sits beside `lib/`, and `LibraryPath` resolves to the real binary. **One residual incoherence:** `plugInfo.json` declares `ResourcePath="resources"` but no `resources/` directory is ever deployed. Low impact for a bare Hydra renderer (PlugRegistry tolerates a missing resource dir), but the build report's claim that the manifest "sits beside resources/ (matching ResourcePath)" is **overstated** — that dir does not exist.

### (c) Docs vs. real code — now match, with two parallel-edit staleness artifacts

- Render-settings tokens: match `GetRenderSettingDescriptors()` exactly. ✅
- Determinism claim: SHA-256/scene-hash/OpenSSL correctly marked PLANNED; doc now states the shipped hash is FNV-1a. ✅ (Note: the determinism reviewer still saw a residual SHA-256 reference in `DETERMINISM_CROSS_REFERENCE.md` / `README.md:20` — see §3; the docs expert says these were banner-gated. Worth a final grep.)
- **Two staleness artifacts from teammate edits landing on the same branch after the docs expert verified against HEAD:**
  1. `CARWASH_IDENTITY.md` cites `rasterizer.cpp` lines 475–480 / comment at 474; the tint moved to ~517–523 (comment ~517) after the determinism edits. The **hex value `#3399E6` is still correct** — only the line cite rotted. Recommend citing the symbol/function instead of line numbers.
  2. `DETERMINISM_CROSS_REFERENCE.md` describes the frame hash as operating over "sampled" pixels; the authoritative path now defaults to `sampleRate=1` (full coverage). The doc now *understates* the guarantee. Reword to "full-coverage FNV-1a via `ComputeAuthoritativeHash()`/`ComputeHash(1)`, with optional sampled preview."

---

## 3. Review flags (changes-needed / high / medium)

**HIGH — determinism call site not authoritative** (`renderPass.cpp:188`, teammate-owned)
The user-visible "VERIFIED" determinism signal still hashes ~1/256 of pixels. **Required follow-up:** switch line 188 to `ComputeAuthoritativeHash()` (or `ComputeHash(1)`). Do not mark #3a fully closed until this lands. *(Not a defect in the determinism expert's diff — they cannot edit that file; flagged as residual risk #1.)*

**HIGH — plugin discovery unverified** (`rebuild_and_install.bat`, deploy)
The fix is necessary but not provably sufficient: registration only occurs if `dso/usd` is on USD's plugin search path (`PXR_PLUGINPATH_NAME` / Houdini's HOUDINI_PATH-derived discovery). Standard Houdini scans the user `dso` dir, but this was **not verified in a live session**. **Required follow-up:** confirm "CarWash" actually appears in a real Houdini 21 session before closing Finding #1.

**MEDIUM — WS receive barrier is single-flight-only** (`comfyClient.cpp` ~2160, networking)
`_wsReceiving` is a single bool, not a refcount. If two `ProcessFrameAsync` calls ever reuse the kept-open WS connection concurrently, thread B's guard destructor clears the flag while thread A is still in `recv()`, and `_WebSocketDisconnect` can `closesocket()` into a live handle — reintroducing the exact use-after-close this fix targets. The "Keep connection open for future use" comment actively invites reuse. **Recommended:** make `_wsReceiving` a `std::atomic<int>` refcount (disconnect waits for `==0`), *or* document/assert single-flight `ProcessFrame`. This is the reason comfyClient's review is medium-confidence; it's a few lines.

**MEDIUM — `resources/` not deployed** (root `CMakeLists.txt`, deploy)
`ResourcePath="resources"` points at a never-deployed dir. Low real impact for a bare renderer. **Recommended:** install an (even empty) `usd/hdCarWash/resources/`, or correct the claim that it's coherent. A future resource load would break.

**LOW (worth noting):**
- WS disconnect caps the wait at ~2s then `closesocket()`s **unconditionally** even if `_wsReceiving` is still set; the "effectively unreachable" comment overstates safety (the inner payload loop does blocking-style `recv` in a `while` loop). Consider an unbounded wait or skip-close-on-timeout. (networking)
- `DeterministicCacheKey` streams float params (`guidanceScale`, control strengths) at default ~6 sig-figs; two values differing beyond 6 sig-figs collide → stale cached render. More likely than the 64-bit hash collision. **Fix:** `std::hexfloat` or fixed precision ≥9. (networking)
- `HdCarWashFrameHash::operator==` compares only the XOR-folded `combined` field, weakening collision resistance vs. comparing all four sub-hashes. Pre-existing; the stronger per-value hashing slightly highlights it. (determinism)
- `numPixels`/`dstCapacity` in `_CopyFramebufferToAOVs` ignore buffer `_depth`; safe (never overflows) but would over-strictly `TF_WARN`+skip any `depth>1` buffer. Document the 2D-AOV assumption. (hydra-aov)

---

## 4. Not done / deferred

- **`renderPass.cpp:188` → authoritative hash** — teammate-owned; one-line cross-team change (the gating item for #3a).
- **`SetDeterministic` wiring** — render-delegate owner must map `deterministicMode` → `SetDeterministic()`. Client exposes the toggle, default-on, so it's safe meanwhile.
- **`comfyClient.cpp` not added to `HDCARWASH_SOURCES`** in `plugin/hdCarWash/CMakeLists.txt` (lines 7–17) — pre-existing build-completeness gap, referenced by `renderPass.cpp` but not listed. Track separately; **this can break the actual build** and is the most concrete deferred item.
- **Dead frustum-cull block** (`rasterizer.cpp` ~354–365, empty-body `outsideNDC`) — left in place; orthogonal to determinism, removing it risks merge friction.
- **`Base64Decode`** kept (tagged `[[maybe_unused]]`) rather than deleted — out of finding scope, may be wanted for debug.
- **Defined-but-unregistered tokens** (`outputDirectory`, `outputFormat`, `comfyui:workflowPath`, `comfyui:timeoutSeconds`, `substrate:autoAnnotate`) documented as "defined but not exposed" rather than deleted.
- **No build/compile run by any expert** — no toolchain + USD/Houdini headers in the sandbox. All five fixes verified by close reading only. The hydra-aov and comfyClient fixes add new API usage (`HdGetComponentFormat`, `GfHalf`, `std::from_chars`, `std::lround`) that has **not been compiler-confirmed**.

---

## 5. Verify & commit plan

A full build needs the Houdini USD libs (`pxr`, `Hd`) — not buildable in this sandbox. Validate on the Threadripper box with Houdini 21 installed:

**Build & compile (catches the un-compiled new API usage):**
1. Add `comfyClient.cpp` to `HDCARWASH_SOURCES` in `plugin/hdCarWash/CMakeLists.txt` first — otherwise the link will be incomplete.
2. `rebuild_and_install.bat` (it configures, builds, then `cmake --install`). Watch for overload-resolution errors in the new `renderPass.cpp` helpers (`GfHalf(float)`, `std::lround`, `uint8_t` casts) and `comfyClient.cpp` (`std::from_chars`, `std::hexfloat` if you apply that nit).

**Deploy discovery (closes Finding #1 HIGH):**
3. Launch Houdini 21 / Solaris, confirm **"CarWash" appears** in the Hydra renderer list. If not, verify `dso/usd` is on `PXR_PLUGINPATH_NAME`. Confirm `usd/hdCarWash/plugInfo.json` + `usd/hdCarWash/lib/hdCarWash.dll` landed beside each other.

**Memory safety (closes #4a/#4b):**
4. Render to a viewport that allocates `UNorm8Vec4` color (standard usdview/Husk) — previously a guaranteed heap overflow, should now produce clamped 8-bit color with no corruption. Ideally run under Application Verifier / ASan-equivalent.

**Determinism (closes #3a/#3b):**
5. Apply the two cross-team one-liners (`renderPass.cpp:188` → authoritative; `SetDeterministic` wiring), then render the same scene+seed twice and confirm identical frame hashes **and** a ComfyUI cache hit. Regenerate any golden hashes — the hash scheme changed and old baselines are invalid.

**Suggested commit message:**
```
Fix top-5: AOV/Encode heap overflows, plugin deploy, determinism, comfyClient hardening

- renderPass: bound AOV writes to buffer->GetFormat()/HdDataSizeOfFormat;
  convert color to UNorm8/Float16/SNorm8 dest formats (fixes 2-4x heap overflow)
- comfyClient: clamp Encode* loops to width*height (heap overflow); atomic
  cancel/ws flags + reset; ParsePort via from_chars (no stoi throw); 30s socket
  timeouts; gate C:/Temp debug log behind HD_CARWASH; race-free WS receive/close;
  deterministic seed + FNV content cache key (default on); drop dead DecodeColorImage
- rasterizer: hash raw IEEE-754 bits (drift-sensitive) + ComputeAuthoritativeHash()
  full-buffer path; default sampleRate 16->1
- build: deploy plugInfo.json + dll coherently to usd/hdCarWash[/lib] via
  cmake --install; de-hardcode Houdini path; if(EXISTS) guards
- docs: match real layout/tokens; FNV-1a (not SHA-256); brand #3399EE->#3399E6

Follow-ups: renderPass.cpp:188 -> ComputeAuthoritativeHash(); wire
deterministicMode -> SetDeterministic(); add comfyClient.cpp to HDCARWASH_SOURCES;
verify "CarWash" appears in live Houdini 21.

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>
```