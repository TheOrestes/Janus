#include "RHI/D3D12/Core.h"

#include <filesystem>

namespace JN::D3D12::Core
{
    IDXGIFactory6* G_pFactory = nullptr;
    ID3D12Debug1*  G_pDebug   = nullptr;
    ID3D12Device*  G_pDevice  = nullptr;

    //-----------------------------------------------------------------------------------------------------------------
    static void EnableDebugLayer()
    {
        ID3D12Debug* pDebug0 = nullptr;
        JN_CHECK_HRESULT(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug0)));
        JN_CHECK_HRESULT(pDebug0->QueryInterface(IID_PPV_ARGS(&G_pDebug)));
        SafeRelease(pDebug0);

        G_pDebug->EnableDebugLayer();
        G_pDebug->SetEnableGPUBasedValidation(TRUE);
    }

    //-----------------------------------------------------------------------------------------------------------------
    static IDXGIAdapter1* PickAdapter()
    {
        IDXGIAdapter1* pAdapter = nullptr;
        JN_CHECK_HRESULT(G_pFactory->EnumAdapterByGpuPreference(
            0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter)));

        DXGI_ADAPTER_DESC1 desc = {};
        pAdapter->GetDesc1(&desc);
        JN_LOG_INFO("GPU: {} ({} MB VRAM)",
                    std::filesystem::path(desc.Description).string(),
                    desc.DedicatedVideoMemory / (1024 * 1024));
        return pAdapter;
    }

    //-----------------------------------------------------------------------------------------------------------------
    static void EnableBreakOnError()
    {
        ID3D12InfoQueue* pQueue = nullptr;
        JN_CHECK_HRESULT(G_pDevice->QueryInterface(IID_PPV_ARGS(&pQueue)));
        pQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        SafeRelease(pQueue);
    }

    //-----------------------------------------------------------------------------------------------------------------
    void Init()
    {
        EnableDebugLayer();

        JN_CHECK_HRESULT(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,
                                            IID_PPV_ARGS(&G_pFactory)));

        IDXGIAdapter1* pAdapter = PickAdapter();
        JN_CHECK_HRESULT(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_0,
                                           IID_PPV_ARGS(&G_pDevice)));
        EnableBreakOnError();
        SafeRelease(pAdapter);
    }

    //-----------------------------------------------------------------------------------------------------------------
    void Shutdown()
    {
        ID3D12DebugDevice* pDebugDevice = nullptr;
        if (SUCCEEDED(G_pDevice->QueryInterface(IID_PPV_ARGS(&pDebugDevice))))
        {
            pDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
        }
        SafeRelease(pDebugDevice);

        SafeRelease(G_pDevice);
        SafeRelease(G_pFactory);
        SafeRelease(G_pDebug);
    }
}
