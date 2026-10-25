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
#include "videowallpaperplayer.h"

#include <QDebug>
#include <QFileInfo>
#include <QUrl>
#include <QTimer>

#include <QMediaPlayer>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

#include <QVideoSink>
#include <QVideoFrame>

VideoWallpaperPlayer::VideoWallpaperPlayer(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_sink(new QVideoSink(this))
{
    // 动态壁纸不需要声音
    m_player->setAudioOutput(nullptr);
    m_player->setVideoSink(m_sink);
    m_player->setLoops(QMediaPlayer::Infinite);

    connect(m_sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        if (frame.isValid()) {
            const QImage image = frame.toImage();
            if (!image.isNull())
                Q_EMIT frameReady(image);
        }
    });

    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state) {
        setPlaying(state == QMediaPlayer::PlayingState);
    });

    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error code, const QString &message) {
        qWarning() << "(Desktop Panel) dynamic wallpaper playback error:"
                   << code << message;
        Q_EMIT errorOccurred(message);
    });
}

VideoWallpaperPlayer::~VideoWallpaperPlayer()
{
}

bool VideoWallpaperPlayer::play(const QString &pathOrUrl)
{
    const QString path = pathOrUrl.startsWith("file:")
        ? QUrl(pathOrUrl).toLocalFile() : pathOrUrl;
    const QFileInfo info(path);
    if (path.isEmpty() || !info.isFile() || !info.isReadable()) {
        qWarning() << "(Desktop Panel) dynamic wallpaper is not a readable file:" << path;
        return false;
    }

    if (m_source == path && m_playing)
        return true;

    m_source = path;
    m_player->stop();
    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();
    return true;
}

void VideoWallpaperPlayer::stop()
{
    m_player->stop();
    m_player->setSource(QUrl());
    m_source.clear();
    setPlaying(false);
}

bool VideoWallpaperPlayer::isPlaying() const
{
    return m_playing;
}

QString VideoWallpaperPlayer::sourcePath() const
{
    return m_source;
}

void VideoWallpaperPlayer::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;

    m_playing = playing;
    Q_EMIT playingChanged(playing);
}

#elif QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)

#include <QAbstractVideoSurface>
#include <QVideoFrame>
#include <QVideoSurfaceFormat>

// Qt5 没有通用的帧回调接口，通过 QAbstractVideoSurface 接收解码帧
class VideoWallpaperPlayer::Surface : public QAbstractVideoSurface
{
public:
    explicit Surface(VideoWallpaperPlayer *parent)
        : QAbstractVideoSurface(parent)
        , m_player(parent)
    {
    }

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType) const override
    {
        // toImage() 能处理所有 RGB 系格式，YUV 交给 Qt 转换
        return QList<QVideoFrame::PixelFormat>()
            << QVideoFrame::Format_RGB32
            << QVideoFrame::Format_ARGB32
            << QVideoFrame::Format_ARGB32_Premultiplied
            << QVideoFrame::Format_RGB24
            << QVideoFrame::Format_BGRA32
            << QVideoFrame::Format_YV12
            << QVideoFrame::Format_NV12;
    }

    bool present(const QVideoFrame &frame) override
    {
        if (frame.isValid()) {
            QVideoFrame copy(frame);
            const QImage image = copy.image();
            if (!image.isNull())
                Q_EMIT m_player->frameReady(image);
        }
        return true;
    }

private:
    VideoWallpaperPlayer *m_player = nullptr;
};

VideoWallpaperPlayer::VideoWallpaperPlayer(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this, QMediaPlayer::VideoSurface))
    , m_surface(new Surface(this))
{
    m_player->setMuted(true);
    m_player->setVideoOutput(reinterpret_cast<QAbstractVideoSurface *>(m_surface));

    connect(m_player, &QMediaPlayer::stateChanged, this,
            [this](QMediaPlayer::State state) {
        setPlaying(state == QMediaPlayer::PlayingState);
    });

    // Qt5 没有无限循环，播放到尾后手动从头再来
    connect(m_player, QOverload<QMediaPlayer::MediaStatus>::of(&QMediaPlayer::mediaStatusChanged),
            this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia && !m_source.isEmpty()) {
            m_player->setPosition(0);
            m_player->play();
        }
    });

    connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
            this, [this](QMediaPlayer::Error code) {
        qWarning() << "(Desktop Panel) dynamic wallpaper playback error:" << code
                   << m_player->errorString();
        Q_EMIT errorOccurred(m_player->errorString());
    });
}

VideoWallpaperPlayer::~VideoWallpaperPlayer()
{
}

bool VideoWallpaperPlayer::play(const QString &pathOrUrl)
{
    const QString path = pathOrUrl.startsWith("file:")
        ? QUrl(pathOrUrl).toLocalFile() : pathOrUrl;
    const QFileInfo info(path);
    if (path.isEmpty() || !info.isFile() || !info.isReadable()) {
        qWarning() << "(Desktop Panel) dynamic wallpaper is not a readable file:" << path;
        return false;
    }

    if (m_source == path && m_playing)
        return true;

    m_source = path;
    m_player->stop();
    m_player->setMedia(QUrl::fromLocalFile(path));
    m_player->play();
    return true;
}

void VideoWallpaperPlayer::stop()
{
    m_player->stop();
    m_player->setMedia(QMediaContent());
    m_source.clear();
    setPlaying(false);
}

bool VideoWallpaperPlayer::isPlaying() const
{
    return m_playing;
}

QString VideoWallpaperPlayer::sourcePath() const
{
    return m_source;
}

void VideoWallpaperPlayer::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;

    m_playing = playing;
    Q_EMIT playingChanged(playing);
}

#else

// Qt 5.15 以下缺乏可用的取帧接口，退化为不可用状态
VideoWallpaperPlayer::VideoWallpaperPlayer(QObject *parent)
    : QObject(parent)
{
}

VideoWallpaperPlayer::~VideoWallpaperPlayer()
{
}

bool VideoWallpaperPlayer::play(const QString &pathOrUrl)
{
    qWarning() << "(Desktop Panel) dynamic wallpaper requires Qt 5.15+ or Qt 6";
    return false;
}

void VideoWallpaperPlayer::stop()
{
}

bool VideoWallpaperPlayer::isPlaying() const
{
    return false;
}

QString VideoWallpaperPlayer::sourcePath() const
{
    return QString();
}

void VideoWallpaperPlayer::setPlaying(bool playing)
{
    Q_UNUSED(playing)
}

#endif
