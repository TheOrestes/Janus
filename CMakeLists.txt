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

# Order matters: Sandbox links against Janus, so Janus must be defined first.
add_subdirectory(Janus)   # the engine, built as a static library
add_subdirectory(Sandbox) # the test application
