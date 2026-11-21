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

#include "debpreview.h"

#include "dfmglobal.h"

#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScrollArea>
#include <QStyle>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QVBoxLayout>

DFM_BEGIN_NAMESPACE

namespace {

// 依赖等字段太长时保留换行显示即可，无需特殊处理
QLabel *createValueLabel(const QString &text, bool richText = false)
{
    QLabel *label = new QLabel(text);

    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    if (richText) {
        label->setTextFormat(Qt::RichText);
        label->setOpenExternalLinks(true);
    } else {
        label->setTextFormat(Qt::PlainText);
    }

    label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    return label;
}

} // namespace

DebPreview::DebPreview(QObject *parent)
    : DFMFilePreview(parent)
{
    initUI();
}

DebPreview::~DebPreview()
{
    if (m_contentWidget)
        m_contentWidget->deleteLater();
}

void DebPreview::initUI()
{
    m_contentWidget = new QWidget();
    m_contentWidget->setFixedSize(760, 520);

    QVBoxLayout *layout = new QVBoxLayout(m_contentWidget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    // 头部：包图标 + 包名/版本
    m_iconLabel = new QLabel(m_contentWidget);
    m_iconLabel->setFixedSize(48, 48);

    m_headerLabel = new QLabel(m_contentWidget);
    m_headerLabel->setTextFormat(Qt::PlainText);
    m_headerLabel->setWordWrap(true);
    m_headerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QFont headerFont = m_headerLabel->font();
    headerFont.setPixelSize(16);
    headerFont.setBold(true);
    m_headerLabel->setFont(headerFont);

    QHBoxLayout *headerLayout = new QHBoxLayout;
    headerLayout->addWidget(m_iconLabel, 0, Qt::AlignTop);
    headerLayout->addWidget(m_headerLabel, 0, Qt::AlignTop);
    layout->addLayout(headerLayout);

    // 错误提示（默认隐藏）
    m_errorLabel = new QLabel(m_contentWidget);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel);

    // 两个标签页：基本信息 / 脚本
    m_tabWidget = new QTabWidget(m_contentWidget);
    layout->addWidget(m_tabWidget, 1);

    // 基本信息页
    m_infoScroll = new QScrollArea(m_tabWidget);
    m_infoScroll->setWidgetResizable(true);
    m_infoScroll->setFrameShape(QFrame::NoFrame);
    m_tabWidget->addTab(m_infoScroll, tr("基本信息"));

    // 脚本页
    // 注意：Qt6 下 DStyle 绘制的 QTabWidget 页面框线内缩约 6px，
    // 边距过小会导致下拉框/文本框绘制到框线上，看起来重叠
    QWidget *scriptPage = new QWidget(m_tabWidget);
    QVBoxLayout *scriptLayout = new QVBoxLayout(scriptPage);
    scriptLayout->setContentsMargins(10, 10, 10, 10);
    scriptLayout->setSpacing(6);

    m_scriptCombo = new QComboBox(scriptPage);
    scriptLayout->addWidget(m_scriptCombo);

    m_scriptView = new QPlainTextEdit(scriptPage);
    m_scriptView->setReadOnly(true);
    m_scriptView->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_scriptView->setWordWrapMode(QTextOption::NoWrap);
    m_scriptView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_scriptView->setFocusPolicy(Qt::NoFocus);
    scriptLayout->addWidget(m_scriptView, 1);

    m_tabWidget->addTab(scriptPage, tr("脚本"));

    connect(m_scriptCombo, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged),
            this, &DebPreview::showScript);
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &DebPreview::onCurrentTabChanged);
}

bool DebPreview::setFileUrl(const DUrl &url)
{
    if (m_url == url)
        return true;

    m_url = url;

    const QString &path = url.toLocalFile();

    // 读取 control 字段（"--" 防止以 "-" 开头的文件名被当作选项）
    bool controlOk = false;
    const QString &controlText = execDpkgDeb({"-f", "--", path}, &controlOk);

    if (!controlOk) {
        showErrorMessage(tr("无法读取此 deb 包的信息，文件可能已损坏，或系统中 dpkg-deb 工具不可用。"));
        m_title = QFileInfo(path).fileName();

        Q_EMIT titleChanged();
        return true;
    }

    const QMap<QString, QString> &fields = parseControlFields(controlText);
    const QString &packageName = fields.value(QStringLiteral("Package"));
    const QString &version = fields.value(QStringLiteral("Version"));
    const QString &summary = fields.value(QStringLiteral("Description")).section(QLatin1Char('\n'), 0, 0);
    const QString &fullDescription = fields.value(QStringLiteral("Description")).section(QLatin1Char('\n'), 1);

    // 头部
    QIcon icon = QIcon::fromTheme(QStringLiteral("application-x-deb"));

    if (icon.isNull())
        icon = QIcon::fromTheme(QStringLiteral("application-vnd.debian.binary-package"));

    if (icon.isNull())
        icon = m_contentWidget->style()->standardIcon(QStyle::SP_FileDialogDetailedView);

    m_iconLabel->setPixmap(icon.pixmap(48, 48));

    if (!summary.isEmpty())
        m_headerLabel->setText(tr("%1 (%2)\n%3").arg(packageName, version, summary));
    else
        m_headerLabel->setText(tr("%1 (%2)").arg(packageName, version));

    m_errorLabel->hide();
    m_tabWidget->show();

    // 重建基本信息页
    if (QWidget *oldPage = m_infoScroll->takeWidget())
        oldPage->deleteLater();

    QWidget *infoPage = new QWidget;
    QFormLayout *form = new QFormLayout(infoPage);
    form->setContentsMargins(10, 10, 10, 10);
    form->setSpacing(8);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);

    auto addRow = [form](const QString &label, QLabel *value) {
        form->addRow(label + QStringLiteral(":"), value);
    };

    addRow(tr("软件包名"), createValueLabel(packageName.isEmpty() ? tr("未知") : packageName));
    addRow(tr("版本号"), createValueLabel(version.isEmpty() ? tr("未知") : version));
    addRow(tr("包架构"), createValueLabel(fields.value(QStringLiteral("Architecture"), tr("未知"))));

    const QString &maintainer = fields.value(QStringLiteral("Maintainer"));

    addRow(tr("维护者"), createValueLabel(maintainer.isEmpty() ? tr("未知") : maintainer));

    const QString &homepage = fields.value(QStringLiteral("Homepage"));

    if (!homepage.isEmpty()) {
        QLabel *homeLabel = createValueLabel(QStringLiteral("<a href=\"%1\">%1</a>").arg(homepage), true);
        addRow(tr("主页"), homeLabel);
    }

    // 当前大小（deb 文件本身）与安装后大小（Installed-Size 字段，单位为 KiB）
    const QFileInfo &fileInfo = QFileInfo(path);
    addRow(tr("当前大小"), createValueLabel(humanSize(fileInfo.size())));

    const QString &installedSize = fields.value(QStringLiteral("Installed-Size"));
    addRow(tr("安装后大小"), createValueLabel(installedSize.isEmpty()
            ? tr("未知") : humanSize(installedSize.toLongLong() * 1024)));

    addRow(tr("所需依赖"), createValueLabel(fields.value(QStringLiteral("Depends"), tr("无"))));

    const QStringList optionalFields = {
        QStringLiteral("Recommends"),
        QStringLiteral("Suggests")
    };

    for (const QString &field : optionalFields) {
        const QString &value = fields.value(field);

        if (!value.isEmpty())
            addRow(field == QStringLiteral("Recommends") ? tr("推荐依赖") : tr("建议依赖"), createValueLabel(value));
    }

    addRow(tr("简介"), createValueLabel(summary.isEmpty() ? tr("无") : summary));

    if (!fullDescription.isEmpty())
        addRow(tr("详细描述"), createValueLabel(fullDescription));

    m_infoScroll->setWidget(infoPage);

    // 维护者脚本按需加载，避免阻塞首次显示
    m_currentPath = path;
    m_scriptsLoaded = false;
    m_scripts.clear();

    m_scriptCombo->blockSignals(true);
    m_scriptCombo->clear();
    m_scriptCombo->hide();
    m_scriptView->setPlainText(tr("切换到此标签页以加载维护者脚本。"));
    m_scriptCombo->blockSignals(false);

    // 若当前已停留在脚本标签页，则立即触发加载
    if (m_tabWidget->currentIndex() == 1)
        loadScripts();

    m_title = QFileInfo(path).fileName();

    Q_EMIT titleChanged();

    return true;
}

DUrl DebPreview::fileUrl() const
{
    return m_url;
}

QWidget *DebPreview::contentWidget() const
{
    return m_contentWidget;
}

QString DebPreview::title() const
{
    return m_title;
}

bool DebPreview::showStatusBarSeparator() const
{
    return true;
}

void DebPreview::onCurrentTabChanged(int index)
{
    if (index == 1)
        loadScripts();
}

void DebPreview::loadScripts()
{
    if (m_scriptsLoaded || m_currentPath.isEmpty())
        return;

    m_scriptsLoaded = true;

    QTemporaryDir tempDir;
    bool extractOk = false;

    if (tempDir.isValid())
        execDpkgDeb({"-e", "--", m_currentPath, tempDir.path()}, &extractOk);

    if (extractOk) {
        const QStringList scriptNames = {
            QStringLiteral("preinst"),
            QStringLiteral("postinst"),
            QStringLiteral("prerm"),
            QStringLiteral("postrm")
        };

        for (const QString &name : scriptNames) {
            QFile scriptFile(tempDir.filePath(name));

            if (!scriptFile.exists())
                continue;

            if (!scriptFile.open(QIODevice::ReadOnly))
                continue;

            m_scripts[name] = DFMGlobal::toUnicode(scriptFile.readAll(), m_currentPath);
        }
    }

    m_scriptCombo->blockSignals(true);
    m_scriptCombo->clear();

    if (m_scripts.isEmpty()) {
        m_scriptCombo->hide();
        m_scriptView->setPlainText(tr("该软件包不包含维护者脚本。"));
    } else {
        m_scriptCombo->show();
        m_scriptCombo->addItems(m_scripts.keys());

        // 用户通常关心 postinst，存在时默认显示
        const int index = m_scripts.contains(QStringLiteral("postinst"))
                ? m_scriptCombo->findText(QStringLiteral("postinst")) : 0;
        m_scriptCombo->setCurrentIndex(index);
    }

    m_scriptCombo->blockSignals(false);

    // 手动刷新脚本内容（上面的 setCurrentIndex 因信号被屏蔽不会触发）
    if (!m_scripts.isEmpty())
        showScript(m_scriptCombo->currentText());
}

void DebPreview::showScript(const QString &name)
{
    m_scriptView->setPlainText(m_scripts.value(name));
}

QMap<QString, QString> DebPreview::parseControlFields(const QString &text)
{
    QMap<QString, QString> result;
    QString currentField;

    for (const QString &line : text.split(QLatin1Char('\n'))) {
        // 多行字段的续行（以空格/制表符开头，"." 表示空行）
        if (line.startsWith(QLatin1Char(' ')) || line.startsWith(QLatin1Char('\t'))) {
            if (currentField.isEmpty())
                continue;

            QString continuation = line.trimmed();

            if (continuation == QLatin1String("."))
                continuation.clear();

            result[currentField] += QLatin1Char('\n') + continuation;
            continue;
        }

        const int index = line.indexOf(QLatin1Char(':'));

        if (index <= 0)
            continue;

        currentField = line.left(index).trimmed();
        result[currentField] = line.mid(index + 1).trimmed();
    }

    return result;
}

QString DebPreview::execDpkgDeb(const QStringList &args, bool *ok, int timeoutMs)
{
    if (ok)
        *ok = false;

    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // 统一输出格式，避免受用户环境影响
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    process.setProcessEnvironment(env);

    process.start(QStringLiteral("dpkg-deb"), args);

    if (!process.waitForStarted(3000) || !process.waitForFinished(timeoutMs))
        return QString();

    if (process.exitCode() != 0)
        return QString();

    if (ok)
        *ok = true;

    return QString::fromUtf8(process.readAllStandardOutput());
}

QString DebPreview::humanSize(qint64 bytes)
{
    if (bytes < 0)
        return DebPreview::tr("未知");

    double size = bytes;
    const QStringList units = { "B", "KB", "MB", "GB", "TB" };
    int unitIndex = 0;

    while (size >= 1024.0 && unitIndex < units.count() - 1) {
        size /= 1024.0;
        ++unitIndex;
    }

    QString value = (unitIndex == 0 || size >= 100.0)
            ? QString::number(size, 'f', 0) : QString::number(size, 'f', 1);

    return QStringLiteral("%1 %2").arg(value, units.at(unitIndex));
}

void DebPreview::showErrorMessage(const QString &message)
{
    m_tabWidget->hide();
    m_iconLabel->clear();
    m_headerLabel->clear();
    m_errorLabel->setText(message);
    m_errorLabel->show();
}

DFM_END_NAMESPACE
