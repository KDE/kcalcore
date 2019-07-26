/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "testreadrecurrenceid.h"
#include "memorycalendar.h"
#include "icalformat.h"
#include "exceptions.h"

#include <QDebug>
#include <QTimeZone>
#include <QTest>

QTEST_MAIN(TestReadRecurrenceId)

void TestReadRecurrenceId::testReadSingleException()
{
    KCalendarCore::ICalFormat format;
    QFile file(QLatin1String(ICALTESTDATADIR) + QLatin1String("test_recurrenceid_single.ics"));
    QVERIFY(file.open(QIODevice::ReadOnly));
//   qDebug() << file.readAll();

    KCalendarCore::Incidence::Ptr i = format.fromString(QString::fromUtf8(file.readAll()));
    if (!i) {
        qWarning() << "Failed to parse incidence!";
        if (format.exception()) {
            qWarning() << format.exception()->arguments();
        }
    }
    QVERIFY(i);
    QVERIFY(i->hasRecurrenceId());
}

void TestReadRecurrenceId::testReadSingleExceptionWithThisAndFuture()
{
    KCalendarCore::ICalFormat format;
    QFile file(QLatin1String(ICALTESTDATADIR) + QLatin1String("test_recurrenceid_thisandfuture.ics"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    KCalendarCore::Incidence::Ptr i = format.fromString(QString::fromUtf8(file.readAll()));
    QVERIFY(i);
    QVERIFY(i->hasRecurrenceId());
    QVERIFY(i->thisAndFuture());
}

void TestReadRecurrenceId::testReadWriteSingleExceptionWithThisAndFuture()
{
    KCalendarCore::MemoryCalendar::Ptr cal(new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    KCalendarCore::ICalFormat format;
    KCalendarCore::Incidence::Ptr inc(new KCalendarCore::Event);
    QTimeZone tz("Europe/Berlin");
    QDateTime startDate(QDate(2015, 1, 2), QTime(3, 4, 5), tz);
    inc->setDtStart(startDate);
    inc->setRecurrenceId(startDate);
    inc->setThisAndFuture(true);
    cal->addIncidence(inc);
    const QString result = format.toString(cal, QString());
    qDebug() << result;

    KCalendarCore::Incidence::Ptr i = format.fromString(result);
    QVERIFY(i);
    QVERIFY(i->hasRecurrenceId());
    QVERIFY(i->thisAndFuture());
    QCOMPARE(i->recurrenceId(), startDate);
}

void TestReadRecurrenceId::testReadExceptionWithMainEvent()
{
    KCalendarCore::MemoryCalendar::Ptr calendar(new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    KCalendarCore::ICalFormat format;
    QFile file(QLatin1String(ICALTESTDATADIR) + QLatin1String("test_recurrenceid.ics"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    format.fromString(calendar, QString::fromUtf8(file.readAll()));
    QCOMPARE(calendar->rawEvents().size(), 2);
}
