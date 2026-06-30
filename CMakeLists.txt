cmake_minimum_required(VERSION 3.20)
project(kaisei VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default install prefix to ~/.local if not specified
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install prefix" FORCE)
endif()

# Options
option(KAISEI_BUILD_TESTS "Build tests" ON)
option(KAISEI_ENABLE_VULKAN "Enable Vulkan backend" OFF)
option(KAISEI_INSTALL_MODULES "Install built-in modules" ON)

# Find system packages
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(HYPRLAND REQUIRED hyprland)

# TODO: glslang and SPIRV-Tools will be needed in Phase 2 for shader fusion
# For now we'll use FetchContent when we implement the fusion engine
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(GLSLANG REQUIRED glslang)
# pkg_check_modules(SPIRV_TOOLS REQUIRED SPIRV-Tools)

# FetchContent for header-only libraries
include(FetchContent)

# tomlplusplus (header-only TOML parser)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)

# CLI11 (header-only CLI parser)
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.1
)

FetchContent_MakeAvailable(tomlplusplus cli11)

# Add glad (OpenGL function loader)
add_library(glad ${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/third_party/glad/include)
set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Add stb_image header-only library
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_SOURCE_DIR}/third_party)

# Core libraries
add_subdirectory(src)

# Tests
if(KAISEI_BUILD_TESTS)
    enable_testing()
    # add_subdirectory(tests)  # TODO: Phase 1
endif()

# Installation
if(KAISEI_INSTALL_MODULES)
    install(DIRECTORY modules/ DESTINATION share/kaisei/modules
            FILES_MATCHING PATTERN "*.toml" PATTERN "*.frag" PATTERN "*.vert")
    install(DIRECTORY shaders/ DESTINATION share/kaisei/shaders
            FILES_MATCHING PATTERN "*.glsl" PATTERN "*.vert" PATTERN "*.frag")
endif()

# Install examples
install(DIRECTORY examples/presets/ DESTINATION share/kaisei/examples/presets
        FILES_MATCHING PATTERN "*.preset")
