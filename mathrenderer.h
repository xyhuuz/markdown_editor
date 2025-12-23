// mathrenderer.h
#ifndef MATHRENDERER_H
#define MATHRENDERER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QTextDocument>

class MathRenderer : public QObject
{
    Q_OBJECT

public:
    explicit MathRenderer(QObject *parent = nullptr);
    QString renderMarkdownWithMath(const QString &markdownText);

private:
    void initializeSymbols();
    QString convertLaTeXToUnicode(const QString &latex);
    QString processFractions(const QString &latex);  // 新增：处理分数
    QString processSquareRoots(const QString &latex);  // 新增：处理平方根
    QString renderMathBlock(const QString &latex);
    QString renderMathInline(const QString &latex);

    QMap<QString, QString> m_symbols;
};

#endif // MATHRENDERER_H
