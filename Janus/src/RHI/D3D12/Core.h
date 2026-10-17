#pragma once

#include "RHI/D3D12/D3D12Common.h"

namespace JN::D3D12::Core
{
    extern IDXGIFactory6*   G_pFactory;
    extern ID3D12Debug1*    G_pDebug;
    extern ID3D12Device*    G_pDevice;

    void Init();
    void Shutdown();

    inline ID3D12Device* GetDevice()
    {
        return G_pDevice;
    }
}
