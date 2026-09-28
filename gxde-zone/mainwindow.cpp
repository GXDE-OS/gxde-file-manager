/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "mainwindow.h"
#include "pushbuttonlist.h"
#include "hotzone.h"
#include <QScreen>
#include <QGuiApplication>
#include <QMediaPlayer>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QMediaPlaylist>
#endif

#include <QPainter>
#include <QGSettings>
#include <dapplication.h>

#include "util/wayland/layershellhelper.h"

ZoneMainWindow::ZoneMainWindow(QWidget *parent)
    : QWidget(parent)
    , m_videoWidget(nullptr)
{
    m_dbusZoneInter = new ZoneInterface("com.gxde.daemon.corneredge", "/com/gxde/daemon/corneredge", QDBusConnection::sessionBus(), this);

    // let the app start without system animation
    setWindowFlags(Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint);
    // let background be transparent
    setAttribute(Qt::WA_TranslucentBackground, true);

    const bool isWayland = Wayland::LayerShellHelper::isWayland();
    m_topMargin = isWayland ? 0 : MAIN_ITEM_TOP_MARGIN;

    // catch the screen that mouse is in
    QScreen *targetScreen = QGuiApplication::screenAt(QCursor::pos());
    if (!targetScreen) {
        targetScreen = QGuiApplication::primaryScreen();
    }

    if (targetScreen) {
        const QRect screen = targetScreen->geometry();
        if (isWayland) {
            this->resize(screen.size());
        } else {
            // set the size and position of this app. Enlarge 30px to height to avoid fade-zone of mouseEvent.
            this->setGeometry(screen.x(), screen.y() - m_topMargin, screen.width(), screen.height() + m_topMargin);
        }
    }

    if (isWayland) {
        Wayland::LayerShellHelper::setFullscreenOverlayRole(
            this, targetScreen, QStringLiteral("dde-shell/hotzone-settings"));
    }

    // set the background
    QWidget *back = new QWidget(this);
    back->setGeometry(0, m_topMargin, this->width(), this->height() - m_topMargin);

    // check demo video gsettings value
    QGSettings gsetting("com.deepin.dde.desktop", "/com/deepin/dde/desktop/");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (gsetting.keys().contains("enableHotzoneVideo") && gsetting.get("enable-hotzone-video").toBool()) {
        m_videoWidget = new DVideoWidget(this);
        m_videoWidget->setSourceVideoPixelRatio(devicePixelRatioF());
        QTimer::singleShot(1000, this, &ZoneMainWindow::onDemoVideo);
    }
#else
    Q_UNUSED(gsetting)
    // DVideoWidget 暂未在 dtk2widget-qt6 中实现，禁用 hotzone 演示视频
#endif

    // init corresponding QList for addButtons()
    m_ButtonNames << tr("Fast Screen Off")
                  << tr("Control Center")
                  << tr("All Windows")
                  << tr("Launcher")
                  << tr("Desktop")
//                  << tr("Text Editor")
                  << tr("Screen Shot")
                  << tr("Screen Record")
                  << tr("Screen OCR")
                  << tr("Screen Scroll")
                  << tr("Color Picker")
//                  << tr("File Manager")
                  << tr("System Monitor")
                  << tr("Notify Center")
                  << tr("None");
    m_ActionStrs << FAST_SCREEN_OFF
                 << CONTROL_CENTER_FROM_LEFT_STR
                 << ALL_WINDOWS_STR
                 << LAUNCHER_STR
                 << SHOW_DESKTOP_STR
//                 << GXDEEDITOR_STR
                 << SCREENSHOT_STR
                 << SCREENRECORD_STR
                 << SCREENOCR_STR
                 << SCREENSCROLL_STR
                 << COLORPICKER_STR
//                 << FILEMANAGER_STR
                 << SYSTEMMONITOR_STR
                 << NOTIFYCENTER_STR
                 << NONE_STR;
    /*m_ActionStrs2 << FAST_SCREEN_OFF
                  << CONTROL_CENTER_FROM_RIGHT_STR
                  << ALL_WINDOWS_STR
                  << LAUNCHER_STR
                  << SHOW_DESKTOP_STR
                  << GXDEEDITOR_STR
                  << SCREENRECORDER_STR
                  << COLORPICKER_STR
                  << FILEMANAGER_STR
                  << SYSTEMMONITOR_STR
                  << NONE_STR;*/
    m_ActionStrs2 = m_ActionStrs;

    QStringList topRightNames = QStringList() << tr("Close Window") << m_ButtonNames;
    QStringList topRightActionStr = QStringList() << CLOSE_MAX_WINDOW_STR << m_ActionStrs2;

    // load 4 corners
    HotZone *hotzone1 = new HotZone(this, false, false, m_topMargin);
    hotzone1->addButtons(m_ButtonNames, m_ActionStrs);

    HotZone *hotzone2 = new HotZone(this, true, false, m_topMargin);
    hotzone2->addButtons(topRightNames, topRightActionStr);

    HotZone *hotzone3 = new HotZone(this, false, true, m_topMargin);
    hotzone3->addButtons(m_ButtonNames, m_ActionStrs);

    HotZone *hotzone4 = new HotZone(this, true, true, m_topMargin);
    hotzone4->addButtons(m_ButtonNames, m_ActionStrs2);

    m_dbusZoneInter->EnableZoneDetected(false);
}

ZoneMainWindow::~ZoneMainWindow()
{
    m_dbusZoneInter->EnableZoneDetected(true);
}

void ZoneMainWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton) {
        releaseKeyboard();
        emit finished();
    }
}

void ZoneMainWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        releaseKeyboard();
        emit finished();
    }
}

void ZoneMainWindow::onDemoVideo()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (!m_videoWidget) return;

    QMediaPlayer *player = new QMediaPlayer(this);

    int x = (rect().right() - 450) / 2;
    int y = (rect().bottom() - 348) / 2;
    m_videoWidget->setGeometry(x, y, 450, 348);

    m_videoWidget->setSource(player);
    QMediaPlaylist *list = new QMediaPlaylist(this);
    list->addMedia(QUrl("qrc:/images/Prompt.mov"));
    list->setPlaybackMode(QMediaPlaylist::Loop);
    player->setPlaylist(list);
    player->play();
#endif
}

void ZoneMainWindow::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    QPainter pa(this);

    pa.fillRect(rect(), QColor(0, 0, 0, 178));
}
