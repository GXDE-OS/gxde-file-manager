TEMPLATE = subdirs

SUBDIRS += \
    dde-image-preview-plugin \
    dde-pdf-preview-plugin \
    dde-text-preview-plugin \
    dde-music-preview-plugin

ARCH = $$QMAKE_HOST.arch

!CONFIG(DISABLE_FFMPEG):!isEqual(BUILD_MINIMUM, YES) {
    !isEqual(ARCH, sw_64):!isEqual(ARCH, mips64):!isEqual(ARCH, mips32) {
        # libgxmr 当前仅有 Qt5 版本，Qt6 编译时禁用此插件
        lessThan(QT_MAJOR_VERSION, 6) {
            SUBDIRS += dde-video-preview-plugin
        } else {
            message("Skipping dde-video-preview-plugin: libgxmr has no Qt6 port")
        }
    }
}
