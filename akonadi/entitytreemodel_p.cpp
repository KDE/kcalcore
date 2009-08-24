/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#include "entitytreemodel_p.h"
#include "entitytreemodel.h"

#include <KDE/KIconLoader>
#include <KDE/KUrl>

#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/collectionstatisticsjob.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <kdebug.h>

using namespace Akonadi;

EntityTreeModelPrivate::EntityTreeModelPrivate( EntityTreeModel *parent )
    : q_ptr( parent ),
      m_collectionFetchStrategy( EntityTreeModel::FetchCollectionsRecursive ),
      m_itemPopulation( EntityTreeModel::ImmediatePopulation ),
      m_includeUnsubscribed( true ),
      m_includeStatistics( false ),
      m_showRootCollection( false )
{
}


int EntityTreeModelPrivate::indexOf( const QList<Node*> &nodes, Entity::Id id ) const
{
  int i = 0;
  foreach ( const Node *node, nodes ) {
    if ( node->id == id )
      return i;
    i++;
  }

  return -1;
}

ItemFetchJob* EntityTreeModelPrivate::getItemFetchJob( const Collection &parent, ItemFetchScope scope ) const
{
  ItemFetchJob *itemJob = new Akonadi::ItemFetchJob( parent, m_session );
  itemJob->setFetchScope( scope );
  return itemJob;
}

ItemFetchJob* EntityTreeModelPrivate::getItemFetchJob( const Item &item, ItemFetchScope scope ) const
{
  ItemFetchJob *itemJob = new Akonadi::ItemFetchJob( item, m_session );
  itemJob->setFetchScope( scope );
  return itemJob;
}

void EntityTreeModelPrivate::runItemFetchJob(ItemFetchJob *itemFetchJob, const Collection &parent) const
{
  Q_Q( const EntityTreeModel );

  // TODO: This hack is probably not needed anymore. Remove it.
  // ### HACK: itemsReceivedFromJob needs to know which collection items were added to.
  // That is not provided by akonadi, so we attach it in a property.
  itemFetchJob->setProperty( ItemFetchCollectionId(), QVariant( parent.id() ) );

  q->connect( itemFetchJob, SIGNAL( itemsReceived( const Akonadi::Item::List& ) ),
              q, SLOT( itemsFetched( const Akonadi::Item::List& ) ) );
  q->connect( itemFetchJob, SIGNAL( result( KJob* ) ),
              q, SLOT( fetchJobDone( KJob* ) ) );

}

void EntityTreeModelPrivate::fetchItems( const Collection &parent )
{
  Q_Q( EntityTreeModel );

  // TODO: Use a more specific fetch scope to get only the envelope for mails etc.
  ItemFetchJob *itemJob = getItemFetchJob(parent, m_monitor->itemFetchScope() );

  runItemFetchJob(itemJob, parent);
}

void EntityTreeModelPrivate::fetchCollections( const Collection &collection, CollectionFetchJob::Type type )
{
  Q_Q( EntityTreeModel );
  CollectionFetchJob *job = new CollectionFetchJob( collection, type, m_session );
  job->fetchScope().setIncludeUnsubscribed( m_includeUnsubscribed );
  job->fetchScope().setIncludeStatistics( m_includeStatistics );
  job->fetchScope().setContentMimeTypes( m_monitor->mimeTypesMonitored() );
  q->connect( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ),
              q, SLOT( collectionsFetched( const Akonadi::Collection::List& ) ) );
  q->connect( job, SIGNAL( result( KJob* ) ),
              q, SLOT( fetchJobDone( KJob* ) ) );
}

void EntityTreeModelPrivate::collectionsFetched( const Akonadi::Collection::List& collections )
{
  // TODO: refactor this stuff into separate methods for listing resources in Collection::root, and listing collections within resources.
  Q_Q( EntityTreeModel );

  Akonadi::AgentManager *agentManager = Akonadi::AgentManager::self();

  Collection::List _collections = collections;

  forever
  {
    int collectionsSize = _collections.size();

    QMutableListIterator<Akonadi::Collection> it(_collections);
    while (it.hasNext())
    {
      const Collection col = it.next();
      const Collection::Id parentId = col.parentCollection().id();
      const Collection::Id colId = col.id();

      if ( m_collections.contains( parentId ) ) {
        insertCollection( col, m_collections.value( parentId ) );

        if ( m_itemPopulation == EntityTreeModel::ImmediatePopulation )
          fetchItems( col );

        if ( m_pendingChildCollections.contains( colId ) )
        {
          QList<Collection::Id> pendingParentIds = m_pendingChildCollections.value( colId );

          foreach(const Collection::Id &id, pendingParentIds)
          {
            Collection pendingCollection = m_pendingCollections.value(id);
            Q_ASSERT( pendingCollection.isValid() );
            insertPendingCollection( pendingCollection, col, it );
            m_pendingCollections.remove(id);
          }
          if ( !it.findNext(col) && !it.findPrevious(col) )
          {
            Q_ASSERT("Something went very wrong" == "false");
          }
          m_pendingChildCollections.remove( colId );
        }

        it.remove();
      } else {
        m_pendingCollections.insert( colId, col );
        if ( !m_pendingChildCollections.value( parentId ).contains( colId ) )
          m_pendingChildCollections[ parentId ].append( colId );
      }
    }

    if ( _collections.isEmpty() )
      break; // forever

    if( _collections.size() == collectionsSize )
    {
      // Didn't process any collections this iteration.
      // Persist them until the next time collectionsFetched receives collections.
      kWarning() << "Some collections could not be inserted into the model yet.";
      break; // forever
    }
  }
}

void EntityTreeModelPrivate::itemsFetched( const Akonadi::Item::List& items )
{
  Q_Q( EntityTreeModel );
  QObject *job = q->sender();

  Q_ASSERT( job );

  const Collection::Id collectionId = job->property( ItemFetchCollectionId() ).value<Collection::Id>();
  Item::List itemsToInsert;
  Item::List itemsToUpdate;

  const Collection collection = m_collections.value( collectionId );

  Q_ASSERT( collection.isValid() );

  const QList<Node*> collectionEntities = m_childEntities.value( collectionId );
  foreach ( const Item &item, items ) {
    if ( indexOf( collectionEntities, item.id() ) != -1 ) {
      itemsToUpdate << item;
    } else {
      if ( m_mimeChecker.wantedMimeTypes().isEmpty() || m_mimeChecker.isWantedItem( item ) ) {
        itemsToInsert << item;
      }
    }
  }
  if ( itemsToInsert.size() > 0 ) {
    const int startRow = m_childEntities.value( collectionId ).size();

    const QModelIndex parentIndex = q->indexForCollection( m_collections.value( collectionId ) );
    q->beginInsertRows( parentIndex, startRow, startRow + items.size() - 1 );
    foreach ( const Item &item, items ) {
      Item::Id itemId = item.id();
      m_items.insert( itemId, item );

      Node *node = new Node;
      node->id = itemId;
      node->parent = collectionId;
      node->type = Node::Item;

      m_childEntities[ collectionId ].append( node );
    }
    q->endInsertRows();
  }

  if ( itemsToUpdate.size() > 0 )
  {
    foreach (const Item &item, itemsToUpdate)
    {
      m_items[ item.id() ].merge( item );
      foreach ( const QModelIndex &idx, q->indexesForItem( item ) )
      {
        q->dataChanged( idx, idx );
      }
    }
  }

}

void EntityTreeModelPrivate::monitoredMimeTypeChanged( const QString & mimeType, bool monitored )
{
  if ( monitored )
    m_mimeChecker.addWantedMimeType( mimeType );
  else
    m_mimeChecker.removeWantedMimeType( mimeType );
}

void EntityTreeModelPrivate::retrieveAncestors(const Akonadi::Collection& collection)
{
  Q_Q( EntityTreeModel );
  // Unlike fetchCollections, this method fetches collections by traversing up, not down.
  CollectionFetchJob *job = new CollectionFetchJob( collection.parentCollection(), CollectionFetchJob::Base, m_session );
  job->fetchScope().setIncludeUnsubscribed( m_includeUnsubscribed );
  job->fetchScope().setIncludeStatistics( m_includeStatistics );
  q->connect( job, SIGNAL( collectionsReceived( const Akonadi::Collection::List& ) ),
              q, SLOT( ancestorsFetched( const Akonadi::Collection::List& ) ) );
  q->connect( job, SIGNAL( result( KJob* ) ),
              q, SLOT( fetchJobDone( KJob* ) ) );
}

void EntityTreeModelPrivate::ancestorsFetched(const Akonadi::Collection::List& collectionList)
{
  // List is a size of one.
  foreach(const Collection &collection, collectionList)
  {
    // We should find a collection already in the tree before we reach the collection root.
    // We're looking to bridge a gap here.
    Q_ASSERT(collection != Collection::root());

    // We already checked this either on the previous recursion or in monitoredCollectionAdded.
    Q_ASSERT(!m_collections.contains(collection.id()));

    m_ancestors.prepend(collection);
    if (m_collections.contains(collection.parentCollection().id()))
    {
      m_ancestors.prepend( m_collections.value(collection.parentCollection().id()) );
      insertAncestors(m_ancestors);
    } else {
      retrieveAncestors(collection);
    }
  }
}

void EntityTreeModelPrivate::insertAncestors(const Akonadi::Collection::List& collectionList)
{
  Collection::List::const_iterator it;
  const Collection::List::const_iterator begin = collectionList.constBegin() + 1;
  const Collection::List::const_iterator end = collectionList.constEnd();
  for (it = begin; it != end; ++it)
  {
    insertCollection(*it, *(it-1));
  }
  m_ancestors.clear();
}

void EntityTreeModelPrivate::insertCollection( const Akonadi::Collection& collection, const Akonadi::Collection& parent )
{
  Q_ASSERT(collection.isValid());
  Q_ASSERT(parent.isValid());

  Q_Q( EntityTreeModel );
  // TODO: Use order attribute of parent if available
  // Otherwise prepend collections and append items. Currently this prepends all collections.

  // Or I can prepend and append for single signals, then 'change' the parent.

//   QList<qint64> childCols = m_childEntities.value( parent.id() );
//   int row = childCols.size();
//   int numChildCols = childCollections.value(parent.id()).size();

  const int row = 0;
  const QModelIndex parentIndex = q->indexForCollection( parent );
  q->beginInsertRows( parentIndex, row, row );
  m_collections.insert( collection.id(), collection );

  Node *node = new Node;
  node->id = collection.id();
  node->parent = parent.id();
  node->type = Node::Collection;
  m_childEntities[ parent.id() ].prepend( node );
  q->endInsertRows();
}

void EntityTreeModelPrivate::insertPendingCollection( const Akonadi::Collection& collection, const Akonadi::Collection& parent, QMutableListIterator<Collection> &colIt )
{
  insertCollection(collection, parent);

  m_pendingCollections.remove( collection.id() );

  if ( m_itemPopulation == EntityTreeModel::ImmediatePopulation )
    fetchItems( collection );

  if (colIt.findPrevious(collection) || colIt.findNext(collection))
  {
    colIt.remove();
  }

  Q_ASSERT(m_collections.contains(parent.id()));

  QList<Collection::Id> pendingChildCollectionsToInsert = m_pendingChildCollections.value(collection.id());

  QList<Collection::Id>::const_iterator it;
  const QList<Collection::Id>::const_iterator begin = pendingChildCollectionsToInsert.constBegin();
  const QList<Collection::Id>::const_iterator end = pendingChildCollectionsToInsert.constEnd();

  for ( it = begin; it != end; ++it )
  {
    insertPendingCollection(m_pendingCollections.value( *it ), collection, colIt);
  }

  m_pendingChildCollections.remove( parent.id() );
}

void EntityTreeModelPrivate::monitoredCollectionAdded( const Akonadi::Collection& collection, const Akonadi::Collection& parent )
{
  // If the resource is removed while populating the model with it, we might still
  // get some monitor signals. These stale/out-of-order signals can't be completely eliminated
  // in the akonadi server due to implementation details, so we also handle such signals in the model silently
  // in all the monitored slots.
  // Stephen Kelly, 28, July 2009

  // This is currently temporarily blocked by a uninitialized value bug in the server.
//   if ( !m_collections.contains( parent.id() ) )
//   {
//     kWarning() << "Got a stale notification for a collection whose parent was already removed." << collection.id() << collection.remoteId();
//     return;
//   }

  // Some collection trees contain multiple mimetypes. Even though server side filtering ensures we
  // only get the ones we're interested in from the job, we have to filter on collections received through signals too.
  if ( !m_mimeChecker.wantedMimeTypes().isEmpty() && !m_mimeChecker.isWantedCollection( collection ) )
    return;

  if (!m_collections.contains(parent.id()))
  {
    // The collection we're interested in is contained in a collection we're not interested in.
    // We download the ancestors of the collection we're interested in to complete the tree.
    m_ancestors.prepend(collection);
    retrieveAncestors(collection);
    return;
  }

  insertCollection(collection, parent);

}

void EntityTreeModelPrivate::monitoredCollectionRemoved( const Akonadi::Collection& collection )
{
  if ( !m_collections.contains( collection.parent() ) )
    return;

  Q_Q( EntityTreeModel );

  // This may be a signal for a collection we've already removed by removing its ancestor.
  if ( !m_collections.contains( collection.id() ) )
  {
    kWarning() << "Got a stale notification for a collection which was already removed." << collection.id() << collection.remoteId();
    return;
  }

  const int row = indexOf( m_childEntities.value( collection.parentCollection().id() ), collection.id() );

  const QModelIndex parentIndex = q->indexForCollection( m_collections.value( collection.parentCollection().id() ) );

  q->beginRemoveRows( parentIndex, row, row );

  // Delete all descendant collections and items.
  removeChildEntities(collection.id());

  // Remove deleted collection from its parent.
  m_childEntities[ collection.parentCollection().id() ].removeAt( row );

  q->endRemoveRows();
}

void EntityTreeModelPrivate::removeChildEntities(Collection::Id colId)
{

  QList<Node*>::const_iterator it;
  QList<Node*> childList = m_childEntities.value(colId);
  const QList<Node*>::const_iterator begin = childList.constBegin();
  const QList<Node*>::const_iterator end = childList.constEnd();
  for (it = begin; it != end; ++it)
  {
    if (Node::Item == (*it)->type)
    {
      m_items.remove((*it)->id);
    } else {
      removeChildEntities((*it)->id);
      m_collections.remove((*it)->id);
    }
  }
  m_childEntities.remove(colId);
}

void EntityTreeModelPrivate::monitoredCollectionMoved( const Akonadi::Collection& collection,
                                                       const Akonadi::Collection& sourceCollection,
                                                       const Akonadi::Collection& destCollection )
{
  if ( !m_collections.contains( collection.id() ) )
  {
    kWarning() << "Got a stale notification for a collection which was already removed." << collection.id() << collection.remoteId();
    return;
  }

  Q_Q( EntityTreeModel );

  const int srcRow = indexOf( m_childEntities.value( sourceCollection.id() ), collection.id() );

  const QModelIndex srcParentIndex = q->indexForCollection( sourceCollection );
  const QModelIndex destParentIndex = q->indexForCollection( destCollection );

  const int destRow = 0; // Prepend collections

// TODO: Uncomment for Qt4.6
//   q->beginMoveRows( srcParentIndex, srcRow, srcRow, destParentIndex, destRow );
//   Node *node = m_childEntities[ sourceCollection.id() ].takeAt( srcRow );
//   m_childEntities[ destCollection.id() ].prepend( node );
//   q->endMoveRows();
}

void EntityTreeModelPrivate::monitoredCollectionChanged( const Akonadi::Collection &collection )
{
  Q_Q( EntityTreeModel );

  if ( !m_collections.contains( collection.id() ) )
  {
    kWarning() << "Got a stale notification for a collection which was already removed." << collection.id() << collection.remoteId();
    return;
  }

  m_collections[ collection.id() ] = collection;

  const QModelIndex index = q->indexForCollection( collection );
  Q_ASSERT( index.isValid() );
  q->dataChanged( index, index );
}

void EntityTreeModelPrivate::monitoredCollectionStatisticsChanged( Akonadi::Collection::Id id,
                                                                   const Akonadi::CollectionStatistics &statistics )
{
  Q_Q( EntityTreeModel );

  if ( !m_collections.contains( id ) ) {
    kWarning() << "Got statistics response for non-existing collection:" << id;
  } else {
    m_collections[ id ].setStatistics( statistics );

    const QModelIndex index = q->indexForCollection( m_collections[ id ] );
    q->dataChanged( index, index );
  }
}

void EntityTreeModelPrivate::monitoredItemAdded( const Akonadi::Item& item, const Akonadi::Collection& collection )
{
  Q_Q( EntityTreeModel );

  if ( !m_collections.contains( collection.id() ) )
  {
    kWarning() << "Got a stale notification for an item whose collection was already removed." << item.id() << item.remoteId();
    return;
  }

  Q_ASSERT( m_collections.contains( collection.id() ) );

  if ( !m_mimeChecker.wantedMimeTypes().isEmpty() && !m_mimeChecker.isWantedItem( item ) )
    return;

  const int row = m_childEntities.value( collection.id() ).size();

  const QModelIndex parentIndex = q->indexForCollection( m_collections.value( collection.id() ) );

  q->beginInsertRows( parentIndex, row, row );
  m_items.insert( item.id(), item );
  Node *node = new Node;
  node->id = item.id();
  node->parent = collection.id();
  node->type = Node::Item;
  m_childEntities[ collection.id() ].append( node );
  q->endInsertRows();
}

void EntityTreeModelPrivate::monitoredItemRemoved( const Akonadi::Item &item )
{
  Q_Q( EntityTreeModel );

  const Collection::List parents = getParentCollections( item );
  if ( parents.isEmpty() )
    return;

  if ( !m_items.contains( item.id() ) )
  {
    kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
    return;
  }

  // TODO: Iterate over all (virtual) collections.
  const Collection collection = parents.first();

  Q_ASSERT( m_collections.contains( collection.id() ) );

  const int row = indexOf( m_childEntities.value( collection.id() ), item.id() );

  const QModelIndex parentIndex = q->indexForCollection( m_collections.value( collection.id() ) );

  q->beginRemoveRows( parentIndex, row, row );
  m_items.remove( item.id() );
  m_childEntities[ collection.id() ].removeAt( row );
  q->endRemoveRows();
}

void EntityTreeModelPrivate::monitoredItemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  Q_Q( EntityTreeModel );

  if ( !m_items.contains( item.id() ) )
  {
    kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
    return;
  }
  m_items[ item.id() ].merge(item);

  const QModelIndexList indexes = q->indexesForItem( item );
  foreach ( const QModelIndex &index, indexes )
  {

    if( !index.isValid() )
    {
      kWarning() << "item has invalid index:" << item.id() << item.remoteId();
    } else {
      q->dataChanged( index, index );
    }
  }
}

void EntityTreeModelPrivate::monitoredItemMoved( const Akonadi::Item& item,
                                                 const Akonadi::Collection& sourceCollection,
                                                 const Akonadi::Collection& destCollection )
{
  Q_Q( EntityTreeModel );

  if ( !m_items.contains( item.id() ) )
  {
    kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
    return;
  }

  Q_ASSERT( m_collections.contains( sourceCollection.id() ) );
  Q_ASSERT( m_collections.contains( destCollection.id() ) );

  const Item::Id itemId = item.id();

  const int srcRow = indexOf( m_childEntities.value( sourceCollection.id() ), itemId );

  const QModelIndex srcIndex = q->indexForCollection( sourceCollection );
  const QModelIndex destIndex = q->indexForCollection( destCollection );

  // Where should it go? Always append items and prepend collections and reorganize them with separate reactions to Attributes?

  const int destRow = q->rowCount( destIndex );

// TODO: Uncomment for Qt4.6
//   q->beginMoveRows( srcIndex, srcRow, srcRow, destIndex, destRow );
//   Node *node = m_childEntities[ sourceItem.id() ].takeAt( srcRow );
//   m_childEntities[ destItem.id() ].append( node );
//   q->endMoveRows();
}

void EntityTreeModelPrivate::monitoredItemLinked( const Akonadi::Item& item, const Akonadi::Collection& collection )
{
  Q_Q( EntityTreeModel );

  if ( !m_items.contains( item.id() ) )
  {
    kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
    return;
  }
  Q_ASSERT( m_collections.contains( collection.id() ) );

  if ( !m_mimeChecker.wantedMimeTypes().isEmpty() && !m_mimeChecker.isWantedItem( item ) )
    return;

  const int row = m_childEntities.value( collection.id() ).size();

  const QModelIndex parentIndex = q->indexForCollection( m_collections.value( collection.id() ) );

  q->beginInsertRows( parentIndex, row, row );
  Node *node = new Node;
  node->id = item.id();
  node->parent = collection.id();
  node->type = Node::Item;
  m_childEntities[ collection.id()].append( node );
  q->endInsertRows();
}

void EntityTreeModelPrivate::monitoredItemUnlinked( const Akonadi::Item& item, const Akonadi::Collection& collection )
{
  Q_Q( EntityTreeModel );

  if ( !m_items.contains( item.id() ) )
  {
    kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
    return;
  }
  Q_ASSERT( m_collections.contains( collection.id() ) );

  const int row = indexOf( m_childEntities.value( collection.id() ), item.id() );

  const QModelIndex parentIndex = q->indexForCollection( m_collections.value( collection.id() ) );

  q->beginInsertRows( parentIndex, row, row );
  m_childEntities[ collection.id() ].removeAt( row );
  q->endInsertRows();
}

void EntityTreeModelPrivate::fetchJobDone( KJob *job )
{
  Q_ASSERT(m_pendingCollections.isEmpty());
  Q_ASSERT(m_pendingChildCollections.isEmpty());

  if ( job->error() ) {
    kWarning() << "Job error: " << job->errorString() << endl;
  }
}

void EntityTreeModelPrivate::copyJobDone( KJob *job )
{
  if ( job->error() ) {
    kWarning() << "Job error: " << job->errorString() << endl;
  }
}

void EntityTreeModelPrivate::moveJobDone( KJob *job )
{
  if ( job->error() ) {
    kWarning() << "Job error: " << job->errorString() << endl;
  }
}

void EntityTreeModelPrivate::updateJobDone( KJob *job )
{
  Q_Q(EntityTreeModel);

  if ( job->error() ) {
    // TODO: handle job errors
    kWarning() << "Job error:" << job->errorString();
  } else {

    ItemModifyJob *modifyJob = qobject_cast<ItemModifyJob *>(job);
    if (!modifyJob)
      return;

    Item item = modifyJob->item();

    Q_ASSERT( item.isValid() );

    m_items[ item.id() ].merge( item );
    QModelIndexList list = q->indexesForItem( item );

    foreach (const QModelIndex &idx, list)
    {
      q->dataChanged( idx, idx );
    }

    // TODO: Is this trying to do the job of collectionstatisticschanged?
//     CollectionStatisticsJob *csjob = static_cast<CollectionStatisticsJob*>( job );
//     Collection result = csjob->collection();
//     collectionStatisticsChanged( result.id(), csjob->statistics() );
  }
}

void EntityTreeModelPrivate::startFirstListJob()
{
  Q_Q(EntityTreeModel);

  if (m_collections.size() > 0)
    return;

  Collection rootCollection;
  // Even if the root collection is the invalid collection, we still need to start
  // the first list job with Collection::root.
  if ( m_showRootCollection ) {
    rootCollection = Collection::root();
    // Notify the outside that we're putting collection::root into the model.
    q->beginInsertRows( QModelIndex(), 0, 0 );
    m_collections.insert( rootCollection.id(), rootCollection );
    m_rootNode = new Node;
    m_rootNode->id = rootCollection.id();
    m_rootNode->parent = -1;
    m_rootNode->type = Node::Collection;
    m_childEntities[ -1 ].append( m_rootNode );
    q->endInsertRows();
  } else {
    // Otherwise store it silently because it's not part of the usable model.
    rootCollection = m_rootCollection;
    m_rootNode = new Node;
    m_rootNode->id = rootCollection.id();
    m_rootNode->parent = -1;
    m_rootNode->type = Node::Collection;
    m_collections.insert( rootCollection.id(), rootCollection );
  }

  // Includes recursive trees. Lower levels are fetched in the onRowsInserted slot if
  // necessary.
  // HACK: fix this for recursive listing if we filter on mimetypes that only exit deeper
  // in the hierarchy
  if ( ( m_collectionFetchStrategy == EntityTreeModel::FetchFirstLevelChildCollections)
    /*|| ( m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsRecursive )*/ ) {
    fetchCollections( rootCollection, CollectionFetchJob::FirstLevel );
  }
  if ( m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsRecursive )
    fetchCollections( rootCollection, CollectionFetchJob::Recursive );
  // If the root collection is not collection::root, then it could have items, and they will need to be
  // retrieved now.

  if ( m_itemPopulation != EntityTreeModel::NoItemPopulation ) {
    if (rootCollection != Collection::root())
      fetchItems( rootCollection );
  }
}

Collection EntityTreeModelPrivate::getParentCollection( Entity::Id id ) const
{
  QHashIterator<Collection::Id, QList<Node*> > iter( m_childEntities );
  while ( iter.hasNext() ) {
    iter.next();
    if ( indexOf( iter.value(), id ) != -1 ) {
      return m_collections.value( iter.key() );
    }
  }

  return Collection();
}

Collection::List EntityTreeModelPrivate::getParentCollections( const Item &item ) const
{
  Collection::List list;
  QHashIterator<Collection::Id, QList<Node*> > iter( m_childEntities );
  while ( iter.hasNext() ) {
    iter.next();
    if ( indexOf( iter.value(), item.id() ) != -1 ) {
      list << m_collections.value( iter.key() );
    }
  }

  return list;
}

Collection EntityTreeModelPrivate::getParentCollection( const Collection &collection ) const
{
  return m_collections.value( collection.parentCollection().id() );
}

Entity::Id EntityTreeModelPrivate::childAt( Collection::Id id, int position, bool *ok ) const
{
  const QList<Node*> list = m_childEntities.value( id );
  if ( list.size() <= position ) {
    *ok = false;
    return 0;
  }

  *ok = true;
  return list.at( position )->id;
}


int EntityTreeModelPrivate::indexOf( Collection::Id parent, Collection::Id collectionId ) const
{
  return indexOf( m_childEntities.value( parent ), collectionId );
}

Item EntityTreeModelPrivate::getItem( Item::Id id) const
{
  if ( id > 0 )
    id *= -1;

  return m_items.value( id );
}
