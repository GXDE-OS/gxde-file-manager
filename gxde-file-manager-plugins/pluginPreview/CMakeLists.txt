add_subdirectory(dde-image-preview-plugin)
add_subdirectory(dde-pdf-preview-plugin)
# OFD 预览依赖 librofd-ffi，在 32 位 i386 架构下不构建
if(NOT GXDE_ARCH MATCHES "^(i386|i[3-6]86)$")
    add_subdirectory(dde-ofd-preview-plugin)
endif()
add_subdirectory(dde-text-preview-plugin)
add_subdirectory(dde-music-preview-plugin)

if(NOT DISABLE_FFMPEG AND NOT BUILD_MINIMUM
   AND NOT GXDE_ARCH MATCHES "^(sw_64|mips64|mips32)$")
    add_subdirectory(dde-video-preview-plugin)
endif()
