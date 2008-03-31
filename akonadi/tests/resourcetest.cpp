/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include <QtCore/QMetaType>
#include <QtCore/QObject>

#include <qtest_kde.h>

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstancecreatejob.h>

using namespace Akonadi;

class ResourceTest : public QObject
{
  Q_OBJECT
  private slots:
    void testResourceManagement()
    {
      QSignalSpy spyAddInstance( AgentManager::self(), SIGNAL(instanceAdded(AgentInstance)) );
      QVERIFY( spyAddInstance.isValid() );
      QSignalSpy spyRemoveInstance( AgentManager::self(), SIGNAL(instanceRemoved(AgentInstance)) );
      QVERIFY( spyRemoveInstance.isValid() );

      AgentType type = AgentManager::self()->type( "akonadi_knut_resource" );
      QVERIFY( type.isValid() );

      QCOMPARE( type.capabilities(), QStringList( "Resource" ) );

      AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type );
      QVERIFY( job->exec() );
      AgentInstance instance = job->instance();
      QVERIFY( instance.isValid() );

      QTest::qWait( 2000 );
      QCOMPARE( spyAddInstance.count(), 1 );
      QCOMPARE( spyAddInstance.first().at( 0 ).value<AgentInstance>(), instance );
      QVERIFY( AgentManager::self()->instance( instance.identifier() ).isValid() );

      job = new AgentInstanceCreateJob( type );
      QVERIFY( job->exec() );
      AgentInstance instance2 = job->instance();
      QVERIFY( !( instance == instance2 ) );
      QTest::qWait( 2000 );
      QCOMPARE( spyAddInstance.count(), 2 );

      AgentManager::self()->removeInstance( instance );
      AgentManager::self()->removeInstance( instance2 );
      QTest::qWait( 2000 );
      QCOMPARE( spyRemoveInstance.count(), 2 );
      QVERIFY( !AgentManager::self()->instances().contains( instance ) );
      QVERIFY( !AgentManager::self()->instances().contains( instance2 ) );
    }

    void testIllegalResourceManagement()
    {
      AgentInstanceCreateJob *job = new AgentInstanceCreateJob( AgentManager::self()->type( "non_existing_resource" ) );
      QVERIFY( job->exec() );
      AgentInstance instance = job->instance();
      QVERIFY( !instance.isValid() );

      // unique agent
      const AgentType type = AgentManager::self()->type( "akonadi_mailthreader_agent" );
      QVERIFY( type.isValid() );
      job = new AgentInstanceCreateJob( type );
      QVERIFY( job->exec() );
      instance = job->instance();
      QVERIFY( !instance.isValid() );
    }
};

QTEST_KDEMAIN( ResourceTest, NoGUI )

#include "resourcetest.moc"
