# Test application that exercises the engine. It owns main().
add_executable(Sandbox src/main.cpp)

# Same Solution Explorer folder-tree pattern as the Janus target.
get_target_property(SANDBOX_SOURCES Sandbox SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${SANDBOX_SOURCES})

# PRIVATE: Sandbox is an end product, nothing links against it.
target_link_libraries(Sandbox PRIVATE Janus)

# Copy Assets/ next to the exe after each build, so GetAssetPath() finds them
# no matter where the exe is launched from.
# ponytail: runs only when Sandbox relinks, so editing an asset alone does not
# copy it. Switch to an ALL custom target if that bites; B14 hot reload will
# read from the source folder instead.
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Assets
            $<TARGET_FILE_DIR:Sandbox>/Assets)

# Agility SDK runtime goes in D3D12/ beside the exe, where D3D12SDKPath will
# point. The debug layer DLL must come from the same package or the debug
# layer fails to load.
# copy_if_different does not create the destination folder, so make it first.
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Sandbox>/D3D12
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${agility_SOURCE_DIR}/build/native/bin/x64/D3D12Core.dll
            ${agility_SOURCE_DIR}/build/native/bin/x64/d3d12SDKLayers.dll
            $<TARGET_FILE_DIR:Sandbox>/D3D12)
