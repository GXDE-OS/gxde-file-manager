#ifndef DFMPREVIEWSIDEBAR_H
#define DFMPREVIEWSIDEBAR_H

#include <QWidget>
#include <durl.h>
#include <QLabel>
#include <QScrollArea>

class DFMPreviewSidebar : public QWidget
{
    Q_OBJECT
public:
    DFMPreviewSidebar(QWidget *parent = 0);
    ~DFMPreviewSidebar();

    void setFileUrl(QList<DUrl> url);
    void refresh();

private:
    QScrollArea* labelAddScroll(QLabel* label);
    void initUI();

    QList<DUrl> m_selectUrlList;
    QLabel *m_fileIcon = nullptr;
    QLabel *m_fileName = nullptr;
    QLabel *m_fileSizeTips = nullptr;
    QLabel *m_fileSize = nullptr;
    QLabel *m_fileMimetype = nullptr;
    QLabel *m_timeCreatedLabel = nullptr;
    QLabel *m_timeModifiedLabel = nullptr;
    bool isInited = false;

    void resizeEvent(QResizeEvent *event) override;
    //QLabel *m_filePath = nullptr;

};

#endif // DFMPREVIEWSIDEBAR_H
