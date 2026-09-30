TEMPLATE = subdirs

SUBDIRS += \
    dde-image-preview-plugin \
    dde-pdf-preview-plugin \
    dde-text-preview-plugin \
    dde-music-preview-plugin

ARCH = $$QMAKE_HOST.arch

!isEqual(ARCH, i386):!isEqual(ARCH, i686) {
    SUBDIRS += dde-ofd-preview-plugin
}

!CONFIG(DISABLE_FFMPEG):!isEqual(BUILD_MINIMUM, YES) {
    !isEqual(ARCH, sw_64):!isEqual(ARCH, mips64):!isEqual(ARCH, mips32) {
        SUBDIRS += dde-video-preview-plugin
    }
}
