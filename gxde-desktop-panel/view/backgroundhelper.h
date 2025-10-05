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

using WMInter = com::deepin::wm;
DWIDGET_USE_NAMESPACE

class BackgroundHelper : public QObject
{
    Q_OBJECT
    friend class CanvasGridView;

public:
    enum WallpaperDisplayMethods {
        KeepAspectRatioByExpanding = 0,  // 适应屏幕（默认）
        IgnoreAspectRatio = 1,  // 拉伸
        KeepAspectRatio = 2,  // 拉伸居中
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
    WallpaperDisplayMethods getWallpaperDisplayMethods();

    void onWMChanged();
    void refreshBackground();
Q_SIGNALS:
    void aboutDestoryBackground(QLabel *l);
    void enableChanged();
    void backgroundGeometryChanged(QLabel *l);
    void backgroundAdded(QLabel *l);
    void weatherImageChanged(QPixmap);
    void onScreenChanged();

private:
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
    QSize m_screenSize;
    QMap<QScreen*, QLabel*> backgroundMap;
    static BackgroundHelper *desktop_instance;
    QNetworkAccessManager m_networkManager;
    QTimer m_weatherTimer;
    WallpaperDisplayMethods m_wallpaperDisplayMethods = WallpaperDisplayMethods::KeepAspectRatioByExpanding;

public:
    static BackgroundHelper* getDesktopInstance();
};

#endif // BACKGROUNDHELPER_H
