# Agent Offload PRD Template
## How to Structure Work for AI Agent Execution

**Version**: 1.0
**Author**: Joe Ibrahim
**Purpose**: Template for decomposing tasks into agent-executable units

---

## The Core Problem

AI agents fail when:
1. **Context is insufficient** — Missing information forces guessing
2. **Success criteria are vague** — "Make it better" isn't actionable
3. **Scope is unbounded** — "Implement the feature" has no stopping point
4. **Dependencies are implicit** — Agent can't know what it doesn't know

AI agents succeed when:
1. **Context is explicit** — Everything needed is stated
2. **Success is verifiable** — Clear pass/fail criteria
3. **Scope is atomic** — One thing, done completely
4. **Dependencies are declared** — Prerequisites are listed

---

## PRD Structure for Agent Offload

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  AGENT OFFLOAD PRD STRUCTURE                                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  1. IDENTITY          Who is this for? What agent type?                     │
│  2. CONTEXT           What does the agent need to know?                     │
│  3. TASK              What exactly should be done?                          │
│  4. CONSTRAINTS       What are the boundaries?                              │
│  5. INPUTS            What files/data are provided?                         │
│  6. OUTPUTS           What artifacts should be produced?                    │
│  7. VERIFICATION      How do we know it worked?                             │
│  8. INTEGRATION       How does this connect to other work?                  │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Template

### 1. IDENTITY

```markdown
## Identity

**Task ID**: [unique identifier, e.g., CARWASH-001]
**Task Name**: [human-readable name]
**Agent Type**: [code-gen | research | review | test | docs]
**Estimated Complexity**: [trivial | simple | moderate | complex | research]
**Parent Task**: [ID of parent task, if subtask]
```

**Why this matters**: Different agent types have different capabilities. A code-gen agent shouldn't be asked to do research. A research agent shouldn't be asked to write production code.

---

### 2. CONTEXT

```markdown
## Context

### Project Context
[2-3 sentences about the project this task belongs to]

### Technical Context
[Relevant architecture decisions, patterns, conventions]

### Why This Task
[What problem does completing this task solve?]

### What Came Before
[Previous tasks that this depends on]

### What Comes After
[Tasks that depend on this one]
```

**Why this matters**: Agents have no memory between sessions. Every piece of context must be explicitly provided.

**Anti-pattern**: "Continue working on the render delegate"
**Good pattern**: "The HdCarWashRenderDelegate (plugin/hdCarWash/renderDelegate.cpp) needs a mesh implementation. The delegate currently supports camera and renderBuffer prims. Add mesh support following the HdEmbree pattern."

---

### 3. TASK

```markdown
## Task

### Objective
[One sentence describing what "done" looks like]

### Scope
[Explicit boundaries — what IS included, what is NOT included]

### Steps
1. [First concrete action]
2. [Second concrete action]
3. ...
[Maximum 7 steps — if more, decompose into subtasks]

### Decision Points
[Where the agent might need to make choices, and guidance for each]
```

**Why this matters**: Ambiguity leads to hallucination or paralysis.

**Anti-pattern**: "Implement the rasterizer"
**Good pattern**: "Implement depth buffer rasterization in rasterizer.cpp. Input: list of triangles (vertex positions). Output: depth buffer (float array, near=0, far=1). Algorithm: scanline with z-buffer. Do NOT implement: motion vectors, normals, edge detection (separate tasks)."

---

### 4. CONSTRAINTS

```markdown
## Constraints

### Must
- [Non-negotiable requirements]
- [Style/convention requirements]
- [Performance requirements]

### Must Not
- [Explicit prohibitions]
- [Anti-patterns to avoid]

### Should
- [Preferences, not requirements]

### May
- [Optional enhancements if time permits]
```

**Why this matters**: Agents will optimize for what you measure. If you don't constrain, they'll make choices you don't want.

**Example**:
```
Must:
- Follow Pixar USD coding conventions (CamelCase methods, _underscoreMembers)
- Use TF_CODING_ERROR for error handling
- Include header guards

Must Not:
- Use exceptions (USD doesn't use them)
- Allocate on hot path
- Modify files outside plugin/hdCarWash/

Should:
- Add comments explaining Hydra concepts
- Match HdEmbree implementation patterns

May:
- Add debug logging behind TF_DEBUG
```

---

### 5. INPUTS

```markdown
## Inputs

### Files to Read
| File | Purpose | What to Extract |
|------|---------|-----------------|
| [path] | [why read this] | [specific info needed] |

### Reference Files
| File | Purpose |
|------|---------|
| [path] | [pattern to follow] |

### Data
| Name | Type | Source |
|------|------|--------|
| [name] | [type] | [where it comes from] |

### Prerequisites
- [ ] [Task ID] must be complete
- [ ] [File] must exist
- [ ] [Service] must be running
```

**Why this matters**: Agents can only work with what they can see. Implicit dependencies cause failures.

---

### 6. OUTPUTS

```markdown
## Outputs

### Files to Create
| File | Purpose | Template/Example |
|------|---------|------------------|
| [path] | [what it does] | [reference file or inline template] |

### Files to Modify
| File | Changes | Lines/Sections |
|------|---------|----------------|
| [path] | [what to change] | [approximate location] |

### Artifacts
| Artifact | Format | Location |
|----------|--------|----------|
| [name] | [format] | [where to put it] |
```

**Why this matters**: Clear output expectations enable verification.

---

### 7. VERIFICATION

```markdown
## Verification

### Automated Checks
```bash
# Commands that should pass after task completion
[command 1]
[command 2]
```

### Manual Checks
- [ ] [Visual inspection criteria]
- [ ] [Behavior to verify]

### Test Cases
| Input | Expected Output |
|-------|-----------------|
| [input 1] | [output 1] |
| [input 2] | [output 2] |

### Definition of Done
- [ ] All automated checks pass
- [ ] All manual checks pass
- [ ] Code compiles without warnings
- [ ] [Additional criteria]
```

**Why this matters**: Without verification, you can't know if the agent succeeded.

---

### 8. INTEGRATION

```markdown
## Integration

### Upstream Dependencies
| Task ID | Output Used |
|---------|-------------|
| [ID] | [what we need from it] |

### Downstream Consumers
| Task ID | What They Need |
|---------|---------------|
| [ID] | [what we produce for them] |

### Integration Points
- [Where this code interfaces with other code]
- [APIs consumed or exposed]

### Rollback Plan
[How to undo if this fails]
```

---

## Example: Phase 1 Mesh Implementation

```markdown
# Agent Offload PRD

## Identity

**Task ID**: CARWASH-P1-MESH
**Task Name**: Implement HdCarWashMesh
**Agent Type**: code-gen
**Estimated Complexity**: moderate
**Parent Task**: CARWASH-P1-AOV-CORE

---

## Context

### Project Context
HdCarWash is a Hydra render delegate that generates semantic AOVs for AI-driven NPR rendering. Phase 1 focuses on implementing geometry sync and AOV generation.

### Technical Context
- Hydra delegates sync geometry via HdMesh base class
- Geometry arrives as topology (face vertex counts, indices) + primvars (positions, normals, UVs)
- Reference implementation: HdEmbree in USD source
- Convention: all CarWash classes prefixed with HdCarWash

### Why This Task
The render delegate needs to receive and process mesh geometry to generate AOVs. Without mesh support, no geometry renders.

### What Came Before
- CARWASH-P0: Basic delegate registration (complete)
- renderDelegate.cpp returns nullptr for mesh creation (needs fix)

### What Comes After
- CARWASH-P1-CAMERA: Camera projection
- CARWASH-P1-RASTER: CPU rasterization

---

## Task

### Objective
Implement HdCarWashMesh class that syncs mesh geometry from Hydra and stores it for rasterization.

### Scope
INCLUDED:
- Sync topology (face vertex counts, face vertex indices)
- Sync points primvar
- Sync normals primvar (if present)
- Sync displayColor primvar (if present)
- Store in internal representation

NOT INCLUDED:
- Instancing (separate task)
- Subdivision surfaces (future)
- Curves, points, volumes (separate tasks)

### Steps
1. Create mesh.h with HdCarWashMesh class declaration
2. Create mesh.cpp with implementation
3. Implement Sync() to receive topology + primvars
4. Implement Finalize() to prepare for rendering
5. Update renderDelegate.cpp CreateRprim() to instantiate HdCarWashMesh
6. Update CMakeLists.txt to include new files

### Decision Points
- **Topology storage**: Use VtArray (matches USD types, avoids conversion)
- **Normal handling**: If no normals provided, compute face normals in Finalize()
- **Memory**: Store one copy, reference from rasterizer (don't duplicate)

---

## Constraints

### Must
- Inherit from HdMesh base class
- Implement all pure virtual methods
- Use TF_CODING_ERROR for errors
- Follow Pixar coding conventions
- Thread-safe Sync() (may be called from multiple threads)

### Must Not
- Throw exceptions
- Allocate per-frame (use persistent storage)
- Modify topology after Finalize()

### Should
- Match HdEmbree mesh implementation patterns
- Add TF_DEBUG logging for sync operations

### May
- Support UV primvar (needed for texture AOV later)

---

## Inputs

### Files to Read
| File | Purpose | What to Extract |
|------|---------|-----------------|
| plugin/hdCarWash/renderDelegate.cpp | Integration point | CreateRprim() signature |
| $USD/pxr/imaging/plugin/hdEmbree/mesh.h | Reference | Class structure |
| $USD/pxr/imaging/plugin/hdEmbree/mesh.cpp | Reference | Sync implementation |

### Reference Files
| File | Purpose |
|------|---------|
| pxr/imaging/hd/mesh.h | Base class interface |
| pxr/imaging/hd/meshTopology.h | Topology types |

### Prerequisites
- [x] CARWASH-P0 complete
- [x] renderDelegate.cpp exists
- [x] CMakeLists.txt exists

---

## Outputs

### Files to Create
| File | Purpose |
|------|---------|
| plugin/hdCarWash/mesh.h | Class declaration |
| plugin/hdCarWash/mesh.cpp | Implementation |

### Files to Modify
| File | Changes |
|------|---------|
| plugin/hdCarWash/renderDelegate.cpp | CreateRprim() returns HdCarWashMesh |
| plugin/hdCarWash/CMakeLists.txt | Add mesh.cpp to sources |

---

## Verification

### Automated Checks
```bash
# Build should succeed
cmake --build build --config Release

# No new warnings
cmake --build build 2>&1 | grep -i warning
```

### Manual Checks
- [ ] Load simple USD with one mesh in Solaris
- [ ] Select CarWash renderer
- [ ] No crash when rendering
- [ ] TF_DEBUG shows mesh sync messages

### Definition of Done
- [ ] mesh.h and mesh.cpp created
- [ ] renderDelegate.cpp creates mesh
- [ ] Compiles without warnings
- [ ] Simple mesh syncs without crash

---

## Integration

### Upstream Dependencies
| Task ID | Output Used |
|---------|-------------|
| CARWASH-P0 | renderDelegate.cpp |

### Downstream Consumers
| Task ID | What They Need |
|---------|---------------|
| CARWASH-P1-RASTER | Mesh topology + primvars |

### Integration Points
- renderDelegate.cpp CreateRprim() instantiates mesh
- Rasterizer will call mesh.GetTopology(), mesh.GetPoints()

### Rollback Plan
- Delete mesh.h, mesh.cpp
- Revert renderDelegate.cpp CreateRprim() to return nullptr
```

---

## Task Decomposition Guidelines

### When to Split

Split a task if:
- More than 7 steps
- Multiple output files in different areas
- Multiple decision points with different expertise needed
- Estimated complexity > moderate

### Atomic Task Characteristics

A well-formed atomic task:
- Has ONE clear objective
- Produces 1-3 related output files
- Can be verified independently
- Takes 15-60 minutes of agent time
- Has no ambiguous decision points

### Dependency DAG

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  TASK DEPENDENCY VISUALIZATION                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  CARWASH-P0 (Foundation)                                                    │
│       │                                                                     │
│       ├──► CARWASH-P1-MESH ──┐                                             │
│       │                      │                                              │
│       ├──► CARWASH-P1-CAMERA ┼──► CARWASH-P1-RASTER ──► CARWASH-P1-AOV     │
│       │                      │                                              │
│       └──► CARWASH-P1-TOKENS ┘                                             │
│                                                                              │
│  Parallel: MESH, CAMERA, TOKENS can run concurrently                       │
│  Serial: RASTER depends on all three                                        │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Agent Handoff Checklist

Before offloading to an agent:

- [ ] Task ID assigned
- [ ] Context section complete (project, technical, why)
- [ ] Objective is one sentence
- [ ] Scope has explicit INCLUDED and NOT INCLUDED
- [ ] Steps are numbered and concrete
- [ ] Constraints have Must/Must Not
- [ ] All input files listed with purpose
- [ ] All output files listed
- [ ] Verification has automated checks
- [ ] Definition of Done is checkboxes
- [ ] Dependencies documented

---

## Meta: Why This Works

This template works because it treats AI agents as **very capable but contextless collaborators**.

They can:
- Write excellent code given clear specifications
- Follow patterns from reference implementations
- Make reasonable decisions within stated constraints

They cannot:
- Guess your preferences
- Know your project history
- Infer implicit requirements
- Ask clarifying questions mid-task

The PRD format front-loads all the context that a human collaborator would gather through conversation, questions, and institutional knowledge.

**The more explicit the PRD, the more autonomous the agent.**

---

*Template version 1.0 | January 2026*
