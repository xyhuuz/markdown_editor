#include "markdowneditor.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>

MarkdownEditor::MarkdownEditor(QWidget *parent)
    : QTextEdit(parent)
{
    setAcceptDrops(true);
}

// 辅助函数，判断URL是否为图片
bool MarkdownEditor::isImageUrl(const QUrl& url) const
{
    if (!url.isLocalFile()) return false;
    QString localPath = url.toLocalFile();
    static const QStringList imageSuffixes = {"png", "jpg", "jpeg", "gif", "svg", "bmp", "webp"};
    return imageSuffixes.contains(QFileInfo(localPath).suffix().toLower(), Qt::CaseInsensitive);
}

void MarkdownEditor::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    // 如果拖拽内容是图片数据或图片文件URL，则接受该动作
    if (mimeData->hasImage() || (mimeData->hasUrls() && !mimeData->urls().isEmpty() && isImageUrl(mimeData->urls().first()))) {
        event->acceptProposedAction();
        qDebug() << "Drag enter: Image detected";
    } else {
        // 否则，执行基类的默认行为（例如允许拖动文本）
        QTextEdit::dragEnterEvent(event);
    }
}

void MarkdownEditor::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    // 检查是否是图片
    bool isImage = mimeData->hasImage() ||
                   (mimeData->hasUrls() && !mimeData->urls().isEmpty() && isImageUrl(mimeData->urls().first()));

    if (isImage) {
        qDebug() << "Drop: Image detected, emitting signal";

        // 获取鼠标位置（Qt6兼容方式）
        QPoint dropPos;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        dropPos = event->position().toPoint();
#else
        dropPos = event->pos();
#endif

        // 发出信号，通知主窗口处理
        emit imageDropped(mimeData, dropPos);
        event->acceptProposedAction();
        // 完全处理事件，不调用基类的dropEvent
        return;
    } else {
        qDebug() << "Drop: Not an image, using default behavior";
        // 否则，执行基类的默认行为
        QTextEdit::dropEvent(event);
    }
}
