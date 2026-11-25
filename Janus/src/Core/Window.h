#pragma once

struct GLFWwindow;

namespace JN
{
    class Window
    {
    public:
        Window(int width, int height, const char* title);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        void PollEvents();
        bool ShouldClose() const;
        GLFWwindow* GetHandle() const;
        void* GetNativeHandle() const; // the Win32 HWND, as void* so this header stays free of windows.h

    private:
        GLFWwindow* m_pHandle = nullptr;
    };
}
