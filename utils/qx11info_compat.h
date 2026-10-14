// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <bernese/x11_info.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
class GXDEX11Info {
public:
    static bool isPlatformX11() { return bernese::X11Info::IsPlatformX11(); }
    static Display *display() { return bernese::X11Info::GetDisplay(); }
    static xcb_connection_t *connection() { return bernese::X11Info::GetConnection(); }
    static unsigned long getTimestamp() { return bernese::X11Info::GetTimestamp(); }
};
using QX11Info = GXDEX11Info;
#else
#include <QX11Info>
#endif
