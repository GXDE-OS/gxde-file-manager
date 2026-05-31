/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "desktop.h"

#include <QDebug>
#include <QApplication>
#include <QStandardPaths>
#include <QStyleOptionViewItem>
#include <QDir>
#include <QDBusConnection>
#include <QScreen>

#include <durl.h>

#include "view/canvasgridview.h"
#include "view/backgroundhelper.h"
#include "presenter/apppresenter.h"

#include "../gxde-wallpaper-chooser/frame.h"

#ifndef DISABLE_ZONE
#include "../gxde-zone/mainwindow.h"
#endif

#include "util/xcb/xcb.h"
#include "util/wayland/layershellhelper.h"

using WallpaperSettings = Frame;

#ifndef DISABLE_ZONE
using ZoneSettings = ZoneMainWindow;
#endif

class DesktopPrivate
{
public:
    CanvasGridView      screenFrame;
    BackgroundHelper *background = nullptr;
    WallpaperSettings *wallpaperSettings{ nullptr };

#ifndef DISABLE_ZONE
    ZoneSettings *zoneSettings { nullptr };
#endif
};

Desktop::Desktop()
    : d(new DesktopPrivate)
{
    d->background = new BackgroundHelper();

    connect(d->background, &BackgroundHelper::enableChanged, this, &Desktop::onBackgroundEnableChanged);
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &Desktop::onBackgroundEnableChanged);
    connect(d->background, &BackgroundHelper::aboutDestoryBackground, this, [this] (QLabel *l) {
        if (l == d->screenFrame.parent()) {
            d->screenFrame.setParent(nullptr);
        }
    }, Qt::DirectConnection);
    // 任意控件改变位置都可能会引起主窗口被其它屏幕上的窗口所遮挡
    connect(d->background, &BackgroundHelper::backgroundGeometryChanged, this, &Desktop::onBackgroundGeometryChanged);
    onBackgroundEnableChanged();
}

Desktop::~Desktop()
{

}

static void setWindowFlag(QWidget *w, Qt::WindowType flag, bool on)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 9, 0)
    if (on) {
        w->setWindowFlags(w->windowFlags() | flag);
    } else {
        w->setWindowFlags(w->windowFlags() & ~flag);
    }
#else
    w->setWindowFlag(flag, on);
#endif
}
#include <QPainter>
void Desktop::onBackgroundEnableChanged()
{
    qInfo() << "Primary Screen:" << qApp->primaryScreen();
    qInfo() << "Background enabled:" << d->background->isEnabled();

    if (d->background->isEnabled()) {
        QLabel *background = d->background->backgroundForScreen(qApp->primaryScreen());
        d->screenFrame.setAttribute(Qt::WA_NativeWindow, false);
        d->screenFrame.setParent(background);
        d->screenFrame.move(0, 0);

        // 桌面图标区域也收到鼠标事件后会触发activateOnMousePress
        // 这会间接激活其父窗口，也就是壁纸，需要设置WA_ShowWithoutActivating
        d->screenFrame.setAttribute(Qt::WA_ShowWithoutActivating, true);
        d->screenFrame.show();

        // 防止复制模式下主屏窗口被遮挡
        // 目前只给X11用，Wayland下走layer-shell-qt5管理
        // 在Wayland上手动把界面提到前面会让壁纸层被提到其它程序窗口的前面
        if (qgetenv("XDG_SESSION_TYPE") != "wayland") {
            background->activateWindow();
            QMetaObject::invokeMethod(background, "raise", Qt::QueuedConnection);
        }

        // 隐藏完全重叠的窗口
        if (qgetenv("XDG_SESSION_TYPE") != "wayland") {
            for (QLabel *l : d->background->allBackgrounds()) {
                if (l != background) {
                    Xcb::XcbMisc::instance().set_window_transparent_input(l->winId(), true);
                    l->setVisible(!background->geometry().contains(l->geometry()));
                } else {
                    Xcb::XcbMisc::instance().set_window_transparent_input(l->winId(), false);
                    l->show();
                }
            }
        }
    } else {
        d->screenFrame.setParent(nullptr);
        setWindowFlag(&d->screenFrame, Qt::FramelessWindowHint, true);
        d->screenFrame.QWidget::setGeometry(qApp->primaryScreen()->geometry());

        // 无壁纸模式：screenFrame 自身作为 LayerBackground 窗口，
        // 需要阻止鼠标点击触发窗口激活，免得窗口被点击后被带到窗口前面
        d->screenFrame.setAttribute(Qt::WA_ShowWithoutActivating, true);
        if (Wayland::LayerShellHelper::isWayland()) {
            // Treelan的Wayland会话支持
            Wayland::LayerShellHelper::setDesktopRole(
                &d->screenFrame, qApp->primaryScreen(),
                QStringLiteral("dde-shell/desktop"));
        } else {
            // 传统X11会话支持
            Xcb::XcbMisc::instance().set_window_type(d->screenFrame.winId(), Xcb::XcbMisc::Desktop);
        }

        if (QWindow* window = d->screenFrame.windowHandle()) {
            window->setOpacity(0.99);
        }
        d->screenFrame.show();
    }
}

void Desktop::onBackgroundGeometryChanged(QWidget *l)
{
    QWidget *primaryBackground = d->screenFrame.parentWidget();

    if (!primaryBackground) {
        return;
    }

    qInfo() << "primaryBackground widget geometry: " << primaryBackground->geometry()
            << "changedBackground widget geometry:" << l->geometry();

    if (qgetenv("XDG_SESSION_TYPE") != "wayland") {
        // 仅在传统X11会话下允许主动把窗体raise到前面
        // 原因请阅读Desktop::onBackgroundEnableChanged()函数的注释
        primaryBackground->activateWindow();
        QMetaObject::invokeMethod(primaryBackground, "raise", Qt::QueuedConnection);
    }

    if (l != primaryBackground && primaryBackground->geometry().contains(l->geometry())) {
        l->hide();
    } else {
        l->show();
    }
}

void Desktop::loadData()
{
    Presenter::instance()->init();
}

void Desktop::loadView()
{
    d->screenFrame.initRootUrl();
}

void Desktop::showWallpaperSettings()
{
    if (d->wallpaperSettings) {
        d->wallpaperSettings->deleteLater();
        d->wallpaperSettings = nullptr;
    }

    d->wallpaperSettings = new WallpaperSettings;
    connect(d->wallpaperSettings, &Frame::done, this, [ = ] {
        d->wallpaperSettings->deleteLater();
        d->wallpaperSettings = nullptr;
    });
    connect(d->wallpaperSettings, &Frame::aboutHide, this, [this] {
        const QString &desktopImage = d->wallpaperSettings->desktopBackground();

        if (!desktopImage.isEmpty())
            d->background->setBackground(desktopImage);
    }, Qt::DirectConnection);

    d->wallpaperSettings->show();
    d->wallpaperSettings->grabKeyboard();
}

#ifndef DISABLE_ZONE
void Desktop::showZoneSettings()
{
    if (d->zoneSettings) {
        d->zoneSettings->deleteLater();
        d->zoneSettings = nullptr;
    }

    d->zoneSettings = new ZoneSettings;
    connect(d->zoneSettings, &ZoneMainWindow::finished, this, [ = ] {
        d->zoneSettings->deleteLater();
        d->zoneSettings = nullptr;
    });

    d->zoneSettings->show();
    d->zoneSettings->grabKeyboard();
}
#endif

void Desktop::initDebugDBus(QDBusConnection &conn)
{
    if (!conn.registerObject(DesktopCanvasPath, &d->screenFrame,
                             QDBusConnection::ExportScriptableSlots)) {
        qDebug() << "registerObject Failed" << conn.lastError();
        exit(0x0004);
    }
}

CanvasGridView *Desktop::getView()
{
    return (&(d->screenFrame));
}

void Desktop::Show()
{
    d->screenFrame.show();
}

void Desktop::EnableBackground(bool v)
{
    if (d && d->background) {
        d->background->setEnabled(v);
        d->background->onWMChanged();
    }
}
