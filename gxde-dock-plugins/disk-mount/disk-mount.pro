PREFIX = /usr
QT              += core widgets concurrent dbus
TEMPLATE         = lib
CONFIG          += plugin c++17 link_pkgconfig
PKGCONFIG       += gio-2.0
greaterThan(QT_MAJOR_VERSION, 5) {
    QT += dtk2widget
    PKGCONFIG += udisks2-qt6
    DEFINES += DFM_USE_QT6
} else {
    PKGCONFIG += dtkwidget udisks2-qt5
}

INCLUDEPATH += /usr/include/gxde-dock
INCLUDEPATH += $$PWD/../../gxde-file-manager-lib/interfaces \
               $$PWD/../../gxde-file-manager-lib

TARGET          = $$qtLibraryTarget(dde-disk-mount-plugin)
DESTDIR          = $$_PRO_FILE_PWD_/../
DISTFILES       += disk-mount.json

DEFINES += QT_MESSAGELOGCONTEXT DFM_NO_FILE_WATCHER

#unix: LIBS += -L$$OUT_PWD/../../gxde-file-manager-lib -lgxde-file-manager

HEADERS += \
    diskmountplugin.h \
    diskcontrolwidget.h \
    diskpluginitem.h \
    diskcontrolitem.h \
    $$PWD/../../gxde-file-manager-lib/interfaces/dfmsettings.h \
    $$PWD/../../gxde-file-manager-lib/interfaces/dfmstandardpaths.h \
    $$PWD/../../gxde-file-manager-lib/interfaces/durl.h \
    dattacheddeviceinterface.h \
    dattachedudisks2device.h \
    dattachedvfsdevice.h

SOURCES += \
    diskmountplugin.cpp \
    diskcontrolwidget.cpp \
    diskpluginitem.cpp \
    diskcontrolitem.cpp \
    $$PWD/../../gxde-file-manager-lib/interfaces/dfmsettings.cpp \
    $$PWD/../../gxde-file-manager-lib/interfaces/dfmstandardpaths.cpp \
    $$PWD/../../gxde-file-manager-lib/interfaces/durl.cpp \
    dattachedudisks2device.cpp \
    dattachedvfsdevice.cpp

TRANSLATIONS += $$PWD/translations/$${TARGET}.ts
TR_EXCLUDE += $$PWD/../../gxde-file-manager-lib/configure/*

# Automating generation .qm files from .ts files
CONFIG(release, debug|release) {
    !system($$PWD/../../gxde-file-manager-lib/generate_translations.sh): error("Failed to generate translation")
#    DEFINES += QT_NO_DEBUG_OUTPUT
}

translations.path = $${PREFIX}/share/$${TARGET}/translations
translations.files = translations/*.qm

target.path = $${PREFIX}/lib/gxde-dock/plugins/system-trays/
INSTALLS += target translations

include($$PWD/../../gxde-file-manager-lib/interfaces/vfs/vfs.pri)

RESOURCES += \
    resources.qrc \
    $$PWD/../../gxde-file-manager-lib/configure.qrc
