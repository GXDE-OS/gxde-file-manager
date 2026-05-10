#-------------------------------------------------
#
# Project created by QtCreator 2015-06-24T09:14:17
#
#-------------------------------------------------

include(../common/common.pri)

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += network

isEmpty(TARGET) {
    TARGET = $$ProjectName
}

TEMPLATE = app
CONFIG += c++17 link_pkgconfig
PKGCONFIG += gio-unix-2.0
greaterThan(QT_MAJOR_VERSION, 5) {
    QT += dtk2widget core5compat
    DEFINES += DFM_USE_QT6
} else {
    PKGCONFIG += dtkwidget
}

DEFINES += QMAKE_TARGET=\\\"$$TARGET\\\" QMAKE_VERSION=\\\"$$VERSION\\\"

DEFINES += QT_MESSAGELOGCONTEXT

isEmpty(QMAKE_ORGANIZATION_NAME) {
    DEFINES += QMAKE_ORGANIZATION_NAME=\\\"deepin\\\"
}

isEmpty(PREFIX){
    PREFIX = /usr
}



CONFIG(release, debug|release) {
    !system($$PWD/translate_ts2desktop.sh): error("Failed to generate translation")
}

CONFIG(debug, debug|release) {
#    LIBS += -lprofiler
#    DEFINES += ENABLE_PPROF
}

SOURCES += \
    main.cpp \
    filemanagerapp.cpp \
    logutil.cpp \
    singleapplication.cpp \
    commandlinemanager.cpp

INCLUDEPATH += $$PWD/../gxde-file-manager-lib $$PWD/.. \
               $$PWD/../utils \
               $$PWD/../gxde-file-manager-lib/interfaces

BINDIR = $$PREFIX/bin
DEFINES += APPSHAREDIR=\\\"$$PREFIX/share/$$TARGET\\\"

target.path = $$BINDIR

desktop.path = $${PREFIX}/share/applications/
isEqual(ARCH, sw_64) | isEqual(ARCH, mips64) | isEqual(ARCH, mips32) {
    desktop.files = $$PWD/mips/$${TARGET}.desktop
}else{
    desktop.files = $${TARGET}.desktop
}

gxde-xdg-user-dirs-update.path = $$BINDIR
gxde-xdg-user-dirs-update.files = $$PWD/gxde-xdg-user-dirs-update

policy.path = $${PREFIX}/share/polkit-1/actions/
policy.files = pkexec/com.deepin.pkexec.gxde-file-manager.policy

pkexec.path = /usr/bin/
pkexec.files = pkexec/gxde-file-manager-pkexec

propertyDialogShell.path = /usr/bin/
propertyDialogShell.files = gxde-property-dialog



INSTALLS += target desktop gxde-xdg-user-dirs-update policy pkexec propertyDialogShell

isEqual(ARCH, sw_64) | isEqual(ARCH, mips64) | isEqual(ARCH, mips32) {
    dde-mips-shs.path = $$BINDIR
    dde-mips-shs.files = $$PWD/mips/gxde-computer.sh \
                         $$PWD/mips/gxde-trash.sh \
                         $$PWD/mips/file-manager.sh

    gxde-file-manager-autostart.path = /etc/xdg/autostart
    gxde-file-manager-autostart.files = $$PWD/mips/gxde-file-manager-autostart.desktop gxde-file-manager-dialog-autostart.desktop
    INSTALLS += dde-mips-shs gxde-file-manager-autostart
}else{
    autostart.path = /etc/xdg/autostart
    autostart.files = gxde-file-manager-xdg-autostart.desktop gxde-file-manager-dialog-autostart.desktop
    INSTALLS += autostart
}

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../gxde-file-manager-lib/release -lgxde-file-manager
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../gxde-file-manager-lib/debug -lgxde-file-manager
else:unix: LIBS += -L$$OUT_PWD/../gxde-file-manager-lib -lgxde-file-manager

CONFIG(debug, debug|release) {
    DEPENDPATH += $$PWD/../gxde-file-manager-lib
    unix:QMAKE_RPATHDIR += $$OUT_PWD/../gxde-file-manager-lib
}

HEADERS += \
    filemanagerapp.h \
    logutil.h \
    singleapplication.h \
    commandlinemanager.h

DISTFILES += \
    mips/gxde-file-manager-autostart.desktop \
    mips/gxde-file-manager.desktop \
    mips/gxde-computer.sh \
    mips/gxde-trash.sh \
    mips/file-manager.sh \
    generate_translations.sh

TRANSLATIONS += $$PWD/translations/$${TARGET}-app.ts \
    $$PWD/translations/$${TARGET}-app_zh_CN.ts

translations.path = $$APPSHAREDIR/translations
translations.files = translations/*.qm

INSTALLS += translations

CONFIG(release, debug|release) {
    !system($$PWD/generate_translations.sh): error("Failed to generate translation")
}
