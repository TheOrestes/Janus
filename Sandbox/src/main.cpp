#include "Core/FileSystem.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Core/Window.h"
#include "RHI/D3D12/Core.h"

int main()
{
    JN::LogInit();

    JN::Window window(1280, 720, "Janus");
    JN_LOG_INFO("Window created");

    JN::D3D12::Core::Init();

    JN_LOG_INFO("Exe dir: {}", JN::GetExecutableDirectory().string());

    std::vector<char> data;
    if (JN::ReadAssetFile("test.txt", data))
    {
        JN_LOG_INFO("test.txt: {} bytes", data.size());
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
            JN_LOG_DEBUG("MoveForward");
        }
    }

    JN::D3D12::Core::Shutdown();

    return 0;
}
