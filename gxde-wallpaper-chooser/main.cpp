/*
 * Copyright (C) 2026 CharOfString <markus_verify@126.com>
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <QDebug>
#include <QEvent>
#include <QFile>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTextCodec>
#include <QTranslator>
#include <QWidget>
#include <DApplication>
#include <DLog>

#include <LayerShellQt/Shell>

#include "frame.h"
#include "util/wayland/layershellhelper.h"
#include "waylandutils.h"

using namespace Dtk::Core;
using namespace Dtk::Widget;

class PopupLayerShellPatcher : public QObject {
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Show) {
            QWidget* w = qobject_cast<QWidget*>(obj);
            if (w && w->windowType() == Qt::Popup) {
                Wayland::LayerShellHelper::fixPopupLayerShell(w);
            }
        }

        return QObject::eventFilter(obj, event);
    }
};

class OutsideClickCloser : public QObject {
public:
    OutsideClickCloser(QWidget* frame, QObject* parent)
        : QObject(parent), m_frame(frame) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::MouseButtonPress) {
            QWidget* w = qobject_cast<QWidget*>(obj);
            if (w) {
                QWidget* top = w->window();
                if (top && top != m_frame
                        && top->windowType() != Qt::Popup
                        && top->windowType() != Qt::ToolTip) {
                    m_frame->hide();
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    QWidget* m_frame;
};

int main(int argc, char* argv[])
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

    const bool isWaylandSession = WaylandUtils::isWaylandSession();

    // startgxde会设置DTK2_XWAYLAND=dxcb, 与原生 Wayland + layer-shell
    // 不兼容, 导致段错误。在构造期间临时屏蔽；构造完成无需恢复
    if (isWaylandSession) {
        qunsetenv("DTK2_XWAYLAND");
        const QString dwaylandPlugin = QLibraryInfo::location(QLibraryInfo::PluginsPath)
            + QStringLiteral("/platforms/libdwayland.so");

        // 更新: 并不能安全地假设GXDE总是支持DWayland
        if (QFile::exists(dwaylandPlugin)) {
            qputenv("QT_QPA_PLATFORM", "dwayland");
        } else {
            qputenv("QT_QPA_PLATFORM", "wayland");
        }

        // 仅在layer-shell的QtWayland集成插件可用时才启用layer-shell，
        // 否则wayland平台插件会因找不到"layer-shell"集成而初始化失败并abort
        if (WaylandUtils::layerShellIntegrationAvailable()) {
            LayerShellQt::Shell::useLayerShell();
        } else {
            qWarning() << "[Wayland] 缺少layer-shell集成插件，已禁用layer-shell以避免崩溃";
        }
    } else {
        DApplication::loadDXcbPlugin();
    }

    DApplication app(argc, argv);

    // Treeland下XSETTINGS 无人持有, DTK 拿不到图标主题名,
    // 从配置文件兜底, 确保选择器有图标可用
    if (isWaylandSession) {
        QSettings qtSettings(QSettings::IniFormat, QSettings::UserScope,
            "deepin", "qt-theme");
        qtSettings.beginGroup("Theme");

        const QString icon_theme = qtSettings.value("IconThemeName").toString();
        if (!icon_theme.isEmpty()) {
            QIcon::setThemeName(icon_theme);
        }
    }

    app.installEventFilter(new PopupLayerShellPatcher(&app));

    app.setOrganizationName("deepin");
    app.setApplicationName("gxde-wallpaper-chooser-wayland");
    app.setTheme("light");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    app.setQuitOnLastWindowClosed(true);

    const QString m_format = "%{time}{yyyyMMdd.HH:mm:ss.zzz}[%{type:1}]"
        "[%{function:-35} %{line:-4} %{threadid} ] %{message}\n";
    DLogManager::setLogFormat(m_format);
    DLogManager::registerConsoleAppender();

    // loadTranslator加载翻译, 但选择器界面字符串属于
    // gxde-desktop-panel的翻译上下文, 而本程序applicationName不同... 手动加载!!
    app.loadTranslator();

    QTranslator* panelTranslator = new QTranslator(&app);
    const QString locale = QLocale::system().name();
    const QString qmDir = "/usr/share/gxde-desktop-panel/translations";
    if (panelTranslator->load("gxde-desktop-panel_" + locale, qmDir)
            || panelTranslator->load(
                "gxde-desktop-panel_" + locale.section('_', 0, 0), qmDir)) {
        app.installTranslator(panelTranslator);
    } else {
        qWarning() << "Failed to load panel translation for locale" << locale;
    }

    qDebug() << "start " << app.applicationName();

    Frame* frame = new Frame;
    QObject::connect(frame, &Frame::done, &app, &QCoreApplication::quit);

    if (Wayland::LayerShellHelper::isWayland()) {
        Wayland::LayerShellHelper::setChooserRole(frame,
            qApp->primaryScreen(), "wallpaper-chooser");
    }

    // 点击选择器之外关闭 (替代 Wayland 下失效的 DRegionMonitor)
    app.installEventFilter(new OutsideClickCloser(frame, &app));

    frame->show();
    frame->grabKeyboard();

    return app.exec();
}
