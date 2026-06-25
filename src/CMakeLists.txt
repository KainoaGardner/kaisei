# Core library
add_library(kaisei-core
    core/module.cpp
    core/preset.cpp
    core/moduleLoader.cpp
    core/presetLoader.cpp
    core/registry.cpp
)

target_include_directories(kaisei-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kaisei-core PUBLIC tomlplusplus::tomlplusplus spdlog::spdlog)

# Fusion library (TODO: implement these files)
# add_library(kaisei-fusion
#     fusion/shaderAnalyzer.cpp
#     fusion/shaderFusion.cpp
#     fusion/dependencyGraph.cpp
#     fusion/optimizationPass.cpp
# )

# target_include_directories(kaisei-fusion PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${GLSLANG_INCLUDE_DIRS}
#     ${SPIRV_TOOLS_INCLUDE_DIRS}
# )
# target_link_libraries(kaisei-fusion PUBLIC
#     kaisei-core
#     ${GLSLANG_LIBRARIES}
#     ${SPIRV_TOOLS_LIBRARIES}
#     spdlog::spdlog
# )

# Backend library - Rendering API abstraction (OpenGL/Vulkan)
add_library(kaisei-backend
    backend/openGLBackend.cpp
)

target_include_directories(kaisei-backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kaisei-backend PUBLIC
    OpenGL::GL
    glad
    glfw
    stb_image
    spdlog::spdlog
)

# Renderer library - Multi-pass rendering
add_library(kaisei-renderer
    renderer/multiPassRenderer.cpp
)

target_include_directories(kaisei-renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kaisei-renderer PUBLIC
    kaisei-core
    kaisei-backend
    spdlog::spdlog
)

# Integration library - Preview window and rendering
add_library(kaisei-integration
    integration/preview/previewWindow.cpp
    integration/preview/previewRenderer.cpp
)

target_include_directories(kaisei-integration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kaisei-integration PUBLIC
    kaisei-core
    kaisei-backend
    kaisei-renderer
    glfw
    spdlog::spdlog
)

# Integration library - Output targets (Preview/Hyprland/Steam Frame) (TODO)
# add_library(kaisei-integration
#     integration/preview/previewWindow.cpp
#     integration/preview/previewRenderer.cpp
#     integration/preview/inputHandler.cpp
#     integration/preview/hotReload.cpp
#     integration/preview/utils/fileWatcher.cpp
#     integration/preview/utils/logger.cpp
#     integration/preview/utils/config.cpp
# )

# target_include_directories(kaisei-integration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(kaisei-integration PUBLIC
#     kaisei-core
#     kaisei-fusion
#     kaisei-backend
#     glfw
#     spdlog::spdlog
#     tomlplusplus::tomlplusplus
# )

# CLI library
add_library(kaisei-cli
    cli/cli.cpp
    cli/moduleCommands.cpp
    cli/presetCommands.cpp
    cli/previewCommands.cpp
)

target_include_directories(kaisei-cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kaisei-cli PUBLIC
    kaisei-core
    kaisei-integration
    CLI11::CLI11
    spdlog::spdlog
)

# Main executable
add_executable(kaisei main.cpp)
target_link_libraries(kaisei PRIVATE kaisei-cli)

# Install executable
install(TARGETS kaisei DESTINATION bin)
