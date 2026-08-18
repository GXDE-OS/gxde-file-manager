/**
 * Copyright (C) 2026 CharOfString
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "layershellhelper.h"

#include <QCursor>
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

void LayerShellHelper::setDesktopIconsRole(QWidget* widget, QScreen* screen,
        const QString& scope) {
    if (!widget || !isWayland()) {
        qWarning()
            << "(Wayland) LayerShell: Cannot set the desktop icons layer-shell role"
            << widget;
        return;
    }

    widget->setWindowFlag(Qt::FramelessWindowHint, true);
    widget->setAttribute(Qt::WA_NativeWindow, true);
    widget->createWinId();

    QWindow* window = widget->windowHandle();
    if (!window) {
        qWarning() << "(Wayland) LayerShell: Invalid desktop icons layer window handle";
        return;
    }

    if (screen) {
        window->setScreen(screen);
    }

    LayerShellQt::Window* layerWindow = LayerShellQt::Window::get(window);
    if (!layerWindow) {
        qWarning() << "Failed to get layer-shell window for desktop icons";
        return;
    }

    LayerShellQt::Window::Anchors anchors;
    anchors |= LayerShellQt::Window::AnchorTop;
    anchors |= LayerShellQt::Window::AnchorBottom;
    anchors |= LayerShellQt::Window::AnchorLeft;
    anchors |= LayerShellQt::Window::AnchorRight;

    layerWindow->setScope(scope);
    layerWindow->setScreenConfiguration(LayerShellQt::Window::ScreenFromQWindow);

    // Keep icons above the wallpaper & below ordinary application windows
    layerWindow->setLayer(LayerShellQt::Window::LayerBottom);
    layerWindow->setAnchors(anchors);

    // exclusive zone = 0: 本 surface 自己不预留空间, 但会被所有正值
    // exclusive zone 推开 —— 合成器把所有排除区(dock 的底边、顶栏、
    // 以及用户自己运行的任何 layer-shell 程序)从工作区里扣掉后, 把本
    // surface 排布到剩余工作区, 即 rect() 就是图标可用的全部区域。
    // 这样无论谁设置了排除区都能被正确尊重, 不需要向具体程序查询几何。
    // dock/顶栏处于 LayerTop, 本 surface 处于 LayerBottom, 它们仍会渲染
    // 在本层之上; 排除区内的鼠标事件由壁纸层(LayerBackground)接收,
    // desktop.cpp 里再转发给 CanvasGridView, 右键菜单/框选仍然可用。
    layerWindow->setExclusiveZone(0);
    layerWindow->setKeyboardInteractivity(
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

void LayerShellHelper::fixPopupLayerShell(QWidget* popup, QScreen* screen) {
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

    // A layer surface belongs to one wl_output and its margins are relative to
    // that output, not to Qt's virtual-desktop origin.  In a multi-monitor
    // layout popup->pos() is still a global position, so using it directly can
    // produce an out-of-range margin on a non-primary output.  Treeland may
    // then move the popup to another output and stretch it to satisfy the
    // anchors.
    if (!screen && window->transientParent()) {
        screen = window->transientParent()->screen();
    }
    if (!screen) {
        screen = QGuiApplication::screenAt(QCursor::pos());
    }
    if (!screen) {
        screen = window->screen();
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen) {
        window->setScreen(screen);

        // QMenu performs another popup-screen selection after the QWidget
        // Show event.  With mixed-DPI adjacent outputs that late selection can
        // overwrite the screen fixed above before LayerShellQt creates the
        // layer surface.  Lock the popup to the requested output for its whole
        // lifetime; the direct screenChanged callback runs before
        // get_layer_surface reads QWindow::screen().
        if (!window->property("_gxde_popup_screen_locked").toBool()) {
            window->setProperty("_gxde_popup_screen_locked", true);
            const QString targetScreenName = screen->name();
            const QSize targetPopupSize = popup->size();
            window->setProperty("_gxde_popup_target_size", targetPopupSize);
            QObject::connect(window, &QWindow::screenChanged, popup,
                [window, targetScreenName, targetPopupSize](QScreen* changedScreen) {
                    qWarning() << "GXDE_POPUP_TRACE screenChanged"
                               << "changedTo" << (changedScreen
                                      ? changedScreen->name() : QString())
                               << "lockedTo" << targetScreenName;
                    if (changedScreen
                            && changedScreen->name() == targetScreenName) {
                        window->resize(targetPopupSize);
                        return;
                    }

                    for (QScreen* candidate : QGuiApplication::screens()) {
                        if (candidate && candidate->name() == targetScreenName) {
                            window->setScreen(candidate);
                            // setScreen() recreates the popup platform window
                            // and resets its native size to QWindow's 640x480
                            // default.  Restore QMenu's final QWidget size
                            // before LayerShellQt sends set_size.
                            window->resize(targetPopupSize);
                            break;
                        }
                    }
                }, Qt::DirectConnection);
        }
    }

    qWarning() << "GXDE_POPUP_TRACE layer"
               << "globalPos" << popup->pos()
               << "popupSize" << popup->size()
               << "requestedScreen" << (screen ? screen->name() : QString())
               << "requestedGeometry" << (screen ? screen->geometry() : QRect())
               << "windowScreenAfterSet" << (window->screen()
                      ? window->screen()->name() : QString());

    LayerShellQt::Window* target_layer_shell_window = LayerShellQt::Window::get(window);
    if (!target_layer_shell_window) {
        return;
    }

    target_layer_shell_window->setScreenConfiguration(
        LayerShellQt::Window::ScreenFromQWindow);

    // 把菜单定位到它期望出现的位置, 即右键光标处
    // 坏消息是若不设anchor，Treeland 会把它摆到屏幕正中
    // 备用方案: 锚定左上角，再用 margin 偏移到鼠标位置
    // 注: popup->pos()为QMenu请求的桌面全局位置；layer-shell margin 则以
    // 当前输出左上角为原点。先把菜单限制在目标输出，再转换成输出局部坐标。
    QPoint globalPos = popup->pos();
    QPoint outputPos = globalPos;
    if (screen) {
        const QRect outputGeometry = screen->geometry();
        const int maxX = qMax(outputGeometry.left(),
            outputGeometry.right() - popup->width() + 1);
        const int maxY = qMax(outputGeometry.top(),
            outputGeometry.bottom() - popup->height() + 1);
        globalPos.setX(qBound(outputGeometry.left(), globalPos.x(), maxX));
        globalPos.setY(qBound(outputGeometry.top(), globalPos.y(), maxY));
        outputPos = globalPos - outputGeometry.topLeft();
    }
    qWarning() << "GXDE_POPUP_TRACE margins"
               << "clampedGlobalPos" << globalPos
               << "outputPos" << outputPos;
    LayerShellQt::Window::Anchors anchors;
    anchors |= LayerShellQt::Window::AnchorTop;
    anchors |= LayerShellQt::Window::AnchorLeft;
    target_layer_shell_window->setAnchors(anchors);
    target_layer_shell_window->setMargins(
        QMargins(outputPos.x(), outputPos.y(), 0, 0));
    target_layer_shell_window->setLayer(LayerShellQt::Window::LayerOverlay);
    target_layer_shell_window->setExclusiveZone(0);

    // 子菜单(有 transient parent，如「用…打开」展开项)设为 None：不要键盘交互
    // 否则它作为独立 layer surface 会 requestActive 抢走激活态
    // Treeland 会把父菜单setActivate(false))，且关闭时 requestInactive把激活
    // 甩给别的窗口，后果就是整条菜单关掉
    const bool isRootPopup =
        !popup->property("_gxde_popup_screen").toString().isEmpty();
    const bool isSubMenu = !isRootPopup
        && window->transientParent() != nullptr;
    target_layer_shell_window->setKeyboardInteractivity(
        isSubMenu ? LayerShellQt::Window::KeyboardInteractivityNone
            : LayerShellQt::Window::KeyboardInteractivityOnDemand);

    // popup菜单在Treeland下会被当作普通toplevel窗口装饰，
    // 导致出现最小化/最大化/关闭按钮
    // 通知合成器我们要求隐藏标题栏
    DPlatformHandle::setEnabledNoTitlebarForWindow(window, true);
}

}  // namespace Wayland
