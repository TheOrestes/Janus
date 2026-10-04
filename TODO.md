# Janus — TODO / Roadmap Tracker

Living checklist mirroring `KNOWLEDGE_BASE.md` §8 (roadmap) and §10 (open decisions).
Check items off as they're done; leave open otherwise. Updated as we go.

## Phase A — Foundation

- [x] A1 — Build skeleton (CMake produces an empty exe)
- [x] A2 — Logging + assertions (spdlog, `JN_LOG_*`, `JN_CHECK_*`/`JN_ASSERT_*`)
- [ ] A3 — Window + input (GLFW, semantic `InputAction`)
- [ ] A4 — Math conventions (handedness, row/col-major, `mul` order — write to header comment)
- [ ] A5 — Filesystem/assets (exe-relative resolution)

## Phase B — D3D12 bring-up (raw, no RHI)

- [ ] B1 — Device + debug layer (select discrete GPU explicitly — two adapters present)
- [ ] B2 — Swapchain + clear
- [ ] B3 — Frames in flight (single fence, monotonic value — §11.2 pattern)
- [ ] B4 — Resize + minimize
- [ ] B5 — Shader compile via DXC (validate + sign — §11.1)
- [ ] B6 — First triangle
- [ ] B7 — Buffers + staging
- [ ] B8 — Depth buffer
- [ ] B9 — Bindless heap + first texture
- [ ] B10 — Per-frame + per-object data
- [ ] B11 — Camera + input
- [ ] B12 — Model loading (Assimp)
- [ ] B13 — Scene + draw list
- [ ] B14 — Hot reload + debug names

## Phase C — Extract the RHI (D3D12-only)

- [ ] C1 — Seam inventory (every D3D12 call the renderer makes)
- [ ] C2 — Opaque handles + backend resource pools
- [ ] C3 — `RHIDevice`/`RHICommandList`/`RHISwapchain` virtual interfaces
- [ ] C4 — `BindlessTable` behind the RHI
- [ ] C5 — Enforce include rule (§6.2) — no API header outside `RHI/<Backend>/`
- [ ] C6 — Verify pixel-identical output vs end of Phase B

## Phase D — Vulkan backend

- [ ] D1 — Instance, validation layers, physical device, queue families
- [ ] D2 — Surface + swapchain, present modes
- [ ] D3 — Sync: timeline semaphores + binary acquire/present
- [ ] D4 — Command pools/buffers per frame slot
- [ ] D5 — Dynamic rendering, clear color
- [ ] D6 — SPIR-V pipeline, first triangle *(resolve §10.1 first)*
- [ ] D7 — Bindless descriptor indexing
- [ ] D8 — VMA, staging uploads, textures
- [ ] D9 — Parity with Phase B feature set

## Phase E — Beyond

- [ ] Real lighting
- [ ] Material system
- [ ] Shadow maps
- [ ] Render graph / frame graph
- [ ] Compute passes
- [ ] GPU-driven rendering (indirect draws)
- [ ] PIX/RenderDoc-driven profiling

## Pre-B1 environment gaps (§14)

- [ ] PIX installed
- [ ] Agility SDK vendoring plan (§10.5)

## Open decisions (§10)

- [ ] 10.1 — Shader strategy: does `ResourceDescriptorHeap[i]` map cleanly to SPIR-V in the DXC version used? (verify empirically before D6, investigate during B5). Also: runtime-only compile, or offline + hot-reload?
- [x] 10.2 — Math library: **DirectXMath** (2026-08-22). Handedness/major-order/`mul` order still to be pinned in A4.
- [x] 10.3 — Build system: **CMake** (2026-08-22)
- [x] 10.4 — Dependency management: **CMake `FetchContent`** (2026-08-22)
- [ ] 10.5 — D3D12 Agility SDK vendoring/deployment plan
- [x] 10.6 — Engine packaging: **static library (`janus.lib`)**, no DLL (2026-08-22)
- [x] 10.7 — Naming conventions: `JN::` namespace, `JN_` macros, `Janus`/`Sandbox` targets (2026-08-22; namespace changed from `jn` to `JN` 2026-10-04)
- [ ] Minor: member prefix — recommendation is plain `m_` (drop `m_p` Hungarian) — decide in A2, then never revisit
