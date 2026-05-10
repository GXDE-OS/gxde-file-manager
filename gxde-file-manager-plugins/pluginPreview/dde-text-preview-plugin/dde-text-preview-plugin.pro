#-------------------------------------------------
#
# Project created by QtCreator 2017-04-17T11:02:31
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = dde-text-preview-plugin
TEMPLATE = lib
CONFIG += c++17 plugin

include(../../../common/common.pri)

greaterThan(QT_MAJOR_VERSION, 5) {
    QT += dtk2widget
    DEFINES += DFM_USE_QT6
} else {
    PKGCONFIG += dtkwidget
}


SOURCES += \
    main.cpp \
    textpreview.cpp

HEADERS += \
    textpreview.h
DISTFILES += \
    dde-text-preview-plugin.json

PLUGIN_INSTALL_DIR = $$PLUGINDIR/previews

DESTDIR = $$top_srcdir/plugins/previews

unix {
    target.path = $$PLUGIN_INSTALL_DIR
    INSTALLS += target
}
