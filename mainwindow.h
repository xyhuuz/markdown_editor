#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mathrenderer.h"  // 新增
#include <QMainWindow>
#include <QDebug>
#include <QString>
#include <QTimer>
#include <QTranslator>  // 新增：翻译器头文件
#include <QNetworkAccessManager>  // 新增：网络访问管理
#include <QNetworkReply>  // 新增：网络回复
#include <QSettings>  // 新增：配置存储
#include <QDir>  // 新增：目录操作

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QMimeData;
class QTextCursor;
class QListWidgetItem; // 添加 QListWidgetItem 的前向声明
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;  // 新增：语言改变事件

private slots:
    void on_markdownEditor_textChanged();
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionExit_triggered();

    // 新增槽函数，用于响应图片拖放信号
    void onImageDropped(const QMimeData *mime, const QPoint &position);
    // 新增：用于响应 listWidget 列表项双击信号的槽函数
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    // 新增：用于响应 listWidget_details 列表项双击信号的槽函数
    void on_listWidget_details_itemDoubleClicked(QListWidgetItem *item);

    // 新增：文本格式化动作槽函数
    void on_actionBolder_triggered();
    void on_actionItalics_triggered();
    void on_actionUnderline_triggered();

    // 新增：编辑动作槽函数
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionCut_triggered();

    // 新增：关于动作槽函数
    void on_actionAbout_triggered();


    // 新增：语言切换槽函数
    void on_actionChinese_triggered();    // 中文
    void on_actionTibetan_triggered();    // 藏文
    void on_actionEnglish_triggered();    // 英文

    // 新增：同步功能槽函数
    void on_actionSync_triggered();       // 开始同步
    void on_actionSyncSettings_triggered(); // 同步设置

    // 新增：网络请求完成槽函数
    void onNetworkReplyFinished(QNetworkReply *reply);

private:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    bool maybeSave();
    void setCurrentFile(const QString &filePath);
    void updateWindowTitle();

    // 这个函数现在只负责文件操作和文本插入
    void handleDroppedImage(const QMimeData *mime, QTextCursor &cursor);

    // 新增：初始化/创建 resources 文件夹并加载笔记列表
    void setupResourcesAndLoadNotes();

    // 新增：根据笔记名称加载笔记文件
    void loadNote(const QString &noteName);

    // 新增：更新详情列表，显示当前笔记文件夹下的文档
    void updateDetailsList(const QString &noteName);

    // 新增：打开PDF文件
    void openPdfFile(const QString &filePath);

    QString processImagesForPreview(const QString &markdownText);

    // 新增：辅助函数，用于文本格式化
    void formatSelectedText(const QString &prefix, const QString &suffix = "");


    // 新增：语言切换相关函数
    void setupLanguageSystem();
    void switchLanguage(const QString &languageCode);
    void loadLanguage(const QString &languageCode);
    void updateLanguageMenu();
    void updatePreview();

    Ui::MainWindow *ui;
    QString currentFilePath;

    // 新增：同步相关函数
    void setupSyncSystem();
    void loadSyncSettings();
    void saveSyncSettings();
    void syncFiles();
    void uploadFile(const QString &localPath, const QString &remotePath);
    void createRemoteDirectory(const QString &remotePath);
    void listRemoteDirectory(const QString &remotePath);
    QString getRemotePath(const QString &localPath);
    void processNextUpload();
    void showSyncResult();

    // 新增：用于存储 resources 文件夹的路径
    QString resourcesPath;

    // 新增：当前选中的笔记名称
    QString currentNoteName;

    // 修改LaTeX渲染器
    MathRenderer *mathRenderer;
    QTimer *previewTimer;


    // 新增：翻译器
    QTranslator *appTranslator;
    QTranslator *qtTranslator;

    // 新增：当前语言
    QString currentLanguage;

    // 新增：同步相关成员变量
    QNetworkAccessManager *networkManager;
    QSettings *syncSettings;
    QString webdavUrl;
    QString webdavUsername;
    QString webdavPassword;
    QString remoteBasePath; // 远程基础路径
    int directoriesToCreateCount;
    int directoriesCreatedCount;
    bool syncConfigured;

    // 新增：同步状态变量
    QList<QPair<QString, QString>> uploadQueue; // 上传队列 <本地路径, 远程路径>
    int successfulUploads;
    int failedUploads;
    bool isSyncing;
};


#endif
