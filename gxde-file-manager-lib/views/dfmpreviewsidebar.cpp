#include "dfmpreviewsidebar.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileInfo>
#include <QImageReader>
#include <QKeyEvent>
#include <QMenu>
#include <QAction>
#include <QGuiApplication>
#include <QClipboard>
#include "dfileinfo.h"

DFMPreviewSidebar::DFMPreviewSidebar(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    //refresh();
}

DFMPreviewSidebar::~DFMPreviewSidebar()
{

}

WrapLabel::WrapLabel(QWidget *parent)
    : QLabel(parent)
{
}

// 按可用宽度对文本进行强制换行（长单词内部也会断开），避免超出侧边栏宽度
static QString wrapTextByWidth(const QString &text, const QFontMetrics &fm, int maxWidth)
{
    if (maxWidth <= 0)
        return text;

    QString result;
    QString line;
    for (const QChar &c : text) {
        if (!line.isEmpty() && fm.horizontalAdvance(line + c) > maxWidth) {
            result += line;
            result += QLatin1Char('\n');
            line.clear();
        }
        line += c;
    }
    result += line;
    return result;
}

void WrapLabel::setWrappedText(const QString &text)
{
    m_sourceText = text;
    QLabel::setText(wrapTextByWidth(text, fontMetrics(), contentsRect().width()));
}

void WrapLabel::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Copy)) {
        copyCleanText();
        event->accept();
        return;
    }

    QLabel::keyPressEvent(event);
}

bool WrapLabel::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        // 抢占 Ctrl+C，避免窗口级的“复制文件”快捷键把文件路径写入剪贴板
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Copy)) {
            event->accept();
            return true;
        }
    } else if (event->type() == QEvent::ContextMenu) {
        // 使用自定义右键菜单，避免默认菜单复制到带换行符的文本
        const QContextMenuEvent *menuEvent = static_cast<QContextMenuEvent *>(event);

        QMenu menu(this);
        QAction *copyAction = menu.addAction(tr("复制"));
        QAction *selectAllAction = menu.addAction(tr("全选"));

        if (QAction *chosen = menu.exec(menuEvent->globalPos())) {
            if (chosen == copyAction) {
                copyCleanText();
            } else if (chosen == selectAllAction) {
                setSelection(0, text().length());
            }
        }

        return true;
    }

    return QLabel::event(event);
}

void WrapLabel::copyCleanText()
{
    QString text = m_sourceText;
    if (hasSelectedText()) {
        // 只复制选中部分时，去掉自动换行产生的换行符
        text = selectedText();
        text.remove(QLatin1Char('\n'));
        text.remove(QLatin1Char('\r'));
        text.remove(QChar(0x2029)); // 段落分隔符
        if (text.isEmpty())
            text = m_sourceText;
    }
    QGuiApplication::clipboard()->setText(text);
}

void DFMPreviewSidebar::initUI()
{
    m_fileIcon = new QLabel(this);
    m_fileName = new WrapLabel(this);
    m_fileSizeTips = new QLabel("<b>" + tr("File Size:") + "</b>");
    m_fileSize = new WrapLabel(this);
    m_imageSizeTips = new QLabel("<b>" + tr("Image Size:") + "</b>");
    m_imageSize = new WrapLabel(this);
    m_fileMimetype = new WrapLabel(this);
    m_timeCreatedLabel = new WrapLabel(this);
    m_timeModifiedLabel = new WrapLabel(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_fileIcon);
    layout->addWidget(new QLabel("<hr/>"));
    layout->addWidget(new QLabel("<b>" + tr("File Name:") + "</b>"));
    layout->addWidget(m_fileName);
    layout->addWidget(m_fileSizeTips);
    layout->addWidget(m_fileSize);
    layout->addWidget(m_imageSizeTips);
    layout->addWidget(m_imageSize);
    layout->addWidget(new QLabel("<b>" + tr("Mimetype:") + "</b>"));
    layout->addWidget(m_fileMimetype);
    layout->addWidget(new QLabel("<b>" + tr("Time read:") + "</b>"));
    layout->addWidget(m_timeCreatedLabel);
    layout->addWidget(new QLabel("<b>" + tr("Time modified:") + "</b>"));
    layout->addWidget(m_timeModifiedLabel);
    /*layout->addWidget(new QLabel(tr("File Path:")));
    layout->addWidget(labelAddScroll(m_filePath));*/
    layout->addStretch(1);

    setLayout(layout);

    m_fileIcon->setAlignment(Qt::AlignCenter);

    // 设置允许拷贝
    m_fileName->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    m_fileSize->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    m_imageSize->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    m_fileMimetype->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    m_timeCreatedLabel->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    m_timeModifiedLabel->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);

    m_fileName->setWordWrap(true);
    m_fileSize->setWordWrap(true);
    m_imageSize->setWordWrap(true);
    m_fileMimetype->setWordWrap(true);
    m_timeCreatedLabel->setWordWrap(true);
    m_timeModifiedLabel->setWordWrap(true);

    isInited = true;
}

QScrollArea* DFMPreviewSidebar::labelAddScroll(QLabel* label)
{
    QScrollArea *scroll_area = new QScrollArea(this);
    scroll_area->setWidget(label);
    scroll_area->setWidgetResizable(true);
    scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    label->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    return scroll_area;
}


void DFMPreviewSidebar::resizeEvent(QResizeEvent *event)
{
    // 刷新 UI
    refresh();
    QWidget::resizeEvent(event);
}

void DFMPreviewSidebar::refresh()
{
    if (!isVisible() || !isInited) {
        // 如果没有显示控件或控件未创建完成，则不进行渲染
        return;
    }
    if (m_selectUrlList.count() <= 0) {
        setDisabled(true);
        return;
    }
    setEnabled(true);
    DUrl fileUrl = m_selectUrlList.at(0);
    QString path = fileUrl.toLocalFile();
    DFileInfo info(path);

    bool isFile = info.isFile();

    m_fileSizeTips->setVisible(isFile);
    m_fileSize->setVisible(isFile);

    // 文件名（按控件宽度强制换行，避免长文件名超出窗口；复制时不带换行符）
    m_fileName->setWrappedText(info.fileName());
    m_fileName->setToolTip(info.fileName());

    // 文件大小
    m_fileSize->setWrappedText(info.sizeDisplayName());
    m_fileSize->setToolTip(info.sizeDisplayName());

    // 图片尺寸（宽x高），仅图片文件显示，其它文件隐藏
    bool showImageSize = false;
    if (info.mimeTypeName().startsWith("image")) {
        const QSize imageSize = QImageReader(path).size();
        if (imageSize.isValid() && imageSize.width() > 0 && imageSize.height() > 0) {
            const QString imageSizeText = QString("%1 x %2").arg(imageSize.width()).arg(imageSize.height());
            m_imageSize->setWrappedText(imageSizeText);
            m_imageSize->setToolTip(imageSizeText);
            showImageSize = true;
        }
    }
    m_imageSizeTips->setVisible(showImageSize);
    m_imageSize->setVisible(showImageSize);

    // MimeType 类型（同样按宽度强制换行，避免长类型名超出窗口）
    m_fileMimetype->setWrappedText(info.mimeTypeDisplayName());

    // 文件访问/修改时间
    m_timeCreatedLabel->setWrappedText(info.lastReadDisplayName());
    m_timeModifiedLabel->setWrappedText(info.lastModifiedDisplayName());

    /*m_filePath->setText(info.absoluteFilePath());
    m_filePath->setToolTip(info.absoluteFilePath());*/

    float ratio = devicePixelRatio();
    m_fileIcon->setPixmap(info.fileIcon().pixmap(width() * 0.9 * ratio,
                                                 width() * 0.9 * ratio));

}

void DFMPreviewSidebar::setFileUrl(QList<DUrl> url)
{
    m_selectUrlList = url;
    refresh();
}
