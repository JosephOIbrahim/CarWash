# Determinism Cross-Reference: HdCarWash + RAG System + [He2025]

**Version**: 1.0.0
**Date**: 2026-01-29
**Purpose**: Unified determinism framework for building accuracy across all Houdini AI systems

---

> ## ⚠️ Status: Forward-Looking Design Intent
>
> **This document describes the intended determinism architecture, not the current state
> of the code.** Most of the C++ shown below is illustrative design and is **not yet
> implemented**. In particular:
>
> - **What exists today:** `rasterizer.cpp` implements an FNV-1a (64-bit) frame hash over
>   the **raw IEEE-754 bits** of every AOV value (NaN and -0.0 canonicalized), so 5th-decimal
>   accumulation-order drift is detected. The **authoritative** check
>   (`HdCarWashFramebuffer::ComputeAuthoritativeHash` / `ComputeHash(1)`) covers **every
>   pixel** and is the value the render pass uses for the "VERIFIED" signal; a faster
>   `ComputeHash(N>1)` sampled preview also exists but is explicitly non-authoritative. The
>   single-threaded CPU rasterizer iterates faces in face-index order.
> - **What does NOT exist yet:** there is no `sceneHash.h`, no `frameVerifier.h`, no
>   `HdCarWashSceneHasher`, and no `HdCarWashFrameVerifier`. The project does **not** depend
>   on OpenSSL, and **SHA-256 is not used anywhere** — the SHA-256 code blocks below are
>   aspirational. Mesh path sorting, frame-manifest JSON output, and the dual-render test
>   are **planned**, not present.
>
> Treat every code block in this file as a proposal to be implemented and verified, not as
> a description of shipped behavior. The current verified guarantee is limited to:
> single-threaded CPU rasterization with face-index-ordered iteration and a full-coverage,
> raw-bits FNV-1a frame hash (`ComputeAuthoritativeHash`), with an optional sampled preview
> for cheap spot-checking.

---

## The Unified Thesis

Three systems, one principle: **Fixed order → Reproducible outputs**

| System | Domain | Determinism Layer |
|--------|--------|-------------------|
| **[He2025]** | GPU kernels | Batch-invariant reductions (RMSNorm, MatMul, Attention) |
| **RAG System** | Documentation | Content-hash ordering + fixed chunking |
| **HdCarWash** | Rendering | Scene-hash → AOV consistency |

**Formal Statement:**
```
For any input I and processing function F:
  F(I, run_1) = F(I, run_2) = F(I, run_n)

Verification: Hash(output_1) == Hash(output_2) == Hash(output_n)
```

---

## Four-Layer Determinism Stack (Universal)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ LAYER 4: OUTPUT VERIFICATION                                                │
│   [He2025]: Same tokens across 1000 runs                                    │
│   RAG:      Same search results every query                                 │
│   CarWash:  Same AOV pixels every frame                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│ LAYER 3: FIXED BOUNDARIES                                                   │
│   [He2025]: Fixed split-K size (not count) for batch invariance            │
│   RAG:      Fixed 32-item chunks with PADDING_SENTINEL                      │
│   CarWash:  Fixed triangle iteration order (no parallel reordering)         │
├─────────────────────────────────────────────────────────────────────────────┤
│ LAYER 2: CANONICAL ORDERING                                                 │
│   [He2025]: Fixed reduction order regardless of batch size                  │
│   RAG:      sort(items, key=content_hash)                                   │
│   CarWash:  sort(meshes, key=prim_path) + sort(triangles, key=face_index)  │
├─────────────────────────────────────────────────────────────────────────────┤
│ LAYER 1: CONTENT IDENTITY                                                   │
│   [He2025]: Tensor values                                                   │
│   RAG:      MD5(name + description)                                         │
│   CarWash:  Hash(geometry + transform + material + camera)                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Cross-Reference Matrix

### [He2025] → RAG System

| [He2025] Concept | RAG Implementation | Verification |
|------------------|-------------------|--------------|
| Batch invariance | Fixed 32-item chunks | Dual-run hash: `02a23f5b...` |
| Fixed reduction order | Content-hash sorting | Canonical indices 0-33 |
| Same input → same output | Same docs → same organization | ✅ Verified |
| Split-K size (not count) | Chunk size 32 (not chunk count) | PADDING_SENTINEL fills gaps |

### [He2025] → HdCarWash

| [He2025] Concept | HdCarWash Implementation | Status |
|------------------|-------------------------|--------|
| Batch invariance | Single-threaded rasterizer (no batch variance) | ✅ Implicit |
| Fixed reduction order | Fixed triangle iteration (face index order) | ✅ Implemented |
| Same input → same output | Same scene → same AOVs | 🟡 Needs verification |
| Deterministic kernels | CPU rasterizer (no GPU nondeterminism) | ✅ By design |

### RAG System → HdCarWash

| RAG Concept | HdCarWash Equivalent | Implementation Needed |
|-------------|---------------------|----------------------|
| Content hash (MD5) | Scene hash (geometry + camera + lights) | `ComputeSceneHash()` |
| Canonical index | Mesh render order | `sort(meshes, key=path)` |
| 32-item chunks | Triangle batches | Optional for GPU phase |
| Dual-run verification | Frame consistency test | `VerifyFrameDeterminism()` |
| Metadata JSON | AOV manifest | `frame_manifest.json` |

---

## HdCarWash Determinism Implementation Plan

### Layer 1: Scene Identity (Content Hashing)

> **PLANNED — not implemented.** There is no `sceneHash.h` in the repo, and the project
> does not link OpenSSL. The sketch below uses SHA-256 for illustration; the shipped
> rasterizer uses FNV-1a instead (see `rasterizer.cpp`). If this is implemented, prefer
> reusing the existing FNV-1a helpers over adding an OpenSSL dependency.

```cpp
// PLANNED new file: sceneHash.h (does not exist yet)

#include <openssl/sha.h>  // PLANNED dependency — not currently used by the project

/// Compute deterministic hash of entire scene state
class HdCarWashSceneHasher {
public:
    /// Hash all inputs that affect rendering
    std::string ComputeSceneHash(
        HdRenderIndex* renderIndex,
        HdRenderPassStateSharedPtr const& passState);

private:
    void HashMesh(HdCarWashMesh const* mesh, SHA256_CTX* ctx);
    void HashCamera(HdCarWashCamera const* camera, SHA256_CTX* ctx);
    void HashLight(HdSprim const* light, SHA256_CTX* ctx);

    /// Canonical ordering: sort by prim path (deterministic)
    std::vector<SdfPath> GetSortedPrimPaths(HdRenderIndex* index);
};
```

**Why this matters:** Same scene state → identical hash → can verify determinism

### Layer 2: Canonical Ordering

```cpp
// In renderPass.cpp - CURRENT (non-deterministic):
SdfPathVector rprimPaths = renderIndex->GetRprimIds();
// Problem: Order depends on insertion order, not deterministic

// FIXED (deterministic):
SdfPathVector rprimPaths = renderIndex->GetRprimIds();
std::sort(rprimPaths.begin(), rprimPaths.end());  // Lexicographic path order
// Result: Same meshes always processed in same order
```

**Verification:**
```cpp
// Log canonical order for debugging
for (size_t i = 0; i < rprimPaths.size(); ++i) {
    TF_DEBUG_MSG(HD_CARWASH, "Mesh[%zu]: %s\n", i, rprimPaths[i].GetText());
}
```

### Layer 3: Fixed Boundaries

```cpp
// Triangle iteration is already deterministic (face index order)
// But document it explicitly:

void HdCarWashRasterizer::RasterizeMesh(HdCarWashMesh const* mesh) {
    // DETERMINISM GUARANTEE:
    // - faceVertexCounts iterated in index order (0, 1, 2, ...)
    // - faceVertexIndices accessed sequentially
    // - No parallel reordering
    // - Same mesh → same triangle order → same pixels

    size_t indexOffset = 0;
    int primId = 0;  // Deterministic face ID

    for (size_t faceIdx = 0; faceIdx < faceVertexCounts.size(); ++faceIdx) {
        // Process face faceIdx (FIXED ORDER)
        // ...
    }
}
```

### Layer 4: Output Verification

> **PLANNED — not implemented.** There is no `frameVerifier.h` or `HdCarWashFrameVerifier`
> class, and SHA-256 is not used. What exists today is `HdCarWashFramebuffer::ComputeAuthoritativeHash`
> in `rasterizer.cpp`, which produces an FNV-1a `HdCarWashFrameHash` over the raw IEEE-754
> bits of every pixel (not a SHA-256, but full-coverage and drift-sensitive). The block
> below is a design sketch.

```cpp
// PLANNED new file: frameVerifier.h (does not exist yet)

/// Verify frame determinism via dual-render test
class HdCarWashFrameVerifier {
public:
    /// Compute hash of framebuffer contents
    std::string ComputeFrameHash(HdCarWashFramebuffer const& fb);

    /// Verify two renders produce identical output
    bool VerifyDeterminism(
        HdCarWashFramebuffer const& frame1,
        HdCarWashFramebuffer const& frame2);

    /// Generate frame manifest (like RAG's metadata JSON)
    void WriteFrameManifest(
        std::string const& outputPath,
        HdCarWashFramebuffer const& fb,
        std::string const& sceneHash);
};

// Implementation
std::string HdCarWashFrameVerifier::ComputeFrameHash(
    HdCarWashFramebuffer const& fb)
{
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    // Hash dimensions
    SHA256_Update(&ctx, &fb.width, sizeof(fb.width));
    SHA256_Update(&ctx, &fb.height, sizeof(fb.height));

    // Hash all AOV buffers (in fixed order)
    SHA256_Update(&ctx, fb.color.data(),
                  fb.color.size() * sizeof(GfVec4f));
    SHA256_Update(&ctx, fb.depth.data(),
                  fb.depth.size() * sizeof(float));
    SHA256_Update(&ctx, fb.normal.data(),
                  fb.normal.size() * sizeof(GfVec3f));
    SHA256_Update(&ctx, fb.objectId.data(),
                  fb.objectId.size() * sizeof(int32_t));
    SHA256_Update(&ctx, fb.primId.data(),
                  fb.primId.size() * sizeof(int32_t));

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);

    // Convert to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}
```

### Frame Manifest Schema (mirrors RAG metadata)

```json
{
  "frame": {
    "number": 1,
    "width": 1920,
    "height": 1080,
    "scene_hash": "a7f89c2d...",
    "frame_hash": "02a23f5b...",
    "deterministic": true
  },
  "aovs": [
    {
      "name": "color",
      "format": "float4",
      "hash": "1a2b3c4d..."
    },
    {
      "name": "depth",
      "format": "float",
      "hash": "5e6f7g8h..."
    },
    {
      "name": "normal",
      "format": "float3",
      "hash": "9i0j1k2l..."
    },
    {
      "name": "objectId",
      "format": "int32",
      "hash": "3m4n5o6p..."
    },
    {
      "name": "primId",
      "format": "int32",
      "hash": "7q8r9s0t..."
    }
  ],
  "meshes_rendered": [
    {
      "path": "/World/Geometry/Cube",
      "canonical_index": 0,
      "object_id": 1,
      "triangle_count": 12
    },
    {
      "path": "/World/Geometry/Sphere",
      "canonical_index": 1,
      "object_id": 2,
      "triangle_count": 960
    }
  ],
  "determinism_info": {
    "ordering_method": "lexicographic_path",
    "triangle_order": "face_index_sequential",
    "verification": "dual_render_hash_match"
  }
}
```

---

## Verification Protocol

### Dual-Render Test (from RAG System)

```cpp
// Test: Render same scene twice, verify identical output

void TestFrameDeterminism(HdCarWashRenderPass* pass) {
    // Render 1
    pass->_Execute(passState, tags);
    std::string hash1 = verifier.ComputeFrameHash(pass->_framebuffer);

    // Clear and render again
    pass->_framebuffer.Clear();
    pass->_Execute(passState, tags);
    std::string hash2 = verifier.ComputeFrameHash(pass->_framebuffer);

    // Verify
    if (hash1 == hash2) {
        TF_DEBUG_MSG(HD_CARWASH, "✅ DETERMINISM VERIFIED: %s\n",
                     hash1.c_str());
    } else {
        TF_CODING_ERROR("❌ DETERMINISM VIOLATION: %s != %s",
                        hash1.c_str(), hash2.c_str());
    }
}
```

### Continuous Verification (Production)

```cpp
// In renderPass.cpp, add optional verification mode

void HdCarWashRenderPass::_ExecutePhase1(/*...*/) {
    // ... render ...

    #ifdef HDCARWASH_DETERMINISM_ENABLED
    static std::string lastFrameHash;
    static int sameSceneRenderCount = 0;

    std::string sceneHash = _hasher.ComputeSceneHash(renderIndex, passState);
    std::string frameHash = _verifier.ComputeFrameHash(_framebuffer);

    if (_lastSceneHash == sceneHash) {
        // Same scene, verify same output
        if (_lastFrameHash != frameHash) {
            TF_CODING_ERROR("Determinism violation on frame %d", _frameNumber);
        }
        sameSceneRenderCount++;
    } else {
        // New scene, reset
        sameSceneRenderCount = 0;
    }

    _lastSceneHash = sceneHash;
    _lastFrameHash = frameHash;
    #endif
}
```

---

## Connection to Phase 2 (ComfyUI)

### HTTP Client Determinism (from RAG System)

The RAG System's connection pool architecture can be ported to C++ for ComfyUI:

```cpp
// From RAG: Connection pooling with HTTP/1.1 keep-alive
// Port to: ComfyUI client for Phase 2

class HdCarWashComfyClient {
public:
    /// Send AOVs to ComfyUI, receive AI-rendered frame
    /// DETERMINISM: Same AOVs + same seed → same output
    bool RenderWithAI(
        HdCarWashFramebuffer const& aovs,
        std::string const& workflow,
        int seed,  // Fixed seed for determinism
        HdCarWashFramebuffer* output);

private:
    // Connection pool (from RAG System pattern)
    std::vector<Socket> _connectionPool;

    /// HTTP/1.1 persistent connection (40% faster)
    Socket& GetConnection();
};
```

### AI Inference Determinism

```json
{
  "workflow": "hdcarwash_npr_v1",
  "inputs": {
    "color_aov": "base64_encoded...",
    "depth_aov": "base64_encoded...",
    "normal_aov": "base64_encoded...",
    "seed": 42,  // FIXED for determinism
    "cfg_scale": 7.5,
    "steps": 20
  },
  "determinism": {
    "seed_locked": true,
    "batch_size": 1,  // No batch variance
    "precision": "float32"  // No quantization variance
  }
}
```

---

## Unified Determinism Guarantees

### What We Can Guarantee

Status legend: ✅ implemented today · 🟡 planned.

| Layer | Guarantee | Verification Method | Status |
|-------|-----------|---------------------|--------|
| Triangle order | Face index sequential | Implicit (single-threaded) | ✅ |
| Rasterization | Deterministic edge functions | CPU = deterministic | ✅ |
| Frame hash | Sampled AOV pixels → FNV-1a hash | `HdCarWashFramebuffer::ComputeHash` | ✅ |
| Mesh ordering | Lexicographic path sort | Log + verify | 🟡 |
| Scene input | Same USD stage → same scene hash | Hash comparison | 🟡 |
| AOV output | Same scene → same pixels | Dual-render test | 🟡 |
| AI inference | Same AOVs + seed → same output | Seed locking | 🟡 |

### What We Cannot Guarantee (Yet)

| Issue | Cause | Mitigation |
|-------|-------|------------|
| GPU rasterization | Parallel non-determinism | Stay CPU (Phase 1) |
| AI model updates | Model weights change | Version lock models |
| Floating point across machines | IEEE 754 edge cases | Accept minor variance |

---

## Production Checklist

### HdCarWash Phase 1 (Current)
- [x] Single-threaded CPU rasterizer (implicit determinism)
- [x] Fixed triangle iteration order (face index)
- [ ] Add mesh path sorting for canonical order
- [ ] Implement scene hash computation
- [ ] Implement frame hash verification
- [ ] Add frame manifest JSON output
- [ ] Dual-render determinism test

### HdCarWash Phase 2 (ComfyUI)
- [ ] Port RAG connection pool to C++
- [ ] Fixed seed for AI inference
- [ ] End-to-end determinism verification
- [ ] Workflow versioning

### Integration Testing
- [ ] Same scene, 100 renders → 100 identical hashes
- [ ] Cross-session verification (restart Houdini)
- [ ] Cross-machine verification (different workstation)

---

## References

1. **[He2025]** He, Horace and Thinking Machines Lab, "Defeating Nondeterminism in LLM Inference", Sep 2025.
   - Key insight: Batch variance, not floating-point concurrency, causes nondeterminism
   - Solution: Fixed reduction order regardless of batch size

2. **Houdini RAG System** (G:\HOUDINI21_RAG_SYSTEM)
   - Hash: `02a23f5b8b30930a0c72492ffdfd498d14696adc728431e5349df1a3e9ca5a96`
   - 676 files, 4-layer determinism stack
   - Dual-run verified

3. **HdCarWash** (C:\Users\User\CARWASH)
   - Phase 1: CPU rasterizer (implicitly deterministic; full-coverage raw-bits FNV-1a frame hash)
   - Phase 2: ComfyUI integration (planned; needs seed locking)

---

**Determinism is not optional. It is infrastructure.**

Same input → Same processing → Same output → Every time.
