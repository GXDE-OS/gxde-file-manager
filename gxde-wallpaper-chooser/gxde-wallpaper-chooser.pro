PANEL_DIR = $$PWD/../gxde-desktop-panel

include($$PANEL_DIR/gxde-desktop-panel-build.pri)
include($$PANEL_DIR/util/util.pri)
include($$PWD/gxde-wallpaper-chooser.pri)

QT       += core gui widgets svg dbus x11extras network concurrent \
            multimediawidgets multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MINOR_VERSION, 7): QT += gui-private
else: QT += platformsupport-private

TEMPLATE    = app
TARGET      = gxde-wallpaper-chooser-wayland
CONFIG      += c++11 link_pkgconfig
PKGCONFIG   += xcb xcb-ewmh xcb-shape gsettings-qt dframeworkdbus
LIBS        += -lLayerShellQtInterface

INCLUDEPATH += $$PANEL_DIR \
               $$PANEL_DIR/view

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
