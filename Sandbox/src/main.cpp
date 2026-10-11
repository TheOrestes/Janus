#include "Core/FileSystem.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Core/Window.h"

int main()
{
    JN::LogInit();

    JN::Window window(1280, 720, "Janus");
    JN_LOG_INFO("Window created");

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

    return 0;
}
