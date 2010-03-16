/*
  Copyright (C) 2009 Stephen Kelly <steveire@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef FAKE_SERVER_DATA_H
#define FAKE_SERVER_DATA_H

#include <QSharedPointer>
#include <QQueue>

#include <akonadi/job.h>

#include "fakeserver.h"
#include "fakesession.h"
#include "fakemonitor.h"
#include "public_etm.h"
#include "fakeakonadiservercommand.h"

using namespace Akonadi;

class FakeServerData : public QObject
{
  Q_OBJECT
public:
  FakeServerData( PublicETM *model, FakeSession *session, FakeMonitor *monitor, QObject *parent = 0 );

  void setCommands( QList<FakeAkonadiServerCommand*> list );

  Entity::Id nextCollectionId() const { return m_nextCollectionId++; }
  Entity::Id nextItemId()       const { return m_nextItemId++;       }

signals:
  void emit_itemsFetched( const Akonadi::Item::List &list );
  void emit_collectionsFetched( const Akonadi::Collection::List &list );

private slots:
  void jobAdded( Akonadi::Job *job );

private:
  void processNotifications();
  bool returnCollections( Entity::Id fetchColId );
  void returnItems( Entity::Id fetchColId );
  void returnEntities( Entity::Id fetchColId );

private:
  PublicETM *m_model;
  FakeSession *m_session;
  FakeMonitor *m_monitor;

  QList<FakeAkonadiServerCommand*> m_commandList;
  QQueue<FakeAkonadiServerCommand*> m_communicationQueue;

  Collection::List m_recentCollections;
  QList<Collection::List> m_collectionSequence;
  QList<QHash<Item::Id, Item> > m_itemSequence;
  FakeAkonadiServer *m_fakeServer;
  int m_jobsActioned;
  QString m_serverDataString;

  mutable Entity::Id m_nextCollectionId;
  mutable Entity::Id m_nextItemId;


};

#endif
