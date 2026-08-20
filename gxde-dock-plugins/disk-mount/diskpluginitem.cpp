/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *               2016 ~ 2018 dragondjf
 *
 * Author:     dragondjf<dingjiangfeng@deepin.com>
 *
 * Maintainer: dragondjf<dingjiangfeng@deepin.com>
 *             zccrs<zhangjide@deepin.com>
 *             Tangtong<tangtong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "diskpluginitem.h"

#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <QIcon>
#include <QApplication>
#include <algorithm>

// 与 dock 框架约定一致：插件图标最大尺寸
#define PLUGIN_ICON_MAX_SIZE 20

// disk 插件图标尺寸，与 sound 等正常插件对齐（24），比框架旧值 20 更贴合面板
#define DISK_ICON_SIZE 24

DiskPluginItem::DiskPluginItem(QWidget *parent)
    : QWidget(parent),
      m_displayMode(Dock::Efficient)
{
}

void DiskPluginItem::setDockDisplayMode(const Dock::DisplayMode mode)
{
    m_displayMode = mode;

    updateIcon();
}

void DiskPluginItem::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter painter(this);

    // 不要在 paintEvent 里调用 updateIcon()：updateIcon() 末尾会 update()，
    // 从而形成“绘制 -> 请求重绘 -> 绘制”的自激循环，在 Wayland 下会把
    // gxde-dock 主线程推到单核 70%+。图标刷新已由 resizeEvent/refreshIcon 覆盖。

    // 居中偏移必须用 pixmap 自身的 DPR（与正常插件一致，修复 HDPI 下图标偏左上）
    painter.drawPixmap(rect().center() - m_icon.rect().center() / m_icon.devicePixelRatioF(), m_icon);
}

void DiskPluginItem::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    updateIcon();
}

QSize DiskPluginItem::sizeHint() const
{
    // 仅一个图标，不需要像 trash 那样占用大尺寸，宽度贴近图标即可避免留白
    return QSize(DISK_ICON_SIZE + 8, DISK_ICON_SIZE + 8);
}

void DiskPluginItem::updateIcon()
{
    // fashion mode icons are no longer needed
    const int size = DISK_ICON_SIZE;
    QIcon icon = QIcon::fromTheme("drive-removable-dock-symbolic");

    // 按当前 DPR 请求 pixmap，HDPI 下图标才不会糊也不会偏小
    m_icon = icon.pixmap(QSize(size, size), devicePixelRatioF());

    update();
}
