#include "Core/FileSystem.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Core/Window.h"
#include "RHI/D3D12/Core.h"

// Agility SDK loader hooks: the D3D12 loader looks for these two exports in
// the exe. Version must match the NuGet package (1.620.1 -> 620).
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 620;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}

int main()
{
    JN::LogInit();

    JN::Window window(1280, 720, "Janus");
    APP_LOG_INFO("Window created");

    JN::D3D12::Core::Init();

    APP_LOG_INFO("Exe dir: {}", JN::GetExecutableDirectory().string());

    std::vector<char> data;
    if (JN::ReadAssetFile("test.txt", data))
    {
        APP_LOG_INFO("test.txt: {} bytes", data.size());
    }

    while (!window.ShouldClose())
    {
        window.PollEvents();

        if (JN::IsActionDown(window, JN::InputAction::Quit))
        {
            break;
        }

        if (JN::IsActionDown(window, JN::InputAction::MoveForward))
        {
            APP_LOG_DEBUG("MoveForward");
        }
    }

    JN::D3D12::Core::Shutdown();

    return 0;
}
