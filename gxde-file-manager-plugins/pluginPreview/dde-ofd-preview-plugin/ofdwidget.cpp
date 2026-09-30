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
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * OFD preview widget backed by the rofd FFI (librofd-ffi) and Cairo.
 */

#include "ofdwidget.h"

#include <QImage>
#include <QHBoxLayout>
#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QtMath>
#include <QUrl>
#include <QLabel>
#include <QListWidgetItem>
#include <QThread>
#include <QtConcurrent>
#include <QScrollBar>
#include <QResizeEvent>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QTimer>
#include <QButtonGroup>
#include <QPushButton>

class OfdWidgetPrivate{
public:
    OfdWidgetPrivate(OfdWidget* qq):
        q_ptr(qq){}

    DListWidget* thumbListWidget = NULL;
    DListWidget* pageListWidget = NULL;
    QHBoxLayout* mainLayout = NULL;
    QScrollBar* thumbScrollBar = NULL;
    QScrollBar* pageScrollBar = NULL;
    QButtonGroup* thumbButtonGroup = NULL;

    QTimer* pageWorkTimer = NULL;
    QTimer* thumbWorkTimer = NULL;
    bool isBadDoc = false;

    rofd_document_t* doc = NULL;
    rofd_renderer_t* renderer = NULL;
    int pageCount = 0;

    OfdInitWorker* ofdInitWorker = NULL;
    QMap<int, QImage> pageMap;

    OfdWidget* q_ptr = NULL;
    Q_DECLARE_PUBLIC(OfdWidget)
};

OfdWidget::OfdWidget(const QString &file, QWidget *parent) :
    QWidget(parent),
    d_ptr(new OfdWidgetPrivate(this))
{
    Q_D(OfdWidget);

    d->pageWorkTimer = new QTimer(this);;
    d->pageWorkTimer->setSingleShot(true);
    d->pageWorkTimer->setInterval(50);
    d->thumbWorkTimer = new QTimer(this);
    d->thumbWorkTimer->setSingleShot(true);
    d->thumbWorkTimer->setInterval(100);

    d->thumbButtonGroup = new QButtonGroup(this);


    initDoc(file);
    initUI();

    if(d->isBadDoc){
        return;
    }

    initConnections();
}

OfdWidget::~OfdWidget()
{
    Q_D(OfdWidget);

    if (d->ofdInitWorker) {
        disconnect(d->ofdInitWorker, &OfdInitWorker::thumbAdded, this, &OfdWidget::onThumbAdded);
        disconnect(d->ofdInitWorker, &OfdInitWorker::pageAdded, this, &OfdWidget::onpageAdded);
    }

    if (d->renderer)
        rofd_renderer_free(d->renderer);
    if (d->doc)
        rofd_document_free(d->doc);
}

int OfdWidget::pageCount() const
{
    Q_D(const OfdWidget);
    return d->pageCount;
}

void OfdWidget::initDoc(const QString& file)
{
    Q_D(OfdWidget);

    rofd_error_t* error = nullptr;
    rofd_status_t status = rofd_document_open(file.toUtf8().constData(), nullptr, &d->doc, &error);

    if (status != ROFD_STATUS_OK || !d->doc) {
        qDebug() << "Cannot open this ofd file: " << file
                 << (error ? rofd_error_get_message(error) : "");
        if (error)
            rofd_error_free(error);
        d->isBadDoc = true;
        return;
    }

    size_t count = 0;
    error = nullptr;
    status = rofd_document_get_page_count(d->doc, &count, &error);
    if (status != ROFD_STATUS_OK || count == 0) {
        qDebug() << "Cannot read page count of ofd file: " << file
                 << (error ? rofd_error_get_message(error) : "");
        if (error)
            rofd_error_free(error);
        d->isBadDoc = true;
        return;
    }
    d->pageCount = static_cast<int>(count);

    rofd_renderer_options_t rendererOptions;
    rofd_renderer_options_init(&rendererOptions, sizeof(rendererOptions));
    error = nullptr;
    status = rofd_renderer_new(&rendererOptions, &d->renderer, &error);
    if (status != ROFD_STATUS_OK || !d->renderer) {
        qDebug() << "Cannot create ofd renderer: "
                 << (error ? rofd_error_get_message(error) : "");
        if (error)
            rofd_error_free(error);
        d->isBadDoc = true;
        return;
    }

    d->ofdInitWorker = new OfdInitWorker(d->doc, d->renderer, d->pageCount);
}

void OfdWidget::initUI()
{
    Q_D(OfdWidget);

    if(d->isBadDoc){
        showBadPage();
        return;
    }

    setContentsMargins(0, 0, 0, 0);
    setFixedSize(qMin(DEFAULT_VIEW_SIZE.width(), (int)(qApp->primaryScreen()->geometry().width() * 0.8)),
                 qMin(DEFAULT_VIEW_SIZE.height(), (int)(qApp->primaryScreen()->geometry().height() * 0.8)));

    d->thumbListWidget = new DListWidget(this);
    d->thumbListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->thumbListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->thumbScrollBar = d->thumbListWidget->verticalScrollBar();
    d->thumbScrollBar->setParent(this);
    d->thumbListWidget->setFixedWidth(96);
    d->thumbListWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    d->thumbListWidget->setAttribute(Qt::WA_MouseTracking);
    d->thumbListWidget->setStyleSheet("QListWidget{"
                                        "border: none;"
                                        "background: white;"
                                        "border-right: 1px solid rgba(0, 0, 0, 0.1);"
                                      "}"
                                      "QListWidget::item{"
                                        "border: none;"
                                      "}");

    d->thumbListWidget->setSpacing(18);

    d->pageListWidget = new DListWidget(this);
    d->pageListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->pageListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->pageListWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    d->pageListWidget->setStyleSheet("QListWidget::item:selected{"
                                        "background: white;"
                                     "}");
    d->pageScrollBar = d->pageListWidget->verticalScrollBar();
    d->pageScrollBar->setParent(this);

    d->mainLayout = new QHBoxLayout;
    d->mainLayout->setContentsMargins(0, 0, 0, 0);
    d->mainLayout->setSpacing(0);
    d->mainLayout->addWidget(d->thumbListWidget);
    d->mainLayout->addWidget(d->pageListWidget);

    setLayout(d->mainLayout);

    initEmptyPages();

    loadThumbSync(0);
    loadPageSync(0);
}

void OfdWidget::initConnections()
{
    Q_D(OfdWidget);

    connect(d->ofdInitWorker, &OfdInitWorker::thumbAdded, this, &OfdWidget::onThumbAdded);
    connect(d->ofdInitWorker, &OfdInitWorker::pageAdded, this, &OfdWidget::onpageAdded);

    connect(d->thumbScrollBar, &QScrollBar::valueChanged, this, &OfdWidget::onThumbScrollBarValueChanged);
    connect(d->pageScrollBar, &QScrollBar::valueChanged, this, &OfdWidget::onPageScrollBarvalueChanged);

    connect(d->pageWorkTimer, &QTimer::timeout, this, &OfdWidget::startLoadCurrentPages);
    connect(d->thumbWorkTimer, &QTimer::timeout, this, &OfdWidget::startLoadCurrentThumbs);
}

void OfdWidget::showBadPage()
{
    QVBoxLayout* layout = new QVBoxLayout;
    QLabel* badLabel = new QLabel(this);
    badLabel->setStyleSheet("QLabel{"
                                "font-size: 20px;"
                                "color: #d0d0d0;"
                            "}");
    badLabel->setText(tr("Cannot preview this file!"));

    layout->addStretch();
    layout->addWidget(badLabel, 0, Qt::AlignHCenter);
    layout->addStretch();
    setLayout(layout);
}

void OfdWidget::onThumbAdded(int index, QImage img)
{
    Q_D(OfdWidget);
    QListWidgetItem* item = d->thumbListWidget->item(index);
    QWidget* w = d->thumbListWidget->itemWidget(item);

    if(!w){
        QPushButton* bnt = new QPushButton(this);
        d->thumbButtonGroup->addButton(bnt);
        bnt->setIcon(QIcon(QPixmap::fromImage(img)));
        bnt->setFixedSize(img.size());
        bnt->setIconSize(QSize(img.width() - 4, img.height()));
        bnt->setCheckable(true);
        bnt->setStyleSheet("QPushButton{"
                            "border: 1px solid rgba(0, 0, 0, 0.2);"
                           "}"
                           "QPushButton:checked{"
                            "border: 2px solid #2ca7f8;"
                           "}");

        if(index == 0){
            bnt->setChecked(true);
        }

        connect(bnt, &QPushButton::clicked, [=]{
            bnt->setChecked(true);
            int row = d->thumbListWidget->row(item);
            d->pageListWidget->setCurrentRow(row);

        });

        d->thumbListWidget->setItemWidget(item, bnt);
        item->setSizeHint(img.size());
    }

    if(d->thumbScrollBar->maximum() == 0){
        d->thumbScrollBar->hide();
    } else {
        d->thumbScrollBar->show();
    }
}

void OfdWidget::onpageAdded(int index, QImage img)
{
    Q_D(OfdWidget);

    d->pageMap.insert(index, img);

    QListWidgetItem* item = d->pageListWidget->item(index);
    QWidget* w = d->pageListWidget->itemWidget(item);

    if(!w){
        img = img.scaled(d->pageListWidget->width(), img.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QImage page(d->pageListWidget->width(), img.height() + 4, QImage::Format_ARGB32_Premultiplied);
        page.fill(Qt::white);
        QPainter p(&page);
        p.drawImage((page.width() - img.width())/2, 2, img);
        if(index < (d->pageCount - 1)){
            QPen pen(QColor(0, 0, 0 , 20));
            p.setPen(pen);
            p.drawLine(0, page.height() - 1, page.width(), page.height() - 1);
        }

        QLabel* pageLabel = new QLabel(this);
        pageLabel->setPixmap(QPixmap::fromImage(page));

        d->pageListWidget->setItemWidget(item, pageLabel);
        item->setSizeHint(page.size());
    }

    if(d->pageScrollBar->maximum() == 0){
        d->pageScrollBar->hide();
    } else {
        d->pageScrollBar->show();
    }

}

void OfdWidget::onThumbScrollBarValueChanged(const int &val)
{
    Q_UNUSED(val)
    Q_D(const OfdWidget);

    d->thumbWorkTimer->stop();
    d->thumbWorkTimer->start();
}

void OfdWidget::onPageScrollBarvalueChanged(const int &val)
{
    Q_UNUSED(val)
    Q_D(const OfdWidget);

    d->pageWorkTimer->stop();
    d->pageWorkTimer->start();

    resizeCurrentPage();

    QListWidgetItem* item = d->pageListWidget->itemAt(d->pageListWidget->width() /2 , 20);
    if(!item){
        return;
    }

    int row = d->pageListWidget->row(item);
    d->thumbListWidget->setCurrentRow(row);
    QListWidgetItem* thumbItem = d->thumbListWidget->item(row);
    if(!thumbItem){
        return;
    }

    QWidget* w = d->thumbListWidget->itemWidget(thumbItem);
    if(!w){
        return;
    }

    QPushButton* bnt = qobject_cast<QPushButton*>(w);
    bnt->setChecked(true);
}

void OfdWidget::startLoadCurrentPages()
{
    Q_D(const OfdWidget);
    QListWidgetItem* item = d->pageListWidget->itemAt(d->pageListWidget->width() / 2, 0);
    if(!item){
        item = d->pageListWidget->itemAt(d->pageListWidget->width() / 2, d->pageListWidget->spacing() * 2 + 1);
    }
    if(item)
    {
        int row = d->pageListWidget->row(item);
        loadPageSync(row);
    }
}

void OfdWidget::startLoadCurrentThumbs()
{
    Q_D(const OfdWidget);
    QListWidgetItem* item = d->thumbListWidget->itemAt(d->thumbListWidget->width() / 2, 0);
    //To prevent this point is int empty area, we get another point again with next pixcel that lager than it spacing
    if(!item){
        item = d->thumbListWidget->itemAt(d->thumbListWidget->width() / 2, d->thumbListWidget->spacing() * 2  + 1);
    }

    if(item)
    {
        int row = d->thumbListWidget->row(item);
        loadThumbSync(row);
    }
}

void OfdWidget::resizeEvent(QResizeEvent *event)
{
    Q_D(OfdWidget);

    QWidget::resizeEvent(event);

    if(d->isBadDoc){
        return;
    }

    if(d->pageScrollBar->maximum() == 0){
        d->pageScrollBar->hide();
    } else {
        d->pageScrollBar->show();
    }

    if(d->thumbScrollBar->maximum() == 0){
        d->thumbScrollBar->hide();
    } else {
        d->thumbScrollBar->show();
    }

    d->thumbScrollBar->setFixedSize(d->thumbScrollBar->sizeHint().width(), event->size().height() - 10);
    d->thumbScrollBar->move(d->thumbListWidget->width() - d->thumbScrollBar->width(), 10);

    d->pageScrollBar->setFixedSize(d->pageScrollBar->sizeHint().width(), event->size().height() - 30);
    d->pageScrollBar->move(event->size().width() - d->pageScrollBar->width(), 30);
    d->pageListWidget->setFixedWidth(width() - d->thumbListWidget->width());

    resizeCurrentPage();
}

void OfdWidget::renderBorder(QImage &img)
{
    QColor color(0, 0, 0, 30);

    QPainter painter(&img);
    QPen pen;
    pen.setColor(color);
    pen.setWidth(1);
    painter.setPen(pen);

    painter.drawRect(0, 0, img.width() - 1, img.height() -1);
}

void OfdWidget::emptyBorder(QImage &img)
{
    QColor color(255, 255, 255);

    QPainter painter(&img);
    QPen pen;
    pen.setColor(color);
    pen.setWidth(2);
    painter.setPen(pen);

    painter.drawRect(0, 0, img.width() - 2, img.height() - 2);
}

void OfdWidget::loadPageSync(const int &index)
{
    Q_D(OfdWidget);

    QFuture<void> future = QtConcurrent::run([=]{
       d->ofdInitWorker->startGetPageImage(index);
    });
    Q_UNUSED(future)
}

void OfdWidget::loadThumbSync(const int &index)
{
    Q_D(OfdWidget);

    QFuture<void> future = QtConcurrent::run([=]{
        d->ofdInitWorker->startGetPageThumb(index);
    });
    Q_UNUSED(future)
}

void OfdWidget::initEmptyPages()
{
    Q_D(OfdWidget);

    for(int i = 0; i < d->pageCount; i ++ ){
        QListWidgetItem* pageItem = new QListWidgetItem;
        pageItem->setSizeHint(DEFAULT_PAGE_SIZE);

        QListWidgetItem* thumbItem = new QListWidgetItem;
        thumbItem->setSizeHint(DEFAULT_THUMB_SIZE);

        d->pageListWidget->addItem(pageItem);
        d->thumbListWidget->addItem(thumbItem);
    }
}

void OfdWidget::resizeCurrentPage()
{
    Q_D(OfdWidget);

    QListWidgetItem* currentItem = d->pageListWidget->itemAt(d->pageListWidget->width() / 2, d->pageListWidget->height() / 2);

    if(!currentItem)
        return;

    int currentRow = d->pageListWidget->row(currentItem);
    int index = currentRow - DISPLAT_PAGE_NUM/2;
    if(index < 0){
        index = 0;
    }
    int counter = 0;
    while(counter < DISPLAT_PAGE_NUM){
        counter ++;

        if(d->pageMap.contains(index)){

            QListWidgetItem* item = d->pageListWidget->item(index);
            if(!item){
                continue;
            }

            QWidget* w = d->pageListWidget->itemWidget(item);
            if(!w){
                continue;
            }

            QLabel* label = qobject_cast<QLabel*>(w);

            QImage img = d->pageMap.value(index);
            img = img.scaled(d->pageListWidget->width(), img.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QImage page(d->pageListWidget->width(), img.height() + 4, QImage::Format_ARGB32_Premultiplied);
            page.fill(Qt::white);

            QPainter p(&page);
            p.drawImage((page.width() - img.width()) / 2, 2, img);

            if(index < (d->pageCount - 1)){
                QPen pen(QColor(0, 0, 0 , 20));
                p.setPen(pen);
                p.drawLine(0, page.height() - 1, page.width(), page.height() - 1);
            }

            label->setPixmap(QPixmap::fromImage(page));
            item->setSizeHint(page.size());

            index ++;
        } else {
            index ++;
            continue;
        }
    }
}

OfdInitWorker::OfdInitWorker(rofd_document_t *doc,
                             rofd_renderer_t *renderer,
                             int pageCount,
                             QObject *parent):
    QObject(parent),
    m_doc(doc),
    m_renderer(renderer),
    m_pageCount(pageCount)
{

}

void OfdInitWorker::startGetPageThumb(int index)
{
    int counter = 0;
    while(counter < DISPLAY_THUMB_NUM){
        counter++;

        //Skip for indexed thumb we got
        if(m_gotThumbIndexes.contains(index)){
            index ++;
            continue;
        }

        QImage img = getRenderedPageImage(index, DEFAULT_PAGE_RENDER_WIDTH);

        if(img.isNull()){
            break;
        } else{
            QImage thumb = getPageThumb(img);
            emit thumbAdded(index, thumb);
            m_gotThumbIndexes << index;
            index ++;
        }

    }
}

void OfdInitWorker::startGetPageImage(int index)
{
    int counter = 0;
    while(counter < DISPLAT_PAGE_NUM){
        counter++;

        //Skip for indexed page we got
        if(m_gotPageIndexes.contains(index)){
            index ++;
            continue;
        }

        QImage img = getRenderedPageImage(index, DEFAULT_PAGE_RENDER_WIDTH);

        if(img.isNull()){
            break;
        } else {
            emit pageAdded(index, img);
            m_gotPageIndexes << index;
            index ++;
        }
    }
}

QImage OfdInitWorker::getPageThumb(const QImage &img) const
{
    QImage newImg = img;
    return newImg.scaled(DEFAULT_THUMB_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage OfdInitWorker::getRenderedPageImage(const int &index, int targetWidth) const
{
    QImage img;

    rofd_error_t* error = nullptr;
    rofd_page_t* page = nullptr;
    rofd_status_t status = rofd_document_get_page(m_doc, static_cast<size_t>(index), &page, &error);

    if (status != ROFD_STATUS_OK || !page) {
        if (error)
            rofd_error_free(error);
        return img;
    }

    rofd_rect_t size_mm = {0.0, 0.0, 0.0, 0.0};
    error = nullptr;
    rofd_page_get_size_mm(page, &size_mm, &error);
    if (error)
        rofd_error_free(error);

    rofd_render_options_t opts;
    rofd_render_options_init(&opts, sizeof(opts));
    opts.dpi = 254.0;
    if (targetWidth > 0 && size_mm.width_mm > 0) {
        const double mmPerPx = 25.4 / opts.dpi;
        const double widthPx = size_mm.width_mm / mmPerPx;
        if (widthPx > 0)
            opts.scale = static_cast<double>(targetWidth) / widthPx;
    }

    int32_t width = 0;
    int32_t height = 0;
    error = nullptr;
    status = rofd_renderer_get_pixel_canvas_size(m_renderer, page, &opts,
                                                 &width, &height, &error);
    if (status != ROFD_STATUS_OK || width <= 0 || height <= 0) {
        if (error)
            rofd_error_free(error);
        rofd_page_free(page);
        return img;
    }

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        rofd_page_free(page);
        return img;
    }

    cairo_t* cr = cairo_create(surface);

    rofd_render_report_t* report = nullptr;
    error = nullptr;
    status = rofd_renderer_render_page_cairo(m_renderer, page, cr, &opts, &report, &error);
    if (status == ROFD_STATUS_OK) {
        cairo_surface_flush(surface);
        unsigned char* data = cairo_image_surface_get_data(surface);
        const int stride = cairo_image_surface_get_stride(surface);
        img = QImage(data, width, height, stride, QImage::Format_ARGB32_Premultiplied).copy();
    }

    if (report)
        rofd_render_report_free(report);
    if (error)
        rofd_error_free(error);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    rofd_page_free(page);

    return img;
}

DListWidget::DListWidget(QWidget *parent):
    QListWidget(parent)
{

}

void DListWidget::mouseMoveEvent(QMouseEvent *e)
{
    QWidget::mouseMoveEvent(e);
}
