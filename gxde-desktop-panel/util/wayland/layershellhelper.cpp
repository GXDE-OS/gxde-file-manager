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

#include "waylandutils.h"

namespace Wayland {

// 运行时(平台级)判定：这里的调用都发生在QGuiApplication之后，用平台插件名
bool LayerShellHelper::isWayland() {
    return WaylandUtils::isWaylandPlatform();
}

// 检测是否是Treeland：先确认真的跑在Wayland平台上，再看会话标识
bool LayerShellHelper::isTreeland() {
    return isWayland() && WaylandUtils::isTreeland();
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
    // Qt6版LayerShellQt去掉了setDesiredOutput(QScreen*)，改为从QWindow::screen()取输出
    // 上面已经window->setScreen(screen)，这里显式声明使用QWindow上的屏幕即可
    target_layer_shell_window->setScreenConfiguration(
        LayerShellQt::Window::ScreenFromQWindow);
    target_layer_shell_window->setLayer(LayerShellQt::Window::LayerBackground);
    target_layer_shell_window->setAnchors(anchors);
    target_layer_shell_window->setExclusiveZone(-1);
    target_layer_shell_window->setKeyboardInteractivity(
        LayerShellQt::Window::KeyboardInteractivityOnDemand);
}

void LayerShellHelper::setChooserRole(QWidget* widget, QScreen* screen,
        const QString& scope) {
    // 安全检查
    if (widget == nullptr) {
        qWarning() << "(LayerShellHelper) ChooserRole: The widget pointer"
            << "is a null pointer!!";
        return;
    }

    if (!isWayland()) {
        qWarning()<< "(LayerShellHelper) ChooserRole: Non-wayland session detected,"
            << "now aborting...";
        return;
    }

    // 配置目标Widget的属性
    widget->setWindowFlag(Qt::FramelessWindowHint, true);
    widget->setAttribute(Qt::WA_NativeWindow, true);
    widget->createWinId();

    QWindow* window = widget->windowHandle();
    if (!window) {
        qWarning()
            << "(LayerShellHelper) ChooserRole: Invalid handle, halted!!";
        return;
    }

    if (screen) {
        window->setScreen(screen);
    }

    LayerShellQt::Window* target_layer_shell_window =
        LayerShellQt::Window::get(window);
    if (!target_layer_shell_window) {
        qWarning()
            << "(LayerShellHelper) ChooserRole: Failed to get"
            << "layer-shell window for: " << widget << ", halted!!";
        return;
    }

    // 壁纸选择器 锚定底边、左右两边, 高度由窗口固定尺寸决定
    LayerShellQt::Window::Anchors anchors;
    anchors |= LayerShellQt::Window::AnchorBottom;
    anchors |= LayerShellQt::Window::AnchorLeft;
    anchors |= LayerShellQt::Window::AnchorRight;

    target_layer_shell_window->setScope(scope);
    // Qt6版LayerShellQt去掉了setDesiredOutput(QScreen*)，改为从QWindow::screen()取输出
    target_layer_shell_window->setScreenConfiguration(
        LayerShellQt::Window::ScreenFromQWindow);
    target_layer_shell_window->setLayer(LayerShellQt::Window::LayerOverlay);
    target_layer_shell_window->setAnchors(anchors);

    // -1: 贴紧锚定边, 忽略其它窗口如任务栏的exclusive zone
    target_layer_shell_window->setExclusiveZone(-1);

    // 选择器需要响应Esc并接受焦点
    target_layer_shell_window->setKeyboardInteractivity(
        LayerShellQt::Window::KeyboardInteractivityOnDemand);

    // 要求合成器隐藏标题栏装饰
    DPlatformHandle::setEnabledNoTitlebarForWindow(window, true);
}

void LayerShellHelper::setPreviewBackdropRole(QWidget* widget,
        QScreen* screen, const QString& scope) {
    // 安全检查
    if (widget == nullptr) {
        qWarning()
            << "(LayerShellHelper) BackdropRole: background role got"
            << "null ptr!!";
        return;
    }

    if (!isWayland()) {
        qWarning()
            << "(LayerShellHelper) BackdropRole: Non-wayland session detected,"
            << "now aborting...";
        return;
    }

    widget->setWindowFlag(Qt::FramelessWindowHint, true);
    widget->setAttribute(Qt::WA_NativeWindow, true);
    widget->createWinId();

    QWindow* window = widget->windowHandle();
    if (!window) {
        qWarning()
            << "(LayerShellHelper) BackdropRole: Invalid handle, halted!!";
        return;
    }

    if (screen) {
        window->setScreen(screen);
    }

    LayerShellQt::Window* target_layer_shell_window =
        LayerShellQt::Window::get(window);
    if (!target_layer_shell_window) {
        qWarning() << "(LayerShellHelper) BackdropRole: Failed to get"
            << "layer-shell window for: " << widget << ", halted!!";
        return;
    }

    // 锚定四边，全屏
    LayerShellQt::Window::Anchors anchors;
    anchors |= LayerShellQt::Window::AnchorTop;
    anchors |= LayerShellQt::Window::AnchorBottom;
    anchors |= LayerShellQt::Window::AnchorLeft;
    anchors |= LayerShellQt::Window::AnchorRight;

    target_layer_shell_window->setScope(scope);
    // Qt6版LayerShellQt去掉了setDesiredOutput(QScreen*)，改为从QWindow::screen()取输出
    target_layer_shell_window->setScreenConfiguration(
        LayerShellQt::Window::ScreenFromQWindow);

    // 与桌面壁纸的区别: 预览背景必须置于真实桌面之上才可见
    target_layer_shell_window->setLayer(LayerShellQt::Window::LayerTop);
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

    // 把菜单定位到它期望出现的位置, 即右键光标处
    // 坏消息是若不设anchor，Treeland 会把它摆到屏幕正中
    // 备用方案: 锚定左上角，再用 margin 偏移到鼠标位置
    // 注: popup->pos()为QMenu请求的弹出位置
    const QPoint pos = popup->pos();
    LayerShellQt::Window::Anchors anchors;
    anchors |= LayerShellQt::Window::AnchorTop;
    anchors |= LayerShellQt::Window::AnchorLeft;
    target_layer_shell_window->setAnchors(anchors);
    target_layer_shell_window->setMargins(QMargins(pos.x(), pos.y(), 0, 0));
    target_layer_shell_window->setLayer(LayerShellQt::Window::LayerOverlay);
    target_layer_shell_window->setExclusiveZone(0);

    // 子菜单(有 transient parent，如「用…打开」展开项)设为 None：不要键盘交互
    // 否则它作为独立 layer surface 会 requestActive 抢走激活态
    // Treeland 会把父菜单setActivate(false))，且关闭时 requestInactive把激活
    // 甩给别的窗口，后果就是整条菜单关掉
    const bool isSubMenu = window->transientParent() != nullptr;
    target_layer_shell_window->setKeyboardInteractivity(
        isSubMenu ? LayerShellQt::Window::KeyboardInteractivityNone
            : LayerShellQt::Window::KeyboardInteractivityOnDemand);

    // popup菜单在Treeland下会被当作普通toplevel窗口装饰，
    // 导致出现最小化/最大化/关闭按钮
    // 通知合成器我们要求隐藏标题栏
    DPlatformHandle::setEnabledNoTitlebarForWindow(window, true);
}

}  // namespace Wayland
