# Test application that exercises the engine. It owns main().
add_executable(Sandbox src/main.cpp)

# PRIVATE: Sandbox is an end product, nothing links against it.
target_link_libraries(Sandbox PRIVATE Janus)
