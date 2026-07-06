PANEL_DIR = $$PWD/../gxde-desktop-panel

include($$PANEL_DIR/gxde-desktop-panel-build.pri)
include($$PANEL_DIR/util/util.pri)
include($$PWD/gxde-wallpaper-chooser.pri)

QT       += core gui widgets svg dbus network concurrent \
            multimediawidgets multimedia
lessThan(QT_MAJOR_VERSION, 6): QT += x11extras
QT       += gui-private

TEMPLATE    = app
TARGET      = gxde-wallpaper-chooser-wayland
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

INCLUDEPATH += $$PANEL_DIR \
               $$PANEL_DIR/view \
               $$PWD/../utils

SOURCES += \
    $$PWD/main.cpp \
    $$PANEL_DIR/view/backgroundhelper.cpp

HEADERS += \
    $$PANEL_DIR/view/backgroundhelper.h

isEmpty(PREFIX) {
    PREFIX = /usr
}

target.path = $${PREFIX}/bin/
INSTALLS += target
