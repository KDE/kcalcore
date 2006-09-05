/*
  This file is part of the kcal library.
  Copyright (C) 2006 Allen Winter <winter@kde.org>

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

#include "testevent.h"
#include "testevent.moc"

QTEST_KDEMAIN( EventTest, NoGUI )

#include "kcal/event.h"
using namespace KCal;

void EventTest::testValidity() {
  QDate dt = QDate::currentDate();
  Event *event = new Event();
  event->setDtStart( QDateTime( dt ) );
  event->setDtEnd( QDateTime( dt ).addDays( 1 ) );
  event->setSummary( "Event1 Summary" );
  event->setDescription( "This is a description of the first event" );
  event->setLocation( "the place" );
  QVERIFY( event->summary() == "Event1 Summary" );
  QVERIFY( event->location() == "the place" );
}

void EventTest::testCompare() {
  QDate dt = QDate::currentDate();
  Event event1;
  event1.setDtStart( QDateTime( dt ) );
  event1.setDtEnd( QDateTime( dt ).addDays( 1 ) );
  event1.setSummary( "Event1 Summary" );
  event1.setDescription( "This is a description of the first event" );
  event1.setLocation( "the place" );

  Event event2;
  event2.setDtStart( QDateTime( dt ).addDays( 1 ) );
  event2.setDtEnd( QDateTime( dt ).addDays( 2 ) );
  event2.setSummary( "Event2 Summary" );
  event2.setDescription( "This is a description of the second event" );
  event2.setLocation( "the other place" );

  QVERIFY( !( event1 == event2 ) );
  QVERIFY( event1.dtEnd() == event2.dtStart() );
  QVERIFY( event2.summary() == "Event2 Summary" );
}
