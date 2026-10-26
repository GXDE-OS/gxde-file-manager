/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
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
#ifndef BACKGROUNDHELPER_H
#define BACKGROUNDHELPER_H

#include <QLabel>

#include <com_deepin_wm.h>
#include <QGSettings>
#include <DWindowManagerHelper>
#include <QNetworkAccessManager>
#include <QFile>
#include <QHash>
#include <QTimer>

using WMInter = com::deepin::wm;
DWIDGET_USE_NAMESPACE

class VideoWallpaperPlayer;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class VideoPresentationWidget;
#endif

class BackgroundHelper : public QObject
{
    Q_OBJECT
    friend class CanvasGridView;

public:
    enum WallpaperDisplayMethods {
        KeepAspectRatioByExpanding = 0,  // 适应屏幕（默认）
        IgnoreAspectRatio = 1,  // 拉伸
        KeepAspectRatio = 2,  // 适应居中
        Center = 3,  // 居中
        BackgroundSpanned = 4  // 跨屏显示
    };

    explicit BackgroundHelper(bool preview = false, QObject *parent = nullptr);
    ~BackgroundHelper();

    bool isEnabled() const;
    void setEnabled(bool enabled);
    QLabel *backgroundForScreen(QScreen *screen) const;
    QList<QLabel*> allBackgrounds() const;

    void setBackground(const QString &path);
    void setVisible(bool visible);
    void setWallpaperDisplayMethods(WallpaperDisplayMethods method);
    Qt::AspectRatioMode wallpaperDisplayMethods2PictureRatioMode(WallpaperDisplayMethods method);
    WallpaperDisplayMethods getWallpaperDisplayMethods() const;

    // ==== 视频动态壁纸 ====
    // pathOrUrl 为空串时停用动态壁纸；文件不存在或不是视频时返回 false。
    // 设置成功后会持久化，面板重启后自动恢复
    bool setDynamicWallpaper(const QString &pathOrUrl);
    // 预览模式专用(壁纸选择器)：播放视频但不持久化、不发变更信号，
    // 供选择器在其预览背景层上实时预览视频壁纸
    bool previewDynamicWallpaper(const QString &pathOrUrl);
    // 只停掉视频预览播放，不动m_dynamicWallpaper与持久化配置
    void stopVideoPreview();
    void stopDynamicWallpaper();
    QString dynamicWallpaperPath() const;
    bool isDynamicWallpaperActive() const;

    // 动态壁纸模式是否生效（视频文件存在且未解码失败），与播放/暂停状态无关。
    // 静态壁纸下返回 false，供桌面右键菜单决定是否显示暂停/播放项
    bool hasDynamicWallpaper() const;
    // 当前动态壁纸是否处于暂停态
    bool isDynamicWallpaperPaused() const;
    // 暂停/恢复动态视频壁纸（保留当前帧与位置）；非动态壁纸时无效
    void pauseDynamicWallpaper();
    void resumeDynamicWallpaper();

    void onWMChanged();
    void refreshBackground();
Q_SIGNALS:
    void aboutDestoryBackground(QLabel *l);
    void enableChanged();
    void backgroundGeometryChanged(QLabel *l);
    void backgroundAdded(QLabel *l);
    void weatherImageChanged(QPixmap);
    void onScreenChanged();
    void dynamicWallpaperChanged(const QString &path);
    void dynamicWallpaperPausedChanged(bool paused);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool isKWin() const;
    bool isDeepinWM() const;
    void updateBackground(QLabel *l);
    void updateBackground();
    void onScreenAdded(QScreen *screen);
    void onScreenRemoved(QScreen *screen);
    void startDownloadWeatherImage();
    void downloadWeatherImageFinished(QNetworkReply *reply);
    void calculateAllScreenSize();
    WallpaperDisplayMethods getWallpaperDisplayMethodsConfigData();
    void startVideoPlayback();
    void stopVideoPlayback();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6：每屏一个 raster 自绘呈现控件（见 videowallpaperplayer.h 的说明）
    void attachVideoWidget(QLabel *background);
    void detachVideoWidget(QLabel *background);
    Qt::AspectRatioMode videoAspectRatioMode() const;
#else
    // Qt5：逐帧 QImage 绘制路径
    void paintVideoFrame(const QImage &frame);
#endif
    static QString dynamicWallpaperConfigPath();
    QString readDynamicWallpaperConfig() const;

    bool m_isLoadWeatherReport = QFile::exists(QDir::homePath() + "/.config/GXDE/dde-file-manager/weatherReport");
    bool m_previuew = false;
    bool m_visible = true;
    bool m_backgroundEnable = true; // 背景是否显示
    QGSettings          *gsettings          = nullptr;
    WMInter             *wmInter            = nullptr;
    DWindowManagerHelper* windowManagerHelper = nullptr;
    int currentWorkspaceIndex = 0;
    QString currentWallpaper;
    QPixmap backgroundPixmap;
    QPixmap m_weatherImage;
    QRect m_screenGeometry;
    QMap<QScreen*, QLabel*> backgroundMap;
    static BackgroundHelper *desktop_instance;
    QNetworkAccessManager m_networkManager;
    QTimer m_weatherTimer;
    QTimer m_surfaceGeometryRefreshTimer;
    WallpaperDisplayMethods m_wallpaperDisplayMethods = WallpaperDisplayMethods::KeepAspectRatioByExpanding;

    VideoWallpaperPlayer *m_videoPlayer = nullptr;
    QString m_dynamicWallpaper;
    bool m_videoFailed = false;
    bool m_dynamicPaused = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // 每块屏一个呈现控件（Qt6 raster 自绘路径，兼容图标层栈序）
    QHash<QLabel *, VideoPresentationWidget *> m_videoWidgets;
#endif

public:
    static BackgroundHelper* getDesktopInstance();
};

#endif // BACKGROUNDHELPER_H
