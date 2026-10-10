#include "Core/Input.h"
#include "Core/Window.h"

#include <GLFW/glfw3.h>

namespace JN
{
    static int ToKey(InputAction action)
    {
        switch (action)
        {
            case InputAction::Quit:        return GLFW_KEY_ESCAPE;
            case InputAction::MoveForward: return GLFW_KEY_W;
            case InputAction::MoveBack:    return GLFW_KEY_S;
            case InputAction::MoveLeft:    return GLFW_KEY_A;
            case InputAction::MoveRight:   return GLFW_KEY_D;
        }
        return GLFW_KEY_UNKNOWN;
    }

    bool IsActionDown(const Window& window, InputAction action)
    {
        return glfwGetKey(window.GetHandle(), ToKey(action)) == GLFW_PRESS;
    }
}
