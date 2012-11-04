#include <QChar>
#include <QRegExp>
#include <QString>
#include <QStringList>

#include "HTMLPrettyPrint.h"

#include <QtDebug>

HTMLPrettyPrint::HTMLPrettyPrint(const QString &source)
    : m_source(source)
{
    tokenize();
}

HTMLPrettyPrint::~HTMLPrettyPrint()
{
    Q_FOREACH (HTMLToken *t, m_tokens) {
        delete t;
        t = 0;
    }
}

QString HTMLPrettyPrint::prettyPrint()
{
    int level = 0;
    bool in_pre = false;
    QStringList builder;
    QString segment;
    HTMLToken *last_token = 0;

    Q_FOREACH (HTMLToken *token, m_tokens) {
        if (!token) {
            continue;
        }

        segment = m_source.mid(token->start, token->len);
        if (!in_pre) {
            segment = cleanSegement(segment);
        }
        if (segment.isEmpty()) {
            continue;
        }

        if (last_token && (token->type == TOKEN_TYPE_TEXT && last_token->type == TOKEN_TYPE_OPEN_TAG)) {
            level++;
        } else {
            if (token->type == TOKEN_TYPE_CLOSE_TAG) {
                if (last_token && last_token->tag != token->tag) {
                    level--;
                }
            // Open tags should only indented further under certain conditions.
            } else if ((!last_token && token->type == TOKEN_TYPE_OPEN_TAG) || (last_token && last_token->type == TOKEN_TYPE_OPEN_TAG)) {
                level++;
            }

            if (token->tag == "pre" || token->tag == "code") {
                in_pre = !in_pre;
            }
        }

        if (level > 0 && !in_pre) {
            builder.append(QString("   ").repeated(level));
        }
        builder.append(segment);
        builder.append("\n");
        qDebug() << ((token->type == TOKEN_TYPE_TEXT) ? "TEXT" : "TAG") << " level: " << level << " - " << segment;

        last_token = token;
    }

    return builder.join("");
}

void HTMLPrettyPrint::tokenize()
{
    QChar c;
    size_t start = 0;

    for (int i = 0; i < m_source.size(); ++i) {
        c = m_source.at(i);

        if (c == '<' || c == '>') {
            HTMLToken *token = new HTMLToken();
            token->start = start;
            token->len = i - start;

            start = i;
            if (c == '<') {
                token->type = TOKEN_TYPE_TEXT;
            } else if (c == '>') {
                token->len++;

                QString segment = m_source.mid(token->start, token->len);
                token->type = tokenType(segment);
                token->tag = tag(segment);

                i++;
                start++;
            }

            m_tokens.append(token);
        }
    }

    // Trailing text.
    if (start != m_source.size()) {
        HTMLToken *token = new HTMLToken();
        token->start = start;
        token->len = m_source.size() - start;
        token->type = TOKEN_TYPE_TEXT;
        m_tokens.append(token);
    }
}

QString HTMLPrettyPrint::cleanSegement(const QString &source)
{
    QString segment = source;

    segment = segment.replace(QRegExp("(\\r\\n)|\\n|\\r"), " ");
    segment = segment.replace(QRegExp("\\s{2,}"), " ");
    segment = segment.replace(QRegExp("^\\s+"), "");
    segment = segment.replace(QRegExp("\\s+$"), "");

    return segment;
}

HTMLPrettyPrint::TOKEN_TYPE HTMLPrettyPrint::tokenType(const QString &source)
{
    if (source.startsWith("</")) {
        return TOKEN_TYPE_CLOSE_TAG;
    } else if (source.endsWith("/>")) {
        return TOKEN_TYPE_SELF_CLOSING_TAG;
    } else if (source.startsWith("<!--")) {
        return TOKEN_TYPE_COMMENT;
    } else if (source.startsWith("<?")) {
        return TOKEN_TYPE_XML_DECL;
    } else if (source.startsWith("<!")) {
        return TOKEN_TYPE_DOC_TYPE;
    }
    return TOKEN_TYPE_OPEN_TAG;
}

QString HTMLPrettyPrint::tag(const QString &source)
{
    QString tag;
    QRegExp tag_cap("</?\\s*([^\\s/>]+)");

    if (tag_cap.indexIn(source) > -1) {
        tag = tag_cap.cap(1);
    }

    return tag.toLower();
}

