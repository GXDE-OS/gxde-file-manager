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

#ifndef DEBPREVIEWPLUGIN_H
#define DEBPREVIEWPLUGIN_H

#include <QObject>
#include <QWidget>
#include <QPointer>
#include <QMap>

#include "dfmfilepreview.h"
#include "durl.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QTabWidget;
class QPlainTextEdit;
class QComboBox;
class QScrollArea;
QT_END_NAMESPACE

DFM_BEGIN_NAMESPACE

class DebPreview : public DFMFilePreview
{
    Q_OBJECT

public:
    explicit DebPreview(QObject* parent = 0);
    ~DebPreview();

    bool setFileUrl(const DUrl &url) Q_DECL_OVERRIDE;
    DUrl fileUrl() const override;

    QWidget *contentWidget() const Q_DECL_OVERRIDE;

    QString title() const Q_DECL_OVERRIDE;
    bool showStatusBarSeparator() const Q_DECL_OVERRIDE;

private slots:
    void showScript(const QString &name);
    void onCurrentTabChanged(int index);

private:
    // 解析 dpkg-deb 输出的 control 字段（含多行字段，如 Description）
    static QMap<QString, QString> parseControlFields(const QString &text);
    // 调用 dpkg-deb，失败时 ok 置为 false 并返回空串
    static QString execDpkgDeb(const QStringList &args, bool *ok = nullptr, int timeoutMs = 10000);
    static QString humanSize(qint64 bytes);

    void initUI();
    void showErrorMessage(const QString &message);
    void loadScripts();

    DUrl m_url;
    QString m_title;

    QWidget *m_contentWidget = nullptr;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_headerLabel = nullptr;

    QTabWidget *m_tabWidget = nullptr;
    QScrollArea *m_infoScroll = nullptr;
    QLabel *m_errorLabel = nullptr;

    QComboBox *m_scriptCombo = nullptr;
    QPlainTextEdit *m_scriptView = nullptr;

    QString m_currentPath;
    bool m_scriptsLoaded = false;
    QMap<QString, QString> m_scripts;
};

DFM_END_NAMESPACE

#endif // DEBPREVIEWPLUGIN_H
