/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *               2016 ~ 2018 dragondjf
 *
 * Author:     dragondjf<dingjiangfeng@deepin.com>
 *
 * Maintainer: dragondjf<dingjiangfeng@deepin.com>
 *             zccrs<zhangjide@deepin.com>
 *             Tangtong<tangtong@deepin.com>
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

#include "computerview.h"
#include "dfilemenu.h"
#include "dfilemenumanager.h"
#include "windowmanager.h"
#include "dfmevent.h"
#include "dfmeventdispatcher.h"
#include "dfmapplication.h"
#include "dfmsettings.h"

#include "app/define.h"
#include "app/filesignalmanager.h"
#include "dfileservices.h"
#include "controllers/pathmanager.h"
#include "controllers/appcontroller.h"
#include "deviceinfo/udisklistener.h"
#include "dabstractfileinfo.h"
#include "interfaces/dfmglobal.h"
#include "interfaces/dfmstandardpaths.h"
#include "gvfs/gvfsmountmanager.h"
#include "singleton.h"
#include "../shutil/fileutils.h"
#include "partman/partition.h"
#include "dabstractfilewatcher.h"
#include "interfaces/dstyleditemdelegate.h"

#include <dslider.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QDebug>
#include <QTextEdit>
#include <QPainter>
#include <QSizePolicy>
#include <QFile>
#include <QStorageInfo>
#include <QSettings>
#include <QUrlQuery>
#include <QScroller>

#include <QStyleHints>
#include <QCollator>
#include <QRegularExpression>
#include <algorithm>

#include <DApplication>

DWIDGET_USE_NAMESPACE

#define CVI_PROGRESSLINE_HEIGHT 4

namespace {

// 磁盘排列优先级：系统盘、数据盘固定在前两位，有盘标的按名称字母序，
// 无盘标（名称回退成容量）的按容量从大到小排在最后。
enum DiskItemRank {
    SystemDiskRank = 0,
    DataDiskRank = 1,
    NamedDiskRank = 2,
    UnnamedDiskRank = 3
};

bool diskItemHasCapacityName(const QString &name)
{
    // GIO 对无卷标的卷会生成 "256 GB"、"1 TB Volume" 之类的名字；卷标为空时
    // fileDisplayName() 回退成 "256.0 GiB"，windowsStyle 下还会追加 " (D:)"。
    // 只要显示名以「数字 + 容量单位」开头，就视为没有卷标。
    static const QRegularExpression pattern(
                QStringLiteral("^\\s*\\d+(?:\\.\\d+)?\\s*(?:B|KB|MB|GB|TB|KiB|MiB|GiB|TiB)(?![A-Za-z])"),
                QRegularExpression::CaseInsensitiveOption);
    return pattern.match(name).hasMatch();
}

int diskItemRank(const ComputerViewItem *item)
{
    if (!item) {
        return NamedDiskRank;
    }
    if (!item->deviceInfo().data()) {
        return diskItemHasCapacityName(item->name()) ? UnnamedDiskRank : NamedDiskRank;
    }

    const QDiskInfo diskInfo = item->deviceInfo()->getDiskInfo();
    if (diskInfo.isNativeCustom() && diskInfo.id() == QStringLiteral("/")
            && diskInfo.mounted_root_uri() == QStringLiteral("/")) {
        return SystemDiskRank;
    }
    if (diskInfo.isNativeCustom() && diskInfo.id() == QStringLiteral("/home")
            && diskInfo.mounted_root_uri() == QStringLiteral("/home")) {
        return DataDiskRank;
    }
    // 界面上只显示容量的盘统一归到最后一组，不能让它们混进按字母排序的一组
    if (diskInfo.name().isEmpty() || diskItemHasCapacityName(item->name())) {
        return UnnamedDiskRank;
    }

    return NamedDiskRank;
}

QString diskItemSortKey(const QString &name)
{
    QString key;
    key.reserve(name.size());

    for (const QChar ch : name) {
        if (ch.script() != QChar::Script_Han) {
            key.append(ch);
            continue;
        }

        // 汉字转成拼音后与拉丁字母一起排序，词库中的声调数字不参与比较
        const QString pinyin = DFMGlobal::toPinyin(QString(ch));
        for (const QChar pyChar : pinyin) {
            if (!pyChar.isDigit())
                key.append(pyChar);
        }
    }

    return key;
}

bool diskItemLessThan(QLayoutItem *lhs, QLayoutItem *rhs)
{
    static const QCollator collator = [] {
        QCollator c;
        c.setNumericMode(true);
        c.setCaseSensitivity(Qt::CaseInsensitive);
        return c;
    }();

    const auto itemOf = [](QLayoutItem *layoutItem) -> ComputerViewItem * {
        QWidget *widget = layoutItem ? layoutItem->widget() : nullptr;
        return widget ? dynamic_cast<ComputerViewItem *>(widget) : nullptr;
    };

    ComputerViewItem *left = itemOf(lhs);
    ComputerViewItem *right = itemOf(rhs);
    if (!left || !right)
        return false;

    const int leftRank = diskItemRank(left);
    const int rightRank = diskItemRank(right);
    if (leftRank != rightRank)
        return leftRank < rightRank;

    // 系统盘、数据盘各自只有一项，保持原顺序
    if (leftRank == SystemDiskRank || leftRank == DataDiskRank)
        return false;

    // 无盘标的盘按容量从大到小
    if (leftRank == UnnamedDiskRank) {
        const qulonglong leftSize = left->deviceInfo()->getDiskInfo().total();
        const qulonglong rightSize = right->deviceInfo()->getDiskInfo().total();
        return leftSize > rightSize;
    }

    const int result = collator.compare(diskItemSortKey(left->name()), diskItemSortKey(right->name()));
    if (result != 0)
        return result < 0;

    return collator.compare(left->name(), right->name()) < 0;
}

} // namespace

TitleLine::TitleLine(const QString &title, QWidget *parent):
    QFrame(parent),
    m_title(title)
{
    initUI();
}

void TitleLine::initUI()
{
    m_titleLable = new QLabel(m_title);
    m_titleLable->setObjectName("TitleLabel");
    m_lineLable = new QFrame();
    m_lineLable->setObjectName("LineLabel");
    m_lineLable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_lineLable->setFixedHeight(1);

    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->addWidget(m_titleLable);
    contentLayout->addSpacing(20);
    contentLayout->addWidget(m_lineLable, 100, Qt::AlignCenter);
    contentLayout->setSpacing(0);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(contentLayout);
}

void TitleLine::resizeEvent(QResizeEvent *event)
{
    m_lineLable->setFixedWidth(event->size().width() - m_titleLable->width() - 20);
}


ComputerViewItem::ComputerViewItem(QWidget *parent):
    FileIconItem(parent)
{
    setAttribute(Qt::WA_Hover, true);
    getIconLabel()->setAttribute(Qt::WA_Hover, false);
    getTextEdit()->setAttribute(Qt::WA_Hover, false);

    getTextEdit()->setReadOnly(true);
    getTextEdit()->setAttribute(Qt::WA_TransparentForMouseEvents);
    getTextEdit()->setTextInteractionFlags(Qt::NoTextInteraction);
    getTextEdit()->setStyleSheet("border:1px solid red");
    getTextEdit()->setObjectName("CVI_TextEdit");
    getTextEdit()->installEventFilter(this);

    m_sizeLabel = new QLabel(this);
    m_sizeLabel->setObjectName("DiskSize");
    m_sizeLabel->hide();

    m_progressLine = new ProgressLine(this);
    m_progressLine->setAlignment(Qt::AlignHCenter);
    m_progressLine->setFrameShape(QFrame::NoFrame);
    m_progressLine->setFixedSize(width(), CVI_PROGRESSLINE_HEIGHT);
    m_progressLine->setRoundRadius(CVI_PROGRESSLINE_HEIGHT / 2);
    m_progressLine->hide();

    connect(qApp, &DApplication::iconThemeChanged, this, &ComputerViewItem::updateStatus);
    connect(this, &ComputerViewItem::inputFocusOut, this, [ = ]{
//        QString newName = getTextEdit()->toPlainText();
        getTextEdit()->setReadOnly(true);
        updateStatus();
    });
}

QIcon ComputerViewItem::getIcon(int size)
{
    if (m_info) {
        return m_info->fileIcon();
    } else if (m_deviceInfo) {
        return m_deviceInfo->fileIcon(size, size);
    }
    return QIcon();
}


DAbstractFileInfoPointer ComputerViewItem::info() const
{
    return m_info;
}

void ComputerViewItem::setInfo(const DAbstractFileInfoPointer &info)
{
    m_info = info;
}


UDiskDeviceInfoPointer ComputerViewItem::deviceInfo() const
{
    return m_deviceInfo;
}

void ComputerViewItem::setDeviceInfo(UDiskDeviceInfoPointer deviceInfo)
{
    m_deviceInfo = deviceInfo;

    QString fstype = PartMan::Partition::getPartitionByDevicePath(m_deviceInfo->getDiskInfo().unix_device()).fs();

    if (fstype == "crypto_LUKS") {
        m_isLocked = true;
    }
}

int ComputerViewItem::windowId()
{
    return WindowManager::getWindowId(this);
}

void ComputerViewItem::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event)
    setChecked(true);
    DFileMenu *menu = nullptr;
    DUrl url;
    QSet<MenuAction> disableList;
    const bool &tabAddable = WindowManager::tabAddableByWinId(windowId());
    if (!tabAddable) {
        disableList << MenuAction::OpenInNewTab << MenuAction::OpenDiskInNewTab;
    }
    if (m_info) {
        menu = DFileMenuManager::createDefaultBookMarkMenu(disableList);
        url = m_info->fileUrl();
    } else if (m_deviceInfo) {
        if (m_deviceInfo->getMountPoint() == "/" && m_deviceInfo->getDiskInfo().isNativeCustom()) {
            menu = DFileMenuManager::createDefaultBookMarkMenu(disableList);
            url =  m_deviceInfo->getMountPointUrl();
        } else if (m_deviceInfo->getDiskInfo().isNativeCustom()) {
            menu = DFileMenuManager::createDefaultBookMarkMenu(disableList);
            url = m_deviceInfo->getMountPointUrl();
        } else {
            disableList |= m_deviceInfo->disableMenuActionList();
            menu = DFileMenuManager::genereteMenuByKeys(
                       m_deviceInfo->menuActionList(DAbstractFileInfo::SingleFile),
                       disableList);
            url = m_deviceInfo->getMountPointUrl();
            url.setQuery(m_deviceInfo->getId());
        }
    }

    if (menu) {
        menu->setEventData(DUrl(), DUrlList() << url, windowId(), this);
        menu->exec();
        menu->deleteLater();
    }
}

void ComputerViewItem::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    setChecked(true);

    if (event->button() == Qt::LeftButton) {
        if (DFMApplication::instance()->appAttribute(DFMApplication::AA_OpenFileMode).toInt() == 0) {
            openUrl();
        }
    }

    FileIconItem::mousePressEvent(event);
}

void ComputerViewItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (DFMApplication::instance()->appAttribute(DFMApplication::AA_OpenFileMode).toInt() == 1) {
            openUrl();
        }
    }

    FileIconItem::mouseDoubleClickEvent(event);
}

bool ComputerViewItem::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (!m_deviceInfo) {
                qWarning() << "device info not valid, can be a bug." << obj;
                break;
            }
            event->accept();
            // do rename here
            {
                DUrl url;
                url.setScheme(DEVICE_SCHEME);
                url.setPath(m_deviceInfo->getId());
                DUrl urlWithNewName = url;
                QUrlQuery query;
                query.addQueryItem("new_name", getTextEdit()->toPlainText());
                urlWithNewName.setQuery(query);
                fileService->renameFile(this, url, urlWithNewName, true);
            }
            // done
            // 重设显示的名字
            setName(m_deviceInfo->fileDisplayName());
            emit inputFocusOut();
            return true;
        case Qt::Key_Escape:
            // 重设显示的名字
            setName(m_deviceInfo->fileDisplayName());
            emit inputFocusOut();
            return true;
        default:
            break;
        }
    }
    return FileIconItem::eventFilter(obj, event);
}

bool ComputerViewItem::event(QEvent *event)
{
    const QEvent::Type type = event->type();

    if (type == QEvent::HoverEnter || type == QEvent::HoverLeave) {
        const bool hovered = (type == QEvent::HoverEnter);

        if (m_hovered != hovered) {
            m_hovered = hovered;
            update();
        }
    }

    if (type == QEvent::Resize) {
        resize(width(), getIconLabel()->height() + getTextEdit()->height() + ICON_MODE_ICON_SPACING + 45);
        adjustPosition();
        return true;
    }
    return FileIconItem::event(event);
}

void ComputerViewItem::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    if (m_checked && getTextEdit()->isReadOnly()) {
        QPainter painter(this);
        DStyledItemDelegate::paintSelectionBox(&painter, QRectF(rect()), 3);
        return;
    }

    if (!m_hovered || m_checked || !getTextEdit()->isReadOnly()) {
        return;
    }

    if (!(DFMApplication::instance()
          && DFMApplication::instance()->genericAttribute(DFMApplication::GA_ShowHoverBox).toBool())) {
        return;
    }

    QPainter painter(this);
    DStyledItemDelegate::paintHoverBox(&painter, QRectF(rect()), 3);
}

void ComputerViewItem::adjustPosition()
{
    m_sizeLabel->setFixedWidth(this->width());
    m_sizeLabel->setAlignment(Qt::AlignCenter);
    m_sizeLabel->move(0, getTextEdit()->y() + getTextEdit()->height());
    m_progressLine->move((this->width() - m_progressLine->width()) / 2, m_sizeLabel->y() + m_sizeLabel->height() + 3);
}

bool ComputerViewItem::checked() const
{
    return m_checked;
}

void ComputerViewItem::setChecked(bool checked)
{
    if (checked != m_checked) {
        m_checked = checked;
        updateStatus();
        update();
        emit checkChanged(checked);
    }
}

void ComputerViewItem::updateStatus()
{
    if (m_checked) {
        setIconSizeState(m_iconSize, QIcon::Normal);
        setDisplayName(m_name);
        if (fontMetrics().horizontalAdvance(m_name) < width()) {
            getTextEdit()->setFixedWidth(fontMetrics().horizontalAdvance(m_name) + 10);
        }
        if (getTextEdit()->isReadOnly()) {
            getTextEdit()->setStyleSheet("background-color: transparent");
        } else {
            getTextEdit()->setStyleSheet("");
        }
    } else {
        setIconSizeState(m_iconSize, QIcon::Normal);
        QString ds = DFMGlobal::elideText(m_name,
                                          QSize(width(), 40),
                                          QTextOption::WrapAtWordBoundaryOrAnywhere,
                                          font(),
                                          Qt::ElideMiddle, TEXT_LINE_HEIGHT);
        getTextEdit()->setStyleSheet("background-color: transparent");
        setDisplayName(ds.remove('\n'));
    }

    if (getHasMemoryInfo()) {
        updateIconPixelWidth();
        m_progressLine->setFixedSize(getPixelWidth(), CVI_PROGRESSLINE_HEIGHT);
        const qlonglong total = m_deviceInfo->getTotal();
        const qlonglong used = total - m_deviceInfo->getFree();
        m_progressLine->setMax(total);
        m_progressLine->setValue(used);
        if (m_progressLine->isHidden()) {
            m_progressLine->show();
        }

        m_sizeLabel->setText(FileUtils::diskUsageString(used, total));

        m_sizeLabel->show();
        adjustPosition();
    } else {
        m_progressLine->setFixedHeight(0);
    }

    if (m_isLocked) {
        if (m_lockedLabel) {
            m_lockedLabel->deleteLater();
        }
        m_lockedLabel = new QLabel(getIconLabel());
        m_lockedLabel->setPixmap(QIcon::fromTheme("emblem-encrypted-locked").pixmap(m_iconSize / 3, m_iconSize / 3));
        m_lockedLabel->move(m_iconSize * 2 / 3, m_iconSize * 2 / 3);
        m_lockedLabel->raise();
        m_lockedLabel->show();
    }

    if (!getTextEdit()->isReadOnly()) {
        // Editing name
        this->setBorderColor(QColor("#2da6f7"));
        getTextEdit()->setFixedWidth(width());
        getTextEdit()->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        getTextEdit()->setFocus();
    } else {
        this->setBorderColor(QColor("#002da6f7")); // argb
        getTextEdit()->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
}

int ComputerViewItem::iconSize() const
{
    return m_iconSize;
}

void ComputerViewItem::setIconSize(int size)
{
    m_iconSize = size;
}

void ComputerViewItem::setIconIndex(int index)
{
    m_iconIndex = index;
}

void ComputerViewItem::setIconSizeState(int iconSize, QIcon::Mode mode)
{
    getIconLabel()->setFixedSize(QSize(iconSize, iconSize));
    if (m_info) {
        getIconLabel()->setPixmap(getIcon(iconSize).pixmap(iconSize, iconSize, mode));
    } else if (m_deviceInfo) {
        getIconLabel()->setPixmap(getIcon(iconSize).pixmap(iconSize, iconSize, mode));
    }
}

void ComputerViewItem::openUrl()
{
    DUrl url;

    if (m_info) {
        url = m_info->fileUrl();
    } else if (m_deviceInfo) {
        url = m_deviceInfo->getMountPointUrl();

        QDiskInfo diskInfo = m_deviceInfo->getDiskInfo();

        if (diskInfo.can_mount() && !diskInfo.can_unmount()) {
            url.setQuery(m_deviceInfo->getId());
            appController->actionOpenDisk(dMakeEventPointer<DFMUrlBaseEvent>(this, url));
            return;
        }
    }

    DFMOpenUrlEvent::DirOpenMode mode = DFMApplication::instance()->appAttribute(DFMApplication::AA_AllwayOpenOnNewWindow).toBool()
                                        ? DFMOpenUrlEvent::ForceOpenNewWindow
                                        : DFMOpenUrlEvent::OpenInCurrentWindow;

    if (mode == DFMOpenUrlEvent::OpenInCurrentWindow)
        DFMEventDispatcher::instance()->processEventAsync<DFMOpenUrlEvent>(this, DUrlList() << url, mode);
    else
        DFMEventDispatcher::instance()->processEvent<DFMOpenUrlEvent>(this, DUrlList() << url, mode);
}

int ComputerViewItem::getPixelWidth() const
{
    return m_pixelWidth;
}

void ComputerViewItem::setPixelWidth(int pixelWidth)
{
    m_pixelWidth = pixelWidth;
}

void ComputerViewItem::updateIconPixelWidth()
{
    const QImage img = getIconLabel()->pixmap().toImage();
    int pixelWidth = 0;
    for (int i = 0; i < img.width(); i++) {
        QColor color = img.pixelColor(i, (int)img.height() / 2);
        if (color.alpha() > 0) {
            pixelWidth ++;
        }
    }
    setPixelWidth(pixelWidth);
}

DUrl ComputerViewItem::getUrl() const
{
    if (m_info) {
        return m_info->fileUrl();
    } else if (m_deviceInfo) {
        QDiskInfo diskInfo = m_deviceInfo->getDiskInfo();

        if (diskInfo.can_mount() && !diskInfo.can_unmount()) {
            DUrl url("mount:");
            QUrlQuery query(url);

            query.addQueryItem("id", m_deviceInfo->getId());
            url.setQuery(query);

            return url;
        } else {
            return m_deviceInfo->getMountPointUrl();
        }
    }

    return DUrl();
}

bool ComputerViewItem::getHasMemoryInfo() const
{
    return m_hasMemoryInfo;
}

void ComputerViewItem::setHasMemoryInfo(bool hasMemoryInfo)
{
    m_hasMemoryInfo = hasMemoryInfo;
}
QString ComputerViewItem::name() const
{
    return m_name;
}

void ComputerViewItem::setName(const QString &name)
{
    m_name = name;
}

QString ComputerViewItem::displayName() const
{
    return m_displayName;
}

void ComputerViewItem::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
    getTextEdit()->setPlainText(m_displayName);
}


int ComputerView::ViewInstanceCount = -1;

ComputerView::ComputerView(QWidget *parent) : QFrame(parent)
{
    initData();
    initUI();
    initConnect();
}

ComputerView::~ComputerView()
{
}

QString ComputerView::scheme()
{
    return COMPUTER_SCHEME;
}

QString ComputerView::viewId() const
{
    return m_viewId;
}

void ComputerView::initData()
{
    m_iconSizes << 48 << 64 << 96 << 128 << 256;
    m_systemPathKeys << "Desktop"
                     << "Videos"
                     << "Music"
                     << "Pictures"
                     << "Documents"
                     << "Downloads";
    ViewInstanceCount += 1;
    m_viewId = QString("ComputerView%1").arg(QString::number(ViewInstanceCount));

}

void ComputerView::initUI()
{
    m_contentArea = new DScrollArea(this);
    m_contentArea->setObjectName("ComputerView");
    m_contentArea->setWidgetResizable(true);

    // 支持触屏滚动，直接使用TouchGesture会导致鼠标双击事件丢失
    QScroller::grabGesture(m_contentArea, QScroller::LeftMouseButtonGesture);
    QScroller *scroller = QScroller::scroller(m_contentArea);
    auto scroller_properties = scroller->scrollerProperties();
#if QT_VERSION < QT_VERSION_CHECK(5,9,0)
    int touchTapDistance = 15;
#else
    int touchTapDistance = QGuiApplication::styleHints()->touchDoubleTapDistance();
#endif
    scroller_properties.setScrollMetric(QScrollerProperties::DragStartDistance, touchTapDistance / 1000);
    scroller->setScrollerProperties(scroller_properties);

    m_statusBar = new DStatusBar(this);
    m_statusBar->setFixedHeight(22);
    m_statusBar->scalingSlider()->setMaximum(m_iconSizes.count() - 1);
    m_statusBar->scalingSlider()->setMinimum(0);
    setIconSizeBySizeIndex(m_currentIconSizeIndex);
    m_statusBar->scalingSlider()->setTickInterval(1);
    m_statusBar->scalingSlider()->setPageStep(1);

    QFrame *contentFrame = new QFrame(this);

    contentFrame->setObjectName("ComputerViewFrame");

    m_systemTitleLine = new TitleLine(tr("My Directories"), this);
    m_systemFlowLayout = new DFlowLayout();
    m_systemFlowLayout->setContentsMargins(20, 20, 20, 20);
    m_systemFlowLayout->setHorizontalSpacing(40);
    m_systemFlowLayout->setVerticalSpacing(40);

    m_nativeTitleLine = new TitleLine(tr("Internal Disk"), this);
    m_nativeFlowLayout = new DFlowLayout();
    m_nativeFlowLayout->setContentsMargins(20, 20, 20, 20);
    m_nativeFlowLayout->setHorizontalSpacing(40);
    m_nativeFlowLayout->setVerticalSpacing(50);

    m_removableTitleLine = new TitleLine(tr("External Disk"), this);
    m_removableFlowLayout = new DFlowLayout();
    m_removableFlowLayout->setContentsMargins(20, 20, 20, 20);
    m_removableFlowLayout->setHorizontalSpacing(40);
    m_removableFlowLayout->setVerticalSpacing(50);

    QVBoxLayout *contentLayout = new QVBoxLayout;
    bool disabledMyDirectories = DFMApplication::genericAttribute(DFMApplication::GA_HiddenMyDirectories).toBool();
    if (!disabledMyDirectories) {
        contentLayout->addWidget(m_systemTitleLine);
        contentLayout->addLayout(m_systemFlowLayout);
        contentLayout->addSpacing(20);
    }
    m_systemTitleLine->setVisible(!disabledMyDirectories);
    contentLayout->addWidget(m_nativeTitleLine);
    contentLayout->addLayout(m_nativeFlowLayout);
    contentLayout->addSpacing(20);
    contentLayout->addWidget(m_removableTitleLine);
    contentLayout->addLayout(m_removableFlowLayout);
    contentLayout->addSpacing(20);
    contentLayout->addStretch();
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentFrame->setLayout(contentLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_contentArea);
    mainLayout->addWidget(m_statusBar);
    setLayout(mainLayout);

    m_contentArea->setWidget(contentFrame);

    loadSystemItems();
    loadCustomItems();
    loadNativeItems();

    if (m_nativeItems.count() == 0) {
        m_nativeTitleLine->hide();
    }

    if (m_removableItems.count() == 0) {
        m_removableTitleLine->hide();
    }

    DFMEvent event(this);
    event.setWindowId(window()->internalWinId());
    const int number = m_systemItems.count() + m_nativeItems.count() + m_removableItems.count();
    m_statusBar->itemCounted(event, number);

    //icon size
    resizeAllItemsBySizeIndex(DFMApplication::instance()->appAttribute(DFMApplication::AA_IconSizeLevel).toInt());
    loadViewState();
}

void ComputerView::initConnect()
{
    DAbstractFileWatcher *devices_watcher = fileService->createFileWatcher(this, DUrl(DEVICE_ROOT), this);
    Q_CHECK_PTR(devices_watcher);
    devices_watcher->startWatcher();

    // Patition get renamed.
    connect(devices_watcher, &DAbstractFileWatcher::fileMoved, this, [ = ](const DUrl & fromUrl, const DUrl & toUrl) {
        Q_UNUSED(toUrl)
        const DAbstractFileInfoPointer &deviceInfoPointer = DFileService::instance()->createFileInfo(nullptr, fromUrl);
        const UDiskDeviceInfo* info = dynamic_cast<UDiskDeviceInfo*>(deviceInfoPointer.data());
        if (!deviceInfoPointer) {
            return;
        }

        DUrl url;
        if (info->getDiskInfo().isNativeCustom()) {
            url = info->getMountPointUrl();
        } else {
            url = info->getMountPointUrl();
            url.setQuery(info->getId());
        }

        ComputerViewItem *cvi = findDeviceViewItemByUrl(url);

        if (!cvi) {
            return;
        }

        QUrlQuery query(toUrl);
        QString newName = query.queryItemValue("new_name");
        if (!newName.isEmpty()) {
            cvi->setName(newName);
        } else {
            cvi->setName(deviceInfoPointer->fileDisplayName());
        }
        cvi->updateStatus();
        // 重命名后名称变化，需要重新排序
        sortDiskItems();
    });

    connect(devices_watcher, &DAbstractFileWatcher::fileAttributeChanged, this, [ = ](const DUrl & url) {
        ComputerViewItem *cvi = findDeviceViewItemByUrl(url);

        if (!cvi) {
            return;
        }

        cvi->updateStatus();
    });

    connect(deviceListener, &UDiskListener::mountAdded, this, &ComputerView::mountAdded);
    connect(deviceListener, &UDiskListener::mountRemoved, this, &ComputerView::mountRemoved);
    connect(deviceListener, &UDiskListener::volumeAdded, this, &ComputerView::volumeAdded);
    connect(deviceListener, &UDiskListener::volumeRemoved, this, &ComputerView::volumeRemoved);
    connect(m_statusBar->scalingSlider(), &DSlider::valueChanged, this, &ComputerView::resizeAllItemsBySizeIndex);
    connect(m_statusBar->scalingSlider(), &DSlider::valueChanged, this, &ComputerView::saveViewState);
    connect(DFMApplication::instance(), &DFMApplication::iconSizeLevelChanged, this, &ComputerView::resizeAllItemsBySizeIndex);
    connect(DFMApplication::instance(), &DFMApplication::genericAttributeChanged, this,
            [this](DFMApplication::GenericAttribute ga, const QVariant &) {
        if (ga != DFMApplication::GA_ShowHoverBox) {
            return;
        }

        const auto updateItems = [](const QMap<QString, ComputerViewItem *> &items) {
            for (ComputerViewItem *item : items) {
                item->update();
            }
        };

        updateItems(m_systemItems);
        updateItems(m_nativeItems);
        updateItems(m_removableItems);
    });
    connect(fileSignalManager, &FileSignalManager::requestRename, this, &ComputerView::onRequestEdit);

}

void ComputerView::loadSystemItems()
{
    foreach (QString key, m_systemPathKeys) {
        QString path = systemPathManager->getSystemPath(key);
        DUrl url = DUrl::fromLocalFile(path);
        const DAbstractFileInfoPointer &fileInfo = DFileService::instance()->createFileInfo(this, url);
        ComputerViewItem *item = new ComputerViewItem;
        item->setInfo(fileInfo);
        item->setName(fileInfo->fileDisplayName());
        m_systemFlowLayout->addWidget(item);
        m_systemItems.insert(path, item);
    }
}

void ComputerView::loadNativeItems()
{
    QStorageInfo storageInfo("/");
    QDiskInfo diskInfo;
    diskInfo.setId("/");
    diskInfo.setType("native");
    diskInfo.setName(systemPathManager->getSystemPathDisplayName("System Disk"));
    diskInfo.setMounted_root_uri("/");
    diskInfo.setCan_mount(false);
    diskInfo.setCan_unmount(false);
    diskInfo.setIsNativeCustom(true);
    diskInfo.setTotal(storageInfo.bytesTotal());
    diskInfo.setFree(storageInfo.bytesFree());
    diskInfo.setUsed(diskInfo.total() - diskInfo.free());
//    diskInfo.setMounted_url(DUrl::fromLocalFile("/"));

    UDiskDeviceInfo *deviceInfo = new UDiskDeviceInfo;
    deviceInfo->setDiskInfo(diskInfo);
    UDiskDeviceInfoPointer device(deviceInfo);

    if (DFMApplication::instance()->genericAttribute(DFMApplication::GA_HiddenSystemPartition).toBool()) {
        qDebug() << "hide root sytem";
    } else {
        volumeAdded(device);
    }

    foreach (UDiskDeviceInfoPointer device, deviceListener->getDeviceList()) {
        volumeAdded(device);
    }
}

void ComputerView::loadCustomItems()
{
    const QStringList &keys = DFMApplication::genericObtuselySetting()->keyList("Disk/Custom");

    for (const QString &key : keys) {
        loadCustomItemsByNameUrl(key, DFMApplication::genericObtuselySetting()->urlValue("Disk/Custom", key).toString());
    }
}

void ComputerView::loadCustomItemsByNameUrl(const QString &id, const QString &url)
{
    QDiskInfo diskInfo;
    diskInfo.setId(id);
    diskInfo.setType("native");
    diskInfo.setName(id);
    diskInfo.setCan_mount(false);
    diskInfo.setCan_unmount(false);
    diskInfo.setMounted_root_uri(url);
//    diskInfo.setMounted_url(DUrl::fromLocalFile(url));
    diskInfo.setIsNativeCustom(true);

    UDiskDeviceInfo *deviceInfo = new UDiskDeviceInfo;
    deviceInfo->setDiskInfo(diskInfo);
    UDiskDeviceInfoPointer device(deviceInfo);
    volumeAdded(device);
}

void ComputerView::updateStatusBar()
{
    ComputerViewItem *checkedItem = NULL;
    foreach (ComputerViewItem *item, m_systemItems) {
        if (item->checked()) {
            checkedItem = item;
        }
    }
    foreach (ComputerViewItem *item, m_nativeItems) {
        if (item->checked()) {
            checkedItem = item;
        }
    }
    foreach (ComputerViewItem *item, m_removableItems) {
        if (item->checked()) {
            checkedItem = item;
        }
    }

    if (checkedItem) {
        DFMEvent event(this);
        DUrlList urlList;
        if (checkedItem->info()) {
            urlList << checkedItem->info()->fileUrl();
        }
        event.setWindowId(window()->internalWinId());
        event.setData(urlList);
        m_statusBar->itemSelected(event, 1);
    } else {
        DFMEvent event(this);
        event.setWindowId(window()->internalWinId());
        const int number = m_systemItems.count() + m_nativeItems.count() + m_removableItems.count();
        m_statusBar->itemCounted(event, number);
    }
}

void ComputerView::loadViewState()
{
    const QVariantMap &value = DFMApplication::appObtuselySetting()->value("FileViewState", COMPUTER_ROOT).toMap();

    if (!value.contains("iconSizeLevel")) {
        return;
    }

    resizeAllItemsBySizeIndex(value.value("iconSizeLevel").toInt());
}

void ComputerView::saveViewState()
{
    m_currentIconSizeIndex = m_statusBar->scalingSlider()->value();

    DFMApplication::appObtuselySetting()->setValue("FileViewState", COMPUTER_ROOT, QVariantMap {
                                                       {"iconSizeLevel", m_currentIconSizeIndex}
                                                   });
}

ComputerViewItem *ComputerView::findDeviceViewItemByUrl(const DUrl &url)
{    
    auto getContextMenuUrl = [](ComputerViewItem *oneItem) {
        DUrl url;
        UDiskDeviceInfoPointer m_deviceInfo = oneItem->deviceInfo();
        if (m_deviceInfo->getDiskInfo().isNativeCustom()) {
            url = m_deviceInfo->getMountPointUrl();
        } else {
            url = m_deviceInfo->getMountPointUrl();
            url.setQuery(m_deviceInfo->getId());
        }
        return url;
    };

    auto getDeviceId = [](ComputerViewItem *oneItem) {
        UDiskDeviceInfoPointer m_deviceInfo = oneItem->deviceInfo();
        return m_deviceInfo->getId(); // something like "/dev/sdb1"
    };

    // Find the item we want to rename in the `Internam devices` section
    foreach (ComputerViewItem *oneItem, m_nativeItems) {
        qDebug() << oneItem->getUrl() << url;
        if (getContextMenuUrl(oneItem) == url) {
            return oneItem;
        }
    }

    // not found? try `removeable devices` section
    foreach (ComputerViewItem *oneItem, m_removableItems) {
        if (getContextMenuUrl(oneItem) == url) {
            return oneItem;
        }

        if (url.scheme() == DEVICE_SCHEME && getDeviceId(oneItem) == url.path()) {
            return oneItem;
        }
    }

    // return if still not found
    return nullptr;
}

QWidget *ComputerView::widget() const
{
    return const_cast<ComputerView *>(this);
}

DUrl ComputerView::rootUrl() const
{
    return DUrl::fromComputerFile("/");
}

bool ComputerView::setRootUrl(const DUrl &url)
{
    return DUrl::fromComputerFile("/") == url;
}

void ComputerView::sortDiskItems()
{
    const auto sortLayout = [](DFlowLayout *layout) {
        if (!layout || layout->count() < 2) {
            return;
        }

        QList<QLayoutItem *> items;
        while (QLayoutItem *layoutItem = layout->takeAt(0)) {
            items.append(layoutItem);
        }

        std::stable_sort(items.begin(), items.end(), diskItemLessThan);

        for (QLayoutItem *layoutItem : items) {
            layout->addItem(layoutItem);
        }
    };

    // 内置盘与外置盘各自排序：系统盘、数据盘、有盘标盘（字母序）、无盘标盘（容量降序）
    sortLayout(m_nativeFlowLayout);
    sortLayout(m_removableFlowLayout);

    const auto logOrder = [](const char *group, DFlowLayout *layout) {
        if (!layout) {
            return;
        }
        QStringList names;
        for (int i = 0; i < layout->count(); ++i) {
            QLayoutItem *layoutItem = layout->itemAt(i);
            QWidget *widget = layoutItem ? layoutItem->widget() : nullptr;
            ComputerViewItem *item = widget ? dynamic_cast<ComputerViewItem *>(widget) : nullptr;
            if (item) {
                names.append(item->name());
            }
        }
        qDebug() << "computer view disk order" << group << names;
    };
    logOrder("internal", m_nativeFlowLayout);
    logOrder("external", m_removableFlowLayout);
}

void ComputerView::volumeAdded(UDiskDeviceInfoPointer device)
{
    qDebug() << "===========volumeAdded=============" << device->getId() << m_nativeItems.contains(device->getId()) << m_removableItems.contains(device->getId());
    qDebug() << device->getDiskInfo();
    const QString id = device->getId();
    const bool isNative = device->getMediaType() == UDiskDeviceInfo::native;
    DFlowLayout *layout = isNative ? m_nativeFlowLayout : m_removableFlowLayout;
    DFlowLayout *otherLayout = isNative ? m_removableFlowLayout : m_nativeFlowLayout;
    QMap<QString, ComputerViewItem *> &items = isNative ? m_nativeItems : m_removableItems;
    QMap<QString, ComputerViewItem *> &otherItems = isNative ? m_removableItems : m_nativeItems;

    const bool alreadyAdded = items.contains(id);
    ComputerViewItem *item = items.value(id);
    if (!item) {
        // 设备类型变化时，把图标从另一个分组迁移过来
        item = otherItems.take(id);
        if (item) {
            otherLayout->removeWidget(item);
        }
    }
    if (!item) {
        item = new ComputerViewItem;
    }
    item->setHasMemoryInfo(true);
    item->setDeviceInfo(device);
    item->setName(device->fileDisplayName());

    if (!alreadyAdded) {
        layout->addWidget(item);
        items.insert(id, item);
    }

    if (isNative) {
        if (m_nativeItems.count() > 0) {
            m_nativeTitleLine->show();
        }
    } else {
        if (m_removableItems.count() > 0) {
            m_removableTitleLine->show();
        }
    }
    updateItemBySizeIndex(m_currentIconSizeIndex, item);

    sortDiskItems();
    updateStatusBar();
}

void ComputerView::volumeRemoved(UDiskDeviceInfoPointer device)
{
    QString deviceId = device->getId();
    QString deviceUuid = device->getDiskInfo().uuid();

    qDebug() << "===========volumeRemoved=============" << deviceId << m_nativeItems.contains(deviceId) << m_removableItems.contains(deviceId);
//    qDebug() << device->getDiskInfo();

    foreach (UDiskDeviceInfoPointer d, deviceListener->getDeviceList()) {
//        qDebug() << d->getDiskInfo().id() << deviceId << d->getDiskInfo().uuid() << deviceUuid;
        if (d->getDiskInfo().id() != deviceId && !deviceUuid.isEmpty() && d->getDiskInfo().uuid() == deviceUuid) {
            deviceId = d->getDiskInfo().id();
        }
    }


    if (m_nativeItems.contains(deviceId)) {
        ComputerViewItem *item = m_nativeItems.value(deviceId);
        m_nativeFlowLayout->removeWidget(item);
        m_nativeItems.remove(deviceId);
        item->setParent(NULL);
        delete item;
        if (m_nativeItems.count() == 0) {
            m_nativeTitleLine->hide();
        }
    } else if (m_removableItems.contains(deviceId)) {
        ComputerViewItem *item = m_removableItems.value(deviceId);
        m_removableFlowLayout->removeWidget(item);
        m_removableItems.remove(deviceId);
        item->setParent(NULL);
        delete item;
        if (m_removableItems.count() == 0) {
            m_removableTitleLine->hide();
        }
    }

    updateStatusBar();
}

void ComputerView::mountAdded(UDiskDeviceInfoPointer device)
{
    qDebug() << "===========mountAdded=============" << device->getId() << m_nativeItems.contains(device->getId()) << m_removableItems.contains(device->getId());
    if (m_nativeItems.contains(device->getId())) {
        m_nativeItems.value(device->getId())->setDeviceInfo(device);
        m_nativeItems.value(device->getId())->updateStatus();
        return;
    } else if (m_removableItems.contains(device->getId())) {
        m_removableItems.value(device->getId())->setDeviceInfo(device);
        return;
    } else {
        volumeAdded(device);
        return;
    }
    updateStatusBar();
}

void ComputerView::mountRemoved(UDiskDeviceInfoPointer device)
{
    qDebug() << "===========mountRemoved=============" << device->getId() << m_nativeItems.contains(device->getId()) << m_removableItems.contains(device->getId());
    qDebug() << device->getDiskInfo();

    if (m_nativeItems.contains(device->getId())) {
        m_nativeItems.value(device->getId())->setDeviceInfo(device);
        return;
    } else if (m_removableItems.contains(device->getId())) {

        if (device->getDiskInfo().has_volume()) {
            m_removableItems.value(device->getId())->setDeviceInfo(device);
        } else {
            ComputerViewItem *item = m_removableItems.value(device->getId());
            m_removableFlowLayout->removeWidget(item);
            m_removableItems.remove(device->getId());
            item->setParent(NULL);
            delete item;
            if (m_removableItems.count() == 0) {
                m_removableTitleLine->hide();
            }
        }
    }
    updateStatusBar();
}

void ComputerView::updateComputerItemByDevice(UDiskDeviceInfoPointer device)
{
    foreach (ComputerViewItem *item, m_nativeItems) {
        if (item->deviceInfo() == device) {
            item->updateStatus();
            return;
        }
    }
    foreach (ComputerViewItem *item, m_removableItems) {
        if (item->deviceInfo() == device) {
            item->updateStatus();
            return;
        }
    }
}

void ComputerView::enlargeIcon()
{
    if (m_currentIconSizeIndex < m_iconSizes.count() - 1) {
        ++m_currentIconSizeIndex;
    }
    resizeAllItemsBySizeIndex(m_currentIconSizeIndex);
}

void ComputerView::shrinkIcon()
{
    if (m_currentIconSizeIndex > 0) {
        --m_currentIconSizeIndex;
    }
    resizeAllItemsBySizeIndex(m_currentIconSizeIndex);
}

void ComputerView::resizeAllItemsBySizeIndex(int index)
{
    for (ComputerViewItem *item : m_systemItems) {
        updateItemBySizeIndex(index, item);
    }
    for (ComputerViewItem *item : m_nativeItems) {
        updateItemBySizeIndex(index, item);
    }
    for (ComputerViewItem *item : m_removableItems) {
        updateItemBySizeIndex(index, item);
    }

    m_currentIconSizeIndex = index;
    setIconSizeBySizeIndex(index);
}

void ComputerView::updateItemBySizeIndex(const int &index, ComputerViewItem *item)
{
    int size = m_iconSizes.at(index);
    item->setFixedWidth(size + 30);
    item->setIconIndex(index);
    item->setIconSize(size);
    item->updateStatus();
}

void ComputerView::onRequestEdit(const DFMUrlBaseEvent &event)
{
    // find item and toggle rename
    DUrl url = event.url();
    ComputerViewItem *item = findDeviceViewItemByUrl(url);

    // return if still not found
    if (!item) {
        return;
    }

    if (event.windowId() != item->windowId()) {
        return;
    }

    item->getTextEdit()->setReadOnly(false);
    item->setName(item->deviceInfo()->fileName());
    item->updateStatus();
    item->getTextEdit()->selectAll();
}

void ComputerView::resizeEvent(QResizeEvent *event)
{
    m_contentArea->widget()->setFixedWidth(event->size().width() - 16);
    QFrame::resizeEvent(event);
}

void ComputerView::wheelEvent(QWheelEvent *event)
{
    if (DFMGlobal::keyCtrlIsPressed()) {
        if (event->angleDelta().y() > 0) {
            enlargeIcon();
        } else {
            shrinkIcon();
        }
    }
    QFrame::wheelEvent(event);
}

void ComputerView::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = m_contentArea->widget()->mapFromParent(event->pos());

    foreach (ComputerViewItem *item, m_systemItems) {

        if (item->geometry().contains(pos)) {
            item->setChecked(true);
        } else {
            item->setChecked(false);
        }
    }
    foreach (ComputerViewItem *item, m_nativeItems) {

        if (item->geometry().contains(pos)) {
            item->setChecked(true);
        } else {
            item->setChecked(false);
        }
    }
    foreach (ComputerViewItem *item, m_removableItems) {

        if (item->geometry().contains(pos)) {
            item->setChecked(true);
        } else {
            item->setChecked(false);
        }
    }

    updateStatusBar();
}

void ComputerView::showEvent(QShowEvent *event)
{
//    deviceListener->refreshAsycGetAllDeviceUsage();

    foreach (ComputerViewItem *item, m_systemItems) {
        item->setChecked(false);
    }
    foreach (ComputerViewItem *item, m_nativeItems) {
        item->setChecked(false);
    }
    foreach (ComputerViewItem *item, m_removableItems) {
        item->setChecked(false);
    }

    setFocus();

    QFrame::showEvent(event);
}

void ComputerView::keyPressEvent(QKeyEvent *event)
{
    DUrlList urls;

    foreach (const ComputerViewItem *item, m_systemItems) {
        if (item->checked()) {
            urls << item->getUrl();
        }
    }
    foreach (const ComputerViewItem *item, m_nativeItems) {
        if (item->checked()) {
            urls << item->getUrl();
        }
    }
    foreach (const ComputerViewItem *item, m_removableItems) {
        if (item->checked()) {
            urls << item->getUrl();
        }
    }

    switch (event->modifiers()) {
    case Qt::NoModifier:
    case Qt::KeypadModifier:
        switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            appController->actionOpen(dMakeEventPointer<DFMUrlListBaseEvent>(this, urls));

            return;
        default: break;
        }

        break;
    case Qt::ControlModifier:
        switch (event->key()) {
        case Qt::Key_N:
            appController->actionNewWindow(dMakeEventPointer<DFMUrlListBaseEvent>(this, urls.isEmpty() ? DUrlList() << DUrl() : urls));
            return;
        case Qt::Key_I:
            appController->actionProperty(dMakeEventPointer<DFMUrlListBaseEvent>(this, urls));

            return;
        case Qt::Key_Down:
            appController->actionOpen(dMakeEventPointer<DFMUrlListBaseEvent>(this, urls));

            return;
        case Qt::Key_T: {
            //do not handle key press event of autoRepeat type
            if (event->isAutoRepeat()) {
                return;
            }

            DUrl url;

            if (urls.count() == 1) {
                url = urls.first();
            } else {
                url = DFMApplication::instance()->appUrlAttribute(DFMApplication::AA_UrlOfNewTab);

                if (!url.isValid()) {
                    url = rootUrl();
                }
            }
            DFMEventDispatcher::instance()->processEvent<DFMOpenNewTabEvent>(this, url);
            return;
        }
        default: break;
        }

        break;
    case Qt::ShiftModifier:
        if (event->key() == Qt::Key_T) {
            appController->actionOpenInTerminal(dMakeEventPointer<DFMUrlListBaseEvent>(this, urls));

            return;
        }

        break;
    case Qt::AltModifier:
    case Qt::AltModifier | Qt::KeypadModifier:
        switch (event->key()) {
        case Qt::Key_Home:
            urls.clear();
            urls << DUrl::fromLocalFile(QDir::homePath());
            // fall through
        case Qt::Key_Down:
            appController->actionOpen(dMakeEventPointer<DFMUrlListBaseEvent>(this, urls));
            return;
        }
        break;

    default: break;
    }

    QFrame::keyPressEvent(event);
}

void ComputerView::mouseMoveEvent(QMouseEvent *event)
{
    // 防止在使用触屏滚动视图的同时导致窗口移动
    event->accept();

    return;
}

void ComputerView::setIconSizeBySizeIndex(int index)
{
    QSignalBlocker blocker(m_statusBar->scalingSlider());
    Q_UNUSED(blocker)
    m_statusBar->scalingSlider()->setValue(index);
}

DScrollArea::DScrollArea(QWidget *parent):
    QScrollArea(parent)
{

}

void DScrollArea::wheelEvent(QWheelEvent *event)
{
    if (DFMGlobal::keyCtrlIsPressed()) {
        return;
    }
    QScrollArea::wheelEvent(event);
}
