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

#include "desktopitemdelegate.h"

#include <QAbstractItemView>
#include <dfileviewhelper.h>

#include "private/dstyleditemdelegate_p.h"
#include "canvasgridview.h"
#include "canvasviewhelper.h"

DesktopItemDelegate::DesktopItemDelegate(DFileViewHelper *parent) :
    DIconItemDelegate(parent)
{
    iconSizes << 32 << 48 << 64 << 96 << 128;
    iconSizeDescriptions << tr("Tiny")
                         << tr("Small")
                         << tr("Medium")
                         << tr("Large")
                         << tr("Super large");
}

DesktopItemDelegate::~DesktopItemDelegate()
{

}

QWidget *DesktopItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    auto widget = DIconItemDelegate::createEditor(parent, opt, index);
    auto helper = qobject_cast<CanvasViewHelper *>(this->parent());
    auto itemSize = sizeHint(QStyleOptionViewItem(), QModelIndex());
    auto cellSize = helper->parent()->cellSize();
    int offset = -1 * ((cellSize.width() - itemSize.width()) % 2);
    widget->setContentsMargins(offset, helper->parent()->cellMargins().top(), 0, 0);
    return widget;
}

void DesktopItemDelegate::updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const {
    const bool isExpandedItem = editor == expandedIndexWidget();
    if (isExpandedItem) {
        editor->setMinimumHeight(0);
        editor->setMaximumHeight(QWIDGETSIZE_MAX);
    }

    DIconItemDelegate::updateEditorGeometry(editor, option, index);

    if (!isExpandedItem) {
        return;
    }

    auto helper = qobject_cast<CanvasViewHelper *>(parent());
    CanvasGridView *view = helper ? helper->parent() : nullptr;
    if (!view) {
        return;
    }

    const QMargins cellMargins = view->cellMargins();
    setExpandedItemSelectionHighlight(
        true, QMargins(cellMargins.left(), cellMargins.top(),
            cellMargins.right(), 0));

    const int fullHeight = editor->heightForWidth(editor->width());
    const int normalHeight = cellMargins.top()
        + sizeHint(QStyleOptionViewItem(), QModelIndex()).height();
    const int desiredHeight = qMax(normalHeight, fullHeight);

    const QRect iconArea = view->iconAreaRect();
    const int availableHeight = qMax(0, iconArea.bottom() - editor->y() + 1);
    editor->setFixedHeight(qMin(desiredHeight, availableHeight));
    view->viewport()->update(editor->geometry());
}

QString DesktopItemDelegate::iconSizeLevelDescription(int i) const
{
    return iconSizeDescriptions.at(i);
}

int DesktopItemDelegate::iconSizeLevel() const
{
    return currentIconSizeIndex;
}

int DesktopItemDelegate::minimumIconSizeLevel() const
{
    return 0;
}

int DesktopItemDelegate::maximumIconSizeLevel() const
{
    return iconSizes.count() - 1;
}

int DesktopItemDelegate::increaseIcon()
{
    return setIconSizeByIconSizeLevel(currentIconSizeIndex + 1);

}

int DesktopItemDelegate::decreaseIcon()
{
    return setIconSizeByIconSizeLevel(currentIconSizeIndex - 1);
}

int DesktopItemDelegate::setIconSizeByIconSizeLevel(int level)
{
    if (level == currentIconSizeIndex) {
        return level;
    }

    if (level >= minimumIconSizeLevel() && level <= maximumIconSizeLevel()) {
        currentIconSizeIndex = level;

        parent()->parent()->setIconSize(iconSizeByIconSizeLevel());

        return currentIconSizeIndex;
    }

    return -1;
}

QSize DesktopItemDelegate::iconSizeByIconSizeLevel() const
{
    int size = iconSizes.at(currentIconSizeIndex);
    return QSize(size, size);
}

void DesktopItemDelegate::updateItemSizeHint()
{
    DIconItemDelegate::updateItemSizeHint();
    int width = parent()->parent()->iconSize().width() * 17 / 10;
    int height = parent()->parent()->iconSize().height()
                 + 10 + 2 * d_ptr->textLineHeight;

    d_ptr->itemSizeHint = QSize(width, height);
}
