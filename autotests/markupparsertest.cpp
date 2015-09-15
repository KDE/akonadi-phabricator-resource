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

#include "../src/liphrary/markup.h"

#include <QObject>
#include <QTest>
#include <QDebug>

class MarkupParserTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void simpleMarkupTest_data();
    void simpleMarkupTest();

    void realDataMarkupTest_data();
    void realDataMarkupTest();
};

void MarkupParserTest::initTestCase()
{
    Phrary::Markup::setPhabricatorUrl(QStringLiteral("http://test.phabricator"));
}

void MarkupParserTest::simpleMarkupTest_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("expected");

    QTest::newRow("bold (start)")  << QStringLiteral("**bar baz** foo")
                                   << QStringLiteral("<b>bar baz</b> foo");
    QTest::newRow("bold (middle)") << QStringLiteral("foo **bar** baz")
                                   << QStringLiteral("foo <b>bar</b> baz");
    QTest::newRow("bold (end)")    << QStringLiteral("foo **bar baz**")
                                   << QStringLiteral("foo <b>bar baz</b>");
    QTest::newRow("bold and asterisk")
                                   << QStringLiteral("foo **bar*baz**")
                                   << QStringLiteral("foo <b>bar*baz</b>");
    QTest::newRow("italic")        << QStringLiteral("foo //bar baz// foo")
                                   << QStringLiteral("foo <i>bar baz</i> foo");
    QTest::newRow("deleted")       << QStringLiteral("~~foo~~ bar baz")
                                   << QStringLiteral("<s>foo</s> bar baz");
    QTest::newRow("underlined")    << QStringLiteral("foo __bar__ baz")
                                   << QStringLiteral("foo <u>bar</u> baz");
    QTest::newRow("monospaced (##)") << QStringLiteral("foo ##monospace## bar")
                                     << QStringLiteral("foo <pre>monospace</pre> bar");
    QTest::newRow("monospace (`)") << QStringLiteral("foo `monospace` bar")
                                   << QStringLiteral("foo <pre>monospace</pre> bar");
    QTest::newRow("nested (b->u)") << QStringLiteral("foo **bar __baz__ bar** foo")
                                   << QStringLiteral("foo <b>bar <u>baz</u> bar</b> foo");
    QTest::newRow("nested (b->(d, #)") << QStringLiteral("**foo ~~bar~~ ##mono##**")
                                       << QStringLiteral("<b>foo <s>bar</s> <pre>mono</pre></b>");
    QTest::newRow("link (http)")   << QStringLiteral("foo [[http://foo.bar/baz | FOO]] bar")
                                   << QStringLiteral("foo <a href=\"http://foo.bar/baz\"> FOO</a> bar");
    QTest::newRow("link (https, no space around |)")
                                   << QStringLiteral("foo [[https://foo.bar/baz|FOO]]")
                                   << QStringLiteral("foo <a href=\"https://foo.bar/baz\">FOO</a>");
    QTest::newRow("link (with inner ]") << QStringLiteral("[[ https://foo.bar | FOO [bar]]]")
                                        << QStringLiteral("<a href=\"https://foo.bar\"> FOO [bar</a>]");
    QTest::newRow("link (with inner \"] \"") 
                                   << QStringLiteral("[[http://foo.bar|FOO [bar] ]]")
                                   << QStringLiteral("<a href=\"http://foo.bar\">FOO [bar] </a>");
    QTest::newRow("link (with nested formatting")
                                   << QStringLiteral("[[http://foo.bar|Foo **bar**]]")
                                   << QStringLiteral("<a href=\"http://foo.bar\">Foo <b>bar</b></a>");
    QTest::newRow("plain link")    << QStringLiteral("Foo http://www.google.com bar")
                                   << QStringLiteral("Foo <a href=\"http://www.google.com\">http://www.google.com</a> bar");
    QTest::newRow("= header =")    << QStringLiteral("= Foo =")
                                   << QStringLiteral("<h1>Foo</h1>");
    QTest::newRow("== header ==")  << QStringLiteral("== Foo ==")
                                   << QStringLiteral("<h2>Foo</h2>");
    QTest::newRow("=== header")    << QStringLiteral("=== Foo")
                                   << QStringLiteral("<h3>Foo</h3>");
    QTest::newRow("header followed by text")
                                   << QStringLiteral("== Foo ==\n\nsome text")
                                   << QStringLiteral("<h2>Foo</h2><br><br>some text");
    QTest::newRow("text followed by header")
                                   << QStringLiteral("foo text\n=== Foo ===\nbar bar")
                                   << QStringLiteral("foo text<br><h3>Foo</h3><br>bar bar");
    QTest::newRow("underline header big")
                                   << QStringLiteral("Foo Bar Header\n==============")
                                   << QStringLiteral("<h1>Foo Bar Header</h1>");
    QTest::newRow("underline header small")
                                   << QStringLiteral("Foo Bar Header\n--------------")
                                   << QStringLiteral("<h2>Foo Bar Header</h2>");
    QTest::newRow("padded monospace")
                                   << QStringLiteral("  This will be monospace")
                                   << QStringLiteral("<pre>This will be monospace</pre>");
    QTest::newRow("padded monospace (deep)")
                                   << QStringLiteral("      This will be monospace")
                                   << QStringLiteral("<pre>    This will be monospace</pre>");
    QTest::newRow("padded monospace (multiline)")
                                   << QStringLiteral("  Foo 1\n      Foo 2\n  Foo 3")
                                   << QStringLiteral("<pre>Foo 1\n    Foo 2\nFoo 3</pre>");
    QTest::newRow("padded monospace (with text around")
                                   << QStringLiteral("Foo bar\n Foo bar bar\n  Foo\n  Bar\n  Foo\n\n Bar\nFoo")
                                   << QStringLiteral("Foo bar<br> Foo bar bar<br><pre>Foo\nBar\nFoo\n</pre><br> Bar<br>Foo");
    QTest::newRow("Backquote monospace")
                                   << QStringLiteral("Foo bar\n```\nThis is\nmonospace\n```\nFoo")
                                   << QStringLiteral("Foo bar<br><pre>This is\nmonospace\n</pre><br>Foo");
    QTest::newRow("Task reference")
                                   << QStringLiteral("Foo foo {T12345} bar bar")
                                   << QStringLiteral("Foo foo <a href=\"http://test.phabricator/T12345\">http://test.phabricator/T12345</a> bar bar");
    QTest::newRow("Differential reference")
                                   << QStringLiteral("Foo foo {D86453} bar bar")
                                   << QStringLiteral("Foo foo <a href=\"http://test.phabricator/D86453\">http://test.phabricator/D86453</a> bar bar");
    QTest::newRow("File reference")
                                   << QStringLiteral("Bar bar {F42} foo foo")
                                   << QStringLiteral("Bar bar <a href=\"http://test.phabricator/F42\">http://test.phabricator/F42</a> foo foo");
    QTest::newRow("Invalid reference (1)")
                                   << QStringLiteral("Bar bar {RANDOM} foo")
                                   << QStringLiteral("Bar bar {RANDOM} foo");
    QTest::newRow("Invalid reference (2)")
                                   << QStringLiteral("Bar bar {F42BLA} foo")
                                   << QStringLiteral("Bar bar {F42BLA} foo");
    QTest::newRow("Invalid reference (3)")
                                   << QStringLiteral("Bar bar { T12} foo")
                                   << QStringLiteral("Bar bar { T12} foo");
    QTest::newRow("Invalid reference (4)")
                                   << QStringLiteral("Bar bar {T24 } foo")
                                   << QStringLiteral("Bar bar {T24 } foo");
    QTest::newRow("Invalid reference (5)")
                                   << QStringLiteral("Bar bar {D12 24} foo")
                                   << QStringLiteral("Bar bar {D12 24} foo");
}

void MarkupParserTest::simpleMarkupTest()
{
    QFETCH(QString, in);
    QFETCH(QString, expected);

    const QString out = Phrary::Markup::markupToHTML(in);
    QCOMPARE(out, expected);
}


void MarkupParserTest::realDataMarkupTest_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("expected");

    QTest::newRow("PHID-XACT-TASK-mbgseu53gpdefbc")
        << QStringLiteral("maybe dirmgr is not running and gpgsm tries to verify certificates via CRLs. I know from scalet that dirmgr is also installed at "
                          "the build system. Its possible to disable dirmgr via config:\n\ngpgsm.conf\n   disable-dirmngr\n\n\nkolab has also the same problem "
                          "under windows and solved that with verious options to disable crl:\nsee also https://git.kolab.org/T678#9624")
        << QStringLiteral("maybe dirmgr is not running and gpgsm tries to verify certificates via CRLs. I know from scalet that dirmgr is also installed at "
                          "the build system. Its possible to disable dirmgr via config:<br><br>gpgsm.conf<br><pre> disable-dirmngr\n</pre><br><br>kolab has also the same problem "
                          "under windows and solved that with verious options to disable crl:<br>see also <a href=\"https://git.kolab.org/T678#9624\">https://git.kolab.org/T678#9624</a>");
    QTest::newRow("PHID-XACT-TASK-3jrh6m3jzexi3fi")
        << QStringLiteral("Millian reported a weird crash in Session, is most probably due to null-ptr somewhere. I fixed some parts of it, but there still can be a "
                          "problem if the server crashes twice in a row very quickly, as everything in the session is async...\n\n\n"
                          "```\n"
                          "#6  0x0000000000000000 in ?? ()\n"
                          "#7  0x00007f9d768a05b9 in QObject::disconnect(QObject const*, char const*, QObject const*, char const*) () from /usr/lib/libQt5Core.so.5\n"
                          "#8  0x00007f9d78f1187f in QObject::disconnect (this=0x7f9d50002f00, receiver=0x1e53a30, member=0x0) at /usr/include/qt/QtCore/qobject.h:361\n"
                          "#9  0x00007f9d78f0f654 in Akonadi::ConnectionThread::quit (this=0x1e53a30) at /home/milian/projects/kf5/src/kde/kdepimlibs/akonadi/src/core/connectionthread.cpp:75\n"
                          "#10 0x00007f9d78f8694d in Akonadi::SessionPrivate::~SessionPrivate (this=0x1e534d0, __in_chrg=<optimized out>) at /home/milian/projects/kf5/src/kde/kdepimlibs/akonadi/src/core/session.cpp:288\n"
                          "#11 0x00007f9d78f6234c in Akonadi::NotificationBusPrivate::~NotificationBusPrivate (this=0x1e534c0, __in_chrg=<optimized out>) at /home/milian/projects/kf5/src/kde/kdepimlibs/akonadi/src/core/notificationbus_p.cpp:38\n"
                          "#12 0x00007f9d78f6238e in Akonadi::NotificationBusPrivate::~NotificationBusPrivate (this=0x1e534c0, __in_chrg=<optimized out>) at /home/milian/projects/kf5/src/kde/kdepimlibs/akonadi/src/core/notificationbus_p.cpp:40\n"
                          "```")
        << QStringLiteral("Millian reported a weird crash in Session, is most probably due to null-ptr somewhere. I fixed some parts of it, but there still can be a "
                          "problem if the server crashes twice in a row very quickly, as everything in the session is async...<br><br><br>"
                          "<pre>"
                          "#6  0x0000000000000000 in ?? ()\n"
                          "#7  0x00007f9d768a05b9 in QObject::disconnect(QObject const*, char const*, QObject const*, char const*) () from /usr/lib/libQt5Core.so.5\n"
                          "#8  0x00007f9d78f1187f in QObject::disconnect (this=0x7f9d50002f00, receiver=0x1e53a30, member=0x0) at /usr/include/qt/QtCore/qobject.h:361\n"
                          "#9  0x00007f9d78f0f654 in Akonadi::ConnectionThread::quit (this=0x1e53a30) at /home/milian/projects/kf5/src/kde/kdepimlibs/akonadi/src/core/connectionthread.cpp:75\n"
                          "#10 0x00007f9d78f8694d in Akonadi::SessionPrivate::~SessionPrivate (this=0x1e534d0, __in_chrg=<optimized out>) at /home/milian/projects/kf5/src/kde/kdepimlibs/akonadi/src/core/session.cpp:288\n"
                          "#11 0x00007f9d78f6234c in Akonadi::NotificationBusPrivate::~NotificationBusPrivate (this=0x1e534c0, __in_chrg=<optimized out>) at /home/milian/projects/kf5/src/kde/kdepimlibs/akonadi/src/core/notificationbus_p.cpp:38\n"
                          "#12 0x00007f9d78f6238e in Akonadi::NotificationBusPrivate::~NotificationBusPrivate (this=0x1e534c0, __in_chrg=<optimized out>) at /home/milian/projects/kf5/src/kde/kdepimlibs/akonadi/src/core/notificationbus_p.cpp:40\n"
                          "</pre>");
}

void MarkupParserTest::realDataMarkupTest()
{
    QFETCH(QString, in);
    QFETCH(QString, expected);

    const QString out = Phrary::Markup::markupToHTML(in);
    if (out != expected) {
        qDebug() << "Actual:  " << out;
        qDebug() << "Expected:" << expected;
    }
    QCOMPARE(out, expected);
}

QTEST_MAIN(MarkupParserTest)

#include "markupparsertest.moc"