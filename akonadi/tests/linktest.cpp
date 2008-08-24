/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include <akonadi/collection.h>
#include <akonadi/control.h>
#include <akonadi/linkjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/unlinkjob.h>

#include <QtCore/QObject>

#include <qtest_kde.h>

using namespace Akonadi;

class LinkTest : public QObject
{
  Q_OBJECT
  private slots:
    void initTestCase()
    {
      Control::start();
    }

    void testLink()
    {
      const Collection col( 12 );
      Item::List items;
      items << Item( 3 ) << Item( 4 ) << Item( 6 );

      LinkJob *link = new LinkJob( col, items, this );
      QVERIFY( link->exec() );

      ItemFetchJob *fetch = new ItemFetchJob( col );
      QVERIFY( fetch->exec() );
      QCOMPARE( fetch->items().count(), 3 );
      foreach ( const Item &item, fetch->items() )
        QVERIFY( items.contains( item ) );

      UnlinkJob *unlink = new UnlinkJob( col, items, this );
      QVERIFY( unlink->exec() );

      fetch = new ItemFetchJob( col );
      QVERIFY( fetch->exec() );
      QCOMPARE( fetch->items().count(), 0 );
    }

};

QTEST_KDEMAIN( LinkTest, NoGUI )

#include "linktest.moc"
