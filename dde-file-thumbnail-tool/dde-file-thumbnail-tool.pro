TEMPLATE = subdirs

!CONFIG(DISABLE_FFMPEG):!isEqual(BUILD_MINIMUM, YES) {
    SUBDIRS += video
}

ARCH = $$QT_ARCH
!isEqual(ARCH, i386):!isEqual(ARCH, i686) {
    SUBDIRS += ofd
}
