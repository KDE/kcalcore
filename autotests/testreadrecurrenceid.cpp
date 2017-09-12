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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "testreadrecurrenceid.h"
#include "memorycalendar.h"
#include "icalformat.h"
#include "icaltimezones.h"
#include "exceptions.h"
#include "utils.h"
#include "setuptzinfo.h"

#include <QDebug>
#include <QTimeZone>

#include <QTest>
QTEST_MAIN(TestReadRecurrenceId)

void TestReadRecurrenceId::initTestCase()
{
    const SetupTzinfo setup;
}

void TestReadRecurrenceId::testReadSingleException()
{
    KCalCore::ICalFormat format;
    QFile file(QLatin1String(ICALTESTDATADIR) + QLatin1String("test_recurrenceid_single.ics"));
    QVERIFY(file.open(QIODevice::ReadOnly));
//   qDebug() << file.readAll();

    KCalCore::Incidence::Ptr i = format.fromString(QString::fromUtf8(file.readAll()));
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
    KCalCore::ICalFormat format;
    QFile file(QLatin1String(ICALTESTDATADIR) + QLatin1String("test_recurrenceid_thisandfuture.ics"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    KCalCore::Incidence::Ptr i = format.fromString(QString::fromUtf8(file.readAll()));
    QVERIFY(i);
    QVERIFY(i->hasRecurrenceId());
    QVERIFY(i->thisAndFuture());
}

void TestReadRecurrenceId::testReadWriteSingleExceptionWithThisAndFuture()
{
    KCalCore::MemoryCalendar::Ptr cal(new KCalCore::MemoryCalendar(QTimeZone::utc()));
    KCalCore::ICalFormat format;
    KCalCore::Incidence::Ptr inc(new KCalCore::Event);
    KCalCore::ICalTimeZoneSource tzsource;
    KDateTime::Spec spec(tzsource.standardZone(QStringLiteral("Europe/Berlin")));
    KDateTime startDate = KDateTime(QDate(2015, 1, 2), QTime(3, 4, 5), spec);
    inc->setDtStart(startDate);
    inc->setRecurrenceId(KCalCore::k2q(startDate));
    inc->setThisAndFuture(true);
    cal->addIncidence(inc);
    const QString result = format.toString(cal, QString());
    qDebug() << result;

    KCalCore::Incidence::Ptr i = format.fromString(result);
    QVERIFY(i);
    QVERIFY(i->hasRecurrenceId());
    QVERIFY(i->thisAndFuture());
    QCOMPARE(i->recurrenceId(), KCalCore::k2q(startDate));
}

void TestReadRecurrenceId::testReadExceptionWithMainEvent()
{
    KCalCore::MemoryCalendar::Ptr calendar(new KCalCore::MemoryCalendar(QTimeZone::utc()));
    KCalCore::ICalFormat format;
    QFile file(QLatin1String(ICALTESTDATADIR) + QLatin1String("test_recurrenceid.ics"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    format.fromString(calendar, QString::fromUtf8(file.readAll()));
    QCOMPARE(calendar->rawEvents().size(), 2);
}
