TEMPLATE = app
QT += core gui
CONFIG += c++17 link_pkgconfig

include(../common.pri)

SOURCES += \
    main.cpp

LIBS += -lrofd_ffi
PKGCONFIG += cairo
