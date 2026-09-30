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

#ifndef OFDWIDGET_H
#define OFDWIDGET_H

#include <QWidget>
#include <QSharedPointer>
#include <QListWidget>
#include <QLabel>

#include <rofd.h>
#include <cairo.h>

#define DEFAULT_VIEW_SIZE QSize(700, 800)
#define DEFAULT_THUMB_SIZE QSize(55, 74)
#define DEFAULT_PAGE_SIZE QSize(800, 1200)
#define DISPLAY_THUMB_NUM 10
#define DISPLAT_PAGE_NUM 5
#define DEFAULT_PAGE_RENDER_WIDTH 800

class OfdWidgetPrivate;
class OfdInitWorker;
class OfdWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OfdWidget(const QString &file, QWidget *parent = 0);
    ~OfdWidget();

    int pageCount() const;

    void initDoc(const QString &file);

    void initUI();
    void initConnections();

    void showBadPage();

public slots:
    void onThumbAdded(int index, QImage img);
    void onpageAdded(int index, QImage img);
    void onThumbScrollBarValueChanged(const int& val);
    void onPageScrollBarvalueChanged(const int& val);
    void startLoadCurrentPages();
    void startLoadCurrentThumbs();

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private:

    void renderBorder(QImage& img);
    void emptyBorder(QImage& img);

    void loadPageSync(const int& index);
    void loadThumbSync(const int& index);
    void initEmptyPages();

    void resizeCurrentPage();

    QSharedPointer<OfdWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(d_ptr), OfdWidget)
};

class OfdInitWorker: public QObject{
    Q_OBJECT
public:
    explicit OfdInitWorker(rofd_document_t *doc,
                           rofd_renderer_t *renderer,
                           int pageCount,
                           QObject* parent = 0);

    void startGetPageThumb(int index);
    void startGetPageImage(int index);

signals:
    void pageAdded(const int& index, const QImage& img);
    void thumbAdded(const int& index, const QImage& img);

private:
    /* Render the given page at the requested pixel width keeping the
     * physical aspect ratio. Returns a null QImage on failure. */
    QImage getRenderedPageImage(const int &index, int targetWidth) const;
    QImage getPageThumb(const QImage &img) const;

    QList<int> m_gotThumbIndexes;
    QList<int> m_gotPageIndexes;

    rofd_document_t *m_doc;
    rofd_renderer_t *m_renderer;
    int m_pageCount;
};

class DListWidget: public QListWidget{
    Q_OBJECT
public:
    explicit DListWidget(QWidget* parent = 0);
protected:
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;

};


#endif // OFDWIDGET_H
