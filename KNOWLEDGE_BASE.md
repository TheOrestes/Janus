# Janus — Renderer Knowledge Base

**Project:** **Janus** — a bindless graphics renderer with two faces: D3D12 and Vulkan.
**Author:** Rupesh (rupeshm.dev@gmail.com)
**Written:** 2026-08-22
**Source material:** `D:\Development\UltimateEngine` (branch `ML`) — the previous engine, being retired.
**Purpose:** Seed document for the from-scratch rewrite.

> **On the name.** Janus is the two-faced Roman god of doorways, transitions, and beginnings — one head, two faces looking in opposite directions. That is precisely the architecture: one renderer, one RHI, two backends. He is also the god of *beginnings*, which suits a deliberate restart. If the design ever drifts far enough that the name stops describing it, treat that as a signal something has gone wrong.

---

## 0. How to use this document (instructions for the assistant)

If I have pasted this document at the start of a session, I am rewriting my renderer from scratch and I want you to act as a **teacher and pair-programmer**, not a code generator.

**Ground rules:**

1. **One step at a time.** Work through the roadmap in §8. Do not jump ahead, do not scaffold five milestones at once, do not produce a 2000-line dump. Finish a milestone, verify it runs, then stop and let me absorb it.
2. **Explain the why before the what.** I am deliberately relearning this material. When you introduce a concept (a barrier, a fence, a descriptor), explain the hardware/driver reason it exists before showing the API call. If you skip the reasoning, you have failed the task.
3. **Check my understanding.** At the end of each milestone, ask me one or two of the self-check questions from §9 relevant to what we just built. If I answer wrong or vaguely, re-explain rather than moving on.
4. **Do not "helpfully" abstract early.** §8 Phase B is deliberately raw D3D12 with no RHI layer. Resist the urge to introduce interfaces before Phase C. Premature abstraction is exactly how the previous engine went wrong.
5. **Flag when I am about to repeat a mistake from §6.** Those are catalogued for a reason.
6. **Assume rust, not ignorance.** I have written a D3D12 renderer before. I do not need "a GPU is a parallel processor." I *do* need the details of why the previous fence pattern was subtly wrong.
7. **Ask before deciding anything in §10.** Those are open decisions that are mine to make.

---

## 1. Context — who I am and what I want

- I built a working D3D12 renderer over ~2 years (see §3). It renders textured cubes and an Assimp-loaded FBX character with a fly camera, using SM 6.6 bindless.
- **I have gone rusty.** I don't remember many of my own design choices or the concepts behind them.
- I considered refactoring the old engine into a dual-backend renderer. **I decided against it.** The advice was to refactor in phases; I chose a rewrite anyway, deliberately, because *the point is the relearning*, not the shortest path to a running renderer. A rewrite forces me back through every concept. That is the goal, not a cost.
- I am fine building D3D12 substantially first, then mapping functionality to Vulkan. I do **not** want strict feature-by-feature or commit-by-commit parity between the two. Rough functional parity at milestone boundaries is enough.
- **Bindless on both backends is a hard requirement**, not a stretch goal.

### Goals

- Deep, durable understanding of modern explicit GPU APIs.
- **Janus**: a generic renderer with a clean backend seam — D3D12 and Vulkan, both bindless.
- Code I can still read and reason about in a year.

### Non-goals

- Shipping a game.
- Cross-platform beyond Windows (Vulkan is for API understanding and portability of *design*, not for Linux support — though I shouldn't gratuitously prevent it).
- Beating any engine on performance.
- Preserving anything from the old codebase verbatim.

---

## 2. Quick summary for a fresh session

> I'm building **Janus**, a graphics renderer, from scratch. My previous attempt was D3D12-only with SM 6.6 bindless; it worked, but the abstraction seam was in the wrong place (scene objects recorded their own draw calls). Janus targets D3D12 + Vulkan behind an RHI, bindless on both. I'm rusty and want to relearn properly — teach me one milestone at a time, explain the why, and don't abstract before Phase C.

---

## 3. What the old engine was (for reference)

### Layer stack

```
Game.exe  main()
  └─ EngineApplication          GLFW window, input callbacks, frame loop
      └─ DirectXApplication     D3D lifetime owner, input forwarding
          └─ D3DRenderer        RTV/DSV, root signature, PSO, barriers
              └─ Scene          singleton, hardcoded contents
                  └─ GameObject  transform base
                      ├─ D3DCube  shared static VB/IB + per-instance texture
                      └─ D3DMesh  per-instance VB/IB from Assimp
              └─ Camera         singleton, view/proj, input state
```

Cross-cutting namespaces (all in `D3DGlobals.h/.cpp`):

| Namespace | Contents |
|---|---|
| `UT::D3D12::CORE` | device, factory, queue, swapchain, global descriptor heap, per-frame allocators/lists/fences, `BeginFrame`/`EndFrame` |
| `UT::D3D12::DAS` | `VertexPC`, `VertexPNBT`, `DrawConstants`, `TransformData`, global mapped transform pointer |
| `UT::D3D12::HELPER` | buffer/texture creation, DXC compile, root signature, PSO, SRV registration |
| `UT::GLOBALS` | window size, frame id, `GFramesInFlight=3`, `GBindlessHeapSize=4096`, `GNextDescriptorSlot`, `InputAction`, path helpers |

### Build topology

- `Engine` → DLL (`UT_BUILD_DLL`, `UT_API` dllexport/import), `Game` → EXE. C++17, MSVC v143, x64, VS solution.
- Post-build `xcopy` of `Engine.dll` + `Game/Assets` next to the exe; assets resolved at runtime against `GetExecutableFolderPath()`.

### Dependencies used

GLFW (submodule, `GLFW_NO_API` + `glfw3native` for HWND), spdlog (submodule), Assimp (prebuilt lib), stb_image (vendored header), DXC (`dxcompiler.lib`, `dxguid.lib`).

### File inventory worth re-reading before starting

```
Engine/src/Core/EngineApplication.cpp     window + input + frame loop      (good)
Engine/src/Core/Logger.h                  spdlog singleton                 (good)
Engine/src/EngineHeader.h                 LOG_* / UT_CHECK_* / UT_ASSERT_* (good)
Engine/src/D3D12Renderer/D3DGlobals.cpp   device layer, DXC compile+sign   (mixed, read §11)
Engine/src/D3D12Renderer/D3DRenderer.cpp  bindless root sig + PSO          (read CreatePSO)
Engine/src/D3D12Renderer/StaticMesh/ModelLoader.cpp   Assimp walk          (good, reusable)
Engine/src/D3D12Renderer/World/Camera.cpp velocity + damping camera        (good)
Game/Assets/Shaders/CubeTextured.hlsl     SM 6.6 bindless VS/PS            (good, tiny)
```

---

## 4. Concepts the old engine taught me (things I should already half-know)

These are the things I need to *recover*, not learn cold:

- **Frames in flight** — CPU runs ahead of GPU by N frames; each frame slot needs its own command allocator, and you must not reset an allocator the GPU is still reading.
- **Fences** — CPU/GPU sync primitive; monotonic counter, signal from queue, wait on CPU via event.
- **Resource states / barriers** — `PRESENT ↔ RENDER_TARGET`, `COPY_DEST → SHADER_RESOURCE`. Transitions are explicit and the debug layer is unforgiving.
- **Heap types** — DEFAULT (GPU-local, fast, needs staging), UPLOAD (CPU-writable, GPU-readable, slower), READBACK.
- **Staging upload pattern** — map upload buffer → memcpy → `CopyBufferRegion`/`CopyTextureRegion` → barrier → execute → wait.
- **Descriptor heaps** — shader-visible vs not; RTV/DSV heaps are CPU-only.
- **Bindless / dynamic resources** — one big descriptor array + integer indices in root/push constants. `ResourceDescriptorHeap[i]` in SM 6.6.
- **Root signature / root constants** — the tiny, fast per-draw data channel.
- **PSO** — pipeline state is monolithic and baked; changing anything means a new PSO.
- **DXC and DXIL signing** — SM 6.x bytecode must be *validated and signed* before the runtime accepts it (see §11.1).
- **Row-major vs column-major, LH vs RH, `mul(v, M)` vs `mul(M, v)`** — a convention choice that must be applied consistently CPU-side and shader-side.
- **DirectXMath load/store discipline** — `XMFLOAT*` for storage, `XMVECTOR`/`XMMATRIX` for computation.

---

## 5. Good decisions from the old engine — carry these forward

Keep these, and keep the *reasons*:

1. **Bindless via `{ albedoIndex, transformIndex }` root constants.**
   `struct DrawConstants { uint32 albedoID; uint32 transformID; uint32 pad0, pad1; }`
   This is the single best decision in the old codebase. It is *already* the API-agnostic model — the same 16 bytes work as D3D12 root constants and Vulkan push constants, and the same shader logic works against `ResourceDescriptorHeap[i]` and `textures[nonuniformEXT(i)]`. Building around bindless sidesteps the hardest part of a dual-backend abstraction: D3D12 descriptor tables and Vulkan descriptor sets do **not** map onto each other cleanly, and any RHI that tries to abstract them generically becomes a nightmare. Bindless makes the problem disappear.

2. **Engine (DLL) / Game (EXE) split with `main()` in the game.**
   Forces the engine to be a library with an actual public surface, rather than a program with a `Render()` hook. Keep it.

3. **Semantic input actions at the platform boundary.**
   GLFW keycodes are translated once in the window callback into `InputAction::FORWARD` etc. and never appear again. Nothing downstream knows GLFW exists.

4. **`ModelLoader` fully decoupled from the graphics API.**
   Takes a path, returns `vector<Vertex>` + `vector<Index>`. Zero API contact. It is 100% portable and was the only file that needed no thought in the portability analysis.

5. **Camera as a physical system.**
   Desired velocity from yaw/pitch basis → `lerp` toward it (accel) → exponential damping when input stops (decel) → integrate by `dt`. Plus pitch clamped to 0.95·π/2 and a first-click guard so the view doesn't snap when the mouse button goes down. That guard exists because of a real bug I fixed once — don't lose it.

6. **Logging + assertion vocabulary.**
   spdlog singleton behind `LOG_*` macros, plus `UT_CHECK_HRESULT` / `UT_ASSERT_HRESULT` / `UT_ASSERT_NULL`, and `UT_NAME_D3D_OBJECT` setting D3D debug names on every resource. Named resources make debug-layer output and PIX/RenderDoc captures dramatically more readable. Do this from day one in *both* backends (`ID3D12Object::SetName` / `VK_EXT_debug_utils` object names).

7. **Shared static geometry for identical meshes.**
   All cubes shared one VB/IB; only texture + transform index were per-instance. Right instinct.

8. **Runtime shader compilation.**
   Edit-and-rerun beats a build step while learning. Keep it, and consider adding hot-reload early — it pays for itself.

9. **Assets copied next to the exe, resolved via `GetExecutableFolderPath()`.**
   Simple and it worked. No CWD dependence.

---

## 6. Flaws and mistakes — do not repeat these

Catalogued with root causes. The assistant should flag me if I start heading toward any of these.

### 6.1 Structural: scene objects recorded their own draw calls — **the big one**

`D3DCube::Render()` and `D3DMesh::Render()` did this:

```cpp
pCommandList->SetGraphicsRoot32BitConstants(0, 4, &dc, 0);
pCommandList->IASetVertexBuffers(0, 1, &m_VBView);
pCommandList->IASetIndexBuffer(&m_IBView);
pCommandList->DrawIndexedInstanced(m_uiIndexCount, 1, 0, 0, 0);
```

**Why it's wrong:** it puts the abstraction boundary in the wrong place. Every scene object becomes a D3D12 object. There is no seam to slide a second backend into, and no place to sort, batch, cull, or instance — because by the time anything could look at the draws, they've already been recorded.

**The fix, from day one:** objects *emit data*, the renderer *submits*.

```cpp
// World layer — no GPU headers anywhere near this
struct DrawItem {
    MeshHandle mesh;
    uint32_t   albedoIndex;
    uint32_t   transformIndex;
};
// GameObject::CollectDraws(std::vector<DrawItem>& out) const;
```

This is worth doing even in a single-backend renderer. It's what unlocks sorting, instancing, culling, and eventually GPU-driven rendering via `ExecuteIndirect` / `vkCmdDrawIndirect`.

### 6.2 Structural: the graphics header leaked everywhere

`Scene.h` and `Camera.h` both `#include "D3D12Renderer/D3DGlobals.h"`, which pulls in `d3d12.h`, `dxgi1_6.h`, `dxcapi.h`. Partly because `UT::GLOBALS` — window size, frame id, `InputAction` — lived *inside* the D3D12 header.

**Root cause:** one header became the dumping ground for anything global.

**The fix:** hard rule, enforced mechanically (grep in a pre-commit hook if needed) —
> **No file outside `RHI/D3D12/` or `RHI/Vulkan/` may include a graphics API header.**

Core/platform globals live in `Core/`, never in a backend header.

### 6.3 Correctness: dynamic buffer with no per-frame versioning — **race**

The transform buffer was a single persistently-mapped upload buffer, 256 rows, written every frame by `GameObject::UpdateConstantBuffer()`. With 3 frames in flight, the CPU overwrites rows the GPU may still be reading from frames N-1 and N-2. It *appeared* to work because the data changes slowly and the artifacts are subtle.

**The fix:** see §11.2. Any CPU-written-per-frame buffer needs N copies or N regions, indexed by frame slot. Bake this into the design; do not bolt it on.

### 6.4 Correctness: resource/view format mismatch

Texture resource created as `DXGI_FORMAT_R8G8B8A8_UINT`, SRV created as `DXGI_FORMAT_R8G8B8A8_UNORM`. Those are not compatible. `UINT` also means the sampler returns integers, not the 0..1 floats the shader expects.

**Root cause:** the resource and its view were created in two different functions with no shared source of truth.

**The fix:** one place owns the format. A texture creation path should return the format alongside the handle, and view creation should use it, never a hardcoded literal.

### 6.5 Static geometry left in upload heaps

`CreateGPUBuffer` (DEFAULT heap) existed but was only used by commented-out code. All actual VB/IB — cubes and meshes — lived in UPLOAD heaps. Correct, but it means static geometry is read over PCIe every frame instead of from VRAM.

**The fix:** static geometry goes to DEFAULT via a staging copy. Build the staging path properly in Phase B and use it.

### 6.6 Window resize never implemented

`CleanupOnWindowResize()` and `RecreateOnWindowResize()` logged a message and did nothing. Swapchain, depth buffers, and viewport were never recreated.

**Root cause:** deferred once, then the swapchain/RTV/DSV creation code was written in a way that assumed it ran exactly once at startup.

**The fix:** design creation and destruction of size-dependent resources as a callable pair from the very first milestone that has a swapchain. Resize is a *lifetime* problem, and lifetime problems are cheap to design in and expensive to retrofit. Also handle minimize (zero-size swapchain) — it crashes naive implementations.

### 6.7 Assimp indices silently truncated to `uint16_t`

`face.mIndices[j]` is `unsigned int`; it was pushed into a `vector<uint16_t>`. Silently wrong above 65,535 vertices.

**The fix:** use `uint32_t` indices by default. The memory saving from 16-bit is not worth the class of bug, and Vulkan/D3D12 both take 32-bit index buffers happily.

### 6.8 Confused ownership: `DirectXApplication` both inherited from and was owned by `EngineApplication`

`EngineApplication` held a `DirectXApplication*` member, *and* `DirectXApplication` derived from `EngineApplication`. `Update`/`Render` were new virtuals not present on the base, so the inheritance bought nothing.

**Root cause:** inheritance used where composition was meant.

**The fix:** the platform layer owns a window and pumps events; the renderer is a separate object it holds. No inheritance relationship between them.

### 6.9 Per-object redundant camera data

Every object wrote the same `View` and `Proj` matrices into its own row of the transform buffer, every frame.

**The fix:** split into per-frame data (view, proj, camera position, time) and per-object data (world matrix, maybe normal matrix). Two buffers, two indices. This also makes the shader's intent obvious.

### 6.10 Singletons for `Scene` and `Camera`

Meyers singletons. Cheap at the time; they make it impossible to have two scenes, two cameras (shadow pass? cubemap capture? split screen?), or to test anything in isolation.

**The fix:** plain objects with explicit ownership. The engine owns the scene; the scene owns objects and cameras; pass references down. If passing a reference feels annoying, that is a design signal, not a reason for a singleton.

### 6.11 Global bump allocator for descriptor slots

`GNextDescriptorSlot++` with no free list, and slot 0 reserved for the transform buffer by an implicit convention buried inside `CreateTransformBuffer`.

**The fix:** a real `BindlessTable` with typed handles and a free list (§7.3). Reserved slots named as constants, not implied by initialization order.

### 6.12 Vertex format stored BiNormal but no Tangent

`VertexPNBT` = Position/Normal/BiNormal/TexCoord. Normal mapping wants **tangent + handedness**, from which the bitangent is derived (`cross(N, T) * w`). Storing the bitangent instead is backwards and wastes space.

**The fix:** `Position, Normal, Tangent(float4 with handedness in .w), TexCoord`.

### 6.13 Unusual fence pattern (worked, but not the idiom)

One fence *and one event per frame index*, each with its own counter. It functions, but it's not how anyone else writes it, which makes it hard to reason about and hard to map onto Vulkan timeline semaphores.

**The fix:** the standard pattern in §11.2.

### 6.14 Process hygiene: many abandoned restarts

Branch list on the old repo: `master`, `RTX`, `HPC_Tracer`, `D3D12`, `Simple_Base`, `Simple_Compute`, `Simple_Forward`, `Simple_AVX`, `ML`. Roughly eight restarts. The one that got furthest (`ML`) is the one that built on accumulated scaffolding rather than replacing it.

**This is not an argument against the current rewrite** — that decision is made, and the goal this time is relearning, not velocity. It *is* an argument for the discipline in §8: **every milestone must end with something that runs and is committed.** No long stretches with nothing on screen. If a milestone can't end in a running program, it's too big — split it.

---

## 7. D3D12 ↔ Vulkan mapping (the technical core)

### 7.1 Bindless — how the two converge

| | D3D12 (SM 6.6) | Vulkan (1.2+ descriptor indexing) |
|---|---|---|
| Storage | One shader-visible CBV/SRV/UAV heap, all types mixed | One descriptor set (set 0), **one unbounded array binding per descriptor type** |
| Shader access | `ResourceDescriptorHeap[i]` — no declaration needed | `textures[nonuniformEXT(i)]` — declared unbounded array |
| Per-draw indices | Root 32-bit constants | Push constants (128 bytes guaranteed minimum — ample) |
| Samplers | Separate sampler heap, or static samplers in the root signature | Separate sampler array binding, or immutable samplers in the layout |
| Slot allocation | One global counter suffices | **Needs one allocator per descriptor type** |
| Bind frequency | `SetDescriptorHeaps` once per frame | `vkCmdBindDescriptorSets` once per frame |

**Design consequence:** make the bindless allocator typed from the start (§7.3), because D3D12's unified heap will happily let me write a single-counter design that cannot be expressed in Vulkan.

**Vulkan feature flags required** (all core in 1.2, via `VkPhysicalDeviceDescriptorIndexingFeatures` / `VkPhysicalDeviceVulkan12Features`):

```
runtimeDescriptorArray
shaderSampledImageArrayNonUniformIndexing
shaderStorageBufferArrayNonUniformIndexing
descriptorBindingSampledImageUpdateAfterBind
descriptorBindingStorageBufferUpdateAfterBind
descriptorBindingPartiallyBound
descriptorBindingVariableDescriptorCount
```

Descriptor pool needs `VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT`; each binding needs
`UPDATE_AFTER_BIND_BIT | PARTIALLY_BOUND_BIT | VARIABLE_DESCRIPTOR_COUNT_BIT`.

Suggested Vulkan set-0 layout (mirrors the D3D12 heap):

```
binding 0 : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE   [] textures
binding 1 : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER  [] structured buffers
binding 2 : VK_DESCRIPTOR_TYPE_SAMPLER         [] samplers
binding 3 : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE   [] UAV-equivalent (later)
```

### 7.2 Everything else

| Concept | D3D12 | Vulkan | Notes |
|---|---|---|---|
| CPU/GPU sync | `ID3D12Fence` | **Timeline semaphore** | Near-identical semantics (monotonic uint64). Use timeline semaphores — the RHI sync abstraction becomes almost trivial. |
| Swapchain sync | Fence + `Present` | Binary semaphores for acquire/present | Acquire/present *cannot* use timeline semaphores. This is the one real wart; isolate it in the backend. |
| Barriers | **Enhanced Barriers** (Agility SDK) | `vkCmdPipelineBarrier2` / `VkDependencyInfo` | Use Enhanced Barriers, not legacy resource states. They were deliberately modeled on Vulkan's stage/access masks, making the mapping ~1:1. Legacy states would force a lossy translation layer. |
| Render targets | `OMSetRenderTargets` | **Dynamic rendering** (`VK_KHR_dynamic_rendering`, core 1.3) | Use dynamic rendering; skip `VkRenderPass`/`VkFramebuffer` entirely. Matches D3D12's model. |
| Memory allocation | D3D12MA | VMA | Same API shape, both GPUOpen. Use both; do not hand-roll suballocation. |
| Shader source | HLSL → DXIL (DXC) | HLSL → SPIR-V (DXC `-spirv`) | One HLSL source, two targets. See §10.1. |
| Pipeline layout | Root signature (separate object) | Descriptor set layouts baked into `VkPipelineLayout` | PSO abstraction must carry layout info. |
| NDC | Y up, depth 0..1 | **Y down**, depth 0..1 | Fix with negative-height viewport (`VK_KHR_maintenance1`, core 1.1), *not* by baking a flip into the projection matrix — keeps `Camera` API-neutral. |
| Command recording | Allocator + list | Pool + buffer | Reset granularity differs slightly; both are per-frame-slot. |
| Debug names | `ID3D12Object::SetName` | `VK_EXT_debug_utils` object names | Do both from day one. |
| Validation | Debug layer + GBV | Validation layers | Enable from the first line of code, never develop with them off. |

### 7.3 The bindless table abstraction

```cpp
// Typed handles so a texture index can never be passed where a buffer index belongs
struct TextureIndex { uint32_t v = ~0u; };
struct BufferIndex  { uint32_t v = ~0u; };

class BindlessTable {
public:
    TextureIndex RegisterTexture(RHITextureHandle);
    BufferIndex  RegisterBuffer(RHIBufferHandle);
    void         Release(TextureIndex);   // free list, not bump-only
    void         Release(BufferIndex);
};
```

D3D12 backs all of these with offsets into one heap; Vulkan backs each with a separate binding array. Callers never know.

---

## 8. Roadmap

Each milestone ends with **a program that runs** and **a commit**. If it can't, split it.

### Phase A — Foundation (no graphics API at all)

| # | Milestone | Concepts | Done when |
|---|---|---|---|
| A1 | Build skeleton | CMake (recommended — two backends + deps), folder layout from §7/§12 | `cmake --build` produces an empty exe |
| A2 | Logging + assertions | spdlog, `LOG_*`, `CHECK_*`/`ASSERT_*` vocabulary | Colored log output, assert breaks into debugger |
| A3 | Window + input | GLFW `GLFW_NO_API`, semantic `InputAction` translation at the boundary | Window opens, WASD/mouse logged as actions, ESC quits, resize + minimize events fire |
| A4 | Math conventions | Decide handedness, row/column major, `mul` order (§10.2). **Write the decision down in code comments.** | A header stating the convention, with helpers |
| A5 | Filesystem/assets | exe-relative resolution, asset root | Can locate and read `Assets/…` |

### Phase B — D3D12 bring-up (raw, deliberately un-abstracted)

> **No RHI in this phase.** Write direct D3D12. The RHI is extracted in Phase C *from working code*. Interfaces designed before a second backend exists are almost always wrong in the same way: they abstract the API you know instead of the intersection of two.

| # | Milestone | Concepts | Done when |
|---|---|---|---|
| B1 | Device + debug layer | Adapter enumeration, feature levels, debug layer, GBV, DRED | Device created, adapter name logged, debug layer clean |
| B2 | Swapchain + clear | Flip-discard, back buffers, RTV heap, `PRESENT ↔ RENDER_TARGET` barrier | Solid color window |
| B3 | **Frames in flight, done right** | Per-slot allocators, one monotonic fence, §11.2 pattern | Clear-color loop, no debug-layer complaints, clean shutdown flush |
| B4 | **Resize + minimize** | Size-dependent resource lifetime as a create/destroy pair | Resize and minimize/restore without crash or leak |
| B5 | Shader compile via DXC | `IDxcCompiler3`, DXIL validation + signing (§11.1), blob lifetime | Trivial shader compiles at runtime, errors reported legibly |
| B6 | First triangle | Root signature, PSO, viewport/scissor | Triangle on screen |
| B7 | Buffers + staging | DEFAULT vs UPLOAD, `CopyBufferRegion`, barriers, D3D12MA | Indexed geometry from a DEFAULT-heap VB/IB |
| B8 | Depth buffer | DSV, depth format, depth state in PSO | Correct occlusion between overlapping geometry |
| B9 | **Bindless heap + first texture** | Shader-visible heap, `ResourceDescriptorHeap[i]`, typed `BindlessTable`, texture upload with row-pitch footprints | Textured quad, index passed via root constants |
| B10 | Per-frame + per-object data | Per-frame ring buffer (§11.2), split camera vs object data (§6.9) | Multiple objects, distinct transforms, no race |
| B11 | Camera + input | View/proj, velocity + damping, pitch clamp, first-click guard | Fly camera matching the old one's feel |
| B12 | Model loading | Assimp → `vector<Vertex>` + `vector<uint32_t>`, tangents | FBX character on screen, textured |
| B13 | Scene + draw list | `DrawItem` emission (§6.1), renderer submits | Old engine's scene reproduced — but objects record nothing |
| B14 | Hot reload + debug names | Shader reload on keypress, `SetName` everywhere | Edit HLSL, see it live |

**End of Phase B = feature parity with the old engine, built correctly.**

### Phase C — Extract the RHI (still D3D12-only)

| # | Milestone | Done when |
|---|---|---|
| C1 | Identify the seam; list every D3D12 call the renderer makes | A written inventory |
| C2 | Opaque handles + backend resource pools (`BufferHandle{uint32}`, not `ID3D12Resource*`) | No raw API pointers above the backend |
| C3 | `RHIDevice`, `RHICommandList`, `RHISwapchain` virtual interfaces | Renderer compiles against interfaces only |
| C4 | `BindlessTable` behind the RHI | Typed handles throughout |
| C5 | Enforce the include rule (§6.2), grep check | No API header outside `RHI/<Backend>/` |
| C6 | Verify: pixel-identical output to end of Phase B | Screenshot diff |

**Use virtual interfaces, not compile-time backend selection.** The vtable cost is irrelevant at these draw counts, and it buys something valuable: both backends alive in one build, selectable by a command-line flag, so a broken Vulkan frame can be A/B'd against a working D3D12 frame in the same session.

### Phase D — Vulkan backend

| # | Milestone | Notes |
|---|---|---|
| D1 | Instance, validation layers, physical device, queue families | Enable §7.1 feature flags here |
| D2 | Surface + swapchain, present modes | GLFW surface creation |
| D3 | Sync: timeline semaphores + binary acquire/present semaphores | Mirror §11.2 |
| D4 | Command pools/buffers per frame slot | |
| D5 | Dynamic rendering, clear color | First Vulkan pixels |
| D6 | SPIR-V pipeline, first triangle | Resolve §10.1 before this |
| D7 | **Bindless descriptor indexing** | The milestone that validates the whole design |
| D8 | VMA, staging uploads, textures | |
| D9 | Parity with Phase B feature set | Same scene, `--backend=vulkan` |

### Phase E — Beyond (where it gets fun)

Real lighting (the old renderer passed normals to the pixel shader and then ignored them), material system, shadow maps, render graph / frame graph, compute passes, GPU-driven rendering via indirect draws, PIX/RenderDoc-driven profiling.

---

## 9. Self-check questions (assistant: ask these)

Use these to verify I actually understand a milestone rather than having watched code appear.

**Frames in flight / sync**
- Why does each frame slot need its own command allocator instead of resetting one?
- What exactly goes wrong if I reset an allocator whose commands the GPU hasn't finished?
- Why is a *monotonically increasing* fence value better than a per-slot counter?
- How does a Vulkan timeline semaphore differ from a D3D12 fence? Where does the analogy break?

**Memory / resources**
- When does data actually need a DEFAULT heap rather than UPLOAD?
- Why does a texture upload need row-pitch footprints when a buffer upload doesn't?
- My transform buffer is written by the CPU every frame and read by the GPU. How many copies do I need, and why exactly that number?

**Barriers**
- What is a barrier actually telling the driver — is it about caches, layout, or scheduling?
- Why do D3D12 Enhanced Barriers map better to Vulkan than legacy resource states?
- What's a layout transition and why does D3D12 not talk about them the way Vulkan does?

**Bindless**
- Why is bindless *easier* to abstract across APIs than descriptor tables/sets?
- Why does Vulkan need `nonuniformEXT` and D3D12 doesn't appear to?
- What does `UPDATE_AFTER_BIND` let me do that I couldn't otherwise?
- Why does my slot allocator need to be per-type even though D3D12 doesn't require it?

**Pipeline / shaders**
- Why must DXIL be signed, and what breaks if I hand the runtime the unsigned blob?
- What exactly is baked into a PSO, and why is it monolithic?
- Where does the descriptor set layout live in Vulkan vs D3D12, and why does that matter for my PSO abstraction?

**Math**
- Which handedness and matrix order did I choose, and where is that written down?
- What is the actual difference between Vulkan and D3D12 NDC, and why is a negative-height viewport the better fix than a matrix flip?

---

## 10. Open decisions — ask me, don't assume

### 10.1 Shader strategy — **resolve before Phase D6, ideally investigate during Phase B5**

HLSL compiled by DXC to both DXIL and SPIR-V is the plan. The open question is how `ResourceDescriptorHeap[i]` lowers to SPIR-V descriptor indexing in the DXC version I end up using — this has been evolving, so **verify empirically rather than trusting any documentation, including this file.**

- **If it maps cleanly:** one HLSL source, two targets, nothing else to do.
- **If it doesn't:** put bindless access behind a macro that expands per-target:
  ```hlsl
  #ifdef TARGET_SPIRV
    #define SAMPLE_TEX(idx, uv)  texturesArray[NonUniformResourceIndex(idx)].Sample(...)
  #else
    #define SAMPLE_TEX(idx, uv)  ResourceDescriptorHeap[idx].Sample(...)
  #endif
  ```
  Cost is one header of macros. Perfectly acceptable; just decide early because it shapes the shader pipeline.

Also decide: runtime compile only, or offline + runtime hot-reload?

### 10.2 Math library and conventions — **DECIDED (2026-08-22): DirectXMath**

> Handedness / major-order / `mul` order are still to be pinned in A4 and written into the math header's comment block.
DirectXMath (Windows-only, SIMD, what I know) vs glm (portable, GLSL-shaped). Then: handedness, row vs column major, `mul(v,M)` vs `mul(M,v)`. **Whatever I pick, it goes in a comment block at the top of the math header and is never silently violated.** The old engine used DirectXMath + row-major + LH + `mul(v,M)`.

### 10.3 Build system — **DECIDED (2026-08-22): CMake**
CMake vs VS solution. Recommendation: **CMake** — two backends, more third-party deps, and the Vulkan SDK integrates more cleanly. It also keeps the door open if I ever want a non-Windows build.

### 10.4 Dependency management — **DECIDED (2026-08-22): CMake `FetchContent`**
Submodules (what I did before, and Assimp was a prebuilt lib that didn't fit that model) vs vcpkg vs CMake `FetchContent`. Recommendation: vcpkg manifest mode, or `FetchContent`. The old mixed approach was the worst of both.

### 10.5 D3D12 Agility SDK
Needed for Enhanced Barriers and current shader models. Decide how to vendor and deploy it early — it has specific requirements about exported symbols and a `D3D12/` subdirectory next to the exe.

### 10.6 Engine-as-DLL, again? — **DECIDED (2026-08-22): static library (`janus.lib`)**

> The clean public surface the DLL used to enforce is now enforced by the §6.2 include rule and the §12 folder structure instead.
It enforced a clean public surface, which was genuinely valuable. It also complicated deployment and required `UT_API` on everything. Static library is simpler. My call.

### 10.7 Naming conventions — **DECIDED**

The engine is **Janus**. Conventions:

| Thing | Convention | Example |
|---|---|---|
| Root namespace | `JN` (short — it gets typed constantly) | `JN::Renderer` |
| Sub-namespaces | `JN::Core`, `JN::RHI`, `JN::World` | `JN::RHI::Device` |
| Macros | `JN_` prefix | `JN_ASSERT`, `JN_LOG_INFO`, `JN_CHECK_HRESULT`, `JN_VK_CHECK` |
| Engine target | `Janus` → `janus.lib` / `janus.dll` | |
| Sample app | `Sandbox` (owns `main()`) | |
| Backend folders | Literal, not themed — clarity beats cleverness | `RHI/D3D12/`, `RHI/Vulkan/` |

Still open (minor, decide in A2 and then never revisit): member prefixes. Old code used `m_p` Hungarian. Recommendation: plain `m_` for members, drop the pointer-type encoding — `m_device`, not `m_pDevice`. Modern tooling makes the type obvious and `m_p` decays into noise the moment a raw pointer becomes a handle or a `unique_ptr`.

---

## 11. Hard-won details worth preserving verbatim

### 11.1 DXC compile + DXIL signing

Non-obvious and cost real time to figure out the first time:

1. Compile with `IDxcCompiler3::Compile` → gives `DXC_OUT_OBJECT`, the **unsigned** DXIL blob.
2. SM 6.x DXIL must be **validated and signed** before the runtime accepts it: `DxcCreateInstance(CLSID_DxcValidator)` → `Validate(pCode, DxcValidatorFlags_InPlaceEdit, &pValResult)` → `pValResult->GetResult(&pSignedBlob)`.
3. `D3D12_SHADER_BYTECODE` must point at the **signed** blob, not the compile output.
4. **Both blobs must stay alive until after `CreateGraphicsPipelineState` returns.** Releasing early gives corrupted-bytecode failures that look like driver bugs.
5. Useful args: `-E <entry> -T <target> -HV 2021`; debug `-Zi -Od -Qembed_debug`; release `-O3`.
6. Always read `DXC_OUT_ERRORS` and log it even on success — warnings live there.

*(Note: when DXC's own signing is unavailable, `dxil.dll` must be present alongside the exe. Check this early; its absence produces confusing failures.)*

### 11.2 The frames-in-flight patterns to use this time

**Fence pattern** (replaces the old per-slot-fence design, §6.13):

```cpp
// One fence, one monotonically increasing value, one recorded value per frame slot.
ID3D12Fence* fence;              // single
uint64_t     nextFenceValue = 1; // monotonic, never resets
uint64_t     frameFenceValue[FRAMES_IN_FLIGHT] = {};

void BeginFrame(uint32_t slot) {
    if (fence->GetCompletedValue() < frameFenceValue[slot]) {
        fence->SetEventOnCompletion(frameFenceValue[slot], event);
        WaitForSingleObject(event, INFINITE);
    }
    allocator[slot]->Reset();
    cmdList[slot]->Reset(allocator[slot], nullptr);
}

void EndFrame(uint32_t slot) {
    cmdList[slot]->Close();
    queue->ExecuteCommandLists(1, lists);
    queue->Signal(fence, nextFenceValue);
    frameFenceValue[slot] = nextFenceValue++;
    swapchain->Present(vsync ? 1 : 0, 0);
}
```

Maps almost line-for-line onto a Vulkan timeline semaphore.

**Dynamic (CPU-written-per-frame) buffer pattern** — fixes §6.3:

```
Allocate ONE upload buffer of size (perFrameSize * FRAMES_IN_FLIGHT).
Frame slot i writes only to the region [i * perFrameSize, (i+1) * perFrameSize).
The bindless index / descriptor offset for that region is selected per frame.
```

Because `BeginFrame` already waited on slot `i`'s fence, that region is provably free. One allocation, no races, and identical in Vulkan.

### 11.3 Texture upload footprints

`GetCopyableFootprints` gives `RowPitch` aligned to `D3D12_TEXTURE_DATA_PITCH_ALIGNMENT` (256), which is **not** `width * bytesPerPixel`. Copy row by row:

```cpp
for (UINT row = 0; row < numRows; ++row)
    memcpy(pUpload + footprint.Offset + row * footprint.Footprint.RowPitch,
           srcPixels + row * rowSizeInBytes,
           rowSizeInBytes);
```

Getting this wrong produces the classic skewed/sheared texture. Vulkan has the same concept via `bufferRowLength` in `VkBufferImageCopy`.

### 11.4 Camera details that came from real bugs

- Clamp pitch to `±(π/2 · 0.95)`, not `±π/2` — exactly at the pole the look vector degenerates.
- Guard the first mouse-move after the button goes down: record the position and *skip that frame's rotation*, otherwise the view snaps by the distance since the last drag.
- `right = cross(forward, worldUp)` — sign depends on handedness; verify A/D empirically rather than reasoning about it. (I got this backwards once and fixed it in a later commit.)

### 11.5 Misc

- `stbi_set_flip_vertically_on_load(true)` was needed for correct UV orientation with the old conventions — re-derive rather than assuming; it depends on the UV convention chosen in A4.
- Assimp flags used: `aiProcess_Triangulate | aiProcess_JoinIdenticalVertices`. Add `aiProcess_CalcTangentSpace` this time (§6.12), and consider `aiProcess_ConvertToLeftHanded` depending on the A4 decision.
- The old `ModelLoader` flattened the entire scene graph into one VB/IB and **dropped node transforms and all materials**. Fine for one character; decide deliberately this time.

---

## 12. Proposed folder structure

```
Janus/                   the engine  (JN:: namespace, JN_ macros)
  src/
    Core/       platform, logging, math, InputAction, filesystem  ← zero graphics headers
    RHI/        interfaces + opaque handles                       ← zero API headers
      RHIDevice.h  RHICommandList.h  RHIResources.h  RHIBindless.h
      D3D12/    D3D12 backend        (the only place d3d12.h appears)
      Vulkan/   Vulkan backend       (the only place vulkan.h appears)
    Renderer/   passes, draw-list building, frame logic            ← RHI only
    World/      Scene, GameObject, Camera                          ← zero GPU types
Sandbox/                 the test application (owns main())
  src/
  Assets/     Shaders/, Models/, Textures/
```

**The one rule that does most of the work:**
> No file outside `RHI/D3D12/` or `RHI/Vulkan/` may include a graphics API header.

---

## 13. Checklist before writing the first line

- [ ] Old repo skimmed once for memory-lane purposes (§3 file list) — then closed and not copy-pasted from
- [ ] §10 decisions made and written down
- [ ] Vulkan SDK installed; `vulkaninfo` confirms descriptor-indexing support on my GPU
- [ ] Agility SDK acquisition/deployment plan settled
- [ ] PIX and RenderDoc installed and confirmed capturing
- [ ] New repo `Janus` initialized, `.gitignore` in place, this document committed to it as `KNOWLEDGE_BASE.md`
- [ ] Committed to the discipline: **every milestone ends with something that runs**

---

---

## 14. Environment verification (2026-08-22)

Checked on the dev machine before A1:

| Item | Result |
|---|---|
| GPU | NVIDIA GeForce RTX 3080 Laptop + Intel UHD Graphics (**two adapters — B1/D1 must select the discrete one deliberately, not index 0**) |
| Vulkan | SDK 1.3.268.0 installed; driver reports API 1.4. All seven §7.1 descriptor-indexing flags `true`, plus `timelineSemaphore`, `dynamicRendering`, `bufferDeviceAddress` |
| Toolchain | VS 2026 Community (MSVC 19.51, toolset 14.51), Windows SDK 10.0.26100 |
| CMake | 4.4.4 in `C:\Program Files\CMake\bin` (on PATH; open a fresh terminal after install). Generator: Visual Studio 18 2026 (solution is `build\Janus.slnx`) |
| RenderDoc | Installed |
| PIX | **Not installed** — needed before B1 |
| Agility SDK | **No plan yet** — §10.5, needed before B1 for Enhanced Barriers |
| DXC | `dxc.exe` + `dxcompiler.dll` ship in the Vulkan SDK (relevant to §10.1 / B5) |

---

*End of knowledge base. Assistant: begin at §8 Phase A1 unless told otherwise, and re-read §0 before responding.*
