#ifndef MARKDOWNEDITOR_H
#define MARKDOWNEDITOR_H

#include <QTextEdit>

class QMimeData;

class MarkdownEditor : public QTextEdit
{
    Q_OBJECT

public:
    explicit MarkdownEditor(QWidget *parent = nullptr);

signals:
    // 定义一个信号，当图片被拖放时发出
    // 参数是 MIME 数据和当时的光标位置
    void imageDropped(const QMimeData *mime, const QPoint &position);

protected:
    // 重写拖放事件处理函数
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    bool isImageUrl(const QUrl& url) const;
};

#endif // MARKDOWNEDITOR_H
