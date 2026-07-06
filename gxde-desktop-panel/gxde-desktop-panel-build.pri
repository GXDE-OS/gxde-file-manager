DEFINES += QT_MESSAGELOGCONTEXT

EDITION=COMMUNITY
ARCH = $$QMAKE_HOST.arch
isEqual(ARCH, sw_64) | isEqual(ARCH, mips64) | isEqual(ARCH, mips32) {
    EDITION=RACCOON
}

isEqual(EDITION, RACCOON) {
    DEFINES += DDE_COMPUTER_TRASH
}

DEFINES += DDE_DBUS_DISPLAY

greaterThan(QT_MAJOR_VERSION, 5) {
    # Qt6: dtk2widget was already added in the main .pro.
    # DPlatformHandle (setEnabledNoTitlebarForWindow) only lives in dtk6gui/DGui,
    # whose headers collide by name with dtk2/DWidget. Add dtk6/DGui with the
    # LOWEST include priority (-idirafter) so dtk2/DWidget still wins the
    # collisions and we only pull the unique dplatformhandle.h from dtk6.
    QMAKE_CXXFLAGS += -idirafter /usr/include/dtk6/DGui
    LIBS += -ldtk6gui
} else {
    PKGCONFIG += dtkwidget dtkgui
    load(dtk_qmake)
}

# add computer/trash icon on professional system
deepin_professional: DEFINES += DDE_COMPUTER_TRASH DISABLE_AUTOMERGE

!isEmpty(DISABLE_SCREENSAVER) {
    DEFINES += DISABLE_SCREENSAVER
}

!isEmpty(DISABLE_WALLPAPER_CAROUSEL) {
    DEFINES += DISABLE_WALLPAPER_CAROUSEL
}
