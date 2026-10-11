#pragma once

// ---------------------------------------------------------------------------
// Janus math conventions
// Never silently violate these. Change them here first, then everywhere.
//
//   Handedness      left-handed, Y up, +Z forward, +X right
//   Vectors         row vectors, transform with v * M (reads left to right:
//                   world * view * proj)
//   Matrix storage  row-major; the CPU uses XMFLOAT4X4. HLSL must declare
//                   #pragma pack_matrix(row_major) and use mul(v, M), so no
//                   transpose is needed between CPU and shader
//   UV origin       top-left, v increases downward (D3D style)
//   Depth           clip-space z in 0..1 (D3D12 and Vulkan agree)
// ---------------------------------------------------------------------------

#include <DirectXMath.h>

namespace JN
{
    using Float2 = DirectX::XMFLOAT2;
    using Float3 = DirectX::XMFLOAT3;
    using Float4 = DirectX::XMFLOAT4;
    using Float4x4 = DirectX::XMFLOAT4X4;

    inline constexpr Float3 WorldRight{ 1.0f, 0.0f, 0.0f };
    inline constexpr Float3 WorldUp{ 0.0f, 1.0f, 0.0f };
    inline constexpr Float3 WorldForward{ 0.0f, 0.0f, 1.0f };
}
