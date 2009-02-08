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

#include "monitortest.h"
#include "test_utils.h"

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/monitor.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/control.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>

#include <QtCore/QVariant>
#include <QtGui/QApplication>
#include <QtTest/QSignalSpy>
#include <qtest_akonadi.h>

using namespace Akonadi;

QTEST_AKONADIMAIN( MonitorTest, NoGUI )

static Collection res3;

Q_DECLARE_METATYPE(Akonadi::Collection::Id)

void MonitorTest::initTestCase()
{
  Control::start();

  res3 = Collection( collectionIdFromPath( "res3" ) );

  // switch all resources offline to reduce interference from them
  foreach ( Akonadi::AgentInstance agent, Akonadi::AgentManager::self()->instances() )
    agent.setIsOnline( false );
}

void MonitorTest::testMonitor_data()
{
  QTest::addColumn<bool>( "fetchCol" );
  QTest::newRow( "with collection fetching" ) << true;
  QTest::newRow( "without collection fetching" ) << false;
}

void MonitorTest::testMonitor()
{
  QFETCH( bool, fetchCol );

  Monitor *monitor = new Monitor( this );
  monitor->setCollectionMonitored( Collection::root() );
  monitor->fetchCollection( fetchCol );
  monitor->itemFetchScope().fetchFullPayload();

  // monitor signals
  qRegisterMetaType<Akonadi::Collection>();
  qRegisterMetaType<Akonadi::Collection::Id>();
  qRegisterMetaType<Akonadi::Item>();
  qRegisterMetaType<Akonadi::CollectionStatistics>();
  QSignalSpy caspy( monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)) );
  QSignalSpy cmspy( monitor, SIGNAL(collectionChanged(const Akonadi::Collection&)) );
  QSignalSpy crspy( monitor, SIGNAL(collectionRemoved(const Akonadi::Collection&)) );
  QSignalSpy csspy( monitor, SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)) );
  QSignalSpy iaspy( monitor, SIGNAL(itemAdded(const Akonadi::Item&, const Akonadi::Collection&)) );
  QSignalSpy imspy( monitor, SIGNAL(itemChanged(const Akonadi::Item&, const QSet<QByteArray>&)) );
  QSignalSpy irspy( monitor, SIGNAL(itemRemoved(const Akonadi::Item&)) );

  QVERIFY( caspy.isValid() );
  QVERIFY( cmspy.isValid() );
  QVERIFY( crspy.isValid() );
  QVERIFY( csspy.isValid() );
  QVERIFY( iaspy.isValid() );
  QVERIFY( imspy.isValid() );
  QVERIFY( irspy.isValid() );

  // create a collection
  Collection monitorCol;
  monitorCol.setParent( res3 );
  monitorCol.setName( "monitor" );
  CollectionCreateJob *create = new CollectionCreateJob( monitorCol, this );
  QVERIFY( create->exec() );
  monitorCol = create->collection();
  QVERIFY( monitorCol.isValid() );
  QTest::qWait(1000); // make sure the DBus signal has been processed

  QCOMPARE( caspy.count(), 1 );
  QList<QVariant> arg = caspy.takeFirst();
  Collection col = arg.at(0).value<Collection>();
  QCOMPARE( col, monitorCol );
  if ( fetchCol )
    QCOMPARE( col.name(), QString("monitor") );
  Collection parent = arg.at(1).value<Collection>();
  QCOMPARE( parent, res3 );

  QVERIFY( cmspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( csspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // add an item
  Item newItem;
  newItem.setMimeType( "application/octet-stream" );
  ItemCreateJob *append = new ItemCreateJob( newItem, monitorCol, this );
  QVERIFY( append->exec() );
  Item monitorRef = append->item();
  QVERIFY( monitorRef.isValid() );
  QTest::qWait(1000);

  QCOMPARE( csspy.count(), 1 );
  arg = csspy.takeFirst();
  QEXPECT_FAIL( "", "Don't know how to handle 'Akonadi::Collection::Id', use qRegisterMetaType to register it. <-- I did this, but it still doesn't work!", Continue );
  QCOMPARE( arg.at(0).value<Akonadi::Collection::Id>(), monitorCol.id() );

  QCOMPARE( iaspy.count(), 1 );
  arg = iaspy.takeFirst();
  Item item = arg.at( 0 ).value<Item>();
  QCOMPARE( item, monitorRef );
  QCOMPARE( item.mimeType(), QString::fromLatin1(  "application/octet-stream" ) );
  Collection collection = arg.at( 1 ).value<Collection>();
  QCOMPARE( collection.id(), monitorCol.id() );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( cmspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // modify an item
  item.setPayload<QByteArray>( "some new content" );
  ItemModifyJob *store = new ItemModifyJob( item, this );
  QVERIFY( store->exec() );
  QTest::qWait(1000);

  QCOMPARE( csspy.count(), 1 );
  arg = csspy.takeFirst();
  QEXPECT_FAIL( "", "Don't know how to handle 'Akonadi::Collection::Id', use qRegisterMetaType to register it. <-- I did this, but it still doesn't work!", Continue );
  QCOMPARE( arg.at(0).value<Collection::Id>(), monitorCol.id() );

  QCOMPARE( imspy.count(), 1 );
  arg = imspy.takeFirst();
  item = arg.at( 0 ).value<Item>();
  QCOMPARE( monitorRef, item );
  QCOMPARE( item.payload<QByteArray>(), QByteArray( "some new content" ) );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( cmspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // delete an item
  ItemDeleteJob *del = new ItemDeleteJob( monitorRef, this );
  QVERIFY( del->exec() );
  QTest::qWait(1000);

  QCOMPARE( csspy.count(), 1 );
  arg = csspy.takeFirst();
  QEXPECT_FAIL( "", "Don't know how to handle 'Akonadi::Collection::Id', use qRegisterMetaType to register it. <-- I did this, but it still doesn't work!", Continue );
  QCOMPARE( arg.at(0).value<Collection::Id>(), monitorCol.id() );
  cmspy.clear();

  QCOMPARE( irspy.count(), 1 );
  arg = irspy.takeFirst();
  Item ref = qvariant_cast<Item>( arg.at(0) );
  QCOMPARE( monitorRef, ref );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( cmspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  imspy.clear();

  // modify a collection
  monitorCol.setName( "changed name" );
  CollectionModifyJob *mod = new CollectionModifyJob( monitorCol, this );
  QVERIFY( mod->exec() );
  QTest::qWait(1000);

  QCOMPARE( cmspy.count(), 1 );
  arg = cmspy.takeFirst();
  col = arg.at(0).value<Collection>();
  QCOMPARE( col, monitorCol );
  if ( fetchCol )
    QCOMPARE( col.name(), QString("changed name") );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( crspy.isEmpty() );
  QVERIFY( csspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );

  // delete a collection
  CollectionDeleteJob *cdel = new CollectionDeleteJob( monitorCol, this );
  QVERIFY( cdel->exec() );
  QTest::qWait(1000);

  QCOMPARE( crspy.count(), 1 );
  arg = crspy.takeFirst();
  QCOMPARE( arg.at(0).value<Collection>().id(), monitorCol.id() );

  QVERIFY( caspy.isEmpty() );
  QVERIFY( cmspy.isEmpty() );
  QVERIFY( csspy.isEmpty() );
  QVERIFY( iaspy.isEmpty() );
  QVERIFY( imspy.isEmpty() );
  QVERIFY( irspy.isEmpty() );
}

#include "monitortest.moc"
