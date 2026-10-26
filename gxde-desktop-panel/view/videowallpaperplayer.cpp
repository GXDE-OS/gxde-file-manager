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

#include <QPainter>
#include <QVideoFrame>
#include <QVideoSink>

// ---------------- VideoPresentationWidget ----------------

VideoPresentationWidget::VideoPresentationWidget(VideoWallpaperPlayer *player,
                                                 QWidget *parent)
    : QWidget(parent)
    , m_player(player)
    , m_sink(new QVideoSink(this))
{
    // 收帧只存引用并请求重绘：无拷贝无转换，绘制随重绘合并自动丢帧
    connect(m_sink, &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame &frame) {
        if (!frame.isValid())
            return;
        m_frame = frame;
        update();
    });
    connect(player, &VideoWallpaperPlayer::errorOccurred, this,
            [this]() {
        m_frame = QVideoFrame();
        update();
    });
}

VideoPresentationWidget::~VideoPresentationWidget()
{
    detach();
}

void VideoPresentationWidget::detach()
{
    if (!m_player)
        return;

    if (m_player->frameSink())
        disconnect(m_player->frameSink(), &QVideoSink::videoFrameChanged,
                   m_sink, &QVideoSink::setVideoFrame);
    m_player = nullptr;
    m_frame = QVideoFrame();
}

void VideoPresentationWidget::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    update();
}

QVideoSink *VideoPresentationWidget::videoSink() const
{
    return m_sink;
}

void VideoPresentationWidget::setInitialFrame(const QVideoFrame &frame)
{
    if (!frame.isValid())
        return;
    m_frame = frame;
    update();
}

void VideoPresentationWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    // 黑底（信箱模式留黑是常规表现）
    p.fillRect(rect(), Qt::black);

    if (m_frame.isValid()) {
        QVideoFrame::PaintOptions options;
        options.aspectRatioMode = m_mode;
        options.backgroundColor = Qt::black;
        // Qt 内部完成映射（HW 帧此处回读）与缩放绘制，天然保持帧率合并
        m_frame.paint(&p, QRectF(rect()), options);
    }
}

// ---------------- VideoWallpaperPlayer ----------------

VideoWallpaperPlayer::VideoWallpaperPlayer(QObject *parent)
    : QObject(parent)
    , m_sink(new QVideoSink(this))
{
    // 常驻 sink 每帧都先存一份引用（不拷贝），供暂停时新屏初始化/
    // 以及外部查询最后一帧
    connect(m_sink, &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame &frame) {
        if (frame.isValid())
            m_lastFrame = frame;
    });
}

VideoWallpaperPlayer::~VideoWallpaperPlayer()
{
    if (m_player) {
        m_player->stop();
        m_player->deleteLater();
    }
}

// 每次播放新建 QMediaPlayer：规避 stop→setSource→play 复用序列的解码
// 线程（VAAPI/NVDEC 上下文）teardown 竞态（实测段错误）。旧实例经
// deleteLater 异步销毁，其解码线程与新的完全隔离。
void VideoWallpaperPlayer::restartPlayer()
{
    if (m_player) {
        m_player->stop();
        m_player->deleteLater();
    }

    m_paused = false;

    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(nullptr);
    m_player->setVideoSink(m_sink);
    m_player->setLoops(QMediaPlayer::Infinite);

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
    restartPlayer();
    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();
    return true;
}

void VideoWallpaperPlayer::stop()
{
    m_source.clear();
    m_paused = false;
    m_lastFrame = QVideoFrame();
    setPlaying(false);

    if (m_player) {
        m_player->stop();
        m_player->deleteLater();
        m_player = nullptr;
    }
}

bool VideoWallpaperPlayer::pause()
{
    if (!m_player || !m_playing || m_paused)
        return false;
    m_player->pause();   // 保留当前帧与位置；playbackStateChanged(Paused)
    m_paused = true;     // 会经 setPlaying(false) 改变 isPlaying()，但暂停态独立记录
    return true;
}

bool VideoWallpaperPlayer::resume()
{
    if (!m_player || !m_paused)
        return false;
    m_player->play();    // 从暂停位置继续；playbackStateChanged(Playing)
    m_paused = false;    // 会经 setPlaying(true) 恢复播放态
    return true;
}

bool VideoWallpaperPlayer::isPaused() const
{
    return m_paused;
}

bool VideoWallpaperPlayer::isPlaying() const
{
    return m_playing;
}

QString VideoWallpaperPlayer::sourcePath() const
{
    return m_source;
}

QWidget *VideoWallpaperPlayer::createPresentation(QWidget *parent)
{
    auto *widget = new VideoPresentationWidget(this, parent);
    // 帧由内部 sink 扇出；QVideoFrame 为引用计数包装，跨 sink 不拷贝
    connect(m_sink, &QVideoSink::videoFrameChanged,
            widget->videoSink(), &QVideoSink::setVideoFrame,
            Qt::DirectConnection);
    // 暂停态新屏用最后一帧初始化，避免短暂黑屏
    if (m_lastFrame.isValid())
        widget->setInitialFrame(m_lastFrame);
    m_presentations.append(widget);
    return widget;
}

void VideoWallpaperPlayer::detachPresentation(QWidget *widget)
{
    auto *presentation = qobject_cast<VideoPresentationWidget *>(widget);
    if (!presentation)
        return;

    disconnect(m_sink, &QVideoSink::videoFrameChanged,
               presentation->videoSink(), &QVideoSink::setVideoFrame);
    presentation->detach();
    m_presentations.removeAll(widget);
}

void VideoWallpaperPlayer::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    for (QWidget *widget : std::as_const(m_presentations)) {
        if (auto *presentation = qobject_cast<VideoPresentationWidget *>(widget))
            presentation->setAspectRatioMode(mode);
    }
}

QVideoSink *VideoWallpaperPlayer::frameSink() const
{
    return m_sink;
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
    m_paused = false;
    m_player->stop();
    m_player->setMedia(QUrl::fromLocalFile(path));
    m_player->play();
    return true;
}

void VideoWallpaperPlayer::stop()
{
    m_paused = false;
    m_player->stop();
    m_player->setMedia(QMediaContent());
    m_source.clear();
    setPlaying(false);
}

bool VideoWallpaperPlayer::pause()
{
    if (!m_player || !m_playing || m_paused)
        return false;
    m_player->pause();
    m_paused = true;
    return true;
}

bool VideoWallpaperPlayer::resume()
{
    if (!m_player || !m_paused)
        return false;
    m_player->play();
    m_paused = false;
    return true;
}

bool VideoWallpaperPlayer::isPaused() const
{
    return m_paused;
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

bool VideoWallpaperPlayer::pause()
{
    return false;
}

bool VideoWallpaperPlayer::resume()
{
    return false;
}

bool VideoWallpaperPlayer::isPaused() const
{
    return false;
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
