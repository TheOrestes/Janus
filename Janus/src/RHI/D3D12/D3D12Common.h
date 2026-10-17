#pragma once

#include "Core/Log.h"

#include <d3d12.h>
#include <dxgi1_6.h>

namespace JN::D3D12
{
    template <typename T>
    void SafeRelease(T*& p)
    {
        if (p)
        {
            p->Release();
            p = nullptr;
        }
    }
}

#define JN_CHECK_HRESULT(expr)                                         \
    do                                                                 \
    {                                                                  \
        const HRESULT jnHr = (expr);                                   \
        JN_CHECK(SUCCEEDED(jnHr), "{} failed, HRESULT 0x{:08X}", #expr, \
                 static_cast<unsigned>(jnHr));                         \
    } while (0)
