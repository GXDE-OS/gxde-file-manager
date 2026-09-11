/*
 * Copyright (C) 2026 CharOfString <root@charofstring.cc>
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

#ifndef UTILS_WALLPAPERUTILS_H_
#define UTILS_WALLPAPERUTILS_H_

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLatin1Char>
#include <QLatin1String>
#include <QStandardPaths>
#include <QString>
#include <QUrl>

namespace WallpaperUtils {

inline QString wallpaperLibraryDir() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/deepin/dde-daemon/appearance/custom-wallpapers");
}

inline QString localPathOf(const QString &pathOrUrl) {
    const QUrl url(pathOrUrl);
    return url.isLocalFile() ? url.toLocalFile() : pathOrUrl;
}

// 判断壁纸是否位于自定义壁纸库中。daemon 的 List("background") 对这类壁纸
// 返回 Deletable=true，选择器据此显示删除按钮。
inline bool isInWallpaperLibrary(const QString &pathOrUrl) {
    const QString localPath = localPathOf(pathOrUrl);
    const QFileInfo info(localPath);
    const QString canonical = info.canonicalFilePath();
    if (canonical.isEmpty())
        return false;

    const QString libraryCanonical = QDir(wallpaperLibraryDir()).canonicalPath();
    return !libraryCanonical.isEmpty()
        && canonical.startsWith(libraryCanonical + QLatin1Char('/'));
}

inline QString fileMd5(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(&file);
    file.close();

    return QString::fromLatin1(hash.result().toHex());
}

inline QString persistWallpaperToLibrary(const QString &pathOrUrl) {
    const QString localPath = localPathOf(pathOrUrl);
    const QFileInfo info(localPath);
    if (!info.isFile() || !info.isReadable())
        return localPath;

    const QString canonical = info.canonicalFilePath();
    if (canonical.isEmpty())
        return localPath;

    const QString libraryDir = wallpaperLibraryDir();
    const QString libraryCanonical = QDir(libraryDir).canonicalPath();
    if (!libraryCanonical.isEmpty()
            && canonical.startsWith(libraryCanonical + QLatin1Char('/'))) {
        return canonical;
    }

    static const char *systemDirs[] = {
        "/usr/share/wallpapers",
        "/usr/local/share/wallpapers",
        "/usr/share/backgrounds",
        "/usr/local/share/backgrounds",
    };
    for (const char *dir : systemDirs) {
        const QString systemCanonical = QDir(QLatin1String(dir)).canonicalPath();
        if (!systemCanonical.isEmpty()
                && canonical.startsWith(systemCanonical + QLatin1Char('/'))) {
            return canonical;
        }
    }

    const QString md5 = fileMd5(canonical);
    if (md5.isEmpty()) {
        return canonical;
    }

    if (!QDir().mkpath(libraryDir))
        return canonical;

    const QString ext = info.suffix().isEmpty()
        ? QString() : QLatin1Char('.') + info.suffix();
    const QString dest = libraryDir + QLatin1Char('/') + md5 + ext;

    if (QFileInfo::exists(dest))
        return dest;

    if (QFile::copy(canonical, dest))
        return dest;

    return canonical;
}

}  // namespace WallpaperUtils

#endif  // UTILS_WALLPAPERUTILS_H_
