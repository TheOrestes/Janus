#pragma once

#include "RHI/D3D12/D3D12Common.h"

namespace JN::D3D12::Core
{
    //-----------------------------------------------------------------------------------------------------------------
    extern IDXGIFactory6*       G_pFactory;
    extern ID3D12Debug1*        G_pDebug;
    extern ID3D12Device*        G_pDevice;
    extern ID3D12CommandQueue*  G_pCommandQueue;
    extern ID3D12Fence*         G_pFence;

    //-----------------------------------------------------------------------------------------------------------------
    void    Init();
    void    Shutdown();

    UINT64  SignalFence();                                  
    void    WaitForFence(UINT64 uiValue);                   
    void    WaitIdle();                                     

    //-----------------------------------------------------------------------------------------------------------------
    inline ID3D12Device* GetDevice()
    {
        return G_pDevice;
    }

    //-----------------------------------------------------------------------------------------------------------------
    inline ID3D12CommandQueue* GetCommandQueue()
    {
        return G_pCommandQueue;
    }
}
