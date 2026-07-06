#-------------------------------------------------
#
# Project created by QtCreator 2015-06-24T09:14:17
#
#-------------------------------------------------

include($$PWD/gxde-desktop-panel-build.pri)
include($$PWD/util/util.pri)
include($$PWD/dbus/dbus.pri)

include($$PWD/../gxde-wallpaper-chooser/gxde-wallpaper-chooser.pri)
include($$PWD/../gxde-zone/gxde-zone.pri)

QT       += core gui widgets svg dbus network concurrent multimediawidgets multimedia
lessThan(QT_MAJOR_VERSION, 6): QT += x11extras
QT       += gui-private

TEMPLATE    = app
TARGET      = gxde-desktop-panel
DESTDIR     = $$BUILD_DIST
CONFIG      += c++17 link_pkgconfig
PKGCONFIG   += xcb xcb-ewmh xcb-shape
LIBS        += -lLayerShellQtInterface
greaterThan(QT_MAJOR_VERSION, 5) {
    QT += dtk2widget core5compat
    PKGCONFIG += gsettings-qt6
    INCLUDEPATH += /usr/include/libdframeworkdbus-qt6-6.0
    LIBS += -ldframeworkdbus-qt6
    DEFINES += DFM_USE_QT6
} else {
    PKGCONFIG += gsettings-qt dframeworkdbus
}

INCLUDEPATH += $$PWD/../gxde-file-manager-lib\
               $$PWD/../utils \
              $$PWD/../gxde-file-manager-lib/interfaces

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../gxde-file-manager-lib/release -lgxde-file-manager
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../gxde-file-manager-lib/debug -lgxde-file-manager
else:unix: LIBS += -L$$OUT_PWD/../gxde-file-manager-lib -lgxde-file-manager

CONFIG(debug, debug|release) {
    DEPENDPATH += $$PWD/../gxde-file-manager-lib
    unix:QMAKE_RPATHDIR += $$OUT_PWD/../gxde-file-manager-lib
}

SOURCES += \
    main.cpp \
    config/config.cpp \
    desktop.cpp \
    view/canvasviewhelper.cpp \
#    view/canvasview.cpp \
    model/dfileselectionmodel.cpp \
    view/canvasgridview.cpp \
    presenter/apppresenter.cpp \
    presenter/gridmanager.cpp \
    dbus/dbusdisplay.cpp \
    presenter/display.cpp \
    presenter/dfmsocketinterface.cpp \
    view/watermaskframe.cpp \
    dbus/dbusdock.cpp \
    config/profile.cpp \
    view/desktopitemdelegate.cpp \
    presenter/gridcore.cpp \
    config/dfmdesktopsettings.cpp \
    view/backgroundhelper.cpp



HEADERS += \
    config/config.h \
    desktop.h \
    view/canvasviewhelper.h \
    model/dfileselectionmodel.h \
    view/private/canvasviewprivate.h \
    global/coorinate.h \
    global/singleton.h \
    view/canvasgridview.h \
    presenter/apppresenter.h \
    presenter/gridmanager.h \
    dbus/dbusdisplay.h \
    presenter/display.h \
    presenter/dfmsocketinterface.h \
    view/watermaskframe.h \
    dbus/dbusdock.h \
    config/profile.h \
    view/desktopitemdelegate.h \
    presenter/gridcore.h \
    deventfilter.h \
    config/dfmdesktopsettings.h \
    view/backgroundhelper.h

RESOURCES += \
    resource/theme/theme.qrc

# Automating generation .qm files from .ts files

CONFIG(release, debug|release) {
    !system($$PWD/translate_generation.sh): error("Failed to generate translation")
    !system($$PWD/translate_ts2desktop.sh): error("Failed to generate translation")
}

TRANSLATIONS += $$PWD/translations/$${TARGET}.ts \
                $$PWD/translations/$${TARGET}_zh_CN.ts


include($$PWD/development.pri)
