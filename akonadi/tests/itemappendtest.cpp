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

#include "control.h"
#include "itemappendtest.h"
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionpathresolver.h>
#include <akonadi/itemappendjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemdeletejob.h>

#include <QtCore/QDebug>

using namespace Akonadi;

#include <qtest_kde.h>

QTEST_KDEMAIN( ItemAppendTest, NoGUI )

static Collection testFolder1;

void ItemAppendTest::initTestCase()
{
  Control::start();

  // get the collections we run the tests on
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  QVERIFY( job->exec() );
  Collection::List list = job->collections();
  Collection res2;
  foreach ( const Collection col, list )
    if ( col.name() == "res2" )
      res2 = col;
  foreach ( const Collection col, list )
    if ( col.name() == "space folder" && col.parent() == res2.id() )
      testFolder1 = col;
}

void ItemAppendTest::testItemAppend_data()
{
  QTest::addColumn<QString>( "remoteId" );

  QTest::newRow( "empty" ) << QString();
  QTest::newRow( "non empty" ) << QString( "remote-id" );
  QTest::newRow( "whitespace" ) << QString( "remote id" );
  QTest::newRow( "quotes" ) << QString ( "\"remote\" id" );
}

void ItemAppendTest::testItemAppend()
{
  QFETCH( QString, remoteId );
  Item ref; // for cleanup

  Item item( -1 );
  item.setRemoteId( remoteId );
  item.setMimeType( "application/octet-stream" );
  ItemAppendJob *job = new ItemAppendJob( item, Collection( testFolder1 ), this );
  QVERIFY( job->exec() );
  ref = job->item();

  ItemFetchJob *fjob = new ItemFetchJob( testFolder1, this );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  QCOMPARE( fjob->items()[0], ref );
  QCOMPARE( fjob->items()[0].remoteId(), remoteId );

  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  QVERIFY( djob->exec() );

  fjob = new ItemFetchJob( testFolder1, this );
  QVERIFY( fjob->exec() );
  QVERIFY( fjob->items().isEmpty() );
}

void ItemAppendTest::testContent_data()
{
  QTest::addColumn<QByteArray>( "data" );

  QTest::newRow( "empty" ) << QByteArray();
  QString utf8string = QString::fromUtf8("äöüß@€µøđ¢©®");
  QTest::newRow( "utf8" ) << utf8string.toUtf8();
  QTest::newRow( "newlines" ) << QByteArray("\nsome\n\nbreaked\ncontent\n\n");
}

void ItemAppendTest::testContent()
{
  QFETCH( QByteArray, data );

  Item item;
  item.setMimeType( "application/octet-stream" );
  item.setPayload( data );

  ItemAppendJob* job = new ItemAppendJob( item, Collection( testFolder1 ), this );
  QVERIFY( job->exec() );
  Item ref = job->item();

  ItemFetchJob *fjob = new ItemFetchJob( testFolder1, this );
  fjob->addFetchPart( Item::PartBody );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  Item item2 = fjob->items().first();
  QCOMPARE( data, item2.payload<QByteArray>() );

  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  QVERIFY( djob->exec() );
}

void ItemAppendTest::testIllegalAppend()
{
  Item item;
  item.setMimeType( "application/octet-stream" );

  // adding item to non-existing collection
  ItemAppendJob *job = new ItemAppendJob( item, Collection( INT_MAX ), this );
  QVERIFY( !job->exec() );

  // adding item with non-existing mimetype
  Item item2;
  item2.setMimeType( "wrong/type" );
  job = new ItemAppendJob( item2, Collection( testFolder1 ), this );
  QVERIFY( !job->exec() );

  // adding item into a collection which can't handle items of this type
  CollectionPathResolver *resolver = new CollectionPathResolver( "res1/foo/bla", this );
  QVERIFY( resolver->exec() );
  const Collection col = Collection( resolver->collection() );
  job = new ItemAppendJob( item, col, this );
  QEXPECT_FAIL( "", "Test not yet implemented in the server.", Continue );
  QVERIFY( !job->exec() );
}

void ItemAppendTest::testMultipartAppend()
{
  Item item;
  item.setMimeType( "application/octet-stream" );
  item.addPart( Item::PartBody, "body data" );
  item.addPart( "EXTRA", "extra data" );
  ItemAppendJob *job = new ItemAppendJob( item, Collection( testFolder1 ), this );
  QVERIFY( job->exec() );
  Item ref = job->item();

  ItemFetchJob *fjob = new ItemFetchJob( ref, this );
  fjob->addFetchPart( Item::PartBody );
  fjob->addFetchPart( "EXTRA" );
  QVERIFY( fjob->exec() );
  QCOMPARE( fjob->items().count(), 1 );
  item = fjob->items().first();
  QCOMPARE( item.part( Item::PartBody ), QByteArray( "body data" ) );
  QCOMPARE( item.part( "EXTRA" ), QByteArray( "extra data" ) );

  ItemDeleteJob *djob = new ItemDeleteJob( ref, this );
  QVERIFY( djob->exec() );
}

#include "itemappendtest.moc"
