#-------------------------------------------------
#
# Project created by QtCreator 2026-11-20T00:00:00
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = dde-deb-preview-plugin
TEMPLATE = lib
CONFIG += c++17 plugin

include(../../../common/common.pri)

greaterThan(QT_MAJOR_VERSION, 5) {
    include(../../../common/dtk2widget-public.pri)
    DEFINES += DFM_USE_QT6
} else {
    PKGCONFIG += dtkwidget
}


SOURCES += \
    main.cpp \
    debpreview.cpp

HEADERS += \
    debpreview.h
DISTFILES += \
    dde-deb-preview-plugin.json

PLUGIN_INSTALL_DIR = $$PLUGINDIR/previews

DESTDIR = $$top_srcdir/plugins/previews

unix {
    target.path = $$PLUGIN_INSTALL_DIR
    INSTALLS += target
}
