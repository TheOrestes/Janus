# B2a: Command queues, command lists, swapchain, RTVs

Diagrams: `img/B2_cpu_gpu_timeline.svg`, `img/B2_command_objects.svg`,
`img/B2_swapchain_rotation.svg`, `img/B2_rtv_heap.svg`.

## Commands are recorded, not executed
- The GPU runs asynchronously from the CPU. Calling `Draw` does not draw; it appends a
  command to a buffer. Nothing runs until the buffer is submitted to a queue.
- Result: CPU records frame N+1 while GPU executes frame N. The GPU is always behind
  the CPU by up to the number of frames in flight.
- Latency cost: with F frames in flight at 60 Hz (16.67 ms per frame), the
  input-to-photon delay grows by about F * 16.67 ms (F = 2 gives about 33 ms).

## The three command objects
- `ID3D12CommandAllocator`: owns the memory commands are written into. `Reset()` frees
  all of it at once. Must not be reset while the GPU still reads from it. One allocator
  can back only one list that is currently recording.
- `ID3D12GraphicsCommandList`: the recording interface. Created open; `Close()` ends
  recording. After submission the list itself can be `Reset()` and reused immediately;
  the allocator behind it cannot.
- `ID3D12CommandQueue`: submits closed lists (`ExecuteCommandLists`) and signals
  fences (`Signal`). Types: DIRECT (graphics, compute, copy), COMPUTE, COPY. Janus uses
  DIRECT for now.

## Swapchain (flip model)
- `DXGI_SWAP_EFFECT_FLIP_DISCARD`: the display flips between buffers instead of copying.
  Minimum 2 buffers; Janus will use 3.
- Each buffer plays one role per frame: on screen, ready and waiting, being rendered
  into. `Present()` rotates the roles. `GetCurrentBackBufferIndex()` gives the index
  of the buffer to render into.
- Memory: a 1280x720 RGBA8 buffer is 1280 * 720 * 4 = 3,686,400 bytes (about 3.5 MiB).
  Three buffers are about 10.5 MiB.
- The swapchain needs the window handle (HWND). GLFW exposes it through
  `glfw3native.h` (`glfwGetWin32Window`), which is why `Window` gets a getter.

## RTV and descriptor heaps
- A render target view (RTV) is a small descriptor telling the GPU how to treat a
  texture as a render target (format, mip, etc.).
- RTVs live in a descriptor heap of type RTV. RTV and DSV heaps are never
  shader-visible.
- Slot address: `handle(i) = heapStart + i * GetDescriptorHandleIncrementSize(RTV)`.
  The increment is driver-defined; never hardcode it.
- One RTV per back buffer, created once. Rendering picks the slot by the current
  back buffer index.

## What B2 builds
Command queue, swapchain (3 buffers, flip-discard), RTV heap with 3 RTVs, a list that
clears the current back buffer, then Present. The barrier that makes the buffer
writable (Enhanced Barriers) is lesson B2b.
