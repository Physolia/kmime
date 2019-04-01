/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "contentindextest.h"

#include <kmime_content.h>
#include <kmime_contentindex.h>

using namespace KMime;

#include <QTest>

QTEST_MAIN(ContentIndexTest)

void ContentIndexTest::testToString()
{
    KMime::ContentIndex ci;
    QCOMPARE(ci.toString(), QString());
    ci.push(1);
    QCOMPARE(ci.toString(), QLatin1String("1"));
    ci.push(2);
    QCOMPARE(ci.toString(), QLatin1String("2.1"));
}

void ContentIndexTest::testFromString()
{
    ContentIndex ci1;
    QVERIFY(!ci1.isValid());

    ContentIndex ci2(QStringLiteral("1.2.bla"));
    QVERIFY(!ci2.isValid());

    ContentIndex ci3(QStringLiteral("1"));
    QVERIFY(ci3.isValid());
    QCOMPARE(ci3.pop(), 1u);
    QVERIFY(!ci3.isValid());

    ContentIndex ci4(QStringLiteral("3.2"));
    QVERIFY(ci4.isValid());
    QCOMPARE(ci4.pop(), 3u);
    QCOMPARE(ci4.pop(), 2u);
    QVERIFY(!ci4.isValid());
}

void ContentIndexTest::testContent()
{
    Content *c1 = new Content();
    QCOMPARE(c1->content(ContentIndex()), c1);
    QCOMPARE(c1->content(ContentIndex(QLatin1String("1"))), (Content *)nullptr);
    QCOMPARE(c1->indexForContent(c1), ContentIndex());

    Content *c11 = new Content();
    // this makes c1 multipart/mixed, ie. c11 will be the second child!
    c1->addContent(c11);
    QCOMPARE(c1->content(ContentIndex(QLatin1String("2"))), c11);
    QCOMPARE(c1->content(ContentIndex(QLatin1String("3"))), (Content *)nullptr);
    QCOMPARE(c1->content(ContentIndex(QLatin1String("2.1"))), (Content *)nullptr);
    QCOMPARE(c1->indexForContent(c1), ContentIndex());
    QCOMPARE(c1->indexForContent(c11), ContentIndex(QLatin1String("2")));
    QCOMPARE(c1->indexForContent(c1->contents().first()), ContentIndex(QLatin1String("1")));

    Content *c12 = new Content();
    c1->addContent(c12);
    // c12 becomes multipart/mixed, ie. c12 will be the second child!
    Content *c121 = new Content();
    c12->addContent(c121);
    QCOMPARE(c1->content(ContentIndex(QLatin1String("3"))), c12);
    QCOMPARE(c1->content(ContentIndex(QLatin1String("3.2"))), c121);
    QCOMPARE(c1->indexForContent(c121), ContentIndex(QLatin1String("3.2")));
    QCOMPARE(c121->index(), ContentIndex(QLatin1String("3.2")));

    QCOMPARE(c1->indexForContent((Content *)nullptr), ContentIndex());
    delete c1;
}

void ContentIndexTest::testNavigation()
{
    ContentIndex ci;
    QCOMPARE(ci.toString(), QString());
    ci.push(1);
    ci.push(2);
    ci.push(3);
    QCOMPARE(ci.toString(), QLatin1String("3.2.1"));
    QCOMPARE(ci.pop(), 3u);
    QCOMPARE(ci.up(), 1u);
    QCOMPARE(ci.toString(), QLatin1String("2"));
}

