include(../common/common.pri)

QT          += core gui widgets

TEMPLATE    = app
TARGET      = gxde-rename-interface-treeland
CONFIG      += c++17 link_pkgconfig
greaterThan(QT_MAJOR_VERSION, 5) {
    # Qt6: DApplication/DDialog/DLineEdit come from dtk2widget,
    # QTextCodec lives in core5compat
    QT += dtk2widget core5compat
} else {
    PKGCONFIG += dtkwidget
}

SOURCES     += main.cpp

isEmpty(PREFIX): PREFIX = /usr

# 翻译：从 .ts 生成 .qm(中文等)，源串为英文(无译文时回退英文)
# 用qmake解析出的lrelease绝对路径，别用裸lrelease：后者是qtchooser提供的
# /usr/bin/lrelease，在deb构建chroot里不保证存在，会导致生成翻译失败
TRANSLATIONS += $$PWD/translations/$${TARGET}_zh_CN.ts
QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
CONFIG(release, debug|release) {
    !system($$QMAKE_LRELEASE $$PWD/translations/*.ts): error("Failed to generate translation")
}

target.path = $$PREFIX/bin/
translations.path = $$PREFIX/share/$${TARGET}/translations
translations.files = translations/*.qm
INSTALLS += target translations
