#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>  // 新增
#include <QLibraryInfo> // 新增

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Markdown Notes");
    a.setApplicationVersion("1.0");

    MainWindow w;
    w.show();
    return a.exec();
}
