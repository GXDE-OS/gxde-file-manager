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

#include "exepreview.h"

#include "dfmglobal.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>
#include <cstring>
#include <functional>

DFM_BEGIN_NAMESPACE

namespace {

// 资源类型
enum {
    kResTypeIcon = 3,       // RT_ICON
    kResTypeGroupIcon = 14, // RT_GROUP_ICON
    kResTypeVersion = 16    // RT_VERSION
};

// 读取小端整数（调用方需保证不越界）
inline quint16 readU16(const uchar *p)
{
    return quint16(p[0]) | (quint16(p[1]) << 8);
}

inline quint32 readU32(const uchar *p)
{
    return quint32(p[0]) | (quint32(p[1]) << 8)
            | (quint32(p[2]) << 16) | (quint32(p[3]) << 24);
}

inline void appendU16(QByteArray &data, quint16 value)
{
    data.append(char(value & 0xff));
    data.append(char(value >> 8));
}

inline void appendU32(QByteArray &data, quint32 value)
{
    data.append(char(value & 0xff));
    data.append(char((value >> 8) & 0xff));
    data.append(char((value >> 16) & 0xff));
    data.append(char(value >> 24));
}

// 32 位对齐
inline int alignOffset(int offset)
{
    return (offset + 3) & ~3;
}

// 读取 UTF-16LE 文本（遇到结束符或达到 maxChars 停止）
QString readUtf16(const uchar *p, int maxChars)
{
    QString result;

    for (int i = 0; i < maxChars; ++i) {
        const QChar ch(readU16(p + i * 2));

        if (ch.isNull())
            break;

        result += ch;
    }

    return result;
}

// 依赖等值太长时换行显示即可，无需特殊处理
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

// VERSIONINFO 资源树节点（VS_VERSIONINFO / StringFileInfo / StringTable / String）
namespace {

struct VersionNode {
    quint16 length = 0;      // 节点总长度（含 32 位对齐填充）
    quint16 valueLength = 0; // 值长度（单位：字）
    quint16 type = 0;        // 0 二进制，1 文本
    QString key;
    int valueOffset = 0;     // 值相对节点起始的偏移（已按 32 位对齐）
};

// 遍历过程中的收集目标
struct VersionWalkState {
    QMap<QString, QString> *strings = nullptr;
    quint16 *langId = nullptr;
    bool *hasLang = nullptr;
};

// 在 data 的 [offset, end) 范围内读取一个节点
bool readVersionNode(const QByteArray &data, int offset, int end, VersionNode *node)
{
    if (offset < 0 || offset + 6 > end || end > data.size())
        return false;

    const uchar *base = reinterpret_cast<const uchar *>(data.constData());
    node->length = readU16(base + offset);

    if (node->length < 6 || offset + node->length > end)
        return false;

    node->valueLength = readU16(base + offset + 2);
    node->type = readU16(base + offset + 4);
    node->key = readUtf16(base + offset + 6, (node->length - 6) / 2);
    node->valueOffset = alignOffset(offset + 6 + (node->key.size() + 1) * 2);

    return true;
}

// 递归遍历 VERSIONINFO 资源树，收集字符串值与语言信息
// 注意：StringTable 节点的键是 8 位十六进制的语言/代码页串（如 "080904b0"），
// 因此不能按键名区分，需以 wValueLength 区分：为 0 的是容器，大于 0 的是叶子
void walkVersionInfo(const QByteArray &data, int offset, int end, VersionWalkState *state, int depth)
{
    // 防御畸形文件构造的过深嵌套
    if (depth > 16)
        return;

    while (offset + 6 <= end) {
        VersionNode node;

        if (!readVersionNode(data, offset, end, &node))
            break;

        const int nodeEnd = offset + node.length;

        if (node.key == QLatin1String("Translation") && node.type == 0) {
            // 值为 (语言 ID, 代码页) 数组，取第一个即可
            if (node.valueOffset + 2 <= nodeEnd && state->langId && state->hasLang) {
                *state->langId = readU16(reinterpret_cast<const uchar *>(data.constData()) + node.valueOffset);
                *state->hasLang = true;
            }
        } else if (node.type == 1 && node.valueLength > 0 && state->strings) {
            // 叶子节点：值为 UTF-16 文本
            const int maxChars = qMin<int>(node.valueLength, (nodeEnd - node.valueOffset) / 2);

            if (maxChars > 0 && node.valueOffset < nodeEnd) {
                (*state->strings)[node.key] = readUtf16(
                    reinterpret_cast<const uchar *>(data.constData()) + node.valueOffset, maxChars);
            }
        } else if (node.type == 1 && node.valueLength == 0) {
            // 容器节点（StringFileInfo / VarFileInfo / StringTable），子节点紧随其后
            walkVersionInfo(data, qMin(node.valueOffset, nodeEnd), nodeEnd, state, depth + 1);
        } else if (node.type == 0 && node.valueLength > 0) {
            // 含二进制值的节点：值为 VS_FIXEDFILEINFO（签名 0xFEEF04BD）时，
            // 其长度固定为 52 字节，跳过后遍历子节点
            const uchar *value = reinterpret_cast<const uchar *>(data.constData()) + node.valueOffset;

            if (node.valueOffset + 52 <= nodeEnd && readU32(value) == 0xFEEF04BDu)
                walkVersionInfo(data, alignOffset(node.valueOffset + 52), nodeEnd, state, depth + 1);
        }

        offset = alignOffset(nodeEnd);
    }
}

} // namespace

ExePreview::ExePreview(QObject *parent)
    : DFMFilePreview(parent)
{
    initUI();
}

ExePreview::~ExePreview()
{
    if (m_contentWidget)
        m_contentWidget->deleteLater();
}

void ExePreview::initUI()
{
    m_contentWidget = new QWidget();
    m_contentWidget->setFixedSize(760, 520);

    QVBoxLayout *layout = new QVBoxLayout(m_contentWidget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    // 头部：程序图标 + 文件名/描述
    m_iconLabel = new QLabel(m_contentWidget);
    m_iconLabel->setFixedSize(96, 96);
    m_iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

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

    m_infoScroll = new QScrollArea(m_contentWidget);
    m_infoScroll->setWidgetResizable(true);
    m_infoScroll->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_infoScroll, 1);
}

bool ExePreview::setFileUrl(const DUrl &url)
{
    if (m_url == url)
        return true;

    m_url = url;

    const QString &path = url.toLocalFile();
    const QFileInfo &fileInfo = QFileInfo(path);

    m_title = fileInfo.fileName();

    PEInfo peInfo;

    // 无法按 PE 格式解析时返回 false，由 FilePreviewDialog 回退到
    // UnknowFilePreview（与普通文件一致的预览界面），不在此处提示错误
    if (!parsePE(path, &peInfo))
        return false;

    VersionInfo verInfo;

    if (peInfo.hasVersionInfo)
        parseVersionInfo(peInfo.versionResource, &verInfo);

    // 头部：优先展示资源中提取的真实图标
    QImage iconImage;

    if (peInfo.hasIcon) {
        const QByteArray &iconData = assembleIcon(peInfo);

        if (!iconData.isEmpty())
            iconImage = QImage::fromData(iconData, "ico");
    }

    if (!iconImage.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(iconImage);

        if (pixmap.width() > 96 || pixmap.height() > 96)
            pixmap = pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        m_iconLabel->setPixmap(pixmap);
    } else {
        QIcon icon = QIcon::fromTheme(QStringLiteral("application-x-executable"));

        if (icon.isNull())
            icon = m_contentWidget->style()->standardIcon(QStyle::SP_FileDialogDetailedView);

        m_iconLabel->setPixmap(icon.pixmap(96, 96));
    }

    const QString &description = verInfo.strings.value(QStringLiteral("FileDescription"));

    if (!description.isEmpty())
        m_headerLabel->setText(tr("%1\n%2").arg(fileInfo.fileName(), description));
    else
        m_headerLabel->setText(fileInfo.fileName());

    // 重建信息页
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

    auto addTextRow = [this, form, &verInfo](const QString &label, const QString &key) {
        const QString &value = verInfo.strings.value(key);
        form->addRow(label + QStringLiteral(":"),
                     createValueLabel(value.isEmpty() ? tr("未知") : value));
    };

    addRow(tr("文件名"), createValueLabel(fileInfo.fileName()));
    addTextRow(tr("文件描述"), QStringLiteral("FileDescription"));
    addTextRow(tr("产品名称"), QStringLiteral("ProductName"));
    addTextRow(tr("公司/作者"), QStringLiteral("CompanyName"));

    // 版本字符串优先，缺失时回退到 VS_FIXEDFILEINFO 中的数值版本
    const QString &fileVersion = verInfo.strings.value(QStringLiteral("FileVersion"));
    addRow(tr("文件版本"), createValueLabel(!fileVersion.isEmpty() ? fileVersion
            : verInfo.hasFixedInfo ? versionString(verInfo.fileVersionMS, verInfo.fileVersionLS)
            : tr("未知")));

    const QString &productVersion = verInfo.strings.value(QStringLiteral("ProductVersion"));
    addRow(tr("产品版本"), createValueLabel(!productVersion.isEmpty() ? productVersion
            : verInfo.hasFixedInfo ? versionString(verInfo.productVersionMS, verInfo.productVersionLS)
            : tr("未知")));

    addTextRow(tr("版权信息"), QStringLiteral("LegalCopyright"));
    addRow(tr("架构"), createValueLabel(machineName(peInfo.machine)));
    addRow(tr("程序类型"), createValueLabel(subsystemName(peInfo.subsystem)));

    if (verInfo.hasLang)
        addRow(tr("语言"), createValueLabel(langName(verInfo.langId)));

    addTextRow(tr("内部名称"), QStringLiteral("InternalName"));
    addTextRow(tr("原始文件名"), QStringLiteral("OriginalFilename"));
    addTextRow(tr("备注"), QStringLiteral("Comments"));

    addRow(tr("文件大小"), createValueLabel(humanSize(fileInfo.size())));
    addRow(tr("修改时间"), createValueLabel(QLocale::system().toString(
            fileInfo.lastModified(), QStringLiteral("yyyy-MM-dd hh:mm"))));

    // PE 头中的编译时间戳，部分编译器会置 0 或伪造，仅作参考
    if (peInfo.timeStamp > 0) {
        addRow(tr("编译时间"), createValueLabel(QLocale::system().toString(
                QDateTime::fromSecsSinceEpoch(quint64(peInfo.timeStamp)),
                QStringLiteral("yyyy-MM-dd hh:mm"))));
    }

    m_infoScroll->setWidget(infoPage);

    Q_EMIT titleChanged();

    return true;
}

DUrl ExePreview::fileUrl() const
{
    return m_url;
}

QWidget *ExePreview::contentWidget() const
{
    return m_contentWidget;
}

QString ExePreview::title() const
{
    return m_title;
}

bool ExePreview::showStatusBarSeparator() const
{
    return true;
}

bool ExePreview::parsePE(const QString &path, PEInfo *info) const
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    // 头部区域（DOS 头 + PE 头 + 节表），预留到 64KB 以容纳节表较多的文件
    const QByteArray header = file.read(qMin<qint64>(file.size(), 65536));

    if (header.size() < 0x40)
        return false;

    const uchar *h = reinterpret_cast<const uchar *>(header.constData());

    // DOS 头：'MZ'，PE 头偏移位于 0x3C
    if (h[0] != 'M' || h[1] != 'Z')
        return false;

    const quint32 peOffset = readU32(h + 0x3C);

    // 注意先判越界再做加法，避免 peOffset 接近 0xFFFFFFFF 时整型回绕
    if (peOffset > quint32(header.size()) - 24 || readU32(h + peOffset) != 0x00004550u)
        return false;

    // COFF 文件头
    const quint16 sectionCount = readU16(h + peOffset + 6);
    const quint16 optionalSize = readU16(h + peOffset + 20);
    const quint32 optionalOffset = peOffset + 24;

    // 可选头
    if (optionalOffset + 2 > quint32(header.size()))
        return false;

    const quint16 magic = readU16(h + optionalOffset);

    if (magic != 0x10b && magic != 0x20b) // PE32 / PE32+
        return false;

    info->machine = readU16(h + peOffset + 4);
    info->timeStamp = readU32(h + peOffset + 8);
    info->subsystem = (optionalOffset + 70 <= quint32(header.size()))
            ? readU16(h + optionalOffset + 68) : 0;

    // 节表（位于可选头之后）
    struct Section {
        quint32 va;
        quint32 virtualSize;
        quint32 rawSize;
        quint32 rawPtr;
    };

    QList<Section> sections;

    for (quint16 i = 0; i < sectionCount; ++i) {
        const quint32 offset = optionalOffset + optionalSize + i * 40;

        if (offset + 40 > quint32(header.size()))
            break;

        sections.append({ readU32(h + offset + 12), readU32(h + offset + 8),
                          readU32(h + offset + 16), readU32(h + offset + 20) });
    }

    // RVA -> 文件偏移
    auto rvaToOffset = [&sections, size = quint32(header.size())](quint32 rva) -> qint64 {
        for (const Section &s : sections) {
            const quint32 sectionSize = qMax(s.virtualSize, s.rawSize);

            if (sectionSize > 0 && rva >= s.va && rva < s.va + sectionSize)
                return qint64(s.rawPtr) + (rva - s.va);
        }

        // 数据位于头部区域时直接对应文件偏移
        return rva < size ? qint64(rva) : qint64(-1);
    };

    // 数据目录第 2 项为资源表
    const quint32 dirCountOffset = optionalOffset + (magic == 0x10b ? 92 : 108);
    const quint32 dirsOffset = optionalOffset + (magic == 0x10b ? 96 : 112);

    if (dirCountOffset + 4 > quint32(header.size()) || dirsOffset + 3 * 8 > quint32(header.size()))
        return true;

    if (readU32(h + dirCountOffset) < 3)
        return true;

    const quint32 resourceRva = readU32(h + dirsOffset + 2 * 8);

    if (resourceRva == 0)
        return true;

    const qint64 resourceOffset = rvaToOffset(resourceRva);

    if (resourceOffset < 0)
        return true;

    auto readRange = [&file](qint64 offset, qint64 size) -> QByteArray {
        if (offset < 0 || size <= 0 || size > (64 << 20))
            return QByteArray();

        if (!file.seek(offset))
            return QByteArray();

        return file.read(size);
    };

    // 资源目录：类型 -> 资源 ID -> 语言 三级树，目录内偏移均相对资源起始
    qint64 totalIconBytes = 0;

    std::function<void(quint32, int, quint32, quint32)> walk;

    walk = [&](quint32 dirOffset, int level, quint32 typeId, quint32 resId) {
        if (level > 2)
            return;

        const QByteArray dir = readRange(resourceOffset + dirOffset, 16);

        if (dir.size() < 16)
            return;

        const uchar *d = reinterpret_cast<const uchar *>(dir.constData());
        const int count = readU16(d + 12) + readU16(d + 14);

        if (count <= 0 || count > 4096)
            return;

        const QByteArray entries = readRange(resourceOffset + dirOffset + 16, count * 8);

        if (entries.size() < count * 8)
            return;

        const uchar *e = reinterpret_cast<const uchar *>(entries.constData());

        for (int i = 0; i < count; ++i) {
            const quint32 id = readU32(e + i * 8);
            const quint32 next = readU32(e + i * 8 + 4);

            if (next & 0x80000000u) {
                // 子目录，同时向子级传递类型与资源 ID
                walk(next & 0x7fffffffu, level + 1,
                     level == 0 ? id : typeId, level == 1 ? id : resId);
            } else if (level == 2) {
                // 数据条目：数据 RVA + 大小
                const QByteArray entry = readRange(resourceOffset + next, 16);

                if (entry.size() < 16)
                    continue;

                const uchar *v = reinterpret_cast<const uchar *>(entry.constData());
                const qint64 dataOffset = rvaToOffset(readU32(v));

                if (dataOffset < 0)
                    continue;

                if (typeId == kResTypeGroupIcon) {
                    if (info->groupIcon.isEmpty())
                        info->groupIcon = readRange(dataOffset, qMin<qint64>(readU32(v + 4), 1 << 20));
                } else if (typeId == kResTypeIcon && !info->icons.contains(resId)
                           && info->icons.size() < 64 && totalIconBytes < (64 << 20)) {
                    const QByteArray iconData = readRange(dataOffset, qMin<qint64>(readU32(v + 4), 32 << 20));

                    totalIconBytes += iconData.size();
                    info->icons[resId] = iconData;
                } else if (typeId == kResTypeVersion && info->versionResource.isEmpty()) {
                    info->versionResource = readRange(dataOffset, qMin<qint64>(readU32(v + 4), 1 << 20));
                }
            }
        }
    };

    walk(0, 0, 0, 0);

    info->hasVersionInfo = !info->versionResource.isEmpty();
    info->hasIcon = !info->groupIcon.isEmpty();

    return true;
}

bool ExePreview::parseVersionInfo(const QByteArray &data, VersionInfo *info) const
{
    VersionNode root;

    if (!readVersionNode(data, 0, data.size(), &root)
            || root.key != QLatin1String("VS_VERSION_INFO"))
        return false;

    // 根节点的值为 VS_FIXEDFILEINFO（52 字节，签名 0xFEEF04BD）
    int children = root.valueOffset;

    if (root.type == 0 && root.valueOffset + 52 <= data.size()) {
        const uchar *base = reinterpret_cast<const uchar *>(data.constData()) + root.valueOffset;

        if (readU32(base) == 0xFEEF04BDu) {
            info->hasFixedInfo = true;
            info->fileVersionMS = readU32(base + 8);
            info->fileVersionLS = readU32(base + 12);
            info->productVersionMS = readU32(base + 16);
            info->productVersionLS = readU32(base + 20);
            children = alignOffset(root.valueOffset + 52);
        }
    }

    VersionWalkState state;
    state.strings = &info->strings;
    state.langId = &info->langId;
    state.hasLang = &info->hasLang;

    walkVersionInfo(data, children, data.size(), &state, 0);

    return true;
}

QByteArray ExePreview::assembleIcon(const PEInfo &info) const
{
    // GRPICONDIRENTRY 为 14 字节，与 ICONDIRENTRY 相比末尾是资源 ID 而非文件偏移
    if (info.groupIcon.size() < 6 + 14)
        return QByteArray();

    const uchar *g = reinterpret_cast<const uchar *>(info.groupIcon.constData());
    const quint16 count = readU16(g + 4);

    if (readU16(g + 2) != 1 || count == 0 || count > 64 || info.groupIcon.size() < 6 + count * 14)
        return QByteArray();

    struct IconEntry {
        uchar header[8]; // 与 ICONDIRENTRY 前 8 字节一致
        QByteArray data;
        quint32 area;
    };

    QList<IconEntry> entries;

    for (quint16 i = 0; i < count; ++i) {
        const uchar *e = g + 6 + i * 14;
        const quint32 id = readU16(e + 12);

        if (!info.icons.contains(id))
            continue;

        IconEntry entry;

        memcpy(entry.header, e, 8);
        entry.data = info.icons.value(id);

        // 宽/高为 0 表示 256
        const quint32 width = entry.header[0] == 0 ? 256 : entry.header[0];
        const quint32 height = entry.header[1] == 0 ? 256 : entry.header[1];
        entry.area = width * height;

        entries.append(entry);
    }

    if (entries.isEmpty())
        return QByteArray();

    // 按面积从大到小排列，Qt 读取 ICO 时默认取第一张图
    std::sort(entries.begin(), entries.end(), [](const IconEntry &a, const IconEntry &b) {
        return a.area > b.area;
    });

    const int iconCount = entries.size();
    qint64 imageOffset = 6 + iconCount * 16;
    qint64 totalSize = imageOffset;

    for (const IconEntry &entry : entries)
        totalSize += entry.data.size();

    QByteArray ico;

    ico.reserve(int(totalSize));

    // ICONDIR
    appendU16(ico, 0);
    appendU16(ico, 1);
    appendU16(ico, quint16(iconCount));

    // ICONDIRENTRY
    for (const IconEntry &entry : entries) {
        ico.append(reinterpret_cast<const char *>(entry.header), 8);
        appendU32(ico, quint32(entry.data.size()));
        appendU32(ico, quint32(imageOffset));
        imageOffset += entry.data.size();
    }

    // 图像数据
    for (const IconEntry &entry : entries)
        ico.append(entry.data);

    return ico;
}

QString ExePreview::machineName(quint16 machine) const
{
    switch (machine) {
    case 0x014c:
        return QStringLiteral("x86 (32 位)");
    case 0x8664:
        return QStringLiteral("x64 (64 位)");
    case 0x01c0:
    case 0x01c4:
        return QStringLiteral("ARM (32 位)");
    case 0xAA64:
        return QStringLiteral("ARM64 (64 位)");
    case 0x0200:
        return QStringLiteral("Itanium");
    default:
        return tr("未知 (0x%1)").arg(machine, 4, 16, QLatin1Char('0'));
    }
}

QString ExePreview::subsystemName(quint16 subsystem) const
{
    switch (subsystem) {
    case 1:
        return tr("原生驱动");
    case 2:
        return tr("图形界面程序");
    case 3:
        return tr("控制台程序");
    case 9:
        return tr("图形界面程序 (Windows CE)");
    case 16:
        return tr("引导应用程序");
    default:
        return tr("未知");
    }
}

QString ExePreview::langName(quint16 langId) const
{
    switch (langId) {
    case 0x0404:
        return tr("繁体中文");
    case 0x0804:
        return tr("简体中文");
    case 0x0409:
        return tr("英语 (美国)");
    case 0x0809:
        return tr("英语 (英国)");
    case 0x0407:
        return tr("德语");
    case 0x040c:
        return tr("法语");
    case 0x0410:
        return tr("意大利语");
    case 0x040a:
        return tr("西班牙语");
    case 0x0411:
        return tr("日语");
    case 0x0412:
        return tr("韩语");
    case 0x0416:
        return tr("葡萄牙语 (巴西)");
    case 0x0419:
        return tr("俄语");
    case 0x0415:
        return tr("波兰语");
    case 0x0413:
        return tr("荷兰语");
    case 0x041d:
        return tr("瑞典语");
    default:
        return QStringLiteral("0x%1").arg(langId, 4, 16, QLatin1Char('0'));
    }
}

QString ExePreview::versionString(quint32 high, quint32 low) const
{
    return QStringLiteral("%1.%2.%3.%4")
            .arg(high >> 16)
            .arg(high & 0xffff)
            .arg(low >> 16)
            .arg(low & 0xffff);
}

QString ExePreview::humanSize(qint64 bytes)
{
    if (bytes < 0)
        return ExePreview::tr("未知");

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

DFM_END_NAMESPACE
