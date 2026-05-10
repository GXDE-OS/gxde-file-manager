// SPDX-FileCopyrightText: 2026 CharOfString
// SPDX-License-Identifier: GPL-3.0-or-later

// 不尝试在Qt6下安装libqt6xdg，实测在GXDE25.3下装这玩意能给你把GXDE桌面核心的包给你卸载一堆。
// 昨天真是给我气炸了，不信去翻docs/images/problem-libqt6xdg-conflict.png

// 所以当前在Qt6下绕过这个库自己解析，Qt5接着用原来的得了

#ifndef GXDE_FILE_MANAGER_LIB_SHUTIL_DFM_XDGDESKTOPFILE_COMPAT_H_
#define GXDE_FILE_MANAGER_LIB_SHUTIL_DFM_XDGDESKTOPFILE_COMPAT_H_

#include <QFile>
#include <QHash>
#include <QIcon>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>

class XdgDesktopFile {
  public:
    XdgDesktopFile() = default;

    bool load(const QString &fileName) {
        mIsValid = false;
        mEntries.clear();
        QFile f(fileName);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream in(&f);
        bool inDesktopEntry = false;

        while (!in.atEnd()) {
            const QString raw = in.readLine();
            const QString line = raw.trimmed();
            if (line.isEmpty() || line.startsWith('#')) {
                continue;
            }

            if (line.startsWith('[') && line.endsWith(']')) {
                inDesktopEntry = (line == QLatin1String("[Desktop Entry]"));
                continue;
            }

            if (!inDesktopEntry) {
                continue;
            }

            const int eq = line.indexOf('=');
            if (eq <= 0) {
                continue;
            }

            const QString key = line.left(eq).trimmed();
            const QString val = line.mid(eq + 1);
            mEntries.insert(key, val);
        }

        mIsValid = !mEntries.isEmpty();
        return mIsValid;
    }

    bool isValid() const {
        return mIsValid;
    }

    QVariant value(const QString &key) const {
        return QVariant(mEntries.value(key));
    }

    // 优先获取本地化名称，举个例子，层层fallback是这么设计的：
    // 键[zh_CN] → 键[zh] → 键
    QString name() const {
        return localizedValue(QStringLiteral("Name"));
    }

    QString iconName() const {
        return mEntries.value(QStringLiteral("Icon"));
    }

    QIcon icon() const {
        const QString n = iconName();

        if (n.isEmpty()) {
            return QIcon();
        }

        if (QFile::exists(n)) {
            return QIcon(n);
        }

        return QIcon::fromTheme(n);
    }

  private:
    QString localizedValue(const QString &baseKey) const {
        const QString locale = QLocale::system().name();  // e.g. zh_CN
        const QString shortLoc = locale.section('_', 0, 0);  // e.g. zh
        const QString withFull = QStringLiteral("%1[%2]").arg(baseKey, locale);
        const QString withShort = QStringLiteral("%1[%2]").arg(baseKey,
            shortLoc);

        if (mEntries.contains(withFull)) {
            return mEntries.value(withFull);
        }

        if (mEntries.contains(withShort)) {
            return mEntries.value(withShort);
        }

        return mEntries.value(baseKey);
    }

    QHash<QString, QString> mEntries;
    bool mIsValid = false;
};

#endif  // GXDE_FILE_MANAGER_LIB_SHUTIL_DFM_XDGDESKTOPFILE_COMPAT_H_
