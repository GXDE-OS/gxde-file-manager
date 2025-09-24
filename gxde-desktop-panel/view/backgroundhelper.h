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
    explicit BackgroundHelper(bool preview = false, QObject *parent = nullptr);
    ~BackgroundHelper();

    bool isEnabled() const;
    void setEnabled(bool enabled);
    QLabel *backgroundForScreen(QScreen *screen) const;
    QList<QLabel*> allBackgrounds() const;

    void setBackground(const QString &path);
    void setVisible(bool visible);
    void setPictureRatioMode(Qt::AspectRatioMode mode);

    void onWMChanged();
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

    bool m_isBackgroundSpanned = QFile::exists(QDir::homePath() + "/.config/GXDE/dde-file-manager/backgroundSpanned");
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
    Qt::AspectRatioMode m_pictureRatioMode = Qt::AspectRatioMode::KeepAspectRatioByExpanding;
    static BackgroundHelper *desktop_instance;
    QNetworkAccessManager m_networkManager;
    QTimer m_weatherTimer;

public:
    static BackgroundHelper* getDesktopInstance();
};

#endif // BACKGROUNDHELPER_H
