#pragma once

namespace JN
{
    class Window;

    enum class InputAction
    {
        Quit,
        MoveForward,
        MoveBack,
        MoveLeft,
        MoveRight
    };

    bool IsActionDown(const Window& window, InputAction action);
}
