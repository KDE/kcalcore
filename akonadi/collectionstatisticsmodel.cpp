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

#include "collectionstatisticsmodel.h"

#include "collection.h"
#include "collectionmodel_p.h"

#include <kdebug.h>
#include <klocale.h>

#include <QtGui/QFont>

using namespace Akonadi;

namespace Akonadi {

class CollectionStatisticsModelPrivate : public CollectionModelPrivate
{
  public:
    enum CountType { Total, Unread };
    Q_DECLARE_PUBLIC( CollectionStatisticsModel )
    CollectionStatisticsModelPrivate( CollectionStatisticsModel *parent )
        : CollectionModelPrivate( parent )
    {}

    qint64 countRecursive( Collection::Id collection, CountType type ) const;
};

}

qint64 CollectionStatisticsModelPrivate::countRecursive( Collection::Id collection,
                                                         CountType type ) const
{
  qint64 result;
  switch ( type ) {
    case Unread: result = collections.value( collection ).statistics().unreadCount();
                 break;
    case Total: result = collections.value( collection ).statistics().count();
                break;
    default: Q_ASSERT( false );
             break;
  }

  QList<Collection::Id> children = childCollections.value( collection );
  foreach( Collection::Id currentCollection, children ) {
    result += countRecursive( currentCollection, type );
  }
  return result;
}

CollectionStatisticsModel::CollectionStatisticsModel( QObject * parent ) :
    CollectionModel( new CollectionStatisticsModelPrivate( this ), parent )
{
  fetchCollectionStatistics( true );
}

int CollectionStatisticsModel::columnCount( const QModelIndex & parent ) const
{
  if ( parent.isValid() && parent.column() != 0 )
    return 0;
  return 3;
}

QVariant CollectionStatisticsModel::data( const QModelIndex & index, int role ) const
{
  Q_D( const CollectionStatisticsModel );
  if ( !index.isValid() )
    return QVariant();

  Collection col = collectionForId( CollectionModel::data( index, CollectionIdRole ).toLongLong() );
  if ( !col.isValid() )
    return QVariant();
  CollectionStatistics statistics = col.statistics();

  qint64 total = statistics.count();
  qint64 unread = statistics.unreadCount();
  qint64 totalRecursive = d->countRecursive( col.id(),
                                             CollectionStatisticsModelPrivate::Total );
  qint64 unreadRecursive = d->countRecursive( col.id(),
                                              CollectionStatisticsModelPrivate::Unread );

  if ( role == CollectionStatisticsTotalRole )
    return total;
  else if ( role == CollectionStatisticsUnreadRole )
    return unread;
  else if ( role == CollectionStatisticsUnreadRecursiveRole )
    return unreadRecursive;
  else if ( role == CollectionStatisticsTotalRecursiveRole )
    return totalRecursive;

  if ( role == Qt::DisplayRole &&
       ( index.column() == 1 || index.column() == 2 ) ) {

    qint64 value = -1;
    switch ( index.column() ) {
      case 1 : value = unread; break;
      case 2 : value = total; break;
    }
    if ( value < 0 )
      return QString();
    else if ( value == 0 )
      return QLatin1String( "-" );
    else
      return QString::number( value );
  }

  if ( role == Qt::TextAlignmentRole && ( index.column() == 1 || index.column() == 2 ) )
    return Qt::AlignRight;

  return CollectionModel::data( index, role );
}

QVariant CollectionStatisticsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    switch ( section ) {
      case 1: return i18nc( "@title:column, number of unread messages", "Unread" );
      case 2: return i18nc( "@title:column, total number of messages", "Total" );
    }

  return CollectionModel::headerData( section, orientation, role );
}

#include "collectionstatisticsmodel.moc"
