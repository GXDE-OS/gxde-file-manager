cmake_minimum_required(VERSION 3.16)

project(gxde-file-manager
    VERSION 1.8.2
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(BUILD_MINIMUM "Build minimum subset (skip optional plugins)" OFF)
option(DISABLE_FFMPEG "Disable FFmpeg dependent components" OFF)
option(DISABLE_JEMALLOC "Disable jemalloc dependency" OFF)
option(GXDE_DISABLE_ANYTHING "Disable deepin-anything based quick search" ON)

set(GXDE_VERSION "${PROJECT_VERSION}")
include(GXDECommon)

add_subdirectory(gxde-file-manager-lib)
add_subdirectory(gxde-file-manager)
add_subdirectory(gxde-file-manager-daemon)
add_subdirectory(gxde-file-manager-plugins)
add_subdirectory(gxde-dock-plugins)
add_subdirectory(gxde-desktop-panel)
add_subdirectory(dde-file-thumbnail-tool)

if(NOT BUILD_MINIMUM)
    add_subdirectory(usb-device-formatter)
endif()
