cmake_minimum_required(VERSION 3.25.0)

# Ensure build artifacts and dependencies are stored in /build, while keeping the .sln in the root
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_CACHEFILE_DIR ${CMAKE_BINARY_DIR})
set(CMAKE_TEMP_DIR ${CMAKE_BINARY_DIR})

# Ensure all CPM dependencies & lock files go into /build
set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/CPM")

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type: Debug, Release" FORCE)
endif()

# Set compiler flags for Release and Debug; -g for debug symbols.
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -g")

# Project name
project(CHIP8)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)  # Static libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)  # Shared libraries

# Ensure per-configuration output directories
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# CPM.cmake Dependency
set(CPM_DOWNLOAD_VERSION "0.38.7") # Use a specific version of CPM
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

# Try downloading using CMake's file(DOWNLOAD)
file(DOWNLOAD
    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
    "${CPM_DOWNLOAD_LOCATION}"
    SHOW_PROGRESS LOG ${CMAKE_BINARY_DIR}/cpm_download.log STATUS cpm_download_status
)

if(EXISTS "${CPM_DOWNLOAD_LOCATION}")
     include(${CPM_DOWNLOAD_LOCATION})
else()
     message(FATAL_ERROR "CPM.cmake could not be downloaded or found at ${CPM_DOWNLOAD_LOCATION}.")
endif()

# Variable to collect library targets to link against
set(LIBS)

## --- Library Dependencies ---

# SDL3
CPMAddPackage("gh:libsdl-org/SDL#release-3.2.10")
list(APPEND LIBS SDL3::SDL3)

# glm
CPMAddPackage("gh:g-truc/glm#1.0.1")
list(APPEND LIBS glm::glm-header-only)

# json
CPMAddPackage("gh:nlohmann/json#v3.11.3")
list(APPEND LIBS nlohmann_json::nlohmann_json)

# nativefiledialog (not a CMake project)
CPMAddPackage(
    NAME nativefiledialog
    GITHUB_REPOSITORY mlabbe/nativefiledialog
    GIT_TAG release_116
    DOWNLOAD_ONLY TRUE
)

# ImGui
CPMAddPackage("gh:ocornut/imgui#v1.91.9b")
# ImGui sources are added via target_sources later, so it doesn't need to be added to LIBS variable

## --- Executable Definition ---
add_executable(CHIP8 # Using CHIP8 as project name from this file
    src/main.cpp
    src/Chip8.cpp
    src/QuirkStorage.cpp
    # Headers in add_executable are usually optional/ignored by generators
    "src/Chip8.h"
    "src/QuirkStorage.h"
    src/Application.cpp
    "src/Application.h"
)

# Set C++ Standard (C++23 as per the provided file)
target_compile_features(CHIP8 PRIVATE cxx_std_23)

# --- Include Directories ---
# Add includes needed by the CHIP8 target
target_include_directories(CHIP8 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src # Project's own source dir
    ${SDL3_INCLUDE_DIRS}            # SDL3 includes (using plural _DIRS variable)
    ${imgui_SOURCE_DIR}             # Base ImGui directory
    ${imgui_SOURCE_DIR}/backends    # ImGui backends directory
    ${nativefiledialog_SOURCE_DIR}/src/include
    # GLM is likely header-only and might add its includes via its target,
    # but explicitly adding ${glm_SOURCE_DIR} might be needed if glm::glm-header-only doesn't propagate it.
    # ${glm_SOURCE_DIR}
)

# Platform-specific NFD source (only one is needed depending on your platform)
if(WIN32)
    target_sources(CHIP8 PRIVATE
        ${nativefiledialog_SOURCE_DIR}/src/nfd_common.c
        ${nativefiledialog_SOURCE_DIR}/src/nfd_win.cpp
    )
elseif(APPLE)
    target_sources(CHIP8 PRIVATE
        ${nativefiledialog_SOURCE_DIR}/src/nfd_common.c
        ${nativefiledialog_SOURCE_DIR}/src/nfd_cocoa.m
    )
elseif(UNIX)
    target_sources(CHIP8 PRIVATE
        ${nativefiledialog_SOURCE_DIR}/src/nfd_common.c
        ${nativefiledialog_SOURCE_DIR}/src/nfd_gtk.c
    )
endif()

# --- Add ImGui Sources (Unconditionally) ---
# Check if ImGui was successfully added by CPM before adding sources
if(imgui_ADDED)
    target_sources(CHIP8 PRIVATE
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    )
endif()


# --- Linking ---
# Link libraries collected in the LIBS variable + any platform specifics
target_link_libraries(CHIP8 PRIVATE ${LIBS})

# --- Platform Specifics (As before) ---
if(WIN32)
    target_link_libraries(CHIP8 PRIVATE version shell32 user32 gdi32)
endif()
if(APPLE)
    target_link_options(CHIP8 PRIVATE "-framework Cocoa -framework Metal -framework IOKit -framework CoreVideo")
endif()
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(CHIP8 PRIVATE Threads::Threads dl m rt)
endif()

# --- Custom Command (ROMs - As before) ---
add_custom_command(
        TARGET CHIP8 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/roms"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/roms"
                COMMENT "Copying roms to output directory."
)