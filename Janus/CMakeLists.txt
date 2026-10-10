# The engine as a static library (decision 10.6): no DLL boundary, and
# everything is linked straight into the exe that uses it.
add_library(Janus STATIC
    src/Core/Log.cpp
    src/Core/Window.cpp
    src/Core/Input.cpp)

# PUBLIC: anything linking Janus can write #include "Core/Log.h".
target_include_directories(Janus PUBLIC src)

# PUBLIC, not PRIVATE: the JN_LOG_* macros expand to spdlog calls inside the
# caller's code, so callers need spdlog's headers and lib as well.
target_link_libraries(Janus PUBLIC spdlog::spdlog)

# PRIVATE: GLFW is used only inside Window.cpp and Input.cpp. No public header
# includes glfw3.h, so Sandbox never sees it. Being a static library, the final
# exe still gets GLFW linked in automatically.
target_link_libraries(Janus PRIVATE glfw)
