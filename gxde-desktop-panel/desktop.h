/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#ifndef DESKTOP_H
#define DESKTOP_H

#include <QObject>
#include <QScopedPointer>

#include "global/singleton.h"

#define DesktopServiceName          "com.deepin.dde.desktop"
#define DesktopServicePath          "/com/deepin/dde/desktop"
#define DesktopServiceInterface     "com.deepin.dde.desktop"

class QDBusConnection;
class CanvasGridView;
class QStyleOptionViewItem;
class DesktopPrivate;
class Desktop : public QObject, public Singleton<Desktop>
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", DesktopServiceInterface)
public:
    void loadData();
    void loadView();

    void showWallpaperSettings();
    void showZoneSettings();

    void initDebugDBus(QDBusConnection &conn);

    CanvasGridView *getView();

public slots:
    void Show();
    Q_SCRIPTABLE void EnableBackground(bool v);

    // ==== 壁纸 DBus 接口（com.deepin.dde.desktop）====
    // 供其它程序设置/查询壁纸。参数支持本地路径和 file:// URL
    //
    // 设置视频动态壁纸：path 为视频文件(常见 mp4/webm/mkv 等)，成功返回 true；
    // path 传空串则停用动态壁纸并回退静态壁纸。
    // 例: qdbus com.deepin.dde.desktop /com/deepin/dde/desktop \
    //      com.deepin.dde.desktop.SetDynamicWallpaper "/home/user/bg.mp4"
    Q_SCRIPTABLE bool SetDynamicWallpaper(const QString &path);
    // 查询当前视频动态壁纸路径，未启用时返回空串
    Q_SCRIPTABLE QString DynamicWallpaper() const;
    // 暂停/恢复当前视频动态壁纸；非动态壁纸时调用无效
    // 例: qdbus com.deepin.dde.desktop /com/deepin/dde/desktop \
    //      com.deepin.dde.desktop.SetDynamicWallpaperPaused true
    Q_SCRIPTABLE void SetDynamicWallpaperPaused(bool paused);
    // 查询当前动态壁纸是否处于暂停态，非动态壁纸时返回 false
    Q_SCRIPTABLE bool DynamicWallpaperPaused() const;
    // 查询当前视频动态壁纸是否正在播放（用于诊断；非动态壁纸/解码失败返回 false）
    Q_SCRIPTABLE bool DynamicWallpaperPlaying() const;
    // 设置静态壁纸（写入 com.deepin.daemon.Appearance 与 GSettings，
    // 生效后会自动停用动态壁纸），成功返回 true
    Q_SCRIPTABLE bool SetWallpaper(const QString &path);

Q_SIGNALS:
    // 视频动态壁纸启用/停用/更换时发出，空串表示已停用
    void DynamicWallpaperChanged(const QString &path);
    // 动态壁纸暂停状态变化时发出
    void DynamicWallpaperPausedChanged(bool paused);

private:
    explicit Desktop();
    ~Desktop();

    void onBackgroundEnableChanged();
    void onBackgroundGeometryChanged(QWidget *l);

    friend class Singleton<Desktop>;
    Q_DISABLE_COPY(Desktop)

    QScopedPointer<DesktopPrivate> d;
};

#endif // DESKTOP_H
