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
#ifndef VIDEOWALLPAPERPLAYER_H
#define VIDEOWALLPAPERPLAYER_H

#include <QObject>
#include <QImage>
#include <QString>

class QMediaPlayer;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class QVideoSink;
#else
class QAbstractVideoSurface;
#endif

/**
 * @brief 视频动态壁纸播放器
 *
 * 负责解码视频并把每一帧以 QImage 的形式抛出，由 BackgroundHelper
 * 绘制到各个屏幕的背景窗口上。静音、无限循环，不创建任何窗口。
 *
 * 解码错误通过 errorOccurred() 通知调用方（QMediaPlayer 的错误是异步
 * 上报的，play() 返回 true 只代表源有效并已开始加载）。
 */
class VideoWallpaperPlayer : public QObject
{
    Q_OBJECT
public:
    explicit VideoWallpaperPlayer(QObject *parent = nullptr);
    ~VideoWallpaperPlayer() override;

    // pathOrUrl 为空或文件不可读时返回 false；是否真正解码成功
    // 由异步的 errorOccurred() 信号反馈
    bool play(const QString &pathOrUrl);
    void stop();

    bool isPlaying() const;
    QString sourcePath() const;

Q_SIGNALS:
    // 每解码出一帧就会发出，frame 可能为空（调用方需判断）
    void frameReady(const QImage &frame);
    void playingChanged(bool playing);
    void errorOccurred(const QString &reason);

private:
    void setPlaying(bool playing);

    QString m_source;
    bool m_playing = false;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMediaPlayer *m_player = nullptr;
    QVideoSink *m_sink = nullptr;
#else
    class Surface;
    QMediaPlayer *m_player = nullptr;
    Surface *m_surface = nullptr;
#endif
};

#endif // VIDEOWALLPAPERPLAYER_H
