#include "pdfviewer.h"
#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QFileInfo>
#include <QSpinBox>
#include <QLabel>
#include <QStatusBar>
#include <QMessageBox>
#include <QApplication>

PdfViewer::PdfViewer(QWidget *parent)
    : QMainWindow(parent)
    , pdfDocument(new QPdfDocument(this))
    , pdfView(new QPdfView(this))
    , pageNavigator(nullptr)  // 初始化页面导航器
    , currentPage(0)
{
    // 设置PDF视图
    pdfView->setDocument(pdfDocument);

    // 设置中心部件
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(pdfView);
    setCentralWidget(centralWidget);

    // 设置窗口属性
    setMinimumSize(800, 600);

    // 创建界面
    setupToolBar();
    setupStatusBar();

    // 连接信号
    connect(pdfView, &QPdfView::zoomFactorChanged, this, &PdfViewer::updatePageNavigation);
}

PdfViewer::~PdfViewer()
{
}

void PdfViewer::setupToolBar()
{
    mainToolBar = addToolBar(tr("主工具栏"));
    mainToolBar->setMovable(false);

    // 页面导航动作
    firstPageAction = mainToolBar->addAction(tr("首页"));
    connect(firstPageAction, &QAction::triggered, this, &PdfViewer::onFirstPage);

    previousPageAction = mainToolBar->addAction(tr("上一页"));
    connect(previousPageAction, &QAction::triggered, this, &PdfViewer::onPreviousPage);

    // 页面选择器
    pageSpinBox = new QSpinBox(this);
    pageSpinBox->setMinimum(1);
    pageSpinBox->setMaximum(1);
    pageSpinBox->setMinimumWidth(60);
    connect(pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PdfViewer::onPageChanged);
    mainToolBar->addWidget(pageSpinBox);

    pageCountLabel = new QLabel(tr(" / 1"), this);
    mainToolBar->addWidget(pageCountLabel);

    nextPageAction = mainToolBar->addAction(tr("下一页"));
    connect(nextPageAction, &QAction::triggered, this, &PdfViewer::onNextPage);

    lastPageAction = mainToolBar->addAction(tr("末页"));
    connect(lastPageAction, &QAction::triggered, this, &PdfViewer::onLastPage);

    mainToolBar->addSeparator();

    // 缩放动作
    zoomOutAction = mainToolBar->addAction(tr("缩小"));
    connect(zoomOutAction, &QAction::triggered, this, &PdfViewer::onZoomOut);

    zoomResetAction = mainToolBar->addAction(tr("实际大小"));
    connect(zoomResetAction, &QAction::triggered, this, &PdfViewer::onZoomReset);

    zoomInAction = mainToolBar->addAction(tr("放大"));
    connect(zoomInAction, &QAction::triggered, this, &PdfViewer::onZoomIn);

    mainToolBar->addSeparator();

    // 适应动作
    fitWidthAction = mainToolBar->addAction(tr("适应宽度"));
    connect(fitWidthAction, &QAction::triggered, this, &PdfViewer::onFitWidth);

    fitPageAction = mainToolBar->addAction(tr("适应页面"));
    connect(fitPageAction, &QAction::triggered, this, &PdfViewer::onFitPage);

    // 打印功能在Qt 6.8中可能需要额外处理，暂时注释掉
    // mainToolBar->addSeparator();
    // printAction = mainToolBar->addAction(tr("打印"));
    // connect(printAction, &QAction::triggered, this, &PdfViewer::onPrint);
}

void PdfViewer::setupStatusBar()
{
    zoomLabel = new QLabel(this);
    statusLabel = new QLabel(this);

    statusBar()->addPermanentWidget(zoomLabel);
    statusBar()->addPermanentWidget(statusLabel);

    updatePageNavigation();
}

bool PdfViewer::loadPdf(const QString &filePath)
{
    if (!QFileInfo::exists(filePath)) {
        QMessageBox::warning(this, tr("错误"), tr("文件不存在: %1").arg(filePath));
        return false;
    }

    // 使用页面数量检查代替错误枚举，提高兼容性
    pdfDocument->load(filePath);

    if (pdfDocument->pageCount() <= 0) {
        QMessageBox::warning(this, tr("错误"), tr("无法加载PDF文件或文件为空: %1").arg(filePath));
        return false;
    }

    // 获取页面导航器
    pageNavigator = pdfView->pageNavigator();

    // 连接页面变化信号
    if (pageNavigator) {
        connect(pageNavigator, &QPdfPageNavigator::currentPageChanged, this, [this](int page) {
            currentPage = page;
            updatePageNavigation();
        });
    }

    // 更新页面导航
    pageSpinBox->setMaximum(pdfDocument->pageCount());
    pageCountLabel->setText(tr(" / %1").arg(pdfDocument->pageCount()));

    // 设置初始页面
    if (pageNavigator) {
        pageNavigator->jump(0, QPointF(), pdfView->zoomFactor());
    }
    currentPage = 0;
    updatePageNavigation();

    setWindowTitle(QString("PDF查看器 - %1").arg(QFileInfo(filePath).fileName()));
    statusLabel->setText(tr("已加载: %1").arg(QFileInfo(filePath).fileName()));

    return true;
}

void PdfViewer::onPageChanged(int page)
{
    // pageSpinBox的值从1开始，但页面索引从0开始
    if (page >= 1 && page <= pdfDocument->pageCount()) {
        currentPage = page - 1;
        if (pageNavigator) {
            pageNavigator->jump(currentPage, QPointF(), pdfView->zoomFactor());
        }
        updatePageNavigation();
    }
}

void PdfViewer::onZoomIn()
{
    pdfView->setZoomFactor(pdfView->zoomFactor() * 1.2);
}

void PdfViewer::onZoomOut()
{
    pdfView->setZoomFactor(pdfView->zoomFactor() / 1.2);
}

void PdfViewer::onZoomReset()
{
    pdfView->setZoomFactor(1.0);
}

void PdfViewer::onFitWidth()
{
    // 简单实现适应宽度 - 根据窗口宽度调整缩放
    if (pdfDocument->pageCount() <= 0) return;

    QSizeF pageSize = pdfDocument->pagePointSize(currentPage);
    if (pageSize.isEmpty()) return;

    int viewWidth = pdfView->width() - 40; // 减去边距
    qreal zoomFactor = static_cast<qreal>(viewWidth) / pageSize.width();
    pdfView->setZoomFactor(zoomFactor);
}

void PdfViewer::onFitPage()
{
    // 简单实现适应页面 - 根据窗口大小调整缩放
    if (pdfDocument->pageCount() <= 0) return;

    QSizeF pageSize = pdfDocument->pagePointSize(currentPage);
    if (pageSize.isEmpty()) return;

    int viewWidth = pdfView->width() - 40;
    int viewHeight = pdfView->height() - 40;

    qreal widthZoom = static_cast<qreal>(viewWidth) / pageSize.width();
    qreal heightZoom = static_cast<qreal>(viewHeight) / pageSize.height();

    pdfView->setZoomFactor(qMin(widthZoom, heightZoom));
}

void PdfViewer::onPrint()
{
    QMessageBox::information(this, tr("信息"), tr("打印功能在当前版本中暂不可用。"));
}

void PdfViewer::onFirstPage()
{
    currentPage = 0;
    if (pageNavigator) {
        pageNavigator->jump(currentPage, QPointF(), pdfView->zoomFactor());
    }
    updatePageNavigation();
}

void PdfViewer::onPreviousPage()
{
    if (currentPage > 0) {
        currentPage--;
        if (pageNavigator) {
            pageNavigator->jump(currentPage, QPointF(), pdfView->zoomFactor());
        }
        updatePageNavigation();
    }
}

void PdfViewer::onNextPage()
{
    if (currentPage < pdfDocument->pageCount() - 1) {
        currentPage++;
        if (pageNavigator) {
            pageNavigator->jump(currentPage, QPointF(), pdfView->zoomFactor());
        }
        updatePageNavigation();
    }
}

void PdfViewer::onLastPage()
{
    currentPage = pdfDocument->pageCount() - 1;
    if (pageNavigator) {
        pageNavigator->jump(currentPage, QPointF(), pdfView->zoomFactor());
    }
    updatePageNavigation();
}

void PdfViewer::updatePageNavigation()
{
    // 更新页面选择器（注意：pageSpinBox从1开始，currentPage从0开始）
    pageSpinBox->blockSignals(true);
    pageSpinBox->setValue(currentPage + 1);
    pageSpinBox->blockSignals(false);

    // 更新缩放标签
    zoomLabel->setText(tr("缩放: %1%").arg(qRound(pdfView->zoomFactor() * 100)));

    // 更新按钮状态
    firstPageAction->setEnabled(currentPage > 0);
    previousPageAction->setEnabled(currentPage > 0);
    nextPageAction->setEnabled(currentPage < pdfDocument->pageCount() - 1);
    lastPageAction->setEnabled(currentPage < pdfDocument->pageCount() - 1);

    // 更新状态栏
    if (pdfDocument->pageCount() > 0) {
        statusLabel->setText(tr("第 %1 页，共 %2 页").arg(currentPage + 1).arg(pdfDocument->pageCount()));
    }
}
