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
class VideoWallpaperPlayer;
class QVideoSink;
class QVideoWidget;
#else
class QAbstractVideoSurface;
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QWidget>
#include <QVideoFrame>

/**
 * @brief raster 自绘的视频呈现控件
 *
 * 不能用 QVideoWidget：它内部走 GL/RHI 合成，内容绘制在窗口 backing
 * store 之上，会盖住同窗口的所有 alien 兄弟（桌面图标层）——图标事件
 * 正常但绘制不可见。本控件是普通 raster alien 控件，与图标层按正常
 * 栈序合成。
 *
 * 帧处理：sink 回调仅保存最新帧引用并 update()（无拷贝/转换），实际
 * 的映射与缩放发生在 paintEvent 的 QVideoFrame::paint() 里，由 Qt 的
 * 重绘合并自然丢帧——忙时不堆积。
 */
class VideoPresentationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPresentationWidget(VideoWallpaperPlayer *player,
                                     QWidget *parent = nullptr);
    ~VideoPresentationWidget() override;

    void detach(); // 断开与播放器的帧转发（销毁前调用）
    void setAspectRatioMode(Qt::AspectRatioMode mode);
    QVideoSink *videoSink() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    VideoWallpaperPlayer *m_player = nullptr;
    QVideoSink *m_sink = nullptr;
    QVideoFrame m_frame;
    Qt::AspectRatioMode m_mode = Qt::KeepAspectRatioByExpanding;
};
#endif

/**
 * @brief 视频动态壁纸播放器
 *
 * Qt6：每屏一个 VideoPresentationWidget 呈现，帧经内部 sink 扇出；
 * 不抛 QImage（4K@60fps 逐帧转换会把 UI 线程饿死）。硬解由 ffmpeg
 * 后端自动协商（可用时），HW 帧仅在绘制时映射。
 *
 * QMediaPlayer 每次播放都新建实例：QMediaPlayer stop→setSource→play
 * 复用序列存在解码线程（含 VAAPI/NVDEC 上下文）teardown 竞态，实测
 * 会导致段错误；换新实例把旧解码器的销毁完全隔离在 deleteLater 里。
 *
 * Qt5.15：无公开 QVideoSink，保留逐帧 QImage 路径（frameReady 信号），
 * 由 BackgroundHelper 绘制到背景窗口。
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // 为一块屏创建接入当前播放流的呈现控件（调用方负责设置父子与几何），
    // 帧由内部 sink 扇出；销毁前须调用 detachPresentation()
    QWidget *createPresentation(QWidget *parent);
    // 屏被移除时归还呈现控件（断开帧转发）
    void detachPresentation(QWidget *widget);
    // 应用填充方式（拉伸/适应/居中/跨屏的映射由调用方决定）
    void setAspectRatioMode(Qt::AspectRatioMode mode);
    // 内部帧 sink（供抓帧等外部转发；Qt5 路径无此概念）
    QVideoSink *frameSink() const;
#endif

Q_SIGNALS:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5 路径：每解码出一帧就会发出，frame 可能为空（调用方需判断）
    void frameReady(const QImage &frame);
#endif
    void playingChanged(bool playing);
    void errorOccurred(const QString &reason);

private:
    void setPlaying(bool playing);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void restartPlayer();
#endif

    QString m_source;
    bool m_playing = false;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMediaPlayer *m_player = nullptr;
    QVideoSink *m_sink = nullptr;          // player 的输出（扇出源，常驻）
    QList<QWidget *> m_presentations;      // VideoPresentationWidget*
#else
    class Surface;
    QMediaPlayer *m_player = nullptr;
    Surface *m_surface = nullptr;
#endif
};

#endif // VIDEOWALLPAPERPLAYER_H
