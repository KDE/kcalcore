/*
    This file is part of the kholidays library.

    Copyright 2010 John Layt <john@layt.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef TESTHOLIDAYREGION_H
#define TESTHOLIDAYREGION_H

#include <QtCore/QObject>

#include "holidays.h"

class QString;
class QDate;

class HolidayRegionTest : public QObject
{
Q_OBJECT
private Q_SLOTS:
    void testGb();
    void testLocations();

private:
    void printMetadata( const QString &location );
    void printHolidays( KHolidays::Holiday::List holidays );
    void parseRegionCalendarYear( const QString &location, int year, const QString &calendarType );
    void parseRegionDateRange( const QString &location, const QDate &startDate, const QDate &endDate );
    void parseRegionDate( const QString &location, const QDate &date );
};

#endif // TESTHOLIDAYREGION_H