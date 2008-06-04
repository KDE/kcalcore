/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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

//@cond PRIVATE

#include "collectionmodel_p.h"
#include "collectionmodel.h"
#include "collectionutils_p.h"

#include "collectionfetchjob.h"
#include "collectionstatistics.h"
#include "collectionstatisticsjob.h"
#include "monitor.h"
#include "session.h"

#include <kdebug.h>
#include <kjob.h>
#include <kiconloader.h>

#include <QtCore/QTimer>

using namespace Akonadi;


void CollectionModelPrivate::collectionRemoved( const Akonadi::Collection &collection )
{
  Q_Q( CollectionModel );
  QModelIndex colIndex = indexForId( collection.id() );
  if ( colIndex.isValid() ) {
    QModelIndex parentIndex = q->parent( colIndex );
    // collection is still somewhere in the hierarchy
    removeRowFromModel( colIndex.row(), parentIndex );
  } else {
    if ( collections.contains( collection.id() ) ) {
      // collection is orphan, ie. the parent has been removed already
      collections.remove( collection.id() );
      childCollections.remove( collection.id() );
    }
  }
}

void CollectionModelPrivate::collectionChanged( const Akonadi::Collection &collection )
{
  Q_Q( CollectionModel );

  // What kind of change is it ?
  Collection::Id oldParentId = collections.value( collection.id() ).parent();
  Collection::Id newParentId = collection.parent();
  if ( newParentId !=  oldParentId && oldParentId >= 0 ) { // It's a move
    removeRowFromModel( indexForId( collections[ collection.id() ].id() ).row(), indexForId( oldParentId ) );
    Collection newParent;
    if ( newParentId == Collection::root().id() )
      newParent = Collection::root();
    else
      newParent = collections.value( newParentId );
    CollectionFetchJob *job = new CollectionFetchJob( newParent, CollectionFetchJob::Recursive, session );
    job->includeUnsubscribed( unsubscribed );
    q->connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                q, SLOT(collectionsChanged(Akonadi::Collection::List)) );
    q->connect( job, SIGNAL( result( KJob* ) ),
                 q, SLOT( listDone( KJob* ) ) );

  }
  else { // It's a simple change
    CollectionFetchJob *job = new CollectionFetchJob( collection, CollectionFetchJob::Base, session );
    job->includeUnsubscribed( unsubscribed );
    q->connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                q, SLOT(collectionsChanged(Akonadi::Collection::List)) );
    q->connect( job, SIGNAL( result( KJob* ) ),
                 q, SLOT( listDone( KJob* ) ) );
  }

}

void CollectionModelPrivate::updateDone( KJob *job )
{
  if ( job->error() ) {
    // TODO: handle job errors
    kWarning( 5250 ) << "Job error:" << job->errorString();
  } else {
    CollectionStatisticsJob *csjob = static_cast<CollectionStatisticsJob*>( job );
    Collection result = csjob->collection();
    collectionStatisticsChanged( result.id(), csjob->statistics() );
  }
}

void CollectionModelPrivate::collectionStatisticsChanged( Collection::Id collection,
                               const Akonadi::CollectionStatistics &statistics )
{
  Q_Q( CollectionModel );

  if ( !collections.contains( collection ) )
    kWarning( 5250 ) << "Got statistics response for non-existing collection:" << collection;
  else {
    collections[ collection ].setStatistics( statistics );

    Collection col = collections.value( collection );
    QModelIndex startIndex = indexForId( col.id() );
    QModelIndex endIndex = indexForId( col.id(), q->columnCount( q->parent( startIndex ) ) - 1 );
    emit q->dataChanged( startIndex, endIndex );
  }
}

void CollectionModelPrivate::listDone( KJob *job )
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Job error: " << job->errorString() << endl;
  }
}

void CollectionModelPrivate::editDone( KJob * job )
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Edit failed: " << job->errorString();
  }
}

void CollectionModelPrivate::dropResult(KJob * job)
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Paste failed:" << job->errorString();
    // TODO: error handling
  }
}

void CollectionModelPrivate::collectionsChanged( const Collection::List &cols )
{
  Q_Q( CollectionModel );
  foreach( Collection col, cols ) {
    if ( collections.contains( col.id() ) ) {
      // collection already known
      col.setStatistics( collections.value( col.id() ).statistics() );
      collections[ col.id() ] = col;
      QModelIndex startIndex = indexForId( col.id() );
      QModelIndex endIndex = indexForId( col.id(), q->columnCount( q->parent( startIndex ) ) - 1 );
      emit q->dataChanged( startIndex, endIndex );
      continue;
    }
    collections.insert( col.id(), col );
    QModelIndex parentIndex = indexForId( col.parent() );
    if ( parentIndex.isValid() || col.parent() == Collection::root().id() ) {
      q->beginInsertRows( parentIndex, q->rowCount( parentIndex ), q->rowCount( parentIndex ) );
      childCollections[ col.parent() ].append( col.id() );
      q->endInsertRows();
    } else {
      childCollections[ col.parent() ].append( col.id() );
    }

    updateSupportedMimeTypes( col );

    // start a statistics job for every collection to get message counts, etc.
    if ( fetchStatistics && !CollectionUtils::isVirtualParent( col ) ) {
      CollectionStatisticsJob* csjob = new CollectionStatisticsJob( col, session );
      q->connect( csjob, SIGNAL(result(KJob*)), q, SLOT(updateDone(KJob*)) );
    }
  }
}

QModelIndex CollectionModelPrivate::indexForId( Collection::Id id, int column )
{
  Q_Q( CollectionModel );
  if ( !collections.contains( id ) )
    return QModelIndex();

  Collection::Id parentId = collections.value( id ).parent();
  // check if parent still exist or if this is an orphan collection
  if ( parentId != Collection::root().id() && !collections.contains( parentId ) )
    return QModelIndex();

  QList<Collection::Id> list = childCollections.value( parentId );
  int row = list.indexOf( id );

  if ( row >= 0 )
    return q->createIndex( row, column, reinterpret_cast<void*>( collections.value( list.at(row) ).id() ) );
  return QModelIndex();
}

bool CollectionModelPrivate::removeRowFromModel( int row, const QModelIndex & parent )
{
  Q_Q( CollectionModel );
  QList<Collection::Id> list;
  Collection parentCol;
  if ( parent.isValid() ) {
    parentCol = collections.value( parent.internalId() );
    Q_ASSERT( parentCol.id() == parent.internalId() );
    list = childCollections.value( parentCol.id() );
  } else {
    parentCol = Collection::root();
    list = childCollections.value( Collection::root().id() );
  }
  if ( row < 0 || row  >= list.size() ) {
    kWarning( 5250 ) << "Index out of bounds:" << row <<" parent:" << parentCol.id();
    return false;
  }

  q->beginRemoveRows( parent, row, row );
  Collection::Id delColId = list.takeAt( row );
  foreach( Collection::Id childColId, childCollections[ delColId ] )
    collections.remove( childColId );
  collections.remove( delColId );
  childCollections.remove( delColId ); // remove children of deleted collection
  childCollections.insert( parentCol.id(), list ); // update children of parent
  q->endRemoveRows();

  return true;
}

bool CollectionModelPrivate::supportsContentType(const QModelIndex & index, const QStringList & contentTypes)
{
  if ( !index.isValid() )
    return false;
  Collection col = collections.value( index.internalId() );
  Q_ASSERT( col.isValid() );
  QStringList ct = col.contentMimeTypes();
  foreach ( const QString &a, ct ) {
    if ( contentTypes.contains( a ) )
      return true;
  }
  return false;
}

void CollectionModelPrivate::init()
{
  Q_Q( CollectionModel );

  session = new Session( QByteArray("CollectionModel-") + QByteArray::number( qrand() ), q );
  QTimer::singleShot( 0, q, SLOT(startFirstListJob()) );

  // monitor collection changes
  monitor = new Monitor();
  monitor->setCollectionMonitored( Collection::root() );
  monitor->fetchCollection( true );

  // ### Hack to get the kmail resource folder icons
  KIconLoader::global()->addAppDir( QLatin1String( "kmail" ) );
  KIconLoader::global()->addAppDir( QLatin1String( "kdepim" ) );

  // monitor collection changes
  q->connect( monitor, SIGNAL(collectionChanged(const Akonadi::Collection&)),
              q, SLOT(collectionChanged(const Akonadi::Collection&)) );
  q->connect( monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)),
              q, SLOT(collectionChanged(Akonadi::Collection)) );
  q->connect( monitor, SIGNAL(collectionRemoved(Akonadi::Collection)),
              q, SLOT(collectionRemoved(Akonadi::Collection)) );
  q->connect( monitor, SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)),
              q, SLOT(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)) );
}

void CollectionModelPrivate::startFirstListJob()
{
  Q_Q( CollectionModel );

  // start a list job
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, session );
  job->includeUnsubscribed( unsubscribed );
  q->connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
              q, SLOT(collectionsChanged(Akonadi::Collection::List)) );
  q->connect( job, SIGNAL(result(KJob*)), q, SLOT(listDone(KJob*)) );
}

//@endcond
