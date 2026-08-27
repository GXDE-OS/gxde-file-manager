/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "display.h"

#include <QScreen>
#include <QApplication>
#include <QtMath>

#include <dbus/dbusdisplay.h>
#include "waylandutils.h"

DesktopDisplay::DesktopDisplay(QObject *parent) : QObject(parent)
{
#ifdef DDE_DBUS_DISPLAY
    m_display = new DBusDisplay(this);
    const auto notifyDisplayGeometryChanged = [this]() {
        auto primaryName = m_display->primary();
        for (auto screen : qApp->screens()) {
            if (screen && screen->name() == primaryName) {
                emit primaryScreenChanged(screen);
                return;
            }
        }
        qCritical() << "Can not find" << primaryName << qApp->screens();
    };
    connect(m_display, &DBusDisplay::PrimaryRectChanged,
            this, notifyDisplayGeometryChanged);
    connect(m_display, &DBusDisplay::ScreenWidthChanged,
            this, notifyDisplayGeometryChanged);
    connect(m_display, &DBusDisplay::ScreenHeightChanged,
            this, notifyDisplayGeometryChanged);
#endif

    connect(qApp, &QApplication::primaryScreenChanged,
            this, &DesktopDisplay::primaryScreenChanged);
}

QRect DesktopDisplay::primaryGeometry()
{
    QScreen *screen = primaryScreen();

#ifdef DDE_DBUS_DISPLAY
    if (!WaylandUtils::isWaylandSession() && m_display && m_display->isValid()) {
        QRect rect = m_display->primaryRect();
        if (rect.isValid() && !rect.isEmpty()) {
            const qreal ratio = screen ? screen->devicePixelRatio() : 1.0;
            if (ratio > 0.0) {
                rect.setSize(QSize(qRound(rect.width() / ratio),
                                   qRound(rect.height() / ratio)));
            }
            return rect;
        }
    }
#endif

    return screen ? screen->geometry() : QRect();
}

QRect DesktopDisplay::primaryAvailableGeometry()
{
    const QRect geometry = primaryGeometry();
    QScreen *screen = primaryScreen();
    if (!screen || geometry.isEmpty()) {
        return geometry;
    }

    const QRect qtGeometry = screen->geometry();
    const QRect qtAvailable = screen->availableGeometry();
    const QMargins reserved(qtAvailable.left() - qtGeometry.left(),
                            qtAvailable.top() - qtGeometry.top(),
                            qtGeometry.right() - qtAvailable.right(),
                            qtGeometry.bottom() - qtAvailable.bottom());
    return geometry.marginsRemoved(reserved);
}

QScreen *DesktopDisplay::primaryScreen()
{
#ifdef DDE_DBUS_DISPLAY
    auto primaryName = m_display->primary();
    for (auto screen : qApp->screens()) {
        if (screen && screen->name() == primaryName) {
            return screen;
        }
    }
    qCritical() << "Can not find" << primaryName;
    return qApp->primaryScreen();
#else
    return qApp->primaryScreen();
#endif
}
