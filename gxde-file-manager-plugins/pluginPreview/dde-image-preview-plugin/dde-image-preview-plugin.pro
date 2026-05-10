#-------------------------------------------------
#
# Project created by QtCreator 2017-03-31T16:00:06
#
#-------------------------------------------------

QT       += widgets

TARGET = dde-image-preview-plugin
TEMPLATE = lib

CONFIG += plugin c++17 link_pkgconfig


include(../../../common/common.pri)

greaterThan(QT_MAJOR_VERSION, 5) {
    QT += dtk2widget
    DEFINES += DFM_USE_QT6
} else {
    PKGCONFIG += dtkwidget
}


SOURCES += \
    imageview.cpp \
    main.cpp \
    imagepreview.cpp

HEADERS += \
    imageview.h \
    imagepreview.h
DISTFILES += dde-image-preview-plugin.json

PLUGIN_INSTALL_DIR = $$PLUGINDIR/previews

DESTDIR = $$top_srcdir/plugins/previews

unix {
    target.path = $$PLUGIN_INSTALL_DIR
    INSTALLS += target
}
