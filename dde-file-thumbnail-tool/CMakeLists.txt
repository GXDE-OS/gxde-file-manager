if(NOT DISABLE_FFMPEG AND NOT BUILD_MINIMUM)
    add_subdirectory(video)
endif()

# OFD 缩略图工具依赖 librofd-ffi，在 32 位 i386 架构下不构建
if(NOT GXDE_ARCH MATCHES "^(i386|i[3-6]86)$")
    add_subdirectory(ofd)
endif()
