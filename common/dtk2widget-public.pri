# The installed DTK qmake module exports DTK's own private Qt build dependencies.
# Consumers need only its public headers/libraries, as in our CMake pkg-config path.
CONFIG += link_pkgconfig
PKGCONFIG += dtk2widget
QT += core gui widgets dbus network concurrent xml
