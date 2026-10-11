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
