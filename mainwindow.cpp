#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pdfviewer.h" // PDF查看器

#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDir>
#include <QMimeData>
#include <QDateTime>
#include <QTextCursor>
#include <QTimer>
#include <QUrl> // 包含 URL 头文件
#include <QCoreApplication> // 用于获取程序路径
#include <QListWidgetItem> // 用于操作列表项
#include <QInputDialog> // 用于输入对话框
#include <QDialog> // 用于关于对话框
#include <QVBoxLayout> // 用于关于对话框布局
#include <QLabel> // 用于关于对话框
#include <QTranslator> // 翻译器
#include <QEvent> // 事件处理
#include <QActionGroup> // 动作组

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , appTranslator(new QTranslator(this))
    , qtTranslator(new QTranslator(this))
    , currentLanguage("zh_CN") // 默认中文
    , mathRenderer(new MathRenderer(this))  // 使用MathRenderer
    , previewTimer(new QTimer(this))
    , directoriesToCreateCount(0)  // 新增
    , directoriesCreatedCount(0)   // 新增
{
    ui->setupUi(this);

    // 设置预览定时器
    previewTimer->setSingleShot(true);
    previewTimer->setInterval(800); // 增加延迟避免频繁渲染
    connect(previewTimer, &QTimer::timeout, this, [this]() {
        updatePreview();
    });

    // 初始化语言系统
    setupLanguageSystem();

    // 初始化同步系统
    setupSyncSystem();

    // 调用新增的函数，创建/加载笔记资源
    setupResourcesAndLoadNotes();

    QFont font = ui->listWidget->font();
    font.setPointSize(14);  // 设置字体大小
    ui->listWidget->setFont(font);

    // 设置详情列表的字体
    QFont detailsFont = ui->listWidget_details->font();
    detailsFont.setPointSize(11);
    ui->listWidget_details->setFont(detailsFont);

    newFile();

    // 将编辑器的 imageDropped 信号连接到主窗口的 onImageDropped 槽
    connect(ui->markdownEditor, &MarkdownEditor::imageDropped, this, &MainWindow::onImageDropped);

    // 连接 listWidget 的双击信号到对应的槽函数
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::on_listWidget_itemDoubleClicked);

    // 连接 listWidget_details 的双击信号到对应的槽函数
    // connect(ui->listWidget_details, &QListWidget::itemDoubleClicked,
    //         this, &MainWindow::on_listWidget_details_itemDoubleClicked,
    //         Qt::UniqueConnection);
    // 连接语言切换信号
    connect(ui->action, &QAction::triggered, this, &MainWindow::on_actionChinese_triggered);
    connect(ui->action_2, &QAction::triggered, this, &MainWindow::on_actionTibetan_triggered);
    connect(ui->actionEnglish, &QAction::triggered, this, &MainWindow::on_actionEnglish_triggered);

    // 连接同步动作信号
    //connect(ui->actionSync, &QAction::triggered, this, &MainWindow::on_actionSync_triggered);
    //connect(ui->actionSyncSettings, &QAction::triggered, this, &MainWindow::on_actionSyncSettings_triggered);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 初始化语言系统
void MainWindow::setupLanguageSystem()
{
    // 创建动作组，使语言菜单项互斥
    QActionGroup *languageGroup = new QActionGroup(this);
    languageGroup->addAction(ui->action);
    languageGroup->addAction(ui->action_2);
    languageGroup->addAction(ui->actionEnglish);
    languageGroup->setExclusive(true);

    // 加载默认语言
    loadLanguage(currentLanguage);
}

// 语言切换槽函数
void MainWindow::on_actionChinese_triggered()
{
    switchLanguage("zh_CN");
}

void MainWindow::on_actionTibetan_triggered()
{
    switchLanguage("bo_CN");
}

void MainWindow::on_actionEnglish_triggered()
{
    switchLanguage("en_US");
}

// 切换语言
void MainWindow::switchLanguage(const QString &languageCode)
{
    if (currentLanguage != languageCode) {
        currentLanguage = languageCode;
        loadLanguage(languageCode);
        updateLanguageMenu();
    }
}

// 加载语言文件
void MainWindow::loadLanguage(const QString &languageCode)
{
    // 移除旧的翻译器
    qApp->removeTranslator(appTranslator);
    qApp->removeTranslator(qtTranslator);

    // 尝试多个可能的翻译文件路径
    QStringList possiblePaths;

    // 1. 应用程序目录下的 translations 文件夹
    possiblePaths << QApplication::applicationDirPath() + "/translations";

    // 2. 源代码目录下的 translations 文件夹（开发时使用）
    possiblePaths << QDir::currentPath() + "/translations";

    // 3. 如果使用 shadow build，可能在构建目录
    possiblePaths << QDir::currentPath();

    // 4. 应用程序目录本身
    possiblePaths << QApplication::applicationDirPath();

    // 尝试加载应用程序翻译
    QString appQmFile;
    if (languageCode == "zh_CN") {
        appQmFile = "zh_CN.qm";  // 根据你的实际文件名调整
    } else if (languageCode == "en_US") {
        appQmFile = "en_US.qm";
    } else if (languageCode == "bo_CN") {
        appQmFile = "bo_CN.qm";
    } else {
        appQmFile = QString("%1.qm").arg(languageCode);
    }

    bool translationLoaded = false;

    // 尝试所有可能的路径
    for (const QString &path : possiblePaths) {
        QString fullPath = path + "/" + appQmFile;
        qDebug() << "Trying to load translation from:" << fullPath;

        if (QFile::exists(fullPath)) {
            if (appTranslator->load(fullPath)) {
                qApp->installTranslator(appTranslator);
                qDebug() << "Successfully loaded translation:" << fullPath;
                translationLoaded = true;
                break;
            }
        }
    }

    if (!translationLoaded) {
        qDebug() << "Failed to load application translation:" << appQmFile;
        qDebug() << "Searched in paths:" << possiblePaths;

        // 列出 translations 目录中的所有文件，帮助调试
        for (const QString &path : possiblePaths) {
            QDir dir(path);
            if (dir.exists()) {
                QStringList files = dir.entryList(QStringList() << "*.qm", QDir::Files);
                qDebug() << "QM files in" << path << ":" << files;
            }
        }
    }

    // 尝试加载Qt基础翻译（可选）
    QString qtQmFile = QString("qt_%1.qm").arg(languageCode);
    for (const QString &path : possiblePaths) {
        QString fullQtPath = path + "/" + qtQmFile;
        if (QFile::exists(fullQtPath) && qtTranslator->load(fullQtPath)) {
            qApp->installTranslator(qtTranslator);
            qDebug() << "Loaded Qt translation:" << fullQtPath;
            break;
        }
    }

    // 重新翻译UI
    ui->retranslateUi(this);

    // 更新动态文本
    updateWindowTitle();

    // 如果当前有打开的笔记，更新状态栏消息
    if (!currentNoteName.isEmpty()) {
        statusBar()->showMessage(tr("语言已切换到: %1").arg(languageCode), 2000);
    }
}

// 更新语言菜单状态
void MainWindow::updateLanguageMenu()
{
    // 根据当前语言设置对应动作为选中状态
    ui->action->setChecked(currentLanguage == "zh_CN");
    ui->action_2->setChecked(currentLanguage == "bo_CN");
    ui->actionEnglish->setChecked(currentLanguage == "en_US");
}

// 处理语言改变事件
void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        // 语言改变时更新UI
        ui->retranslateUi(this);
        updateWindowTitle();
    }
    QMainWindow::changeEvent(event);
}

// 修改关于对话框，使其支持多语言
void MainWindow::on_actionAbout_triggered()
{
    // 创建关于对话框
    QDialog *aboutDialog = new QDialog(this);
    aboutDialog->setWindowTitle(tr("关于 Markdown Notes"));
    aboutDialog->setFixedSize(600, 280);

    // 设置对话框布局
    QVBoxLayout *layout = new QVBoxLayout(aboutDialog);

    // 添加程序图标和名称
    QLabel *titleLabel = new QLabel(tr("Markdown Notes\n2025信息科学技术学院校级大创项目"), aboutDialog);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);

    // 添加版本信息
    QLabel *versionLabel = new QLabel(tr("版本 1.0"), aboutDialog);
    versionLabel->setAlignment(Qt::AlignCenter);

    // 添加描述信息
    QLabel *descLabel = new QLabel(tr("一个简单的Markdown笔记应用程序\n\n"
                                      "支持Markdown编辑和预览\n"
                                      "支持图片拖拽和PDF查看"), aboutDialog);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);

    // 添加版权信息
    QLabel *copyrightLabel = new QLabel(tr("© 2025 徐以浩 范晨扬 李鹏宇 杨翔 次晋"), aboutDialog);
    copyrightLabel->setAlignment(Qt::AlignCenter);

    // 将标签添加到布局
    layout->addWidget(titleLabel);
    layout->addWidget(versionLabel);
    layout->addWidget(descLabel);
    layout->addWidget(copyrightLabel);

    // 显示对话框
    aboutDialog->exec();

    // 清理内存
    aboutDialog->deleteLater();
}

// 初始化同步系统
void MainWindow::setupSyncSystem()
{
    networkManager = new QNetworkAccessManager(this);
    syncSettings = new QSettings("MarkdownNotes", "SyncConfig", this);
    syncConfigured = false;
    isSyncing = false;

    // 连接网络回复信号
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::onNetworkReplyFinished);

    // 加载同步设置
    loadSyncSettings();
}

// 加载同步设置
void MainWindow::loadSyncSettings()
{
    webdavUrl = syncSettings->value("webdav/url").toString();
    webdavUsername = syncSettings->value("webdav/username").toString();
    webdavPassword = syncSettings->value("webdav/password").toString();
    remoteBasePath = syncSettings->value("webdav/remote_path", "MarkdownNotes/").toString(); // 去掉开头的斜杠

    // 确保webdavUrl以斜杠结尾，remoteBasePath不以斜杠开头
    if (!webdavUrl.endsWith('/')) {
        webdavUrl += '/';
    }
    if (remoteBasePath.startsWith('/')) {
        remoteBasePath = remoteBasePath.mid(1); // 去掉开头的斜杠
    }
    if (!remoteBasePath.endsWith('/')) {
        remoteBasePath += '/';
    }

    syncConfigured = !webdavUrl.isEmpty() && !webdavUsername.isEmpty() && !webdavPassword.isEmpty();
}

// 保存同步设置
void MainWindow::saveSyncSettings()
{
    syncSettings->setValue("webdav/url", webdavUrl);
    syncSettings->setValue("webdav/username", webdavUsername);
    syncSettings->setValue("webdav/password", webdavPassword);
    syncSettings->setValue("webdav/remote_path", remoteBasePath);
    syncSettings->sync();

    syncConfigured = !webdavUrl.isEmpty() && !webdavUsername.isEmpty() && !webdavPassword.isEmpty();
}

// 同步设置槽函数
void MainWindow::on_actionSyncSettings_triggered()
{
    bool ok;
    QString url = QInputDialog::getText(this, tr("同步设置"),
                                        tr("WebDAV服务器地址:\n(例如: https://dav.jianguoyun.com/dav)"), // 去掉结尾的斜杠
                                        QLineEdit::Normal, webdavUrl, &ok);
    if (!ok) return;

    QString username = QInputDialog::getText(this, tr("同步设置"),
                                             tr("用户名:"),
                                             QLineEdit::Normal, webdavUsername, &ok);
    if (!ok) return;

    QString password = QInputDialog::getText(this, tr("同步设置"),
                                             tr("密码:"),
                                             QLineEdit::Password, webdavPassword, &ok);
    if (!ok) return;

    QString remotePath = QInputDialog::getText(this, tr("同步设置"),
                                               tr("远程路径:\n(例如: MarkdownNotes/)"), // 去掉开头的斜杠
                                               QLineEdit::Normal, remoteBasePath, &ok);
    if (!ok) return;

    webdavUrl = url;
    webdavUsername = username;
    webdavPassword = password;
    remoteBasePath = remotePath;

    // 确保格式正确
    if (!webdavUrl.endsWith('/')) {
        webdavUrl += '/';
    }
    if (remoteBasePath.startsWith('/')) {
        remoteBasePath = remoteBasePath.mid(1); // 去掉开头的斜杠
    }
    if (!remoteBasePath.endsWith('/')) {
        remoteBasePath += '/';
    }

    saveSyncSettings();

    if (syncConfigured) {
        QMessageBox::information(this, tr("成功"), tr("同步设置已保存！"));
    }
}

// 开始同步槽函数
void MainWindow::on_actionSync_triggered()
{
    if (!syncConfigured) {
        QMessageBox::warning(this, tr("警告"),
                             tr("请先配置同步设置！\n\n点击\"同步设置\"配置WebDAV服务器信息。"));
        return;
    }

    if (isSyncing) {
        QMessageBox::information(this, tr("提示"), tr("同步正在进行中，请稍候..."));
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("开始同步"),
                                  tr("确定要开始同步吗？这将上传本地笔记到WebDAV服务器。"),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        syncFiles();
    }
}

// 同步文件
void MainWindow::syncFiles()
{
    if (isSyncing) return;

    statusBar()->showMessage(tr("开始同步..."));
    isSyncing = true;
    successfulUploads = 0;
    failedUploads = 0;
    uploadQueue.clear();

    // 检查resources目录
    QDir resourcesDir(resourcesPath);
    if (!resourcesDir.exists()) {
        QMessageBox::warning(this, tr("错误"), tr("资源文件夹不存在！"));
        isSyncing = false;
        return;
    }

    // 遍历所有笔记文件夹和文件
    QStringList noteFolders = resourcesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    // 首先收集所有需要创建的目录
    QSet<QString> directoriesToCreate;
    directoriesToCreate.insert(remoteBasePath); // 根目录

    for (const QString &noteFolder : noteFolders) {
        QString notePath = resourcesPath + "/" + noteFolder;
        QDir noteDir(notePath);

        // 添加笔记目录
        QString remoteNoteDir = remoteBasePath + noteFolder + "/";
        directoriesToCreate.insert(remoteNoteDir);

        // 检查并添加assets文件夹
        QString assetsPath = notePath + "/assets";
        QDir assetsDir(assetsPath);
        if (assetsDir.exists()) {
            // 添加assets目录
            QString remoteAssetsDir = remoteNoteDir + "assets/";
            directoriesToCreate.insert(remoteAssetsDir);

            // 上传assets文件夹中的文件
            QStringList assetFiles = assetsDir.entryList(QDir::Files);
            for (const QString &assetFile : assetFiles) {
                QString localAssetPath = assetsPath + "/" + assetFile;
                QString remoteAssetPath = getRemotePath(localAssetPath);
                uploadQueue.append(qMakePair(localAssetPath, remoteAssetPath));
                qDebug() << "添加assets文件到上传队列:" << localAssetPath << "->" << remoteAssetPath;
            }
        }

        // 上传笔记文件夹中的所有文件
        QStringList files = noteDir.entryList(QDir::Files);
        for (const QString &file : files) {
            QString localFilePath = notePath + "/" + file;
            QString remoteFilePath = getRemotePath(localFilePath);
            uploadQueue.append(qMakePair(localFilePath, remoteFilePath));
            qDebug() << "添加笔记文件到上传队列:" << localFilePath << "->" << remoteFilePath;
        }
    }

    if (uploadQueue.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("没有找到需要同步的文件。"));
        isSyncing = false;
        return;
    }

    qDebug() << "需要创建的目录:" << directoriesToCreate;
    qDebug() << "需要上传的文件数量:" << uploadQueue.size();

    // 按路径长度排序，确保父目录先创建
    QList<QString> sortedDirectories = directoriesToCreate.values();
    std::sort(sortedDirectories.begin(), sortedDirectories.end(),
              [](const QString &a, const QString &b) { return a.length() < b.length(); });

    directoriesToCreateCount = sortedDirectories.size();
    directoriesCreatedCount = 0;

    qDebug() << "需要创建" << directoriesToCreateCount << "个目录";

    // 创建所有必要的目录
    for (const QString &dir : sortedDirectories) {
        createRemoteDirectory(dir);
    }

    // 注意：文件上传会在目录创建完成后自动开始（在onNetworkReplyFinished中）
}

// 获取远程路径
QString MainWindow::getRemotePath(const QString &localPath)
{
    QString relativePath = QDir(resourcesPath).relativeFilePath(localPath);
    // 将Windows路径分隔符转换为Unix风格
    relativePath.replace('\\', '/');

    // 确保路径正确
    QString remotePath = remoteBasePath + relativePath;
    qDebug() << "计算远程路径 - 本地:" << localPath << "相对:" << relativePath << "远程:" << remotePath;

    return remotePath;
}

// 创建远程目录
void MainWindow::createRemoteDirectory(const QString &remotePath)
{
    QUrl url(webdavUrl + remotePath);
    QNetworkRequest request(url);

    // 设置认证
    QString auth = webdavUsername + ":" + webdavPassword;
    QByteArray authData = auth.toUtf8().toBase64();
    request.setRawHeader("Authorization", "Basic " + authData);

    // WebDAV MKCOL方法用于创建目录
    QNetworkReply *reply = networkManager->sendCustomRequest(request, "MKCOL");

    // 设置用户属性以便在回复处理中识别
    reply->setProperty("operation", "createDir");
    reply->setProperty("remotePath", remotePath);
}

// 上传文件
void MainWindow::uploadFile(const QString &localPath, const QString &remotePath)
{
    qDebug() << "开始上传文件 - 本地:" << localPath << "远程:" << remotePath;

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件:" << localPath;
        failedUploads++;
        processNextUpload();
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QUrl url(webdavUrl + remotePath);
    QNetworkRequest request(url);

    // 设置认证
    QString auth = webdavUsername + ":" + webdavPassword;
    QByteArray authData = auth.toUtf8().toBase64();
    request.setRawHeader("Authorization", "Basic " + authData);
    request.setRawHeader("Content-Type", "application/octet-stream");

    qDebug() << "发送PUT请求到:" << url.toString();

    // 发送PUT请求
    QNetworkReply *reply = networkManager->put(request, fileData);

    // 设置用户属性以便在回复处理中识别
    reply->setProperty("operation", "uploadFile");
    reply->setProperty("localPath", localPath);
    reply->setProperty("remotePath", remotePath);

    statusBar()->showMessage(tr("正在上传: %1").arg(QFileInfo(localPath).fileName()));
}

// 处理下一个上传
void MainWindow::processNextUpload()
{
    if (uploadQueue.isEmpty()) {
        // 所有文件处理完成
        showSyncResult();
        return;
    }

    QPair<QString, QString> nextFile = uploadQueue.takeFirst();
    uploadFile(nextFile.first, nextFile.second);
}

// 网络回复处理
void MainWindow::onNetworkReplyFinished(QNetworkReply *reply)
{
    QString operation = reply->property("operation").toString();
    QString remotePath = reply->property("remotePath").toString();
    QString localPath = reply->property("localPath").toString();

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    QVariant reasonPhrase = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);

    qDebug() << "网络回复 - 操作:" << operation
             << "路径:" << remotePath
             << "状态码:" << statusCode.toInt()
             << "原因:" << reasonPhrase.toString()
             << "错误:" << reply->errorString();

    if (reply->error() == QNetworkReply::NoError) {
        if (operation == "createDir") {
            qDebug() << "成功创建目录:" << remotePath;
            directoriesCreatedCount++;

            // 检查是否所有目录都已创建
            if (directoriesCreatedCount >= directoriesToCreateCount) {
                qDebug() << "所有目录创建完成，开始上传文件";
                processNextUpload();
            }
        } else if (operation == "uploadFile") {
            qDebug() << "成功上传文件:" << localPath << "到" << remotePath;
            successfulUploads++;
            processNextUpload();
        }
    } else {
        qDebug() << "操作失败:" << operation << reply->errorString();

        if (operation == "createDir") {
            directoriesCreatedCount++;

            // 如果目录已存在（409 Conflict），也算成功
            if (statusCode.toInt() == 409) {
                qDebug() << "目录已存在:" << remotePath;
            }

            // 检查是否所有目录都已处理
            if (directoriesCreatedCount >= directoriesToCreateCount) {
                qDebug() << "所有目录处理完成，开始上传文件";
                processNextUpload();
            }
        } else if (operation == "uploadFile") {
            failedUploads++;
            qDebug() << "上传失败:" << localPath << reply->errorString();
            processNextUpload();
        }
    }

    reply->deleteLater();
}

// 显示同步结果
void MainWindow::showSyncResult()
{
    isSyncing = false;

    QString message;
    if (failedUploads == 0) {
        message = tr("同步完成！成功上传 %1 个文件").arg(successfulUploads);
        QMessageBox::information(this, tr("成功"), message);
    } else {
        message = tr("同步完成，但有错误！\n成功: %1, 失败: %2")
                      .arg(successfulUploads).arg(failedUploads);
        QMessageBox::warning(this, tr("警告"), message);
    }

    statusBar()->showMessage(message, 5000);
}

// 文本格式化辅助函数
void MainWindow::formatSelectedText(const QString &prefix, const QString &suffix)
{
    QTextCursor cursor = ui->markdownEditor->textCursor();

    if (cursor.hasSelection()) {
        // 获取选中的文本
        QString selectedText = cursor.selectedText();

        // 在选中的文本前后添加格式符号
        QString formattedText = prefix + selectedText + suffix;

        // 替换选中的文本
        cursor.insertText(formattedText);
    } else {
        // 如果没有选中文本，插入格式符号并将光标放在中间
        cursor.insertText(prefix + suffix);

        // 移动光标到两个符号之间
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, suffix.length());

        // 设置新的光标位置
        ui->markdownEditor->setTextCursor(cursor);
    }
}

// 加粗动作
void MainWindow::on_actionBolder_triggered()
{
    QTextCursor cursor = ui->markdownEditor->textCursor();
    // 如果没有选中文本，直接返回，不做任何操作
    if (!cursor.hasSelection()) {
        return;
    }
    formatSelectedText("**", "**");
}

// 斜体动作
void MainWindow::on_actionItalics_triggered()
{
    QTextCursor cursor = ui->markdownEditor->textCursor();
    // 如果没有选中文本，直接返回，不做任何操作
    if (!cursor.hasSelection()) {
        return;
    }
    formatSelectedText("*", "*");
}

// 下划线动作（Markdown中没有直接的下划线语法，使用HTML标签）
void MainWindow::on_actionUnderline_triggered()
{
    QTextCursor cursor = ui->markdownEditor->textCursor();
    // 如果没有选中文本，直接返回，不做任何操作
    if (!cursor.hasSelection()) {
        return;
    }
    formatSelectedText("<u>", "</u>");
}

// 复制动作
void MainWindow::on_actionCopy_triggered()
{
    ui->markdownEditor->copy();
}

// 粘贴动作
void MainWindow::on_actionPaste_triggered()
{
    ui->markdownEditor->paste();
}

// 剪切动作
void MainWindow::on_actionCut_triggered()
{
    ui->markdownEditor->cut();
}

// 关于动作
/*void MainWindow::on_actionAbout_triggered()
{
    // 创建关于对话框
    QDialog *aboutDialog = new QDialog(this);
    aboutDialog->setWindowTitle(tr("关于 Markdown Notes"));
    aboutDialog->setFixedSize(370, 220);

    // 设置对话框布局
    QVBoxLayout *layout = new QVBoxLayout(aboutDialog);

    // 添加程序图标和名称
    QLabel *titleLabel = new QLabel(tr("Markdown Notes\n2025信息科学技术学院校级大创项目"), aboutDialog);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);

    // 添加版本信息
    QLabel *versionLabel = new QLabel(tr("版本 1.0"), aboutDialog);
    versionLabel->setAlignment(Qt::AlignCenter);

    // 添加描述信息
    QLabel *descLabel = new QLabel(tr("一个简单的Markdown笔记应用程序\n\n"
                                      "支持Markdown编辑和预览\n"
                                      "支持图片拖拽和PDF查看"), aboutDialog);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);

    // 添加版权信息
    QLabel *copyrightLabel = new QLabel(tr("© 2025 徐以浩 范晨扬 李鹏宇 杨翔 次晋"), aboutDialog);
    copyrightLabel->setAlignment(Qt::AlignCenter);

    // 将标签添加到布局
    layout->addWidget(titleLabel);
    layout->addWidget(versionLabel);
    layout->addWidget(descLabel);
    layout->addWidget(copyrightLabel);

    // 显示对话框
    aboutDialog->exec();

    // 清理内存
    aboutDialog->deleteLater();
}
*/
// 新增函数：初始化/创建 resources 文件夹并加载笔记列表
void MainWindow::setupResourcesAndLoadNotes()
{
    // 1. 获取程序可执行文件所在目录，并确定 resources 文件夹的路径
    resourcesPath = QCoreApplication::applicationDirPath() + "/resources";
    QDir resourcesDir(resourcesPath);

    // 2. 检查路径是否存在，如果不存在则创建
    if (!resourcesDir.exists()) {
        // mkpath可以递归创建路径，比mkdir更安全
        if (!resourcesDir.mkpath(".")) {
            QMessageBox::critical(this, tr("错误"), tr("无法创建笔记存储文件夹: %1").arg(resourcesPath));
            return;
        }
    }

    // 3. 刷新笔记列表
    ui->listWidget->clear(); // 清空旧列表
    // 设置过滤，只查找目录
    resourcesDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    // 获取所有子目录的名称列表
    QStringList noteList = resourcesDir.entryList();
    // 将笔记名称添加到 listWidget
    ui->listWidget->addItems(noteList);
}

// 新增槽函数：处理 listWidget 列表项的双击事件
void MainWindow::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    /*
    // 如果当前文件已修改，则提示用户保存
    if (maybeSave()) {
        // 加载被双击的笔记
        loadNote(item->text());
    }
    */


    // 保存当前笔记名称，以便在保存后加载
    QString targetNoteName = item->text();


    // 检查是否需要保存当前笔记
    if (!maybeSave()) {
        return; // 用户取消了操作
    }

    // 加载新笔记
    loadNote(targetNoteName);

}

// 新增槽函数：处理 listWidget_details 列表项的双击事件
void MainWindow::on_listWidget_details_itemDoubleClicked(QListWidgetItem *item)
{

    QString fileName = item->text();
    QString filePath = resourcesPath + "/" + currentNoteName + "/" + fileName;
    qDebug() << "[DEBUG] PDF double-clicked:" << filePath;
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();

    if (suffix == "md" || suffix == "markdown") {
        // 如果是Markdown文件，检查是否需要保存当前笔记
        if (!maybeSave()) {
            return;
        }

        // 加载Markdown文件
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
            QMessageBox::warning(this, tr("警告"), tr("无法打开文件: %1\n错误: %2").arg(fileName, file.errorString()));
            return;
        }

        // 屏蔽 textChanged 信号
        disconnect(ui->markdownEditor, &QTextEdit::textChanged,
                   this, &MainWindow::on_markdownEditor_textChanged);

        // 设置当前文件路径
        setCurrentFile(filePath);

        // 读取文件内容
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        // 设置文本内容
        ui->markdownEditor->setPlainText(content);

        // 设置窗口为未修改状态
        setWindowModified(false);

        // 恢复信号连接
        connect(ui->markdownEditor, &QTextEdit::textChanged,
                this, &MainWindow::on_markdownEditor_textChanged);

        statusBar()->showMessage(tr("文档 '%1' 已加载").arg(fileName), 2000);
    }
    else if (suffix == "pdf") {
        // 如果是PDF文件，打开PDF查看器
        openPdfFile(filePath);
    }
    else {
        QMessageBox::information(this, tr("信息"), tr("不支持的文件格式: %1").arg(suffix));
    }
}

// 新增函数：更新详情列表，显示当前笔记文件夹下的文档
void MainWindow::updateDetailsList(const QString &noteName)
{
    ui->listWidget_details->clear();
    currentNoteName = noteName;

    QString notePath = resourcesPath + "/" + noteName;
    QDir noteDir(notePath);

    if (!noteDir.exists()) {
        return;
    }

    // 设置过滤器，查找.md和.pdf文件
    QStringList filters;
    filters << "*.md" << "*.markdown" << "*.pdf";
    noteDir.setNameFilters(filters);
    noteDir.setFilter(QDir::Files);

    QStringList fileList = noteDir.entryList();

    // 为不同类型的文件设置不同的图标或显示方式
    for (const QString &fileName : fileList) {
        QListWidgetItem *item = new QListWidgetItem(fileName, ui->listWidget_details);
        QFileInfo fileInfo(fileName);
        QString suffix = fileInfo.suffix().toLower();

        // 可以根据文件类型设置不同的图标
        if (suffix == "pdf") {
            item->setForeground(Qt::blue);
            item->setToolTip(tr("PDF文档"));
        } else {
            item->setToolTip(tr("Markdown文档"));
        }
    }
}

// 新增函数：打开PDF文件
void MainWindow::openPdfFile(const QString &filePath)
{
    qDebug() << "[DEBUG] openPdfFile called with:" << filePath;
    PdfViewer *pdfViewer = new PdfViewer(this);
    pdfViewer->setWindowTitle(QString("PDF查看器 - %1").arg(QFileInfo(filePath).fileName()));

    if (pdfViewer->loadPdf(filePath)) {
        pdfViewer->show();
    } else {
        QMessageBox::warning(this, tr("错误"), tr("无法打开PDF文件: %1").arg(filePath));
        delete pdfViewer;
    }
}

// 新增函数：根据笔记名称加载笔记
void MainWindow::loadNote(const QString &noteName)
{
    /*
    // 假设笔记文件名为 "笔记名.md"，存放在 "resources/笔记名/" 目录下
    QString filePath = resourcesPath + "/" + noteName + "/" + noteName + ".md";

    QFile file(filePath);

    // 检查文件是否存在且可读
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("警告"), tr("无法打开笔记文件: %1\n错误: %2").arg(QFileInfo(filePath).fileName(), file.errorString()));
        return;
    }

    // 设置当前文件路径，这对于图片等相对路径的正确显示至关重要
    setCurrentFile(filePath);

    // 读取文件内容并设置到编辑器中
    QTextStream in(&file);
    ui->markdownEditor->setPlainText(in.readAll());
    file.close();

    statusBar()->showMessage(tr("笔记 '%1' 已加载").arg(noteName), 2000);

    */

    QString filePath = resourcesPath + "/" + noteName + "/" + noteName + ".md";

    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("警告"), tr("无法打开笔记文件: %1\n错误: %2").arg(QFileInfo(filePath).fileName(), file.errorString()));
        return;
    }

    // 完全屏蔽 textChanged 信号，直到加载完成
    disconnect(ui->markdownEditor, &QTextEdit::textChanged,
               this, &MainWindow::on_markdownEditor_textChanged);

    // 设置当前文件路径
    setCurrentFile(filePath);

    // 读取文件内容
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // 设置文本内容（此时信号被屏蔽，不会触发on_markdownEditor_textChanged）
    ui->markdownEditor->setPlainText(content);

    // 立即设置窗口为未修改状态
    setWindowModified(false);

    // 恢复信号连接
    connect(ui->markdownEditor, &QTextEdit::textChanged,
            this, &MainWindow::on_markdownEditor_textChanged);

    // 更新详情列表
    updateDetailsList(noteName);

    statusBar()->showMessage(tr("笔记 '%1' 已加载").arg(noteName), 2000);
}

// 当图片被拖放到编辑器时，这个槽会被调用
void MainWindow::onImageDropped(const QMimeData *mime, const QPoint &position)
{
    // 根据鼠标位置获取光标
    QTextCursor cursor = ui->markdownEditor->cursorForPosition(position);
    handleDroppedImage(mime, cursor);
}

// 处理图片的核心逻辑（与之前类似，但现在能被正确调用）
void MainWindow::handleDroppedImage(const QMimeData *mime, QTextCursor &cursor)
{
    qDebug() << "handleDroppedImage called";

    if (currentFilePath.isEmpty()) {
        qDebug() << "Current file path is empty, prompting to save";
        QMessageBox::warning(this, tr("请先保存"), tr("在添加图片前，请先保存当前的 Markdown 文件。"));
        if (!saveFile()) {
            return;
        }
    }

    QString sourceImagePath;
    QImage image;
    QString baseName;

    if (mime->hasUrls()) {
        sourceImagePath = mime->urls().first().toLocalFile();
        baseName = QFileInfo(sourceImagePath).fileName();
    } else if (mime->hasImage()) {
        image = qvariant_cast<QImage>(mime->imageData());
        baseName = QString("paste_img_%1.png").arg(QDateTime::currentMSecsSinceEpoch());
    }

    if (sourceImagePath.isEmpty() && image.isNull()) {
        return;
    }

    QFileInfo mdFileInfo(currentFilePath);
    QDir mdDir = mdFileInfo.dir();
    QString assetsFolderName = "assets";
    if (!mdDir.exists(assetsFolderName)) {
        mdDir.mkdir(assetsFolderName);
    }

    QString destinationPath = mdDir.filePath(assetsFolderName + QDir::separator() + baseName);

    if (QFile::exists(destinationPath)) {
        QString originalBaseName = QFileInfo(destinationPath).baseName();
        QString suffix = QFileInfo(destinationPath).suffix();
        int counter = 1;
        do {
            baseName = QString("%1_%2.%3").arg(originalBaseName).arg(counter++).arg(suffix);
            destinationPath = mdDir.filePath(assetsFolderName + QDir::separator() + baseName);
        } while (QFile::exists(destinationPath));
    }

    bool success = false;
    if (!sourceImagePath.isEmpty()) {
        success = QFile::copy(sourceImagePath, destinationPath);
    } else if (!image.isNull()) {
        success = image.save(destinationPath);
    }

    if (success) {
        QString relativePath = QDir(mdFileInfo.path()).relativeFilePath(destinationPath);
        relativePath.replace('\\', '/');
        cursor.insertText(QString("\n![%1](%2)\n").arg(QFileInfo(baseName).baseName(), relativePath));
    } else {
        QMessageBox::warning(this, tr("错误"), tr("无法将图片保存到 assets 文件夹。"));
    }
}

QString MainWindow::processImagesForPreview(const QString &markdownText)
{
    // 简单的正则表达式匹配 Markdown 图片语法
    QRegularExpression imagePattern("!\\[([^\\]]*)\\]\\(([^)]+)\\)");
    QString result = markdownText;

    // 替换所有图片为带样式的 HTML
    result.replace(imagePattern, "<img src=\"\\2\" alt=\"\\1\" style=\"max-width: 10px; height: auto;\">");

    // 将剩余的 Markdown 转换为 HTML
    // 这里需要手动实现或使用其他库，因为 setMarkdown 会覆盖我们的处理

    return result;
}

// 当左侧编辑器文本变化时，更新右侧的预览
void MainWindow::on_markdownEditor_textChanged()
{
    // 停止之前的定时器，重新开始延迟
    previewTimer->stop();
    previewTimer->start();

    setWindowModified(true);
}

// 延迟预览更新函数
void MainWindow::updatePreview()
{
    QString markdownText = ui->markdownEditor->toPlainText();

    if (markdownText.contains('$')) {
        // 使用改进的数学渲染器
        QString html = mathRenderer->renderMarkdownWithMath(markdownText);

        // 添加CSS样式来美化数学公式
        QString styledHtml = QString(
                                 "<style>"
                                 ".math-block {"
                                 "  text-align: center;"
                                 "  margin: 1em 0;"
                                 "  padding: 0.5em;"
                                 "  border: 1px solid #ccc;"
                                 "  background: #f9f9f9;"
                                 "  font-family: 'Cambria Math', 'DejaVu Math', serif;"
                                 "  font-size: 12pt;"
                                 "}"
                                 ".math-inline {"
                                 "  font-family: 'Cambria Math', 'DejaVu Math', serif;"
                                 "  background: #f0f0f0;"
                                 "  padding: 0.1em 0.3em;"
                                 "  border-radius: 3px;"
                                 "  font-size: 16pt;"
                                 "}"
                                 "</style>"
                                 "%1"
                                 ).arg(html);

        ui->htmlPreview->setHtml(styledHtml);
    } else {
        // 如果没有数学公式，使用Qt内置Markdown渲染
        ui->htmlPreview->setMarkdown(markdownText);
    }
}

// --- 文件操作和核心逻辑 (包含问题3的修复) ---

void MainWindow::setCurrentFile(const QString &filePath)
{
    currentFilePath = filePath;
    setWindowModified(false);

    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        QString basePath = fileInfo.absolutePath();

        // 设置预览窗口的搜索路径
        ui->htmlPreview->setSearchPaths({basePath});

        // 如果使用HTML渲染，需要设置基础URL
        QUrl baseUrl = QUrl::fromLocalFile(basePath + "/");
        ui->htmlPreview->document()->setBaseUrl(baseUrl);
    } else {
        ui->htmlPreview->setSearchPaths({});
        ui->htmlPreview->document()->setBaseUrl(QUrl());
    }

    updateWindowTitle();
}
////////////////////////////////////////////////
void MainWindow::on_actionNew_triggered()
{
    newFile();
}

void MainWindow::on_actionOpen_triggered()
{
    if (maybeSave()) {
        openFile();
    }
}

void MainWindow::on_actionSave_triggered()
{
    saveFile();
}

void MainWindow::on_actionSaveAs_triggered()
{
    saveFileAs();
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

/////////////////////////////////////////////

void MainWindow::openFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("打开文件"), "", tr("Markdown 文件 (*.md *.markdown);;所有文件 (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("警告"), tr("无法打开文件: %1").arg(file.errorString()));
        return;
    }

    setCurrentFile(filePath); // 先设置路径，让预览器知道基准

    QTextStream in(&file);
    ui->markdownEditor->setPlainText(in.readAll()); // 后加载文本，触发 on_markdownEditor_textChanged
    file.close();

    statusBar()->showMessage(tr("文件已加载"), 2000);
}

// ... 其他函数 (newFile, saveFile, saveFileAs, maybeSave, updateWindowTitle, closeEvent) 保持不变即可 ...
// 为了完整性，这里贴出所有函数

void MainWindow::newFile()
{

    ui->markdownEditor->clear();
    setCurrentFile(QString());
    ui->listWidget_details->clear();
    currentNoteName.clear();
    // 重置预览
    ui->htmlPreview->clear();
}

bool MainWindow::saveFile()
{
    if (currentFilePath.isEmpty()) {
        return saveFileAs();
    } else {
        QFile file(currentFilePath);
        if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
            QMessageBox::warning(this, tr("警告"), tr("无法保存文件: %1").arg(file.errorString()));
            return false;
        }

        QTextStream out(&file);
        out << ui->markdownEditor->toPlainText();
        file.close();

        setWindowModified(false);
        statusBar()->showMessage(tr("文件已保存"), 2000);
        setupResourcesAndLoadNotes();

        // 如果当前有选中的笔记，更新详情列表
        if (!currentNoteName.isEmpty()) {
            updateDetailsList(currentNoteName);
        }
        return true;
    }
}

bool MainWindow::saveFileAs()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("另存为"), "", tr("Markdown 文件 (*.md *.markdown);;所有文件 (*.*)"));
    if (filePath.isEmpty()) {
        return false;
    }
    setCurrentFile(filePath);
    return saveFile();
}

bool MainWindow::maybeSave()
{
    if (!isWindowModified()) {
        return true;
    }

    const QMessageBox::StandardButton ret =
        QMessageBox::warning(this, tr("Markdown Notes"),
                             tr("文档已被修改。\n"
                                "你想保存你的更改吗?"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret) {
    case QMessageBox::Save:
        return saveFile();
    case QMessageBox::Cancel:
        return false;
    default:
        break;
    }
    return true;
}

void MainWindow::updateWindowTitle()
{
    QString shownName = currentFilePath.isEmpty() ? "未命名.md" : QFileInfo(currentFilePath).fileName();
    setWindowTitle(QString("%1[*] - %2").arg(shownName, QCoreApplication::applicationName()));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}


