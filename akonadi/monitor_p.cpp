/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

// @cond PRIVATE

#include "monitor_p.h"

#include "collectionfetchjob.h"
#include "collectionstatistics.h"
#include "itemfetchjob.h"
#include "notificationmessage_p.h"
#include "session.h"

#include <kdebug.h>

using namespace Akonadi;

static const int PipelineSize = 5;

MonitorPrivate::MonitorPrivate(Monitor * parent) :
  q_ptr( parent ),
  nm( 0 ),
  monitorAll( false ),
  collectionCache( 3*PipelineSize ), // needs to be at least 3x pipeline size for the collection move case
  itemCache( PipelineSize ), // needs to be at least 1x pipeline size
  fetchCollection( false ),
  fetchCollectionStatistics( false )
{
}

void MonitorPrivate::init()
{
  QObject::connect( &collectionCache, SIGNAL(dataAvailable()), q_ptr, SLOT(dataAvailable()) );
  QObject::connect( &itemCache, SIGNAL(dataAvailable()), q_ptr, SLOT(dataAvailable()) );
}

bool MonitorPrivate::connectToNotificationManager()
{
  NotificationMessage::registerDBusTypes();

  if ( !nm )
    nm = new org::freedesktop::Akonadi::NotificationManager( QLatin1String( "org.freedesktop.Akonadi" ),
                                                     QLatin1String( "/notifications" ),
                                                     QDBusConnection::sessionBus(), q_ptr );
  else
    return true;

  if ( !nm ) {
    kWarning() << "Unable to connect to notification manager";
  } else {
    QObject::connect( nm, SIGNAL(notify(Akonadi::NotificationMessage::List)),
             q_ptr, SLOT(slotNotify(Akonadi::NotificationMessage::List)) );
    return true;
  }
  return false;
}

int MonitorPrivate::pipelineSize() const
{
  return PipelineSize;
}

bool MonitorPrivate::acceptNotification(const NotificationMessage & msg)
{
  if ( isSessionIgnored( msg.sessionId() ) )
    return false;
  switch ( msg.type() ) {
    case NotificationMessage::InvalidType:
      kWarning() << "Received invalid change notification!";
      return false;
    case NotificationMessage::Item:
      return isItemMonitored( msg.uid(), msg.parentCollection(), msg.parentDestCollection(), msg.mimeType(), msg.resource() )
          || isCollectionMonitored( msg.parentCollection(), msg.resource() )
          || isCollectionMonitored( msg.parentDestCollection(), msg.resource() );
    case NotificationMessage::Collection:
      return isCollectionMonitored( msg.uid(), msg.resource() )
          || isCollectionMonitored( msg.parentCollection(), msg.resource() )
          || isCollectionMonitored( msg.parentDestCollection(), msg.resource() );
  }
  Q_ASSERT( false );
  return false;
}

void MonitorPrivate::dispatchNotifications()
{
  while ( pipeline.size() < pipelineSize() && !pendingNotifications.isEmpty() ) {
    const NotificationMessage msg = pendingNotifications.dequeue();
    if ( ensureDataAvailable( msg ) && pipeline.isEmpty() )
      emitNotification( msg );
    else
      pipeline.enqueue( msg );
  }
}

bool MonitorPrivate::ensureDataAvailable( const NotificationMessage &msg )
{
  bool allCached = true;
  if ( fetchCollection ) {
    if ( !collectionCache.ensureCached( msg.parentCollection(), mCollectionFetchScope ) )
      allCached = false;
    if ( msg.operation() == NotificationMessage::Move && !collectionCache.ensureCached( msg.parentDestCollection(), mCollectionFetchScope ) )
      allCached = false;
  }
  if ( msg.operation() == NotificationMessage::Remove )
    return allCached; // the actual object is gone already, nothing to fetch there

  if ( msg.type() == NotificationMessage::Item && !mItemFetchScope.isEmpty() ) {
    if ( !itemCache.ensureCached( msg.uid(), mItemFetchScope ) )
      allCached = false;
  } else if ( msg.type() == NotificationMessage::Collection && fetchCollection ) {
    if ( !collectionCache.ensureCached( msg.uid(), mCollectionFetchScope ) )
      allCached = false;
  }
  return allCached;
}

void MonitorPrivate::emitNotification( const NotificationMessage &msg )
{
  const Collection parent = collectionCache.retrieve( msg.parentCollection() );
  Collection destParent;
  if ( msg.operation() == NotificationMessage::Move )
    destParent = collectionCache.retrieve( msg.parentDestCollection() );

  if ( msg.type() == NotificationMessage::Collection ) {
    const Collection col = collectionCache.retrieve( msg.uid() );
    emitCollectionNotification( msg, col, parent, destParent );
  } else if ( msg.type() == NotificationMessage::Item ) {
    const Item item = itemCache.retrieve( msg.uid() );
    emitItemNotification( msg, item, parent, destParent );
  }
}

void MonitorPrivate::dataAvailable()
{
  while ( !pipeline.isEmpty() ) {
    const NotificationMessage msg = pipeline.head();
    if ( ensureDataAvailable( msg ) ) {
      emitNotification( msg );
      pipeline.dequeue();
    } else {
      break;
    }
  }
  dispatchNotifications();
}

void MonitorPrivate::updatePendingStatistics( const NotificationMessage &msg )
{
  if ( msg.type() == NotificationMessage::Item ) {
    notifyCollectionStatisticsWatchers( msg.parentCollection(), msg.resource() );
  } else if ( msg.type() == NotificationMessage::Collection && msg.operation() == NotificationMessage::Remove ) {
    // no need for statistics updates anymore
    recentlyChangedCollections.remove( msg.uid() );
  }
}

void MonitorPrivate::slotSessionDestroyed( QObject * object )
{
  Session* session = qobject_cast<Session*>( object );
  if ( session )
    sessions.removeAll( session->sessionId() );
}

void MonitorPrivate::slotStatisticsChangedFinished( KJob* job )
{
  if ( job->error() ) {
    kWarning() << "Error on fetching collection statistics: " << job->errorText();
  } else {
    CollectionStatisticsJob *statisticsJob = static_cast<CollectionStatisticsJob*>( job );
    emit q_ptr->collectionStatisticsChanged( statisticsJob->collection().id(),
                                             statisticsJob->statistics() );
  }
}

void MonitorPrivate::slotFlushRecentlyChangedCollections()
{
  foreach( Collection::Id collection, recentlyChangedCollections ) {
    if ( fetchCollectionStatistics ) {
      fetchStatistics( collection );
    } else {
      static const CollectionStatistics dummyStatistics;
      emit q_ptr->collectionStatisticsChanged( collection, dummyStatistics );
    }
  }
  recentlyChangedCollections.clear();
}

void MonitorPrivate::slotNotify( const NotificationMessage::List &msgs )
{
  foreach ( const NotificationMessage &msg, msgs ) {
    invalidateCaches( msg );
    if ( acceptNotification( msg ) ) {
      updatePendingStatistics( msg );
      NotificationMessage::appendAndCompress( pendingNotifications, msg );
    }
  }

  dispatchNotifications();
}

void MonitorPrivate::emitItemNotification( const NotificationMessage &msg, const Item &item,
                                             const Collection &collection, const Collection &collectionDest  )
{
  Q_ASSERT( msg.type() == NotificationMessage::Item );
  Collection col = collection;
  Collection colDest = collectionDest;
  if ( !col.isValid() ) {
    col = Collection( msg.parentCollection() );
    col.setResource( QString::fromUtf8( msg.resource() ) );
  }
  if ( !colDest.isValid() ) {
    colDest = Collection( msg.parentDestCollection() );
    // FIXME setResource here required ?
  }
  Item it = item;
  if ( !it.isValid() || msg.operation() == NotificationMessage::Remove ) {
    it = Item( msg.uid() );
    it.setRemoteId( msg.remoteId() );
    it.setMimeType( msg.mimeType() );
  }
  if ( !it.parentCollection().isValid() ) {
    if ( msg.operation() == NotificationMessage::Move )
      it.setParentCollection( colDest );
    else
      it.setParentCollection( col );
  }
  switch ( msg.operation() ) {
    case NotificationMessage::Add:
      emit q_ptr->itemAdded( it, col );
      break;
    case NotificationMessage::Modify:
      emit q_ptr->itemChanged( it, msg.itemParts() );
      break;
    case NotificationMessage::Move:
      emit q_ptr->itemMoved( it, col, colDest );
      break;
    case NotificationMessage::Remove:
      emit q_ptr->itemRemoved( it );
      emit q_ptr->itemRemoved( it, col );
      break;
    case NotificationMessage::Link:
      emit q_ptr->itemLinked( it, col );
      break;
    case NotificationMessage::Unlink:
      emit q_ptr->itemUnlinked( it, col );
      break;
    default:
      kDebug() << "Unknown operation type" << msg.operation() << "in item change notification";
      break;
  }
}

void MonitorPrivate::emitCollectionNotification( const NotificationMessage &msg, const Collection &col,
                                                   const Collection &par, const Collection &dest )
{
  Q_ASSERT( msg.type() == NotificationMessage::Collection );
  Collection collection = col;
  if ( !collection.isValid() || msg.operation() == NotificationMessage::Remove ) {
    collection = Collection( msg.uid() );
    collection.setResource( QString::fromUtf8( msg.resource() ) );
    collection.setRemoteId( msg.remoteId() );
  }

  Collection parent = par;
  if ( !parent.isValid() )
    parent = Collection( msg.parentCollection() );
  Collection destination = dest;
  if ( !destination.isValid() )
    destination = Collection( msg.parentDestCollection() );

  if ( !collection.parentCollection().isValid() ) {
    if ( msg.operation() == NotificationMessage::Move )
      collection.setParentCollection( destination );
    else
      collection.setParentCollection( parent );
  }

  switch ( msg.operation() ) {
    case NotificationMessage::Add:
      emit q_ptr->collectionAdded( collection, parent );
      break;
    case NotificationMessage::Modify:
      emit q_ptr->collectionChanged( collection );
      break;
    case NotificationMessage::Move:
      emit q_ptr->collectionMoved( collection, parent, destination );
      break;
    case NotificationMessage::Remove:
      emit q_ptr->collectionRemoved( collection );
      break;
    default:
      kDebug() << "Unknown operation type" << msg.operation() << "in collection change notification";
  }
}

void MonitorPrivate::invalidateCaches( const NotificationMessage &msg )
{
  // remove invalidates
  if ( msg.operation() == NotificationMessage::Remove ) {
    if ( msg.type() == NotificationMessage::Collection ) {
      collectionCache.invalidate( msg.uid() );
    } else if ( msg.type() == NotificationMessage::Item ) {
      itemCache.invalidate( msg.uid() );
    }
  }

  // modify removes the cache entry, as we need to re-fetch
  if ( msg.operation() == NotificationMessage::Modify ) {
    if ( msg.type() == NotificationMessage::Collection ) {
      collectionCache.update( msg.uid(), mCollectionFetchScope );
    } else if ( msg.type() == NotificationMessage::Item ) {
      itemCache.update( msg.uid(), mItemFetchScope );
    }
  }
}

// @endcond
