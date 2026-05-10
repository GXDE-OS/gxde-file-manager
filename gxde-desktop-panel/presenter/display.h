/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#pragma once

#include <QObject>
#include "../global/singleton.h"

class QScreen;
class DBusDisplay;
class DesktopDisplay: public QObject, public Singleton<DesktopDisplay>
{
    Q_OBJECT

    friend class Singleton<DesktopDisplay>;
public:
    explicit DesktopDisplay(QObject *parent = 0);

    QScreen *primaryScreen();

signals:
    void primaryScreenChanged(QScreen *screen);

private:
    DBusDisplay *m_display = nullptr;
};

