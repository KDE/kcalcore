/*
    This file is part of the kcal library.

    Copyright (c) 2006 Narayan Newton <narayannewton@gmail.com> 

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

#include <qtest_kde.h>

#include "testperiod.h"
#include "testperiod.moc"

QTEST_KDEMAIN( PeriodTest, NoGUI )

#include "kcal/period.h"
using namespace KCal;

void PeriodTest::testValidity() 
{
  Period p1(QDateTime(QDate(2006, 8, 30)), Duration(60));
  QVERIFY( p1.hasDuration() );
  QVERIFY( p1.duration() == 60 );
  QVERIFY( p1.start() == QDateTime(QDate(2006, 8, 30)) );
}

void PeriodTest::testCompare() 
{
  Period p1(QDateTime(QDate(2006, 8, 30)), Duration(24*60*60));
  Period p2(QDateTime(QDate(2006, 8, 30)), Duration(23*60*60));
  QVERIFY( (p2 < p1) ? true : false );
}
