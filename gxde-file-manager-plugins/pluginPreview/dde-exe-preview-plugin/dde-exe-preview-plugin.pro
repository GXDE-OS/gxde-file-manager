#-------------------------------------------------
#
# Project created by QtCreator 2026-11-21T00:00:00
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = dde-exe-preview-plugin
TEMPLATE = lib
CONFIG += c++17 plugin

# 插件由文件管理器以 dlopen 方式加载：链接 jemalloc 后会因
# “无法在静态 TLS 块中分配内存”导致整个插件加载失败（空格预览失效），
# 因此参照 debian/rules 对 Qt6 的处理，在此显式禁用
DISABLE_JEMALLOC = 1

include(../../../common/common.pri)

greaterThan(QT_MAJOR_VERSION, 5) {
    include(../../../common/dtk2widget-public.pri)
    DEFINES += DFM_USE_QT6
} else {
    PKGCONFIG += dtkwidget
}


SOURCES += \
    main.cpp \
    exepreview.cpp

HEADERS += \
    exepreview.h
DISTFILES += \
    dde-exe-preview-plugin.json

PLUGIN_INSTALL_DIR = $$PLUGINDIR/previews

DESTDIR = $$top_srcdir/plugins/previews

unix {
    target.path = $$PLUGIN_INSTALL_DIR
    INSTALLS += target
}
