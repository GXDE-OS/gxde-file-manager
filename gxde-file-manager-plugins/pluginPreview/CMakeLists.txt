add_subdirectory(dde-image-preview-plugin)
add_subdirectory(dde-pdf-preview-plugin)
add_subdirectory(dde-text-preview-plugin)
add_subdirectory(dde-music-preview-plugin)

if(NOT DISABLE_FFMPEG AND NOT BUILD_MINIMUM
   AND NOT GXDE_ARCH MATCHES "^(sw_64|mips64|mips32)$"
   AND GXDE_QT_VERSION_MAJOR LESS 6)
    add_subdirectory(dde-video-preview-plugin)
endif()
