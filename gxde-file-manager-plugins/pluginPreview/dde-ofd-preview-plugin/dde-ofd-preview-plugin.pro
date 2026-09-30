#-------------------------------------------------
#
# Project created for OFD preview support (rofd)
#
#-------------------------------------------------

QT       += core gui widgets concurrent

TARGET = dde-ofd-preview-plugin
TEMPLATE = lib

LIBS += -lrofd_ffi
PKGCONFIG += cairo

CONFIG += c++17 plugin link_pkgconfig

include(../../../common/common.pri)

greaterThan(QT_MAJOR_VERSION, 5) {
    QT += dtk2widget
    DEFINES += DFM_USE_QT6
} else {
    PKGCONFIG += dtkwidget
}


SOURCES += \
    ofdwidget.cpp \
    main.cpp \
    ofdpreview.cpp

HEADERS += \
    ofdwidget.h \
    ofdpreview.h
DISTFILES += dde-ofd-preview-plugin.json

PLUGIN_INSTALL_DIR = $$PLUGINDIR/previews

DESTDIR = $$top_srcdir/plugins/previews

unix {
    target.path = $$PLUGIN_INSTALL_DIR
    INSTALLS += target
}

RESOURCES += \
    theme.qrc
