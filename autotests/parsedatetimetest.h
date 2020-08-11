/*
    SPDX-FileCopyrightText: 2015 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PARSEDATETIMETEST_H
#define PARSEDATETIMETEST_H

#include <QObject>

class ParseDateTimeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testParseDateTime();
    void testParseDateTime_data();
};

#endif
