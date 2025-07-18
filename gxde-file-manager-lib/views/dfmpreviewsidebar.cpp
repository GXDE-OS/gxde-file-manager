#include "dfmpreviewsidebar.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileInfo>
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

void DFMPreviewSidebar::initUI()
{
    m_fileIcon = new QLabel(this);
    m_fileName = new QLabel(this);
    m_fileSizeTips = new QLabel("<b>" + tr("File Size:") + "</b>");
    m_fileSize = new QLabel(this);
    m_fileMimetype = new QLabel(this);
    m_timeCreatedLabel = new QLabel(this);
    m_timeModifiedLabel = new QLabel(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_fileIcon);
    layout->addWidget(new QLabel("<hr/>"));
    layout->addWidget(new QLabel("<b>" + tr("File Name:") + "</b>"));
    layout->addWidget(m_fileName);
    layout->addWidget(m_fileSizeTips);
    layout->addWidget(m_fileSize);
    layout->addWidget(new QLabel("<b>" + tr("Mimetype:") + "</b>"));
    layout->addWidget(m_fileMimetype);
    layout->addWidget(new QLabel("<b>" + tr("Time read:") + "</b>"));
    layout->addWidget(m_timeCreatedLabel);
    layout->addWidget(new QLabel("<b>" + tr("Time modified:") + "</b>"));
    layout->addWidget(m_timeModifiedLabel);
    /*layout->addWidget(new QLabel(tr("File Path:")));
    layout->addWidget(labelAddScroll(m_filePath));*/

    setLayout(layout);

    m_fileIcon->setAlignment(Qt::AlignCenter);

    // 设置允许拷贝
    m_fileName->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    m_fileSize->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    m_fileMimetype->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    m_timeCreatedLabel->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    m_timeModifiedLabel->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);

    m_fileName->setWordWrap(true);
    m_fileSize->setWordWrap(true);
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

    // 文件名
    m_fileName->setText(info.fileName());
    m_fileName->setToolTip(info.fileName());

    // 文件大小
    m_fileSize->setText(info.sizeDisplayName());
    m_fileSize->setToolTip(info.sizeDisplayName());

    // MimeType 类型
    m_fileMimetype->setText(info.mimeTypeDisplayName());

    // 文件访问/修改时间
    m_timeCreatedLabel->setText(info.lastReadDisplayName());
    m_timeModifiedLabel->setText(info.lastModifiedDisplayName());

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
