/*
 * Copyright (C) 2026 GXDE Project
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "videothumbgrabber.h"
#include "constants.h"
#include "videowallpaperplayer.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QMediaPlayer>
#include <QPainter>
#include <QPen>
#include <QPointer>
#include <QPolygon>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoSink>

VideoThumbGrabber::VideoThumbGrabber(QObject *parent)
    : QObject(parent)
{
}

VideoThumbGrabber *VideoThumbGrabber::instance()
{
    static VideoThumbGrabber grabber;
    return &grabber;
}

QString VideoThumbGrabber::cacheFileForKey(const QString &key, qreal scale) const
{
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QDir::separator() + qApp->applicationVersion()
        + QDir::separator() + QString::number(scale);
    QDir::root().mkpath(cacheDir);
    // 后缀 .png 与 ThumbnailManager 存的原生QPixmap缓存文件区分开
    return QDir(cacheDir).absoluteFilePath(key + ".png");
}

// 抓取失败时的占位图：深色底 + 白色播放三角
QPixmap VideoThumbGrabber::loadGrabbedFrame(const QString &videoPath, qreal scale) const
{
    Q_UNUSED(videoPath);

    const int w = qRound(ItemWidth * scale);
    const int h = qRound(ItemHeight * scale);

    QPixmap pix(w, h);
    pix.fill(QColor(24, 26, 30));

    QPainter pa(&pix);
    pa.setRenderHint(QPainter::Antialiasing);

    const qreal triSize = qMin(w, h) * 0.22;
    QPolygon triangle;
    triangle << QPoint(qRound(w / 2.0 - triSize / 2), qRound(h / 2.0 - triSize / 2))
             << QPoint(qRound(w / 2.0 - triSize / 2), qRound(h / 2.0 + triSize / 2))
             << QPoint(qRound(w / 2.0 + triSize), qRound(h / 2.0));
    pa.setPen(Qt::NoPen);
    pa.setBrush(QColor(255, 255, 255, 190));
    pa.drawPolygon(triangle);

    return pix;
}

void VideoThumbGrabber::grab(const QString &key, const QString &videoPath, qreal scale)
{
    // 磁盘缓存命中直接返回
    const QString cacheFile = cacheFileForKey(key, scale);
    const QPixmap cached(cacheFile);
    if (!cached.isNull()) {
        Q_EMIT thumbnailFounded(key, cached);
        return;
    }

    // 同一视频的重复抓取请求：等第一份结果即可，避免开多个解码器
    if (m_pendingRequests.contains(key))
        return;
    m_pendingRequests.insert(key);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6：直连 QMediaPlayer + QVideoSink 抓第一帧
    auto *player = new QMediaPlayer(this);
    auto *sink = new QVideoSink(player);
    player->setAudioOutput(nullptr);
    player->setVideoSink(sink);
    player->setLoops(1);

    const QPointer<VideoThumbGrabber> guard(this);

    auto finishWithPixmap = [this, guard, player, key, scale, cacheFile](const QPixmap &pix) {
        if (!guard)
            return;

        player->deleteLater();
        m_pendingRequests.remove(key);

        if (pix.isNull()) {
            Q_EMIT thumbnailFounded(key, QPixmap());
            return;
        }

        pix.save(cacheFile, "PNG");
        Q_EMIT thumbnailFounded(key, pix);
    };

    // 抓到第一帧即收工
    connect(sink, &QVideoSink::videoFrameChanged, this,
            [this, guard, player, sink, key, scale,
             finishWithPixmap](const QVideoFrame &frame) {
        if (!frame.isValid())
            return;
        if (!guard || !m_pendingRequests.contains(key))
            return; // 已被超时/错误路径收尾

        disconnect(sink, &QVideoSink::videoFrameChanged, this, nullptr);
        player->stop();

        const QSize target(qRound(ItemWidth * scale), qRound(ItemHeight * scale));
        QImage image = frame.toImage();
        if (image.isNull()) {
            finishWithPixmap(QPixmap());
            return;
        }

        image = image.scaled(target, Qt::KeepAspectRatioByExpanding,
                             Qt::SmoothTransformation);
        if (image.width() > target.width() || image.height() > target.height()) {
            image = image.copy(QRect((image.width() - target.width()) / 2,
                    (image.height() - target.height()) / 2,
                    target.width(), target.height()));
        }

        QPixmap pix = QPixmap::fromImage(image);
        pix.setDevicePixelRatio(scale);
        finishWithPixmap(pix);
    });

    // 解码器迟迟不出帧（损坏文件/无可解码器）：超时兜底，给占位图
    QTimer::singleShot(8 * 1000, this, [this, guard, player, key, scale, finishWithPixmap]() {
        if (!guard || !m_pendingRequests.contains(key))
            return;

        player->stop();
        qWarning() << "(VideoThumbGrabber) grab timed out for" << key;
        finishWithPixmap(loadGrabbedFrame(QString(), scale));
    });

    connect(player, &QMediaPlayer::errorOccurred, this,
            [this, guard, player, key, scale, finishWithPixmap](QMediaPlayer::Error,
                                                                const QString &reason) {
        if (!guard || !m_pendingRequests.contains(key))
            return; // 已被首帧/超时路径收尾

        player->stop();
        qWarning() << "(VideoThumbGrabber) failed to grab" << key << ":" << reason;
        finishWithPixmap(loadGrabbedFrame(QString(), scale));
    });

    player->setSource(QUrl::fromLocalFile(videoPath));
    player->play();
#else
    auto *player = new VideoWallpaperPlayer(this);
    const QPointer<VideoThumbGrabber> guard(this);

    // 抓到第一帧即收工：播放会继续到帧回调到达为止，先停掉再取图
    QMetaObject::Connection frameConn = connect(player, &VideoWallpaperPlayer::frameReady,
            this, [this, guard, player, key, scale, cacheFile](const QImage &frame) {
        if (!guard)
            return;

        QObject::disconnect(player, &VideoWallpaperPlayer::frameReady, this, nullptr);
        player->stop();
        player->deleteLater();
        m_pendingRequests.remove(key);

        if (frame.isNull()) {
            Q_EMIT thumbnailFounded(key, QPixmap());
            return;
        }

        const QSize target(qRound(ItemWidth * scale), qRound(ItemHeight * scale));
        QPixmap pix = QPixmap::fromImage(frame.scaled(target,
                Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        if (pix.width() > target.width() || pix.height() > target.height()) {
            pix = pix.copy(QRect((pix.width() - target.width()) / 2,
                    (pix.height() - target.height()) / 2, target.width(), target.height()));
        }
        pix.setDevicePixelRatio(scale);

        // 缓存QPixmap::save按dpr保存的是设备像素尺寸, 与读回时一致
        pix.save(cacheFile, "PNG");

        Q_EMIT thumbnailFounded(key, pix);
    });

    connect(player, &VideoWallpaperPlayer::errorOccurred, this,
            [this, guard, player, key, frameConn](const QString &reason) {
        if (!guard)
            return;

        QObject::disconnect(frameConn);
        player->deleteLater();
        if (!m_pendingRequests.contains(key))
            return;
        m_pendingRequests.remove(key);

        qWarning() << "(VideoThumbGrabber) failed to grab" << key << ":" << reason;
        Q_EMIT thumbnailFounded(key, QPixmap());
    });

    // 解码器迟迟不出帧(损坏文件/无可解码器)：超时兜底，给占位图
    QTimer::singleShot(8 * 1000, this, [this, guard, player, key, scale, frameConn]() {
        if (!guard || !m_pendingRequests.contains(key))
            return;
        m_pendingRequests.remove(key);

        QObject::disconnect(frameConn);
        if (player)
            player->deleteLater();

        qWarning() << "(VideoThumbGrabber) grab timed out for" << key;
        Q_EMIT thumbnailFounded(key, loadGrabbedFrame(QString(), scale));
    });

    if (!player->play(videoPath)) {
        QObject::disconnect(frameConn);
        player->deleteLater();
        m_pendingRequests.remove(key);
        qWarning() << "(VideoThumbGrabber) cannot play" << videoPath;
        Q_EMIT thumbnailFounded(key, loadGrabbedFrame(QString(), scale));
    }
#endif
}
