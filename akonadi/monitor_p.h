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

#ifndef AKONADI_MONITOR_P_H
#define AKONADI_MONITOR_P_H

#include "monitor.h"
#include "collection.h"
#include "collectionstatisticsjob.h"
#include "collectionfetchscope.h"
#include "item.h"
#include "itemfetchscope.h"
#include "job.h"
#include <akonadi/private/notificationmessage_p.h>
#include "notificationmanagerinterface.h"
#include "entitycache_p.h"

#include <kmimetype.h>

#include <QtCore/QObject>
#include <QtCore/QTimer>

namespace Akonadi {

class Monitor;

/**
 * @internal
 */
class MonitorPrivate
{
  public:
    MonitorPrivate( Monitor *parent );
    virtual ~MonitorPrivate() {}
    void init();

    Monitor *q_ptr;
    Q_DECLARE_PUBLIC( Monitor )
    org::freedesktop::Akonadi::NotificationManager *nm;
    Collection::List collections;
    QSet<QByteArray> resources;
    QSet<Item::Id> items;
    QSet<QString> mimetypes;
    bool monitorAll;
    QList<QByteArray> sessions;
    ItemFetchScope mItemFetchScope;
    CollectionFetchScope mCollectionFetchScope;
    CollectionCache collectionCache;
    ItemCache itemCache;
    QQueue<NotificationMessage> pendingNotifications;
    QQueue<NotificationMessage> pipeline;
    bool fetchCollection;
    bool fetchCollectionStatistics;

    bool isCollectionMonitored( Collection::Id collection, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) || resources.contains( resource ) )
        return true;
      return false;
    }

    bool isItemMonitored( Item::Id item, Collection::Id collection, Collection::Id collectionDest,
                          const QString &mimetype, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) ||
           isCollectionMonitored( collectionDest ) ||items.contains( item ) ||
           resources.contains( resource ) || isMimeTypeMonitored( mimetype ) )
        return true;
      return false;
    }

    bool isSessionIgnored( const QByteArray &sessionId ) const
    {
      return sessions.contains( sessionId );
    }

    bool connectToNotificationManager();
    bool acceptNotification( const NotificationMessage &msg );
    void dispatchNotifications();
    bool ensureDataAvailable( const NotificationMessage &msg );
    void emitNotification( const NotificationMessage &msg );
    void updatePendingStatistics( const NotificationMessage &msg );
    void invalidateCaches( const NotificationMessage &msg );

    virtual int pipelineSize() const;

    // private slots
    void dataAvailable();
    void slotSessionDestroyed( QObject* );
    void slotStatisticsChangedFinished( KJob* );
    void slotFlushRecentlyChangedCollections();

    virtual void slotNotify( const NotificationMessage::List &msgs );

    void emitItemNotification( const NotificationMessage &msg, const Item &item = Item(),
                               const Collection &collection = Collection(), const Collection &collectionDest = Collection() );
    void emitCollectionNotification( const NotificationMessage &msg, const Collection &col = Collection(),
                                     const Collection &par = Collection(), const Collection &dest = Collection() );

  private:
    // collections that need a statistics update
    QSet<Collection::Id> recentlyChangedCollections;

    bool isCollectionMonitored( Collection::Id collection ) const
    {
      if ( collections.contains( Collection( collection ) ) )
        return true;
      if ( collections.contains( Collection::root() ) )
        return true;
      return false;
    }

    bool isMimeTypeMonitored( const QString& mimetype ) const
    {
      if ( mimetypes.contains( mimetype ) )
        return true;

      KMimeType::Ptr mimeType = KMimeType::mimeType( mimetype, KMimeType::ResolveAliases );
      if ( mimeType.isNull() )
        return false;

      foreach ( const QString &mt, mimetypes ) {
        if ( mimeType->is( mt ) )
          return true;
      }

      return false;
    }

    void fetchStatistics( Collection::Id colId )
    {
      CollectionStatisticsJob *job = new CollectionStatisticsJob( Collection( colId ), q_ptr );
      QObject::connect( job, SIGNAL(result(KJob*)), q_ptr, SLOT(slotStatisticsChangedFinished(KJob*)) );
    }

    void notifyCollectionStatisticsWatchers( Collection::Id collection, const QByteArray &resource )
    {
      if ( isCollectionMonitored( collection, resource ) ) {
        if (recentlyChangedCollections.empty() )
          QTimer::singleShot( 500, q_ptr, SLOT(slotFlushRecentlyChangedCollections()) );
        recentlyChangedCollections.insert( collection );
      }
    }
};

}

#endif
