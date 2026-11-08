# 3.28 is a floor, not a target: it's the oldest version we promise to support.
cmake_minimum_required(VERSION 3.28)
project(Janus LANGUAGES CXX) # C++ only, skips C compiler detection

# C++20 is required (no silent fallback to an older standard), and
# EXTENSIONS OFF means standard C++20, not a compiler-specific dialect.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Third-party deps are fetched at configure time (decision 10.4). The tag is
# pinned for reproducible builds; a shallow clone keeps the download small.
include(FetchContent)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.3
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(spdlog) # provides the spdlog::spdlog target

# GLFW: windowing + input only. It's not a graphics API, so using it outside
# RHI/ doesn't break the include rule. These options skip building parts we
# don't need; they must be set BEFORE MakeAvailable, which reads them.
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(glfw) # provides the glfw target

# Agility SDK (decision 10.5): a NuGet package is just a zip, so it is fetched
# at configure time like the other deps. 1.620.1 ships SDK version 620; that
# number must match the D3D12SDKVersion export in the exe.
# DOWNLOAD_NAME forces a .zip extension so CMake unpacks it.
FetchContent_Declare(agility
    URL           https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.620.1
    DOWNLOAD_NAME agility.zip)
FetchContent_MakeAvailable(agility)
# Not a CMake project, so only agility_SOURCE_DIR is defined, no target.

# Order matters: Sandbox links against Janus, so Janus must be defined first.
add_subdirectory(Janus)   # the engine, built as a static library
add_subdirectory(Sandbox) # the test application
