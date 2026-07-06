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

#ifndef UTILS_WAYLANDUTILS_H_
#define UTILS_WAYLANDUTILS_H_

#include <QByteArray>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLatin1String>
#include <QLibraryInfo>
#include <QString>

namespace WaylandUtils {

inline bool isWaylandSession() {
    return qgetenv("XDG_SESSION_TYPE") == QByteArrayLiteral("wayland");
}

// wlr-layer-shell依赖QtWayland的"layer-shell" shell-integration插件(由layer-shell-qt
// 提供)。部分发行版的Qt6包只带库不带这个插件，此时若仍调用
// LayerShellQt::Shell::useLayerShell()，wayland平台插件会因找不到"layer-shell"集成而
// 初始化失败并直接abort(黑屏)。调useLayerShell前先用本函数探测插件是否存在。
inline bool layerShellIntegrationAvailable() {
    const QString plugin = QLibraryInfo::location(QLibraryInfo::PluginsPath)
        + QStringLiteral("/wayland-shell-integration/liblayer-shell.so");
    return QFileInfo::exists(plugin);
}

inline bool isWaylandPlatform() {
    if (!qGuiApp) {
        return false;
    }
    return QGuiApplication::platformName().toLower().contains(
        QLatin1String("wayland"));
}

inline bool isTreeland() {
    if (!isWaylandSession()) {
        return false;
    }

    static const char* kSessionEnvs[] = {
        "XDG_SESSION_DESKTOP", "DESKTOP_SESSION",
        "XDG_CURRENT_DESKTOP", "GDMSESSION",
    };
    for (const char* env : kSessionEnvs) {
        if (qEnvironmentVariable(env).toLower().contains(
                QLatin1String("treeland"))) {
            return true;
        }
    }
    return false;
}

}  // namespace WaylandUtils

#endif  // UTILS_WAYLANDUTILS_H_
