/**
 * Copyright (C) 2026 CharOfString
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#ifndef LAYERSHELLHELPER_H
#define LAYERSHELLHELPER_H

#include <QString>
#include <QScreen>
#include <QWidget>

namespace Wayland {

class LayerShellHelper {
public:
    static bool isWayland();
    static bool isTreeland();
    static void setDesktopRole(QWidget* widget, QScreen* screen,
        const QString& scope);
    static void setChooserRole(QWidget* widget, QScreen* screen,
        const QString& scope);
    static void setPreviewBackdropRole(QWidget* widget, QScreen* screen,
        const QString& scope);
    static void fixPopupLayerShell(QWidget* popup);
};

}  // namespace Wayland

#endif // LAYERSHELLHELPER_H
