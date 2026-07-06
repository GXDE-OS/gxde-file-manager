/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include <QDebug>
#include <QDBusError>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QThreadPool>
#include <QPixmapCache>
#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QSettings>
#include <QWidget>
#include <QWindow>
#include <QElapsedTimer>
#include <QCursor>
#include <QMouseEvent>
#include <QSet>

#include <DLog>
#include <DApplication>

#include <unistd.h>

#include <LayerShellQt/Shell>

#include <dfmglobal.h>
#include <dfmapplication.h>

#include "util/dde/ddesession.h"
#include "util/wayland/layershellhelper.h"
#include "waylandutils.h"

#include "config/config.h"
#include "desktop.h"
#include "view/canvasgridview.h"

#include "deventfilter.h"

// DBus
#include "filedialogmanager_adaptor.h"
#include "dbusfiledialogmanager.h"
#include "filemanager1_adaptor.h"
#include "dbusfilemanager1.h"


using namespace Dtk::Core;
using namespace Dtk::Widget;

DFM_USE_NAMESPACE

static bool registerDialogDBus()
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.\n"
                 "Please check your system settings and try again.\n");
        return false;
    }

    // add our D-Bus interface and connect to D-Bus
    if (!QDBusConnection::sessionBus().registerService("com.deepin.filemanager.filedialog")) {
        qWarning("Cannot register the \"com.deepin.filemanager.filedialog\" service.\n");
        return false;
    }

    DBusFileDialogManager *manager = new DBusFileDialogManager();
    Q_UNUSED(new FiledialogmanagerAdaptor(manager));

    if (!QDBusConnection::sessionBus().registerObject("/com/deepin/filemanager/filedialogmanager", manager)) {
        qWarning("Cannot register to the D-Bus object: \"/com/deepin/filemanager/filedialogmanager\"\n");
        manager->deleteLater();
        return false;
    }

    return true;
}

static bool registerFileManager1DBus()
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.\n"
                 "Please check your system settings and try again.\n");
        return false;
    }

    // add our D-Bus interface and connect to D-Bus
    if (!QDBusConnection::sessionBus().registerService("org.freedesktop.FileManager1")) {
        qWarning("Cannot register the \"org.freedesktop.FileManager1\" service.\n");
        return false;
    }

    DBusFileManager1 *manager = new DBusFileManager1();
    Q_UNUSED(new FileManager1Adaptor(manager));

    if (!QDBusConnection::sessionBus().registerObject("/org/freedesktop/FileManager1", manager)) {
        qWarning("Cannot register to the D-Bus object: \"/org/freedesktop/FileManager1\"\n");
        manager->deleteLater();
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    // Treeland下遇到了桌面文件重命名/ESC键失效的问题
    // 具体原因分析请见: ./doc/notes/treeland-desktop-panel-key-focus-issue.md
    // 打了补丁，当前Treeland下的重命名由子项目gxde-rename-interface-treeland负责
    // 其它WM的重命名依然走原版逻辑

    // Fixed the locale codec to utf-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

    if (QFile("/usr/lib/gxde-desktop-panel/plugins/platform/libdxcb.so").exists()) {
        qDebug() << "load dxcb from local path: /usr/lib/gxde-desktop-panel/plugins/platform/libdxcb.so";
        qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", "/usr/lib/gxde-desktop-panel/plugins/platform");
    }

    // Treeland/startgxde会设置DTK2_XWAYLAND=dxcb
    // 其与原生 Wayland + layer-shell不兼容，导致段错误
    // 需要在DApplication构造期间暂时屏蔽，构造完再恢复
    // 这样保证面板不走D-XCB，但叫子进程仍然能走D-XCB
    const QByteArray savedDtk2XWayland = qgetenv("DTK2_XWAYLAND");

    if (WaylandUtils::isWaylandSession()) {
        // Wayland下不再使用D-XCB插件，改让layer-shell-qt接手
        qunsetenv("DTK2_XWAYLAND");
        const QString dwaylandPlugin = QLibraryInfo::location(QLibraryInfo::PluginsPath)
            + QStringLiteral("/platforms/libdwayland.so");

        // 更新: 并不能安全地假设GXDE总是支持DWayland
        if (QFile::exists(dwaylandPlugin)) {
            qputenv("QT_QPA_PLATFORM", "dwayland");
        } else {
            qputenv("QT_QPA_PLATFORM", "wayland");
        }

        // 仅在layer-shell的QtWayland集成插件可用时才启用layer-shell。
        // 否则wayland平台插件会因找不到"layer-shell"集成而初始化失败并abort(黑屏)。
        if (WaylandUtils::layerShellIntegrationAvailable()) {
            LayerShellQt::Shell::useLayerShell();
        } else {
            qWarning() << "[Wayland] 缺少layer-shell的QtWayland集成插件"
                          "(wayland-shell-integration/liblayer-shell.so)，"
                          "已禁用layer-shell以避免崩溃；壁纸/桌面需要该插件才能作为背景层显示。";
        }
    } else {
        // 传统X11会话下仍然加载D-XCB插件
        DApplication::loadDXcbPlugin();
    }

    DApplication app(argc, argv);

    // The problem is that Qt6 does NOT auto register QByteArrayList w/
    // Qt D-Bus any more. Hence, udisks2-qt6 property reads of ay list (e.g.
    // Filesystem.MountPoints, etc.) SILENTLY returns empty, then it trys to
    // re-mount, and meet fail of already mounted. So this is why optical
    // drive is ALWAYS empty.
    // Manually resitering it now.
    qDBusRegisterMetaType<QByteArrayList>();

    // 现在Desktop-panel在Wayland下是个纯Wayland app，测试时发现在Treeland下XSETTINGS
    // selection无人持有导致DTreelandPlatformInterface返回空，于是DTK的DIconProxyEngine拿不到
    // 主题的图标名
    // 为这种情况兜底，发生这种情况直接读取配置文件，确保QIconLoader有图标可用
    if (WaylandUtils::isWaylandSession()) {
        QSettings qtSettings(QSettings::IniFormat, QSettings::UserScope,
            "deepin", "qt-theme");
        qtSettings.beginGroup("Theme");

        const QString icon_theme = qtSettings.value("IconThemeName").toString();
        if (!icon_theme.isEmpty()) {
            qDebug() << "(Wayland mode) IconLoader: fallback icon theme from config:"
                << icon_theme;
            QIcon::setThemeName(icon_theme);
        }
    }

    // 注意：不能在这里清理 layer-shell 环境变量。此时壁纸等窗口尚未创建
    // 此时unset壁纸窗口就不会成为layer-shell表面，不仅乱飞还会被dock裁掉底部的那一块

    // 此处原有unset的相关代码，被我挪到 Show() 创建窗口之后去了

    // 别急，还有第二关：在Wayland下，弹出的右键菜单也会给认为是一个layer-shell surface，导致菜单占满全屏
    // 表象就是菜单直接糊满全屏。
    // 计划是安装一个事件过滤器，Popup类窗口一显示旧解掉其anchor

    // 第三关(仅 Treeland)：带子菜单的项(如「用…打开」) hover出子菜单后往下移，整条菜单消失
    // 调试发现: 子菜单(layer 表面)关闭时，QtWayland 会同步给父菜单发一个
    // QEvent::Close (独立 layer 表面没有,xdg_popup的grab链，Qt的popup链判定把父菜单一起关了)，
    // 全程没有 WindowDeactivate/FocusOut。对策：拦掉这个**误发**的 Close ——
    // 当「刚有子菜单活动」且「没有鼠标/键盘触发关闭」且「鼠标仍在菜单链区域内」
    // 时，吞掉父菜单的 Close。
    // 真正的关闭 (点外部、点击菜单项、Esc/键盘操作) 不满足条件，照常放行。
    class PopupLayerShellPatcher : public QObject {
    public:
        using QObject::QObject;

    protected:
        bool eventFilter(QObject* obj, QEvent* event) override {
            QWidget* w = qobject_cast<QWidget*>(obj);
            if (w && w->windowType() == Qt::Popup) {
                QWindow* wh = w->windowHandle();
                const bool hasTransientParent = wh && wh->transientParent();
                const bool isKnownSubMenu = hasTransientParent
                    || m_subMenuPopups.contains(w);

                if (event->type() == QEvent::Show) {
                    const bool isSubMenu = hasTransientParent
                        || hasOtherVisiblePopup(w);

                    m_visiblePopups.insert(w);
                    if (isSubMenu) {
                        m_subMenuPopups.insert(w);
                        m_subMenuActivityTimer.restart();
                        m_lastSubMenuRect = w->geometry();
                    } else {
                        m_rootPopups.insert(w);
                    }

                    Wayland::LayerShellHelper::fixPopupLayerShell(w);
                } else if (event->type() == QEvent::MouseMove) {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    m_lastCursorPos = mouseEvent->globalPos();
                    m_lastCursorPosTimer.restart();
                } else if (event->type() == QEvent::MouseButtonPress
                           || event->type() == QEvent::MouseButtonRelease
                           || event->type() == QEvent::MouseButtonDblClick) {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    m_lastCursorPos = mouseEvent->globalPos();
                    m_lastCursorPosTimer.restart();
                    m_lastMouseButtonTimer.restart();
                } else if (event->type() == QEvent::KeyPress
                           || event->type() == QEvent::ShortcutOverride) {
                    m_lastKeyEventTimer.restart();
                } else if (event->type() == QEvent::Move
                           || event->type() == QEvent::Resize) {
                    if (isKnownSubMenu) {
                        m_lastSubMenuRect = w->geometry();
                    }
                } else if (event->type() == QEvent::Close && !isKnownSubMenu
                           && Wayland::LayerShellHelper::isTreeland()) {
                    const QPoint cursorPos =
                        m_lastCursorPosTimer.isValid() && m_lastCursorPosTimer.elapsed() < 1000
                            ? m_lastCursorPos : QCursor::pos();
                    const QRect menuGuardRect = popupChainRect(w)
                        .marginsAdded(QMargins(32, 32, 32, 32));
                    const bool subMenuRecentlyActive =
                        hasVisibleSubMenu()
                        || (m_subMenuActivityTimer.isValid()
                            && m_subMenuActivityTimer.elapsed() < 1500);
                    const bool noRecentMouseButton =
                        !m_lastMouseButtonTimer.isValid()
                        || m_lastMouseButtonTimer.elapsed() > 350;
                    const bool noRecentKey =
                        !m_lastKeyEventTimer.isValid()
                        || m_lastKeyEventTimer.elapsed() > 350;

                    if (subMenuRecentlyActive && noRecentMouseButton && noRecentKey
                            && menuGuardRect.contains(cursorPos)
                            && QGuiApplication::mouseButtons() == Qt::NoButton) {
                        // 必须 ignore() 把 QCloseEvent 标记为未接受，QWidget::close() 才会放弃隐藏；
                        // 只 return true(拦截分发)不改 accepted 标志，菜单照样会关。
                        event->ignore();
                        return true;
                    }
                } else if ((event->type() == QEvent::Close || event->type() == QEvent::Hide)
                           && isKnownSubMenu) {
                    // 记下「子菜单刚关闭」的时刻，用于识别紧随其后的父菜单误关 Close
                    m_subMenuActivityTimer.restart();
                    m_lastSubMenuRect = w->geometry();
                    if (event->type() == QEvent::Hide) {
                        m_visiblePopups.remove(w);
                    }
                } else if (event->type() == QEvent::Hide) {
                    m_visiblePopups.remove(w);
                } else if (event->type() == QEvent::Destroy) {
                    m_visiblePopups.remove(w);
                    m_rootPopups.remove(w);
                    m_subMenuPopups.remove(w);
                }
            }
            return QObject::eventFilter(obj, event);
        }

    private:
        bool hasOtherVisiblePopup(QWidget* current) const {
            for (QWidget* popup : m_visiblePopups) {
                if (popup && popup != current && popup->isVisible()) {
                    return true;
                }
            }
            return false;
        }

        bool hasVisibleSubMenu() const {
            for (QWidget* popup : m_subMenuPopups) {
                if (popup && popup->isVisible()) {
                    return true;
                }
            }
            return false;
        }

        QRect popupChainRect(QWidget* closingPopup) const {
            QRect rect;
            for (QWidget* popup : QApplication::topLevelWidgets()) {
                if (!popup || popup->windowType() != Qt::Popup
                        || !popup->isVisible()) {
                    continue;
                }

                rect = rect.isNull() ? popup->geometry()
                    : rect.united(popup->geometry());
            }

            if (closingPopup) {
                rect = rect.isNull() ? closingPopup->geometry()
                    : rect.united(closingPopup->geometry());
            }

            if (!m_lastSubMenuRect.isNull()
                    && m_subMenuActivityTimer.isValid()
                    && m_subMenuActivityTimer.elapsed() < 1500) {
                rect = rect.isNull() ? m_lastSubMenuRect
                    : rect.united(m_lastSubMenuRect);
            }

            return rect;
        }

        QSet<QWidget*> m_visiblePopups;
        QSet<QWidget*> m_rootPopups;
        QSet<QWidget*> m_subMenuPopups;
        QElapsedTimer m_subMenuActivityTimer;
        QElapsedTimer m_lastCursorPosTimer;
        QElapsedTimer m_lastMouseButtonTimer;
        QElapsedTimer m_lastKeyEventTimer;
        QPoint m_lastCursorPos;
        QRect m_lastSubMenuRect;
    };

    app.installEventFilter(new PopupLayerShellPatcher(&app));

    bool preload = false;
    bool fileDialogOnly = false;

    for (const QString &arg : app.arguments()) {
        if (arg == "--preload") {
            preload = true;
            break;
        }
        if (arg == "--file-dialog-only") {
            fileDialogOnly = true;
            break;
        }
    }

    if (fileDialogOnly && getuid() != 0) {
        // --file-dialog-only should only used by `root`.
        qDebug() << "Current UID != 0, the `--file-dialog-only` argument is ignored.";
        fileDialogOnly = false;
    }

    if (fileDialogOnly) {
        app.setQuitOnLastWindowClosed(false);
    }

    app.setOrganizationName("deepin");
    app.setApplicationName("gxde-desktop-panel");
    app.setApplicationVersion(DApplication::buildVersion((GIT_VERSION)));
    app.setTheme("light");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    const QString m_format = "%{time}{yyyyMMdd.HH:mm:ss.zzz}[%{type:1}][%{function:-35} %{line:-4} %{threadid} ] %{message}\n";
    DLogManager::setLogFormat(m_format);
    DLogManager::registerConsoleAppender();

    if (!preload) {
        DLogManager::registerFileAppender();
    }

    app.loadTranslator();

    // init application object
    DFMApplication fmApp;
    Q_UNUSED(fmApp)

    qDebug() << "start " << app.applicationName() << app.applicationVersion();

    if (!preload && !fileDialogOnly) {
        QDBusConnection conn = QDBusConnection::sessionBus();

        if (!conn.registerService(DesktopServiceName)) {
            qDebug() << "registerService Failed, maybe service exist" << conn.lastError();
            exit(0x0002);
        }

        if (!conn.registerObject(DesktopServicePath, Desktop::instance(),
                                 QDBusConnection::ExportAllSlots |
                                 QDBusConnection::ExportAllSignals |
                                 QDBusConnection::ExportAllProperties)) {
            qDebug() << "registerObject Failed" << conn.lastError();
            exit(0x0003);
        }

        Desktop::instance()->initDebugDBus(conn);
    }

    // init pixmap cache size limit, 20MB * devicePixelRatio
    QPixmapCache::setCacheLimit(20 * 1024 * app.devicePixelRatio());

    QThreadPool::globalInstance()->setMaxThreadCount(MAX_THREAD_COUNT);

    if (!fileDialogOnly) {
        Config::instance();
    }

    DFMGlobal::installTranslator();

    if (!fileDialogOnly) {
        Desktop::instance()->loadData();
    }

    if (preload) {
        QTimer::singleShot(1000, &app, &QCoreApplication::quit);
    } else {
        if (!fileDialogOnly) {
            Desktop::instance()->Show();
            Desktop::instance()->loadView();
        }
    }

    if (WaylandUtils::isWaylandSession()) {
        // 平台插件已初始化完毕，恢复DTK2_XWAYLAND供子进程使用
        if (!savedDtk2XWayland.isEmpty()) {
            qputenv("DTK2_XWAYLAND", savedDtk2XWayland);
        }

        // 清除 layer-shell 和 QPA 平台环境变量
        // 防止子进程，比如文件管理器等，继承后窗口行为异常
        qunsetenv("QT_WAYLAND_SHELL_INTEGRATION");
        qunsetenv("QT_QPA_PLATFORM");
    }

    DFMGlobal::autoLoadDefaultPlugins();
    DFMGlobal::autoLoadDefaultMenuExtensions();
    DFMGlobal::initPluginManager();
    DFMGlobal::initMimesAppsManager();
    DFMGlobal::initDialogManager();
    DFMGlobal::initOperatorRevocation();
    DFMGlobal::initTagManagerConnect();
    DFMGlobal::initThumbnailConnection();

    if  (!preload) {
        // Notify gxde-desktop-panel start up
        if (!fileDialogOnly) {
            Dde::Session::RegisterDdeSession();
        }

        // ---------------------------------------------------------------------------
        // ability to show file selection dialog
        if (!registerDialogDBus()) {
            qWarning() << "Register dialog dbus failed.";
            if (fileDialogOnly) {
                return 1;
            }
        }

        if (!registerFileManager1DBus()) {
            qWarning() << "Register org.freedesktop.FileManager1 DBus service is failed";
        }
    }

    DFMGlobal::IsFileManagerDiloagProcess = true; // for compatibility.
    // ---------------------------------------------------------------------------

    DEventFilter *event_filter{ new DEventFilter{&app} };
    app.installEventFilter(event_filter);

    return app.exec();
}
