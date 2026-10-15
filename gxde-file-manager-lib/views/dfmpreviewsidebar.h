#ifndef DFMPREVIEWSIDEBAR_H
#define DFMPREVIEWSIDEBAR_H

#include <QWidget>
#include <durl.h>
#include <QLabel>
#include <QScrollArea>

// 支持按宽度自动换行、但复制时不带换行符的标签
class WrapLabel : public QLabel
{
public:
    explicit WrapLabel(QWidget *parent = nullptr);

    // 设置文本并按当前控件宽度自动换行，同时保留原始文本供复制使用
    void setWrappedText(const QString &text);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;

private:
    void copyCleanText();

    QString m_sourceText;
};

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
    WrapLabel *m_fileName = nullptr;
    QLabel *m_fileSizeTips = nullptr;
    WrapLabel *m_fileSize = nullptr;
    WrapLabel *m_fileMimetype = nullptr;
    WrapLabel *m_timeCreatedLabel = nullptr;
    WrapLabel *m_timeModifiedLabel = nullptr;
    bool isInited = false;

    void resizeEvent(QResizeEvent *event) override;
    //QLabel *m_filePath = nullptr;

};

#endif // DFMPREVIEWSIDEBAR_H
