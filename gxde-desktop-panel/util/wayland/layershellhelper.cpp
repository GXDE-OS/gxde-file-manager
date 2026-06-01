/**
 * Copyright (C) 2026 CharOfString
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "layershellhelper.h"

#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <QWindow>

#include <LayerShellQt/Window>
#include <DPlatformHandle>

namespace Wayland {

bool LayerShellHelper::isWayland() {
    if (!qGuiApp) {
        return false;
    }

    const QString plat_name = QGuiApplication::platformName().toLower();
    bool result = false;

    if (plat_name == "wayland") {
        result = true;
    } else if (plat_name == "dwayland") {
        result = true;
    } else if (plat_name.contains("wayland")) {
        result = true;
    }

    return result;
}

void LayerShellHelper::setDesktopRole(QWidget* widget, QScreen* screen,
        const QString& scope) {
    if (widget == nullptr) {
        qWarning() << "The widget pointer that needs a desktop role passed in"
            << "is a null pointer!!";
        return;
    }

    if (!isWayland()) {
        qWarning()<< "Non-wayland session detected,"
            << "aborting to set wayland desktop role...";
        return;
    }

    // 配置目标Widget的属性
    widget->setWindowFlag(Qt::FramelessWindowHint, true);
    widget->setAttribute(Qt::WA_NativeWindow, true);
    widget->createWinId();

    QWindow* window = widget->windowHandle();
    if (!window) {
        qWarning() << "Invalid desktop layer window handle, halted!!";
        return;
    }

    if (screen) {
        qInfo() << "Valid screen, now setting to window...";
        window->setScreen(screen);
    }

    // 配置layer-shell
    LayerShellQt::Window* target_layer_shell_window = LayerShellQt::Window::get(window);
    if (!target_layer_shell_window) {
        qWarning() << "Failed to get layer-shell window for: " << widget
            << ", halted!!";
        return;
    }

    // 屏幕四边都设置anchor，代表四边拉伸
    LayerShellQt::Window::Anchors anchors;
    anchors |= LayerShellQt::Window::AnchorTop;
    anchors |= LayerShellQt::Window::AnchorBottom;
    anchors |= LayerShellQt::Window::AnchorLeft;
    anchors |= LayerShellQt::Window::AnchorRight;

    // 设置layer-shell属性
    target_layer_shell_window->setScope(scope);
    target_layer_shell_window->setDesiredOutput(screen);
    target_layer_shell_window->setLayer(LayerShellQt::Window::LayerBackground);
    target_layer_shell_window->setAnchors(anchors);
    target_layer_shell_window->setExclusiveZone(-1);
    target_layer_shell_window->setKeyboardInteractivity(
        LayerShellQt::Window::KeyboardInteractivityNone);
}

void LayerShellHelper::fixPopupLayerShell(QWidget* popup) {
    if (popup == nullptr) {
        qWarning() << "The popup pointer that needs a desktop role passed in"
            << "is a null pointer!!";
        return;
    }

    if (!isWayland()) {
        qWarning()<< "Non-wayland session detected,"
            << "aborting to set wayland desktop role...";
        return;
    }

    popup->createWinId();

    QWindow* window = popup->windowHandle();
    if (!window) {
        qWarning() << "Invalid popup window handle, halted!!";
        return;
    }

    LayerShellQt::Window* target_layer_shell_window = LayerShellQt::Window::get(window);
    if (!target_layer_shell_window) {
        return;
    }

    // popup不设任何anchor

    // 其它属性
    target_layer_shell_window->setAnchors({});
    target_layer_shell_window->setLayer(LayerShellQt::Window::LayerOverlay);
    target_layer_shell_window->setExclusiveZone(0);
    target_layer_shell_window->setKeyboardInteractivity(
        LayerShellQt::Window::KeyboardInteractivityOnDemand);

    // popup菜单在Treeland下会被当作普通toplevel窗口装饰，
    // 导致出现最小化/最大化/关闭按钮
    // 通知合成器我们要求隐藏标题栏
    DPlatformHandle::setEnabledNoTitlebarForWindow(window, true);
}

}  // namespace Wayland
