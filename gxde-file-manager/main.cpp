/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *               2016 ~ 2018 dragondjf
 *
 * Author:     dragondjf<dingjiangfeng@deepin.com>
 *
 * Maintainer: dragondjf<dingjiangfeng@deepin.com>
 *             zccrs<zhangjide@deepin.com>
 *             Tangtong<tangtong@deepin.com>
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

#include <signal.h>

#include "durl.h"
#include "dfmglobal.h"
#include "dabstractfileinfo.h"
#include "dfileservices.h"
#include "dfmeventdispatcher.h"

#include "filemanagerapp.h"
#include "logutil.h"
#include "singleapplication.h"

#include "app/define.h"
#include "app/filesignalmanager.h"

#include "commandlinemanager.h"

#include "dialogs/dialogmanager.h"
#include "shutil/fileutils.h"
#include "shutil/mimesappsmanager.h"
#include "waylandutils.h"
#include "dialogs/openwithdialog.h"
#include "controllers/appcontroller.h"
#include "singleton.h"
#include "gvfs/gvfsmountmanager.h"

// DBus
#include "views/themeconfig.h"
#include "dfmapplication.h"

#include <dthememanager.h>

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QSettings>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>
#include <QDBusMetaType>
#include <QProcess>
#include <QLocalSocket>
#include <QPixmapCache>

#include <pwd.h>

#ifdef ENABLE_PPROF
#include <gperftools/profiler.h>
#endif

#define fileManagerApp FileManagerApp::instance()

// blumia: DDE not yet got fully support about session management, so when logout or shutdown,
//         the config file won't save. On mips64el, sw, arm, there will be a "warm-up" process
//         running in the background (gxde-file-manager -d) and the file manager instance will
//         not got exit when all visible DFM window got closed, so that means the file manager
//         config file will never got saved.
// blumia: Handling SIGTERM is a workaround way to fix that issue, but we still need to add
//         session management support to DDE.
void handleSIGTERM(int sig)
{
    qDebug() << "!SIGTERM!" << sig;

    if (qApp) {
        qApp->quit();
    }
}

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
#ifdef ENABLE_PPROF
    ProfilerStart("pprof.prof");
#endif
    // Fixed the locale codec to utf-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

    if (qEnvironmentVariableIsSet("PKEXEC_UID")) {
        const quint32 pkexecUID = qgetenv("PKEXEC_UID").toUInt();
        DApplication::customQtThemeConfigPathByUserHome(getpwuid(pkexecUID)->pw_dir);
    }

    // Wayland会话下启用原生Wayland，若显式指定QT_QPA_PLATFORM=dxcb则不使用Wayland
    // 优先检测dwayland QPA插件（Deepin Wayland集成），可用时强制dwayland，
    // 否则回退到原生wayland
    // 如果使用原生Wayland则不恢复DTK2_XWAYLAND，使DApplication::isWayland()返回真，这样DTK走
    // Wayland的窗体装饰路径
    const bool waylandSession = WaylandUtils::isWaylandSession();
    const bool userForcedPlatform = !qEnvironmentVariable("QT_QPA_PLATFORM").isEmpty();
    if (waylandSession && !userForcedPlatform) {
        qunsetenv("DTK2_XWAYLAND");
        const QString currentDE = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();
        const QString dwaylandPlugin = QLibraryInfo::location(QLibraryInfo::PluginsPath)
            + QStringLiteral("/platforms/libdwayland.so");

        // 更新: 并不能安全地假设GXDE总是支持DWayland
        if (QFile::exists(dwaylandPlugin)) {
            qputenv("QT_QPA_PLATFORM", "dwayland");
            qInfo() << "(DWayland) Startup: Setting DWayland...";
        } else {
            qputenv("QT_QPA_PLATFORM", "wayland");
            qInfo() << "(Wayland) Startup: Falling back to wayland...";
        }
    } else {
        SingleApplication::loadDXcbPlugin();
    }
    SingleApplication::initSources();
    SingleApplication app(argc, argv);

    // The problem is that Qt6 does NOT auto register QByteArrayList w/
    // Qt D-Bus any more. Hence, udisks2-qt6 property reads of ay list (e.g.
    // Filesystem.MountPoints, etc.) SILENTLY returns empty, then it trys to
    // re-mount, and meet fail of already mounted. So this is why optical
    // drive is ALWAYS empty.
    // Manually resitering it now.
    qDBusRegisterMetaType<QByteArrayList>();

    // 测试时发现在Treeland下XSETTINGS selection无人持有导致DTreelandPlatformInterface
    // 返回空，于是DTK的DIconProxyEngine拿不到主题的图标名
    // 为这种情况兜底，发生这种情况直接读取配置文件，确保QIconLoader有图标可用
    if (WaylandUtils::isWaylandSession()) {
        QSettings qtSettings(QSettings::IniFormat, QSettings::UserScope,
            "deepin", "qt-theme");
        qtSettings.beginGroup("Theme");
        const QString icon_theme = qtSettings.value("IconThemeName").toString();
        if (!icon_theme.isEmpty()) {
            qDebug() << "Wayland: fallback icon theme from config:"
                << icon_theme;
            QIcon::setThemeName(icon_theme);
        }
    }

    app.setOrganizationName(QMAKE_ORGANIZATION_NAME);
    app.setApplicationName(QMAKE_TARGET);
    app.loadTranslator();
    app.setApplicationDisplayName(app.translate("Application", "Deepin File Manager"));
    app.setApplicationVersion(DApplication::buildVersion((QMAKE_VERSION)));
    QIcon icon(":/images/images/gxde-file-manager_96.svg");
    app.setProductIcon(icon);
    app.setApplicationAcknowledgementPage("https://www.gxde.top/" + qApp->applicationName());
    app.setApplicationDescription(app.translate("Application", "File Manager is a file management tool independently "
                                                               "developed by Deepin Technology, featured with searching, "
                                                               "copying, trash, compression/decompression, file property "
                                                               "and other file management functions."));
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

#ifdef DISABLE_QUIT_ON_LAST_WINDOW_CLOSED
    app.setQuitOnLastWindowClosed(false);
#endif

    DFMGlobal::installTranslator();

    LogUtil::registerLogger();

    // init application object
    DFMApplication fmApp;
    Q_UNUSED(fmApp)

    // init pixmap cache size limit, 20MB * devicePixelRatio
    QPixmapCache::setCacheLimit(20 * 1024 * app.devicePixelRatio());

    CommandLineManager::instance()->process();

    // working dir
    if (CommandLineManager::instance()->isSet("w")) {
        QDir::setCurrent(CommandLineManager::instance()->value("w"));
    }

    // open as root
    if (CommandLineManager::instance()->isSet("r")) {
        QStringList args = app.arguments().mid(1);
        args.removeAll(QStringLiteral("-r"));
        args.removeAll(QStringLiteral("--root"));
        args.removeAll(QStringLiteral("-w"));
        args.removeAll(QStringLiteral("--working-dir"));
        QProcess::startDetached("gxde-file-manager-pkexec", args, QDir::currentPath());

        return 0;
    }

    if (CommandLineManager::instance()->isSet("h") || CommandLineManager::instance()->isSet("v")) {
        return app.exec();
    }

    QString uniqueKey = app.applicationName();

    bool isSingleInstance  = app.setSingleInstance(uniqueKey);

    if (isSingleInstance) {
        // init app
        Q_UNUSED(FileManagerApp::instance())

        if (CommandLineManager::instance()->isSet("d")) {
            fileManagerApp;
#ifdef AUTO_RESTART_DEAMON
            QWidget w;
            w.setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
            w.setAttribute(Qt::WA_TranslucentBackground);
            w.resize(0, 0);
            w.show();
#endif
        } else {
            CommandLineManager::instance()->processCommand();
        }

        signal(SIGTERM, handleSIGTERM);

#ifdef ENABLE_PPROF
        int request = app.exec();

        ProfilerStop();

        return request;
#else
        int ret = app.exec();
#ifdef AUTO_RESTART_DEAMON
        app.closeServer();
        QProcess::startDetached(QString("%1 -d").arg(QString(argv[0])));
#endif
        return ret;
#endif
    } else {
        QByteArray data;
        bool is_set_get_monitor_files = false;

        for (const QString &arg : app.arguments()) {
            if (arg == "--get-monitor-files")
                is_set_get_monitor_files = true;

            if (!arg.startsWith("-") && QFile::exists(arg))
                data.append(QDir(arg).absolutePath().toLocal8Bit().toBase64());
            else
                data.append(arg.toLocal8Bit().toBase64());

            data.append(' ');
        }

        if (!data.isEmpty())
            data.chop(1);

        QLocalSocket *socket = SingleApplication::newClientProcess(uniqueKey, data);
        QWidget w;
        w.setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
        w.setAttribute(Qt::WA_TranslucentBackground);
        w.resize(1, 1);
        w.show();

        if (is_set_get_monitor_files && socket->error() == QLocalSocket::UnknownSocketError) {
            socket->waitForReadyRead();

            for (const QByteArray &i : socket->readAll().split(' '))
                qDebug() << QString::fromLocal8Bit(QByteArray::fromBase64(i));
        }

        return 0;
    }
}
