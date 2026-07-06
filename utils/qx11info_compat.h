// SPDX-FileCopyrightText: 2026 CharOfString
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt6没QtX11Extras/X11Info头，做了点所需要的最小实现。
// Qt5下还是接着用QtX11Extras/X11Info头，毕竟人家有。

#ifndef UTILS_QX11INFO_COMPAT_H_
#define UTILS_QX11INFO_COMPAT_H_

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <qpa/qplatformnativeinterface.h>

#include <X11/Xlib.h>

extern "C" {
    #include <xcb/xcb.h>
}

class QX11Info {
  public:
    // Qt6没有QX11Info::isPlatformX11()，用平台插件名判定
    static bool isPlatformX11() {
        if (auto* app = qGuiApp) {
            return app->platformName() == QLatin1String("xcb");
        }
        return false;
    }

    static Display* display() {
        if (auto* app = qGuiApp) {
            if (auto* ni = app->platformNativeInterface()) {
                return reinterpret_cast<Display *>(ni->
                    nativeResourceForIntegration("display"));
            }
        }

        return nullptr;
    }

    static xcb_connection_t* connection() {
        if (auto* app = qGuiApp) {
            if (auto* ni = app->platformNativeInterface()) {
                return reinterpret_cast<xcb_connection_t *>(ni->
                    nativeResourceForIntegration("connection"));
            }
        }

        return nullptr;
    }

    static int appScreen() {
        if (auto *primary = QGuiApplication::primaryScreen()) {
            return QGuiApplication::screens().indexOf(primary);
        }

        return 0;
    }

    static unsigned long appRootWindow(int screen = -1) {
        Display *d = display();
        if (!d) {
            return 0;
        }

        if (screen < 0) {
            screen = DefaultScreen(d);
        }

        return RootWindow(d, screen);
    }

    static unsigned long appTime() {
        if (auto* app = qGuiApp) {
            if (auto* ni = app->platformNativeInterface()) {
                return reinterpret_cast<quintptr>(ni->
                    nativeResourceForScreen("apptime",
                        QGuiApplication::primaryScreen()));
            }
        }
        return 0;
    }

    static unsigned long appUserTime() {
        if (auto* app = qGuiApp) {
            if (auto* ni = app->platformNativeInterface()) {
                return reinterpret_cast<quintptr>(ni->
                    nativeResourceForScreen("appusertime",
                        QGuiApplication::primaryScreen()));
            }
        }
        return 0;
    }

    static void setAppTime(unsigned long time) {
        if (auto* app = qGuiApp) {
            if (auto* ni = app->platformNativeInterface()) {
                typedef void (*SetAppTimeFunc)(QScreen *, xcb_timestamp_t);
                if (auto fn = reinterpret_cast<SetAppTimeFunc>(ni
                        ->nativeResourceFunctionForScreen("setapptime"))) {
                    fn(QGuiApplication::primaryScreen(),
                        static_cast<xcb_timestamp_t>(time));
                }
            }
        }
    }

    // 简化实现：原 Qt5 用法仅用于唤起子进程的时间戳，appUserTime即可。
    static unsigned long getTimestamp() {
        return appUserTime();
    }
};

#else  // Qt5下直接用可用的QX11Info头

#include <QX11Info>

#endif

#endif  // UTILS_QX11INFO_COMPAT_H_
