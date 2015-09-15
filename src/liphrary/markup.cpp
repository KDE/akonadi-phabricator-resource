/*
 * Copyright 2015  Daniel Vrátil <dvratil@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "markup.h"

#include <QStack>
#include <QTextStream>
#include <QString>

#include <QDebug>


/**
 * This is a very simple, naive and buggy parser for the Phabricator markup syntax.
 * It does not support everything and the stuff it supports it supports within
 * certain limits. Certainly not a referencial implementations :-)
 *
 * TODO: proper parser is needed. Maybe we could fork whatever Phabricator uses
 * and port it to C++?
 *
 * TODO: https://secure.phabricator.com/book/phabricator/article/remarkup/
 * Missing features:
 *      @usermentions
 *      #projectmentions
 *      [[wiki links]]
 *      plain http:// links
 *      > quoted test
 *      - unordered lists
 *      * unordered lists
 *      # ordered lists
 *      icons
 *      tables
 *      navigation
 */



namespace Phrary {
namespace Markup {

class Private {
public:
    QString phabricatorUrl;
};

}
}

Q_GLOBAL_STATIC(Phrary::Markup::Private, sPrivate)

using namespace Phrary;

void Markup::setPhabricatorUrl(const QString &url)
{
    sPrivate->phabricatorUrl = url;
}

static QString htmlTagForMarkupStyle(const QChar &markup)
{
    if (markup == QLatin1Char('*')) {
        return QStringLiteral("b");
    } else if (markup == QLatin1Char('/')) {
        return QStringLiteral("i");
    } else if (markup == QLatin1Char('_')) {
        return QStringLiteral("u");
    } else if (markup == QLatin1Char('~')) {
        return QStringLiteral("s");
    } else if (markup == QLatin1Char('#') || markup == QLatin1Char('`')) {
        return QStringLiteral("pre");
    } else {
        return QString();
    }
}

static bool parseHeader(const QString &text, int &i, QTextStream &outStream, QChar &buff)
{
    Q_UNUSED(buff);

    if (i > 0 && text[i - 1] != '\n') {
        return false;
    }

    bool isDashes = true;
    int depth = 0;
    while (i + depth < text.size() && text[i + depth] == '-') {
        ++depth;
    }

    if (depth == 0) {
        isDashes = false;
        while (i + depth < text.size() && text[i + depth] == '=') {

            ++depth;
        }
    }
    if (depth == 0) {
        return false;
    }

    int eol = text.indexOf(QLatin1Char('\n'), i + depth); // trailing "=" are optional
    if (eol == -1) {
        eol = text.size();
    }

    // if the "===" span over the entire line, assume "Blabla\n======" format
    if (eol == i + depth) {
        if (i < depth + 1) {
            return false;
        }

        const int realDepth = (text[i] == '-') ? 2 : 1;
        int prevEol = qMax(text.lastIndexOf(QLatin1Char('\n'), i - 2), 0);
        const QString header = text.midRef(prevEol, i - 1 - prevEol).trimmed().toString();
        // Dirty trick: outStream.seek() does not work, we need to trim the actual string
        // Removes the header data written by previous parsers because they did not know
        // this is a header.
        outStream.string()->resize(outStream.pos() - header.size());
        outStream << QStringLiteral("<h%1>%2</h%1>").arg(realDepth).arg(header);
        i += depth;
    } else if (!isDashes) {
        i += depth;
        int end = text.indexOf(QLatin1Char('='), i);
        if (end == -1) {
            end = eol;
        }
        const QString header = text.midRef(i, qMin(end, eol) - i).trimmed().toString();
        if (depth > 0) {
            outStream << QStringLiteral("<h%1>%2</h%1>").arg(depth).arg(header);
        }

        // Read until end of line
        for (; i < text.size() - 1 && text[i + 1] != '\n'; ++i);
    } else if (isDashes) {
        return false;
    }

    return true;
}

static bool parseIndentedPre(const QString &text, int &i, QTextStream &outStream)
{
    static const int MinIndent = 2;

    if (i > 0 && text[i - 1] != QLatin1Char('\n')) {
        return false;
    }

    bool isFirstLine = true;
    Q_FOREVER {
        for (int j = 0; j < MinIndent; j++) {
            if (text.size() <= i + j || text[i + j] != QLatin1Char(' ')) {
                if (!isFirstLine) {
                    outStream << QStringLiteral("</pre>");
                    if (text.size() > i + j) {
                        outStream << QStringLiteral("<br>");
                    }
                }
                return !isFirstLine;
            }
        }

        i += MinIndent;
        if (isFirstLine) {
            outStream << QStringLiteral("<pre>");
            isFirstLine = false;
        }

        int eol = text.indexOf(QLatin1Char('\n'), i);
        if (eol == -1) {
            eol = text.size() - 1;
        }

        outStream << text.mid(i, eol - i + 1);
        i = eol + 1;
    }

    return false;
}

static bool parseBackquotePre(const QString &text, int &i, QTextStream &outStream)
{
    if (i > 0 && text[i - 1] != QLatin1Char('\n')) {
        return false;
    }

    if (text.mid(i, 3) != QLatin1String("```")) {
        return false;
    }

    outStream << QStringLiteral("<pre>");
    i += 4; // read past ```\n

    int end = text.indexOf(QStringLiteral("```"), i);
    if (end == -1) {
        end = text.size();
    }

    outStream << text.mid(i, end - i);
    outStream << QStringLiteral("</pre>");
    // skip the backticks
    i = end + 2;

    return true;
}


QString Phrary::Markup::markupToHTML(const QString &text)
{
    struct MarkupMap {
        char markup;
        char html;
    };

    QString out;
    out.reserve(text.size()); // reserve at least text.size() characters
    QTextStream outStream(&out, QIODevice::ReadWrite);

    QChar c;
    QChar prev;
    QChar buff;
    QStack<QChar> styleStack;
    for (int i = 0; i < text.size(); ++i) {
        c = text[i];
        if (parseHeader(text, i, outStream, buff)) {
            prev = c;
            continue;
        } else if (parseIndentedPre(text, i, outStream)) {
            prev = c;
            continue;
        } else if (parseBackquotePre(text, i, outStream)) {
            prev = c;
            continue;
        } else if (text.mid(i, 3) == QStringLiteral("://")) {
            int linkStart = i;
            while (linkStart > 0 && !text[linkStart].isSpace() && text[linkStart] != QLatin1Char('\n')) {
                --linkStart;
            }
            int linkEnd = i;
            while (linkEnd < text.size() && !text[linkEnd].isSpace() && text[linkEnd] != QLatin1Char('\n')) {
                ++linkEnd;
            }
            outStream.string()->resize(outStream.string()->size() - (i - linkStart) + 1);
            const QString link = text.mid(linkStart + 1, linkEnd - linkStart - 1);
            outStream << QStringLiteral("<a href=\"%1\">%1</a>").arg(link);
            i = linkEnd - 1;
        } else if (c == QLatin1Char('`')) {
            if (!styleStack.isEmpty() && styleStack.top() == c) {
                styleStack.pop();
                outStream << QStringLiteral("</pre>");
            } else {
                styleStack.push(c);
                outStream << QStringLiteral("<pre>");
            }
            c = QLatin1Char('>');
        } else if  (c == QLatin1Char('*') || c == QLatin1Char('/') || c == QLatin1Char('#') || c == QLatin1Char('~') || c == QLatin1Char('_')) {
            if (c == prev) {
                buff = 0;
                if (!styleStack.isEmpty() && styleStack.top() == c) {
                    styleStack.pop();
                    outStream << QStringLiteral("</%1>").arg(htmlTagForMarkupStyle(c));
                } else {
                    styleStack.push(c);
                    outStream << QStringLiteral("<%1>").arg(htmlTagForMarkupStyle(c));
                }
                c = QLatin1Char('>');
            } else {
                buff = c;
            }
        } else if (c == QLatin1Char('[')) {
            if (c == prev) {
                buff = 0;
                int start = -1, end = -1;
                for (; i < text.size(); ++i) {
                    if (text[i] == QLatin1Char('|')) {
                        end = i;
                        break;
                    } else if (!text[i].isSpace() && start == -1) {
                        start = i + 1;
                    } else if (end == -1 && start > -1 && text[i] == QLatin1Char(' ')) {
                        end = i;
                    }
                }
                const QString url = text.mid(start, end - start).trimmed();
                if (url.startsWith(QLatin1String("http"))) {
                    outStream << QStringLiteral("<a href=\"%1\">").arg(url);
                }
                c = QLatin1Char('>');
            } else {
                buff = c;
            }
        } else if (c == QLatin1Char(']')) {
            if (c == prev) {
                buff = 0;
                outStream << QStringLiteral("</a>");
                c = QLatin1Char('>');
            } else {
                buff = c;
            }
        } else if (c == QLatin1Char('{') && (text[i + 1] == QLatin1Char('F') || text[i + 1] == QLatin1Char('T') || text[i + 1] == QLatin1Char('D'))) {
            const int end = text.indexOf(QLatin1Char('}'), i);
            if (end == -1) {
                buff = c;
                continue;
            }
            // Spaces in { } are not allowed
            if (text.indexOf(QLatin1Char(' '), i) < end) {
                buff = c;
                continue;
            }
            bool ok = false;
            text.mid(i + 2, end - i - 2).toInt(&ok);
            if (!ok) {
                buff = c;
                continue;
            }

            outStream << QStringLiteral("<a href=\"%1/%2\">%1/%2</a>").arg(sPrivate->phabricatorUrl, text.mid(i + 1, end - i - 1));
            i = end;
        } else {
            if (!buff.isNull()) {
                if (buff == QLatin1Char('\n')) {
                    outStream << QStringLiteral("<br>");
                } else {
                    outStream << buff;
                }
                buff = 0;
            }
            if (c == QLatin1Char('\n')) {
                outStream << QStringLiteral("<br>");
            } else {
                outStream << c;
            }
        }

        prev = c;
    }

    if (!buff.isNull()) {
        outStream << buff;
    }

    return out;
}

