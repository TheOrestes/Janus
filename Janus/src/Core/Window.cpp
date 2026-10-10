#include "Core/Window.h"
#include "Core/Log.h"

#include <GLFW/glfw3.h>

namespace JN
{
    // GLFW reports failures only through this callback; route them into our log.
    static void OnGlfwError(int code, const char* description)
    {
        JN_LOG_ERROR("GLFW error {}: {}", code, description);
    }

    Window::Window(int width, int height, const char* title)
    {
        glfwSetErrorCallback(OnGlfwError);
        JN_CHECK(glfwInit(), "glfwInit failed");

        // No OpenGL context: D3D12/Vulkan create their own swapchain for this window.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_pHandle = glfwCreateWindow(width, height, title, nullptr, nullptr);
        JN_CHECK(m_pHandle != nullptr, "glfwCreateWindow failed");
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_pHandle);
        glfwTerminate();
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(m_pHandle) != 0;
    }

    GLFWwindow* Window::GetHandle() const
    {
        return m_pHandle;
    }
}
