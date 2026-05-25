# CarWash — Project Index

Houdini USD/Hydra render delegate for AI-powered video generation via ComfyUI/LTX-2.

**Repository:** https://github.com/JosephOIbrahim/CarWash (private)

> This index reflects the actual repository tree. Directories and files that earlier drafts
> referenced but that do not exist (e.g. `workflows/`, `python/`, `schema/`, `comfyui/`,
> `houdini/`, `scripts/`, pytest suites, `light.*`) have been removed. Planned-but-not-yet-built
> items are called out as such and tracked in `state/plan.md`.

---

## Architecture Overview

```
Houdini (USD/Hydra) -> CarWash Delegate -> ComfyUI (HTTP + WebSocket :8188) -> LTX-2/Gemma -> Video
```

Current reality: the CPU rasterizer produces depth/normal AOVs; the ComfyUI client exists and is
now in the build, but the full AOV -> ComfyUI -> video path is still being wired up
(see `state/plan.md`, Phase C).

---

## Directory Structure (actual)

```
CarWash/
├── CLAUDE.md                   # Orchestrator instructions
├── CMakeLists.txt              # Top-level build
├── DETERMINISM_CROSS_REFERENCE.md
├── INDEX.md
├── README.md
├── rebuild_and_install.bat
├── automation/                 # Python automation (synapse_*.py, carwash_automation.py)
├── branding/
├── docs/
├── plugin/
│   ├── hdCarWash/              # Hydra render delegate (C++)
│   │   ├── api.h
│   │   ├── tokens.{cpp,h}
│   │   ├── debugCodes.{cpp,h}
│   │   ├── rendererPlugin.{cpp,h}
│   │   ├── renderDelegate.{cpp,h}
│   │   ├── renderPass.{cpp,h}
│   │   ├── renderBuffer.{cpp,h}
│   │   ├── mesh.{cpp,h}
│   │   ├── camera.{cpp,h}
│   │   ├── rasterizer.{cpp,h}
│   │   ├── comfyClient.{cpp,h} # ComfyUI HTTP/WebSocket client + workflow builder
│   │   ├── third_party/        # Vendored single-headers: stb_image.h, json.hpp (nlohmann)
│   │   └── CMakeLists.txt
│   └── plugInfo.json           # Hydra plugin registration
├── state/                      # Orchestrator state (plan, beliefs, decisions, tasks, ...)
├── test/                       # Houdini-side: test_carwash_render.py, launch_carwash_test.bat
└── tests/
    └── cpp/                    # C++ unit-test target (CTest) — scaffold, being populated
```

---

## Core Components (C++ Hydra Delegate)

| File | Purpose |
|------|---------|
| `plugin/hdCarWash/rendererPlugin.cpp` | Plugin registration (`HdCarWashRendererPlugin`) |
| `plugin/hdCarWash/renderDelegate.cpp` | `HdRenderDelegate` implementation, prim factories, render settings |
| `plugin/hdCarWash/renderPass.cpp` | Render execution; CPU rasterize path (full AI path WIP) |
| `plugin/hdCarWash/rasterizer.cpp` | CPU depth/normal/id rasterizer + frame hash (determinism) |
| `plugin/hdCarWash/comfyClient.cpp` | ComfyUI HTTP/WebSocket client, LTX-2 workflow builder |
| `plugin/hdCarWash/mesh.cpp` | USD mesh -> triangles |
| `plugin/hdCarWash/camera.cpp` | USD camera -> matrices |
| `plugin/hdCarWash/renderBuffer.cpp` | AOV buffer management |
| `plugin/hdCarWash/tokens.cpp` | USD token definitions |
| `plugin/hdCarWash/debugCodes.cpp` | `TF_DEBUG` codes |
| `plugin/hdCarWash/api.h` | Export macros |
| `plugin/hdCarWash/third_party/stb_image.h` | Vendored PNG decode (public domain, v2.30) |
| `plugin/hdCarWash/third_party/json.hpp` | Vendored JSON parser (nlohmann/json v3.11.3) |
| `plugin/plugInfo.json` | Hydra plugin manifest |

---

## Model Configuration (LTX-2)

CarWash targets the LTX-2 family with Gemma 3 12B text encoding. The exact ComfyUI node graph
is being modernized to match the current canonical Lightricks/ComfyUI-LTXVideo workflows
(`CheckpointLoaderSimple`, `LTXVGemmaCLIPModelLoader`, `LTXVImgToVideoInplace`/`ConditionOnly`,
tiled VAE decode, `CreateVideo`/`SaveVideo`). See `state/tasks/t011/findings.md` for the verified
node mapping and `state/tasks/t012/findings.md` for the depth-video conditioning contract.

---

## Build

```bash
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_PREFIX_PATH="C:/Program Files/Side Effects Software/Houdini 21.x"
cmake --build build --config Release
```

---

## Usage

1. Start ComfyUI (HTTP + WebSocket on port 8188).
2. Launch Houdini 21+, create a LOP network with geometry.
3. Add a Render Settings node, select the "CarWash" renderer.
4. Render — depth/normal AOVs are produced; the AI video path is being wired up.

---

## Orchestrator State

This repo is managed under the Orchestrator workflow in `CLAUDE.md`. Durable state lives in
`state/` — `plan.md` (task graph + EXIT_CRITERIA), `beliefs.md`, `decisions.md`,
`open_questions.md`, `parked.md`, and per-task artifacts under `state/tasks/`.
