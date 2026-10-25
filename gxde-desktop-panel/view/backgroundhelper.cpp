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
#include "backgroundhelper.h"
#include "videowallpaperplayer.h"
#include "util/xcb/xcb.h"
#include "util/wayland/layershellhelper.h"
#include "waylandutils.h"
#include "wallpaperutils.h"
#include "../presenter/display.h"

#include <QNetworkReply>
#include <QPushButton>
#include <QGridLayout>
#include <dapplication.h>
#include <QPointer>
#include <QEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QtMath>

BackgroundHelper *BackgroundHelper::desktop_instance = nullptr;

namespace {

QSize pixelSize(const QSize& logicalSize, qreal pixelRatio) {
    return QSize(qRound(logicalSize.width() * pixelRatio),
        qRound(logicalSize.height() * pixelRatio));
}

QRect pixelRect(const QRect& logicalRect, const QPoint& logicalOrigin,
        qreal pixelRatio) {
    return QRect(QPoint(qRound((logicalRect.x() - logicalOrigin.x()) * pixelRatio),
        qRound((logicalRect.y() - logicalOrigin.y()) * pixelRatio)),
            pixelSize(logicalRect.size(), pixelRatio));
}

}

BackgroundHelper::BackgroundHelper(bool preview, QObject* parent)
        : QObject(parent), m_previuew(preview)
            , windowManagerHelper(DWindowManagerHelper::instance()) {
    if (!preview) {
        connect(windowManagerHelper, &DWindowManagerHelper::windowManagerChanged,
                this, &BackgroundHelper::onWMChanged);
        connect(windowManagerHelper, &DWindowManagerHelper::hasCompositeChanged,
                this, &BackgroundHelper::onWMChanged);
        desktop_instance = this;
    }

    onWMChanged();

    m_weatherTimer.setInterval(30 * 60 * 1000);
    connect(&m_weatherTimer, &QTimer::timeout, this, &BackgroundHelper::startDownloadWeatherImage);
    connect(&m_networkManager, &QNetworkAccessManager::finished,
            this, &BackgroundHelper::downloadWeatherImageFinished);
    if (m_isLoadWeatherReport) {
        m_weatherTimer.start();
        startDownloadWeatherImage();
    }

    connect(this, &BackgroundHelper::onScreenChanged, this, &BackgroundHelper::calculateAllScreenSize);

    // QScreen notifications can be missing or delayed while a VM guest output
    // is resized.  A Wayland layer surface still receives a configure/resize,
    // so coalesce that event and repaint every background from the effective
    // surface geometry.
    m_surfaceGeometryRefreshTimer.setSingleShot(true);
    connect(&m_surfaceGeometryRefreshTimer, &QTimer::timeout, this, [this] {
        calculateAllScreenSize();
        for (QLabel *background : backgroundMap) {
            Q_EMIT backgroundGeometryChanged(background);
        }
    });

    // 用于动态检测配置文件
    QFileSystemWatcher* fileWatcher = new QFileSystemWatcher(this);
    fileWatcher->addPath(QDir::homePath() + "/.config/GXDE/dde-file-manager/wallpaperDisplayMethod");
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, &BackgroundHelper::refreshBackground);
}

BackgroundHelper::~BackgroundHelper() {
    if (desktop_instance == this) {
        desktop_instance = nullptr;
    }

    for (QLabel *l : backgroundMap) {
        l->hide();
        l->deleteLater();
    }
}

bool BackgroundHelper::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        QLabel *background = qobject_cast<QLabel *>(watched);
        if (background && backgroundMap.values().contains(background)) {
            m_surfaceGeometryRefreshTimer.start();
        }
    }

    return QObject::eventFilter(watched, event);
}

BackgroundHelper* BackgroundHelper::getDesktopInstance()
{
    return desktop_instance;
}

bool BackgroundHelper::isEnabled() const
{
    //不绘制壁纸
    if(!m_backgroundEnable)
    {
        return false;
    }

    // Wayland
    if (Wayland::LayerShellHelper::isWayland()) {
        return true;
    }

    // 只支持kwin，或未开启混成的桌面环境
    return windowManagerHelper->windowManagerName() == DWindowManagerHelper::KWinWM ||
        !windowManagerHelper->hasComposite();
}

void BackgroundHelper::setEnabled(bool enabled)
{
    m_backgroundEnable = enabled;
}

QLabel *BackgroundHelper::backgroundForScreen(QScreen *screen) const
{
    return backgroundMap.value(screen);
}

QList<QLabel *> BackgroundHelper::allBackgrounds() const
{
    return backgroundMap.values();
}

void BackgroundHelper::setBackground(const QString &path)
{
    qInfo() << "path:" << path;

    const QString newWallpaper = path.startsWith("file:") ? QUrl(path).toLocalFile() : path;

    if (m_previuew) {
        // 预览模式(壁纸选择器)：切回静态图预览时只需停掉视频预览，
        // 不能动持久化配置，也不能发DBus变更信号
        stopVideoPlayback();
    } else if (!currentWallpaper.isEmpty() && newWallpaper != currentWallpaper) {
        // 用户通过控制中心/壁纸选择器设置了新的静态壁纸时，
        // 动态壁纸让位并被移除（含多工作区切换到不同静态壁纸的情况）
        stopDynamicWallpaper();
    }

    currentWallpaper = newWallpaper;
    backgroundPixmap = QPixmap(currentWallpaper);

    refreshBackground();
}

void BackgroundHelper::setVisible(bool visible)
{
    m_visible = visible;

    for (QLabel *l : backgroundMap) {
        l->setVisible(visible);
    }
}

bool BackgroundHelper::isKWin() const
{
    return windowManagerHelper->windowManagerName() == DWindowManagerHelper::KWinWM;
}

bool BackgroundHelper::isDeepinWM() const
{
    return windowManagerHelper->windowManagerName() == DWindowManagerHelper::DeepinWM;
}

static bool wmDBusIsValid()
{
    return QDBusConnection::sessionBus().interface()->isServiceRegistered("com.deepin.wm");
}

void BackgroundHelper::onWMChanged()
{
    if (m_previuew || isEnabled()) {
        if (wmInter) {
            return;
        }

        wmInter = new WMInter("com.deepin.wm", "/com/deepin/wm", QDBusConnection::sessionBus(), this);
        gsettings = new QGSettings("com.deepin.dde.appearance", "", this);

        if (!m_previuew) {
            connect(wmInter, &WMInter::WorkspaceSwitched, this, [this] (int, int to) {
                currentWorkspaceIndex = to;
                updateBackground();
            });

            connect(gsettings, &QGSettings::changed, this, [this] (const QString &key) {
                if (key == "backgroundUris") {
                    updateBackground();
                }
            });
        }

        connect(qApp, &QGuiApplication::screenAdded, this, &BackgroundHelper::onScreenAdded);
        connect(qApp, &QGuiApplication::screenRemoved, this, &BackgroundHelper::onScreenRemoved);

        // 初始化窗口
        for (QScreen *s : qApp->screens()) {
            onScreenAdded(s);
        }

        // 初始化背景图
        updateBackground();

        // 恢复上次的视频动态壁纸（持久化在用户配置里）
        const QString savedDynamicWallpaper = readDynamicWallpaperConfig();
        if (!savedDynamicWallpaper.isEmpty()) {
            qInfo() << "(Desktop Panel) restoring dynamic wallpaper:" << savedDynamicWallpaper;
            m_dynamicWallpaper = savedDynamicWallpaper;
            m_videoFailed = false;
            if (!m_previuew)
                startVideoPlayback();
        }
    } else {
        if (!wmInter) {
            return;
        }

        // 清理数据
        if(gsettings)
        {
            gsettings->deleteLater();
            gsettings = nullptr;
        }

        if (wmInter) {
            wmInter->deleteLater();
            wmInter = nullptr;
        }

        currentWallpaper.clear();
        currentWorkspaceIndex = 0;
        backgroundPixmap = QPixmap();

        // 停止视频动态壁纸（保留配置文件，重新启用后仍可恢复）
        stopVideoPlayback();
        m_dynamicWallpaper.clear();
        m_videoFailed = false;

        disconnect(qApp, &QGuiApplication::screenAdded, this, &BackgroundHelper::onScreenAdded);
        disconnect(qApp, &QGuiApplication::screenRemoved, this, &BackgroundHelper::onScreenRemoved);

        // 销毁窗口
        for (QScreen *s : backgroundMap.keys()) {
            onScreenRemoved(s);
        }
    }

    Q_EMIT enableChanged();
}

void BackgroundHelper::updateBackground(QLabel *l)
{
    if (!l || backgroundPixmap.isNull()) {
        return;
    }

    // 动态壁纸正常播放时背景由视频帧绘制(paintVideoFrame)，
    // 跳过静态壁纸的重绘以省 CPU；播放失败后会自动回退到这里
    if (m_videoPlayer && m_videoPlayer->isPlaying() && !m_videoFailed)
        return;

    QScreen* s = backgroundMap.key(l, nullptr);
    if (!s) {
        qWarning() << "(Desktop Panel) MultiScr: Cannot find the screen for background" << l;
        return;
    }

    const qreal pixelRatio = s->devicePixelRatio();
    // A layer surface anchored to all four edges is resized by the compositor.
    // In VirtualBox/VMware that configure can arrive before Qt refreshes the
    // corresponding QScreen geometry, so use the surface's effective size for
    // painting instead of scaling the wallpaper to a stale screen size.
    const QSize logicalTargetSize = l->size().isEmpty()
        ? s->geometry().size() : l->size();
    const QSize targetPixelSize = pixelSize(logicalTargetSize, pixelRatio);
    if (targetPixelSize.isEmpty() || pixelRatio <= 0)
        return;

    QPixmap pix = backgroundPixmap;

    if (m_wallpaperDisplayMethods == WallpaperDisplayMethods::BackgroundSpanned) {
        qreal canvasPixelRatio = 1.0;
        for (QScreen* screen : QGuiApplication::screens()) {
            if (screen) {
                canvasPixelRatio = qMax(canvasPixelRatio, screen->devicePixelRatio());
            }
        }

        const QSize canvasPixelSize = pixelSize(m_screenGeometry.size(), canvasPixelRatio);
        if (canvasPixelSize.isEmpty()) {
            return;
        }

        pix = pix.scaled(canvasPixelSize, Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation);
        if (pix.width() > canvasPixelSize.width() || pix.height() > canvasPixelSize.height()) {
            pix = pix.copy(QRect((pix.width() - canvasPixelSize.width()) / 2,
                (pix.height() - canvasPixelSize.height()) / 2,
                canvasPixelSize.width(), canvasPixelSize.height()));
        }

        const QRect logicalGeometry = l->geometry().isValid()
            ? l->geometry() : s->geometry();
        pix = pix.copy(pixelRect(logicalGeometry, m_screenGeometry.topLeft(),
            canvasPixelRatio));
        if (pix.size() != targetPixelSize) {
            pix = pix.scaled(targetPixelSize, Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation);
        }
    } else if (m_wallpaperDisplayMethods != WallpaperDisplayMethods::Center) {
        pix = pix.scaled(targetPixelSize,
                         wallpaperDisplayMethods2PictureRatioMode(m_wallpaperDisplayMethods),
                         Qt::SmoothTransformation);
    }

    if (m_wallpaperDisplayMethods != WallpaperDisplayMethods::BackgroundSpanned
            && (pix.width() > targetPixelSize.width()
                || pix.height() > targetPixelSize.height())) {
        pix = pix.copy(QRect((pix.width() - targetPixelSize.width()) / 2.0,
                (pix.height() - targetPixelSize.height()) / 2.0,
                targetPixelSize.width(),
                targetPixelSize.height()));
    }
    // 只有在 KeepAspectRatio 模式（居中）下的背景才设置居中
    if (m_wallpaperDisplayMethods == WallpaperDisplayMethods::KeepAspectRatio ||
        m_wallpaperDisplayMethods == WallpaperDisplayMethods::Center) {
        l->setAlignment(Qt::AlignCenter);
    }
    else {
        l->setAlignment(Qt::AlignLeft);
    }

    pix.setDevicePixelRatio(pixelRatio);
    l->setPixmap(pix);

    qInfo() << s << currentWallpaper << pix;
}

void BackgroundHelper::updateBackground()
{
    QString path = wmDBusIsValid() ? wmInter->GetCurrentWorkspaceBackground() : QString();

    if (path.isEmpty()
            // 调用失败时会返回 "The name com.deepin.wm was not provided by any .service files"
            // 此时 wmInter->isValid() = true, 且 dubs last error type 为 NoError
            || (!path.startsWith("/") && !path.startsWith("file:"))) {
        path = gsettings->get("background-uris").toStringList().value(currentWorkspaceIndex);

        if (path.isEmpty())
            return;
    }

    setBackground(path);
}

void BackgroundHelper::startDownloadWeatherImage()
{
    QNetworkRequest request;
    request.setUrl(QUrl("https://wttr.in/~.png?lang=zh&transparency=200&tqnp"));
    m_networkManager.get(request);
}

void BackgroundHelper::downloadWeatherImageFinished(QNetworkReply *reply)
{
    QByteArray bytes = reply->readAll();
    QPixmap pixmap;
    pixmap.loadFromData(bytes);
    m_weatherImage = pixmap;
    emit weatherImageChanged(m_weatherImage);
}

void BackgroundHelper::onScreenAdded(QScreen* screen) {
    if (!screen || backgroundMap.contains(screen)) {
        return;
    }

    QLabel *l = new QLabel();
    QLabel *weather = new QLabel();
    QPointer<QScreen> screenGuard(screen);
    connect(this, &BackgroundHelper::weatherImageChanged, weather, [weather, this, screenGuard](){
        weather->setVisible(m_isLoadWeatherReport);
        if (!screenGuard)
            return;

        const QRect rect = screenGuard->geometry();
        // 在壁纸跨屏模式下，天气预报只显示在最右上角的屏幕
        if (m_wallpaperDisplayMethods == WallpaperDisplayMethods::BackgroundSpanned) {
            if (rect.top() != m_screenGeometry.top()
                    || rect.right() != m_screenGeometry.right()) {
                weather->clear();
                return;
            }
        }
        weather->setPixmap(m_weatherImage);
    });
    weather->setAlignment(Qt::AlignRight);
    weather->setPixmap(m_weatherImage);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(weather);
    layout->addStretch();

    l->setLayout(layout);

    backgroundMap[screen] = l;
    l->installEventFilter(this);

    l->createWinId();
    l->windowHandle()->setScreen(screen);
    l->setGeometry(screen->geometry());
    l->setStyleSheet("background: transparent;");
    l->setAlignment(Qt::AlignCenter);

    QPointer<QLabel> labelGuard(l);

    if (m_previuew) {
        Qt::WindowFlags flags =
            l->windowFlags() | Qt::BypassWindowManagerHint;
        // Wayland下预览背景需要接受鼠标点击 (点击空白处关闭选择器),
        // 所以不设WindowDoesNotAcceptFocus
        if (!WaylandUtils::isWaylandSession()) {
            flags |= Qt::WindowDoesNotAcceptFocus;
        }
        l->setWindowFlags(flags);
    } else {
        if (!WaylandUtils::isWaylandSession()) {
            Xcb::XcbMisc::instance().set_window_type(l->winId(), Xcb::XcbMisc::Desktop);
        }
    }

    // Wayland补丁
    if (Wayland::LayerShellHelper::isWayland()) {
        // 理论上layer-shell会处理好窗口边框的事，但Wayland合成器实现众多...
        // 为了防止谁阴我一手，主动开FramelessWindowHint
        l->setWindowFlag(Qt::FramelessWindowHint, true);

        if (m_previuew) {
            // 预览背景: 必须置于桌面壁纸)之上才可见,
            // 且需要接受鼠标点击 (点击空白处关闭选择器)
            l->setAttribute(Qt::WA_ShowWithoutActivating, false);
            Wayland::LayerShellHelper::setPreviewBackdropRole(
                l, screen, QStringLiteral("wallpaper-chooser-backdrop"));
        } else {
            l->setAttribute(Qt::WA_ShowWithoutActivating, true);

            // Treeland支持
            Wayland::LayerShellHelper::setDesktopRole(
                l, screen, QStringLiteral("dde-shell/desktop"));
        }
    }

    if (m_visible)
        l->show();
    else
        qDebug() << "Disable show the background widget, of screen:" << screen << screen->geometry();

    const auto refreshScreenGeometry = [labelGuard, screenGuard, this] {
        if (!labelGuard || !screenGuard)
            return;

        qDebug() << "(Panel) MultiScr: screen geometry changed:" << screenGuard << screenGuard->geometry();

        labelGuard->setGeometry(screenGuard->geometry());

        calculateAllScreenSize();
        Q_EMIT backgroundGeometryChanged(labelGuard);
    };
    connect(screen, &QScreen::geometryChanged, l, [refreshScreenGeometry] {
        refreshScreenGeometry();
    });
    connect(screen, &QScreen::logicalDotsPerInchChanged, l, [refreshScreenGeometry] {
        refreshScreenGeometry();
    });
    connect(DesktopDisplay::instance(), &DesktopDisplay::primaryScreenChanged,
            l, [this, labelGuard, screenGuard](QScreen *primary) {
        if (!labelGuard || !screenGuard || primary != screenGuard) {
            return;
        }

        const QRect geometry = DesktopDisplay::instance()->primaryGeometry();
        if (!geometry.isEmpty()) {
            labelGuard->setGeometry(geometry);
        }
        calculateAllScreenSize();
        Q_EMIT backgroundGeometryChanged(labelGuard);
    });

    // 可能是由QGuiApplication引发的新屏幕添加，此处应该为新对象添加背景图
    updateBackground(l);

    Q_EMIT backgroundGeometryChanged(l);
    Q_EMIT backgroundAdded(l);

    qInfo() << screen << screen->geometry();

    Q_EMIT onScreenChanged();
}

void BackgroundHelper::calculateAllScreenSize()
{
    QRect geometry;
    for (QScreen* screen : QGuiApplication::screens()) {
        if (!screen) {
            continue;
        }

        QLabel *background = backgroundMap.value(screen, nullptr);
        // Prefer the layer surface geometry after a compositor configure.  It
        // is the authoritative size while QScreen catches up with a VM resize.
        const QRect screenGeometry = background && background->geometry().isValid()
            ? background->geometry() : screen->geometry();
        geometry = geometry.isNull() ? screenGeometry : geometry.united(screenGeometry);
    }

    m_screenGeometry = geometry;
    for (QLabel *l: backgroundMap) {
        updateBackground(l);
    }
}

void BackgroundHelper::onScreenRemoved(QScreen *screen)
{
    if (QLabel *l = backgroundMap.take(screen)) {
        Q_EMIT aboutDestoryBackground(l);

        l->deleteLater();
    }

    qInfo() << screen;

    Q_EMIT onScreenChanged();
}

void BackgroundHelper::refreshBackground()
{
    m_isLoadWeatherReport = QFile::exists(QDir::homePath() + "/.config/GXDE/dde-file-manager/weatherReport");
    m_wallpaperDisplayMethods = getWallpaperDisplayMethodsConfigData();
    if (m_isLoadWeatherReport) {
        m_weatherTimer.start();
        startDownloadWeatherImage();
    }
    else {
        m_weatherTimer.stop();
        emit weatherImageChanged(QPixmap());
    }
    calculateAllScreenSize();
}

void BackgroundHelper::setWallpaperDisplayMethods(WallpaperDisplayMethods method)
{
    m_wallpaperDisplayMethods = method;

    if (!QFile::exists(QDir::homePath() + "/.config/GXDE/dde-file-manager/")) {
        QDir dir(QDir::homePath() + "/.config/GXDE/dde-file-manager/");
        dir.mkpath(QDir::homePath() + "/.config/GXDE/dde-file-manager/");
    }
    QFile file(QDir::homePath() + "/.config/GXDE/dde-file-manager/wallpaperDisplayMethod");
    file.open(QFile::WriteOnly);
    file.write(QString::number(method).toUtf8());
    file.close();

    refreshBackground();
}

BackgroundHelper::WallpaperDisplayMethods BackgroundHelper::getWallpaperDisplayMethods() const {
    return m_wallpaperDisplayMethods;
}

Qt::AspectRatioMode BackgroundHelper::wallpaperDisplayMethods2PictureRatioMode
    (BackgroundHelper::WallpaperDisplayMethods method)
{
    switch (method) {
    case BackgroundHelper::KeepAspectRatioByExpanding:
    case BackgroundHelper::BackgroundSpanned:
        return Qt::AspectRatioMode::KeepAspectRatioByExpanding;
        break;
    case BackgroundHelper::IgnoreAspectRatio:
        return Qt::AspectRatioMode::IgnoreAspectRatio;
    case BackgroundHelper::KeepAspectRatio:
        return Qt::AspectRatioMode::KeepAspectRatio;
    default:
        return Qt::AspectRatioMode::KeepAspectRatioByExpanding;
        break;
    }
}

BackgroundHelper::WallpaperDisplayMethods BackgroundHelper::getWallpaperDisplayMethodsConfigData()
{
    QFile file(QDir::homePath() + "/.config/GXDE/dde-file-manager/wallpaperDisplayMethod");
    if (!file.exists()) {
        return BackgroundHelper::WallpaperDisplayMethods::KeepAspectRatioByExpanding;
    }
    file.open(QFile::ReadOnly);
    QString data = file.readAll();
    file.close();
    return static_cast<BackgroundHelper::WallpaperDisplayMethods>(data.toInt());
}

QString BackgroundHelper::dynamicWallpaperConfigPath()
{
    return QDir::homePath() + "/.config/GXDE/dde-file-manager/dynamicWallpaper";
}

QString BackgroundHelper::readDynamicWallpaperConfig() const
{
    QFile file(dynamicWallpaperConfigPath());
    if (!file.exists() || !file.open(QFile::ReadOnly))
        return QString();

    const QString path = QString::fromUtf8(file.readAll()).trimmed();
    file.close();

    // 配置指向的文件已被删除（例如移动设备拔出）时视为没有动态壁纸
    if (path.isEmpty() || !QFile::exists(path))
        return QString();

    return path;
}

bool BackgroundHelper::setDynamicWallpaper(const QString &pathOrUrl)
{
    QString path = pathOrUrl.startsWith("file:") ? QUrl(pathOrUrl).toLocalFile() : pathOrUrl;
    path = path.trimmed();

    // 空路径 = 停用动态壁纸
    if (path.isEmpty()) {
        stopDynamicWallpaper();
        return true;
    }

    const QFileInfo info(path);
    if (!info.isFile() || !info.isReadable()) {
        qWarning() << "(Desktop Panel) dynamic wallpaper file is not readable:" << path;
        return false;
    }

    if (!WallpaperUtils::isVideoFile(path)) {
        qWarning() << "(Desktop Panel) dynamic wallpaper is not a video file:" << path;
        return false;
    }

    // 拷贝到壁纸库，避免视频来自可移动设备、拔出后壁纸丢失
    const QString persisted = WallpaperUtils::persistWallpaperToLibrary(path);
    m_dynamicWallpaper = persisted;
    m_videoFailed = false;

    // 持久化，面板重启后自动恢复
    QDir configDir = QDir(QDir::homePath() + "/.config/GXDE/dde-file-manager");
    if (!configDir.exists())
        configDir.mkpath(".");
    QFile file(dynamicWallpaperConfigPath());
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        file.write(persisted.toUtf8());
        file.close();
    } else {
        qWarning() << "(Desktop Panel) failed to save dynamic wallpaper config:"
                   << dynamicWallpaperConfigPath() << file.errorString();
    }

    if (!m_previuew && isEnabled())
        startVideoPlayback();

    Q_EMIT dynamicWallpaperChanged(persisted);
    return true;
}

bool BackgroundHelper::previewDynamicWallpaper(const QString &pathOrUrl)
{
    const QString path = WallpaperUtils::localPathOf(pathOrUrl.trimmed());
    if (!WallpaperUtils::isVideoFile(path)) {
        qWarning() << "(Desktop Panel) dynamic wallpaper preview is not a video file:" << path;
        return false;
    }

    m_dynamicWallpaper = path;
    m_videoFailed = false;

    if (isEnabled())
        startVideoPlayback();

    return true;
}

void BackgroundHelper::stopVideoPreview()
{
    stopVideoPlayback();
    m_videoFailed = false;
}

void BackgroundHelper::stopDynamicWallpaper()
{
    stopVideoPlayback();
    m_videoFailed = false;

    if (m_dynamicWallpaper.isEmpty())
        return;

    m_dynamicWallpaper.clear();
    QFile::remove(dynamicWallpaperConfigPath());

    // 恢复静态壁纸
    refreshBackground();
    Q_EMIT dynamicWallpaperChanged(QString());
}

QString BackgroundHelper::dynamicWallpaperPath() const
{
    return m_dynamicWallpaper;
}

bool BackgroundHelper::isDynamicWallpaperActive() const
{
    return m_videoPlayer && m_videoPlayer->isPlaying();
}

void BackgroundHelper::startVideoPlayback()
{
    if (m_dynamicWallpaper.isEmpty())
        return;

    if (!m_videoPlayer) {
        m_videoPlayer = new VideoWallpaperPlayer(this);
        connect(m_videoPlayer, &VideoWallpaperPlayer::frameReady,
                this, &BackgroundHelper::paintVideoFrame);
        connect(m_videoPlayer, &VideoWallpaperPlayer::errorOccurred,
                this, [this](const QString &reason) {
            // 文件损坏/缺少解码器等异步解码错误：
            // 停止播放并回退到静态壁纸（保留配置，下次启动重试）
            qWarning() << "(Desktop Panel) dynamic wallpaper falls back to static:" << reason;
            m_videoFailed = true;
            stopVideoPlayback();
            refreshBackground();
        });
    }

    m_videoFailed = false;
    m_videoPlayer->play(m_dynamicWallpaper);
}

void BackgroundHelper::stopVideoPlayback()
{
    if (m_videoPlayer)
        m_videoPlayer->stop();
}

void BackgroundHelper::paintVideoFrame(const QImage &frame)
{
    if (frame.isNull() || !m_visible || m_dynamicWallpaper.isEmpty())
        return;

    for (auto it = backgroundMap.constBegin(); it != backgroundMap.constEnd(); ++it) {
        QScreen *s = it.key();
        QLabel *l = it.value();
        if (!s || !l)
            continue;

        const qreal pixelRatio = s->devicePixelRatio();
        const QSize logicalTargetSize = l->size().isEmpty()
            ? s->geometry().size() : l->size();
        const QSize targetPixelSize = pixelSize(logicalTargetSize, pixelRatio);
        if (targetPixelSize.isEmpty() || pixelRatio <= 0)
            continue;

        QImage img = frame;
        if (m_wallpaperDisplayMethods == WallpaperDisplayMethods::IgnoreAspectRatio) {
            // 拉伸
            img = img.scaled(targetPixelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            l->setAlignment(Qt::AlignLeft);
        } else if (m_wallpaperDisplayMethods == WallpaperDisplayMethods::KeepAspectRatio
                   || m_wallpaperDisplayMethods == WallpaperDisplayMethods::Center) {
            // 适应居中/居中：不裁剪，QLabel 按对齐方式居中显示
            if (m_wallpaperDisplayMethods == WallpaperDisplayMethods::KeepAspectRatio) {
                img = img.scaled(targetPixelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            l->setAlignment(Qt::AlignCenter);
        } else {
            // 适应屏幕(默认)/跨屏：铺满屏幕并居中裁剪
            img = img.scaled(targetPixelSize, Qt::KeepAspectRatioByExpanding,
                             Qt::SmoothTransformation);
            if (img.width() > targetPixelSize.width()
                    || img.height() > targetPixelSize.height()) {
                img = img.copy(QRect((img.width() - targetPixelSize.width()) / 2,
                        (img.height() - targetPixelSize.height()) / 2,
                        targetPixelSize.width(),
                        targetPixelSize.height()));
            }
            l->setAlignment(Qt::AlignLeft);
        }

        QPixmap pix = QPixmap::fromImage(img);
        pix.setDevicePixelRatio(pixelRatio);
        l->setPixmap(pix);
    }
}
