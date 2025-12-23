QT       += core gui widgets pdf pdfwidgets printsupport svg network

CONFIG   += c++20


TARGET = MarkdownNotes
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    markdowneditor.cpp \
    mathrenderer.cpp \
    pdfviewer.cpp

HEADERS += \
    mainwindow.h \
    markdowneditor.h \
    mathrenderer.h \
    pdfviewer.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

TRANSLATIONS += \
    translations/zh_CN.ts \
    translations/en_US.ts \
    translations/bo_CN.ts

# 可选：指定翻译文件目录
TRANSLATIONS_DIR = translations
