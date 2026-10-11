# The engine as a static library (decision 10.6): no DLL boundary, and
# everything is linked straight into the exe that uses it.
# Headers are listed too, only so they show up in Visual Studio's Solution
# Explorer; the compiler finds them through #include regardless.
add_library(Janus STATIC
    src/Core/FileSystem.h
    src/Core/FileSystem.cpp
    src/Core/Log.h
    src/Core/Log.cpp
    src/Core/Window.h
    src/Core/Window.cpp
    src/Core/Input.h
    src/Core/Input.cpp
    src/Core/Math.h)

# Show the files in Solution Explorer as the folder tree on disk (Core/...)
# instead of one flat list. The tree is rooted at src/, so Core/Log.h appears
# under a "Core" folder. Follow this pattern for every new target.
get_target_property(JANUS_SOURCES Janus SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${JANUS_SOURCES})

# PUBLIC: anything linking Janus can write #include "Core/Log.h".
target_include_directories(Janus PUBLIC src)

# PUBLIC, not PRIVATE: the JN_LOG_* macros expand to spdlog calls inside the
# caller's code, so callers need spdlog's headers and lib as well.
target_link_libraries(Janus PUBLIC spdlog::spdlog)

# PRIVATE: GLFW is used only inside Window.cpp and Input.cpp. No public header
# includes glfw3.h, so Sandbox never sees it. Being a static library, the final
# exe still gets GLFW linked in automatically.
target_link_libraries(Janus PRIVATE glfw)
