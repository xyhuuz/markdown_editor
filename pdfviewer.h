#ifndef PDFVIEWER_H
#define PDFVIEWER_H

#include <QMainWindow>
#include <QPdfDocument>
#include <QPdfView>
#include <QPdfPageNavigator>
#include <QSpinBox>
#include <QLabel>

class PdfViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit PdfViewer(QWidget *parent = nullptr);
    ~PdfViewer();

    bool loadPdf(const QString &filePath);

private slots:
    void onPageChanged(int page);
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void onFitWidth();
    void onFitPage();
    void onPrint();
    void onFirstPage();
    void onPreviousPage();
    void onNextPage();
    void onLastPage();
    void updatePageNavigation();

private:
    void setupToolBar();
    void setupStatusBar();

    QPdfDocument *pdfDocument;
    QPdfView *pdfView;
    QPdfPageNavigator *pageNavigator;  // 新增：页面导航器

    // 工具栏组件
    QToolBar *mainToolBar;
    QAction *firstPageAction;
    QAction *previousPageAction;
    QAction *nextPageAction;
    QAction *lastPageAction;
    QSpinBox *pageSpinBox;
    QLabel *pageCountLabel;
    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *zoomResetAction;
    QAction *fitWidthAction;
    QAction *fitPageAction;
    QAction *printAction;

    // 状态栏组件
    QLabel *zoomLabel;
    QLabel *statusLabel;

    // 当前页面
    int currentPage;
};

#endif // PDFVIEWER_H
