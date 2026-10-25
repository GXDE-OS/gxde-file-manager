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
#ifndef VIDEOTHUMBGRABBER_H
#define VIDEOTHUMBGRABBER_H

#include <QObject>
#include <QPixmap>
#include <QSet>

class VideoWallpaperPlayer;

/**
 * @brief 视频壁纸缩略图抓取器
 *
 * 从视频里取第一帧生成缩略图，结果按 ThumbnailManager 的缓存布局
 * 落盘(CacheLocation/<version>/<scale>/<percentKey>.png)，信号签名与
 * ThumbnailManager::thumbnailFounded 一致，WallpaperItem 可以直接复用
 * onThumbnailFounded 槽。
 *
 * 抓取失败(无法解码/超时)时发出一个占位缩略图(深色底+播放三角)，
 * 保证列表项不会是一片空白。
 */
class VideoThumbGrabber : public QObject
{
    Q_OBJECT
public:
    static VideoThumbGrabber *instance();

    // key 为 WallpaperItem 使用的 QUrl::toPercentEncoding(m_path)，
    // videoPath 为视频的实际本地路径
    void grab(const QString &key, const QString &videoPath, qreal scale);

Q_SIGNALS:
    void thumbnailFounded(const QString &key, const QPixmap &pixmap);

private:
    explicit VideoThumbGrabber(QObject *parent = nullptr);

    QString cacheFileForKey(const QString &key, qreal scale) const;
    QPixmap loadGrabbedFrame(const QString &videoPath, qreal scale) const;

    QSet<QString> m_pendingRequests;
};

#endif // VIDEOTHUMBGRABBER_H
