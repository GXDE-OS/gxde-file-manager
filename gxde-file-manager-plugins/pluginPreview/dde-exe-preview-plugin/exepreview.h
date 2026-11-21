/*
 * Copyright (C) 2026 GXDE OS <https://gxde.top/>
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

#ifndef EXEPREVIEWPLUGIN_H
#define EXEPREVIEWPLUGIN_H

#include <QObject>
#include <QWidget>
#include <QPointer>
#include <QMap>

#include "dfmfilepreview.h"
#include "durl.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QScrollArea;
QT_END_NAMESPACE

DFM_BEGIN_NAMESPACE

class ExePreview : public DFMFilePreview
{
    Q_OBJECT

public:
    explicit ExePreview(QObject* parent = 0);
    ~ExePreview();

    bool setFileUrl(const DUrl &url) Q_DECL_OVERRIDE;
    DUrl fileUrl() const override;

    QWidget *contentWidget() const Q_DECL_OVERRIDE;

    QString title() const Q_DECL_OVERRIDE;
    bool showStatusBarSeparator() const Q_DECL_OVERRIDE;

private:
    // 从 PE 文件中解析出的信息
    struct PEInfo {
        quint16 machine = 0;        // 目标架构（IMAGE_FILE_MACHINE_*）
        quint16 subsystem = 0;      // 子系统（IMAGE_SUBSYSTEM_*）
        quint32 timeStamp = 0;      // 编译时间戳（可能不可靠）
        bool hasVersionInfo = false;
        bool hasIcon = false;
        QByteArray versionResource; // RT_VERSION 原始数据
        QByteArray groupIcon;       // RT_GROUP_ICON 原始数据
        QMap<quint32, QByteArray> icons; // RT_ICON 资源（id -> 数据）
    };

    // 解析 VERSIONINFO 资源得到的字符串表
    struct VersionInfo {
        QMap<QString, QString> strings;
        quint16 langId = 0;
        bool hasLang = false;
        bool hasFixedInfo = false;
        quint32 fileVersionMS = 0;      // VS_FIXEDFILEINFO 中的数值版本
        quint32 fileVersionLS = 0;
        quint32 productVersionMS = 0;
        quint32 productVersionLS = 0;
    };

    bool parsePE(const QString &path, PEInfo *info) const;
    bool parseVersionInfo(const QByteArray &data, VersionInfo *info) const;
    QByteArray assembleIcon(const PEInfo &info) const;

    QString machineName(quint16 machine) const;
    QString subsystemName(quint16 subsystem) const;
    QString langName(quint16 langId) const;
    QString versionString(quint32 high, quint32 low) const;
    static QString humanSize(qint64 bytes);

    void initUI();

    DUrl m_url;
    QString m_title;

    QWidget *m_contentWidget = nullptr;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_headerLabel = nullptr;

    QScrollArea *m_infoScroll = nullptr;
};

DFM_END_NAMESPACE

#endif // EXEPREVIEWPLUGIN_H
