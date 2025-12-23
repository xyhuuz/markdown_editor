// mathrenderer.cpp
#include "mathrenderer.h"
#include <QRegularExpression>
#include <QTextDocument>
#include <QStack>

MathRenderer::MathRenderer(QObject *parent)
    : QObject(parent)
{
    initializeSymbols();
}

void MathRenderer::initializeSymbols()
{
    // 基本希腊字母
    m_symbols["\\alpha"] = "α";
    m_symbols["\\beta"] = "β";
    m_symbols["\\gamma"] = "γ";
    m_symbols["\\delta"] = "δ";
    m_symbols["\\epsilon"] = "ε";
    m_symbols["\\zeta"] = "ζ";
    m_symbols["\\eta"] = "η";
    m_symbols["\\theta"] = "θ";
    m_symbols["\\iota"] = "ι";
    m_symbols["\\kappa"] = "κ";
    m_symbols["\\lambda"] = "λ";
    m_symbols["\\mu"] = "μ";
    m_symbols["\\nu"] = "ν";
    m_symbols["\\xi"] = "ξ";
    m_symbols["\\pi"] = "π";
    m_symbols["\\rho"] = "ρ";
    m_symbols["\\sigma"] = "σ";
    m_symbols["\\tau"] = "τ";
    m_symbols["\\upsilon"] = "υ";
    m_symbols["\\phi"] = "φ";
    m_symbols["\\chi"] = "χ";
    m_symbols["\\psi"] = "ψ";
    m_symbols["\\omega"] = "ω";

    // 大写希腊字母
    m_symbols["\\Gamma"] = "Γ";
    m_symbols["\\Delta"] = "Δ";
    m_symbols["\\Theta"] = "Θ";
    m_symbols["\\Lambda"] = "Λ";
    m_symbols["\\Xi"] = "Ξ";
    m_symbols["\\Pi"] = "Π";
    m_symbols["\\Sigma"] = "Σ";
    m_symbols["\\Upsilon"] = "Υ";
    m_symbols["\\Phi"] = "Φ";
    m_symbols["\\Psi"] = "Ψ";
    m_symbols["\\Omega"] = "Ω";

    // 数学符号
    m_symbols["\\infty"] = "∞";
    m_symbols["\\partial"] = "∂";
    m_symbols["\\nabla"] = "∇";
    m_symbols["\\sum"] = "∑";
    m_symbols["\\prod"] = "∏";
    m_symbols["\\int"] = "∫";
    m_symbols["\\pm"] = "±";
    m_symbols["\\mp"] = "∓";
    m_symbols["\\times"] = "×";
    m_symbols["\\div"] = "÷";
    m_symbols["\\cdot"] = "·";
    m_symbols["\\leq"] = "≤";
    m_symbols["\\geq"] = "≥";
    m_symbols["\\neq"] = "≠";
    m_symbols["\\approx"] = "≈";
    m_symbols["\\propto"] = "∝";
    m_symbols["\\in"] = "∈";
    m_symbols["\\notin"] = "∉";
    m_symbols["\\subset"] = "⊂";
    m_symbols["\\supset"] = "⊃";
    m_symbols["\\cup"] = "∪";
    m_symbols["\\cap"] = "∩";
    m_symbols["\\wedge"] = "∧";
    m_symbols["\\vee"] = "∨";
    m_symbols["\\neg"] = "¬";
    m_symbols["\\forall"] = "∀";
    m_symbols["\\exists"] = "∃";
    m_symbols["\\emptyset"] = "∅";

    // 新增：分数相关符号
    m_symbols["\\frac"] = ""; // 特殊处理
    //新增：平方根符号
    m_symbols["\\sqrt"] = ""; // 特殊处理
    m_symbols["\\surd"] = "√";
}

QString MathRenderer::renderMarkdownWithMath(const QString &markdownText)
{
    QString result = markdownText;

    // 首先渲染所有数学公式
    // 处理块级公式 $$...$$
    static QRegularExpression blockPattern(R"(\$\$(.*?)\$\$)");
    QRegularExpressionMatchIterator blockIt = blockPattern.globalMatch(result);

    while (blockIt.hasNext()) {
        QRegularExpressionMatch match = blockIt.next();
        QString latex = match.captured(1).trimmed();
        QString rendered = renderMathBlock(latex);
        result.replace(match.captured(0), rendered);
    }

    // 处理行内公式 $...$
    static QRegularExpression inlinePattern(R"(\$(.*?)\$)");
    QRegularExpressionMatchIterator inlineIt = inlinePattern.globalMatch(result);

    while (inlineIt.hasNext()) {
        QRegularExpressionMatch match = inlineIt.next();
        QString latex = match.captured(1).trimmed();
        QString rendered = renderMathInline(latex);
        result.replace(match.captured(0), rendered);
    }

    // 现在将剩余的Markdown文本转换为HTML
    QTextDocument doc;
    doc.setMarkdown(result);

    QString html = doc.toHtml();
    /* 把样式写进 <head> */
    html.insert(html.indexOf("<head>") + 6,
                R"(<style>
                 body { font-family: "Microsoft YaHei"; font-size: 14pt; color: #333; }
               </style>)");
    return html;
}

QString MathRenderer::convertLaTeXToUnicode(const QString &latex)
{
    QString result = latex;

    // 先处理平方根（需要优先处理，因为平方根内可能包含其他符号）
    result = processSquareRoots(result);

    // 先处理分数（需要优先处理，因为分数可能包含其他符号）
    result = processFractions(result);

    // 替换已知符号
    for (auto it = m_symbols.begin(); it != m_symbols.end(); ++it) {
        if (!it.value().isEmpty()) { // 跳过需要特殊处理的命令（如\frac）
            result.replace(it.key(), it.value());
        }
    }

    // 处理上下标
    // 上标
    result.replace("^2", "²");
    result.replace("^3", "³");
    result.replace("^1", "¹");
    result.replace("^0", "⁰");
    result.replace("^n", "ⁿ");

    // 下标
    result.replace("_1", "₁");
    result.replace("_2", "₂");
    result.replace("_0", "₀");
    result.replace("_n", "ₙ");

    // 处理更复杂的上下标模式
    static QRegularExpression superscriptPattern(R"(\^\{([^}]+)\})");
    QRegularExpressionMatchIterator supIt = superscriptPattern.globalMatch(result);
    while (supIt.hasNext()) {
        QRegularExpressionMatch match = supIt.next();
        QString exp = match.captured(1);
        // 这里可以添加更多上标字符映射
        if (exp == "2") result.replace(match.captured(0), "²");
        else if (exp == "3") result.replace(match.captured(0), "³");
        // 对于无法映射的情况，保持原样
    }

    static QRegularExpression subscriptPattern(R"(_\{([^}]+)\})");
    QRegularExpressionMatchIterator subIt = subscriptPattern.globalMatch(result);
    while (subIt.hasNext()) {
        QRegularExpressionMatch match = subIt.next();
        QString exp = match.captured(1);
        // 这里可以添加更多下标字符映射
        if (exp == "1") result.replace(match.captured(0), "₁");
        else if (exp == "2") result.replace(match.captured(0), "₂");
        // 对于无法映射的情况，保持原样
    }

    return result;
}

// 新增：处理平方根
QString MathRenderer::processSquareRoots(const QString &latex)
{
    QString result = latex;

    // 匹配 \sqrt[次数]{被开方数}
    static QRegularExpression sqrtWithIndexPattern(R"(\\sqrt\[([^\[\]]*)\]\{([^{}]*)\})");
    QRegularExpressionMatchIterator sqrtWithIndexIt = sqrtWithIndexPattern.globalMatch(result);

    while (sqrtWithIndexIt.hasNext()) {
        QRegularExpressionMatch match = sqrtWithIndexIt.next();
        QString index = match.captured(1);
        QString radicand = match.captured(2);

        // 递归处理指数和被开方数中的其他LaTeX命令
        index = convertLaTeXToUnicode(index);
        radicand = convertLaTeXToUnicode(radicand);

        // 创建带次数的根号显示
        QString sqrtHtml = QString(
                               "<span style=\"display: inline-block; vertical-align: middle; position: relative;\">"
                               "<span style=\"position: absolute; top: -0.5em; left: 0.5em; font-size: 0.7em;\">%1</span>"
                               "<span style=\"border-top: 1px solid; margin-left: 0.8em;\">%2</span>"
                               "<span style=\"position: absolute; left: 0; top: 0; font-size: 1.2em;\">√</span>"
                               "</span>"
                               ).arg(index, radicand);

        result.replace(match.captured(0), sqrtHtml);
    }

    // 匹配 \sqrt{被开方数}（普通平方根）
    static QRegularExpression sqrtPattern(R"(\\sqrt\{([^{}]*)\})");
    QRegularExpressionMatchIterator sqrtIt = sqrtPattern.globalMatch(result);

    while (sqrtIt.hasNext()) {
        QRegularExpressionMatch match = sqrtIt.next();
        QString radicand = match.captured(1);

        // 递归处理被开方数中的其他LaTeX命令
        radicand = convertLaTeXToUnicode(radicand);

        // 创建普通平方根显示
        QString sqrtHtml = QString(
                               "<span style=\"display: inline-block; vertical-align: middle;\">"
                               "<span style=\"border-top: 1px solid; margin-left: 0.5em;\">%1</span>"
                               "<span style=\"margin-left: 0.1em; font-size: 1.2em;\">√</span>"
                               "</span>"
                               ).arg(radicand);

        result.replace(match.captured(0), sqrtHtml);
    }

    return result;
}

// 新增：处理分数
QString MathRenderer::processFractions(const QString &latex)
{
    QString result = latex;

    // 匹配 \frac{分子}{分母}
    static QRegularExpression fracPattern(R"(\\frac\{([^{}]*)\}\{([^{}]*)\})");
    QRegularExpressionMatchIterator fracIt = fracPattern.globalMatch(result);

    while (fracIt.hasNext()) {
        QRegularExpressionMatch match = fracIt.next();
        QString numerator = match.captured(1);
        QString denominator = match.captured(2);

        // 递归处理分子和分母中的其他LaTeX命令
        numerator = convertLaTeXToUnicode(numerator);
        denominator = convertLaTeXToUnicode(denominator);

        // 创建分数显示（使用HTML表格模拟分数布局）
        QString fractionHtml = QString(
                                   "<span style=\"display: inline-block; text-align: center; vertical-align: middle; margin: 0 0.1em;\">"
                                   "<span style=\"display: block; padding: 0 0.1em; border-bottom: 1px solid; font-size: 0.8em;\">%1/</span>"
                                   "<span style=\"display: block; padding: 0 0.1em; font-size: 0.8em;\">%2</span>"
                                   "</span>"
                                   ).arg(numerator, denominator);

        result.replace(match.captured(0), fractionHtml);
    }

    return result;
}

QString MathRenderer::renderMathBlock(const QString &latex)
{
    QString converted = convertLaTeXToUnicode(latex);
    return QString("<div style=\"text-align: center; margin: 1em 0; padding: 0.5em; "
                   "border: 1px solid #ccc; background: #f9f9f9; font-family: 'Microsoft YaHei', '微软雅黑', sans-serif; font-size: 14px; line-height: 1.5;\">"
                   "%1</div>").arg(converted);
}

QString MathRenderer::renderMathInline(const QString &latex)
{
    QString converted = convertLaTeXToUnicode(latex);
    return QString("<span style=\"font-family: 'Microsoft YaHei', '微软雅黑', sans-serif !important; "
                   "background: #f0f0f0 !important; padding: 0.1em 0.3em !important; border-radius: 3px !important; "
                   "font-size: 14px !important; line-height: 1.5 !important;\">"
                   "%1</span>").arg(converted);
}
