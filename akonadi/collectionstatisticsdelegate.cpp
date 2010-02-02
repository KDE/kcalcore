/*
    Copyright (c) 2008 Thomas McGuire <thomas.mcguire@gmx.net>

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

#include "collectionstatisticsdelegate.h"
#include "collectionstatisticsmodel.h"

#include <kcolorscheme.h>
#include <kdebug.h>

#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>
#include <QtGui/QStyleOptionViewItemV4>
#include <QtGui/QTreeView>

#include "entitytreemodel.h"
#include "collectionstatistics.h"
#include "collection.h"

using namespace Akonadi;

namespace Akonadi {

enum CountType
{
  UnreadCount,
  TotalCount
};

class CollectionStatisticsDelegatePrivate
{
  public:
    QTreeView *parent;
    bool drawUnreadAfterFolder;

    CollectionStatisticsDelegatePrivate( QTreeView *treeView )
        : parent( treeView ),
          drawUnreadAfterFolder( false )
    {
    }

    template<CountType countType>
    qint64 getCountRecursive( const QModelIndex &index ) const
    {
      Collection collection = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );
      CollectionStatistics statistics = collection.statistics();
      qint64 count = countType == UnreadCount ? statistics.unreadCount() : statistics.count();

      if ( index.model()->hasChildren( index ) )
      {
        for ( int row = 0; row < index.model()->rowCount( index ); row++ )
        {
          static const int column = 0;
          count += getCountRecursive<countType>( index.model()->index( row, column, index ) );
        }
      }
      return count;
    }
};

}

CollectionStatisticsDelegate::CollectionStatisticsDelegate( QTreeView *parent )
  : QStyledItemDelegate( parent ),
    d_ptr( new CollectionStatisticsDelegatePrivate( parent ) )
{
}

CollectionStatisticsDelegate::~CollectionStatisticsDelegate()
{
  delete d_ptr;
}

void CollectionStatisticsDelegate::setUnreadCountShown( bool enable )
{
  Q_D( CollectionStatisticsDelegate );
  d->drawUnreadAfterFolder = enable;
}

bool CollectionStatisticsDelegate::unreadCountShown() const
{
  Q_D( const CollectionStatisticsDelegate );
  return d->drawUnreadAfterFolder;
}

void CollectionStatisticsDelegate::initStyleOption( QStyleOptionViewItem *option,
                                                    const QModelIndex &index ) const
{
  QStyleOptionViewItemV4 *noTextOption =
      qstyleoption_cast<QStyleOptionViewItemV4 *>( option );
  QStyledItemDelegate::initStyleOption( noTextOption, index );
  noTextOption->text.clear();
}

void CollectionStatisticsDelegate::paint( QPainter *painter,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index ) const
{
  Q_D( const CollectionStatisticsDelegate );

  // First, paint the basic, but without the text. We remove the text
  // in initStyleOption(), which gets called by QStyledItemDelegate::paint().
  QStyledItemDelegate::paint( painter, option, index );

  // No, we retrieve the correct style option by calling intiStyleOption from
  // the superclass.
  QStyleOptionViewItemV4 option4 = option;
  QStyledItemDelegate::initStyleOption( &option4, index );
  QString text = option4.text;

  // Now calculate the rectangle for the text
  QStyle *s = d->parent->style();
  const QWidget *widget = option4.widget;
  QRect textRect = s->subElementRect( QStyle::SE_ItemViewItemText, &option4, widget );

   // When checking if the item is expanded, we need to check that for the first
  // column, as Qt only recogises the index as expanded for the first column
  QModelIndex firstColumn = index.model()->index( index.row(), 0, index.parent() );
  bool expanded = d->parent->isExpanded( firstColumn );

  if ( option.state & QStyle::State_Selected ) {
    painter->save();
    painter->setPen( option.palette.highlightedText().color() );
  }

  Collection collection = index.sibling( index.row(), 0 ).data( EntityTreeModel::CollectionRole ).value<Collection>();

  Q_ASSERT(collection.isValid());

  CollectionStatistics statistics = collection.statistics();

  qint64 unreadCount = statistics.unreadCount();
  qint64 unreadRecursiveCount = d->getCountRecursive<UnreadCount>( index.sibling( index.row(), 0 ) );

  // Draw the unread count after the folder name (in parenthesis)
  if ( d->drawUnreadAfterFolder && index.column() == 0 ) {
    // Construct the string which will appear after the foldername (with the
    // unread count)
    QString unread;
//     qDebug() << expanded << unreadCount << unreadRecursiveCount;
    if ( expanded && unreadCount > 0 )
      unread = QString( QLatin1String(" (%1)") ).arg( unreadCount );
    else if ( !expanded ) {
      if ( unreadCount != unreadRecursiveCount )
        unread = QString( QLatin1String(" (%1 + %2)") ).arg( unreadCount ).arg( unreadRecursiveCount - unreadCount );
      else if ( unreadCount > 0 )
        unread = QString( QLatin1String(" (%1)") ).arg( unreadCount );
    }

    painter->save();

    if ( !unread.isEmpty() ) {
      QFont font = painter->font();
      font.setBold( true );
      painter->setFont( font );
    }

    // Squeeze the folder text if it is to big and calculate the rectangles
    // where the folder text and the unread count will be drawn to
    QString folderName = text;
    QFontMetrics fm( painter->fontMetrics() );
    int unreadWidth = fm.width( unread );
    if ( fm.width( folderName ) + unreadWidth > textRect.width() ) {
      folderName = fm.elidedText( folderName, Qt::ElideRight,
                                  textRect.width() - unreadWidth );
    }
    int folderWidth = fm.width( folderName );
    QRect folderRect = textRect;
    QRect unreadRect = textRect;
    folderRect.setRight( textRect.left() + folderWidth );
    unreadRect.setLeft( folderRect.right() );

    // Draw folder name and unread count
    painter->drawText( folderRect, Qt::AlignLeft, folderName );
    KColorScheme::ColorSet cs = ( option.state & QStyle::State_Selected ) ?
                                 KColorScheme::Selection : KColorScheme::View;
    QColor unreadColor = KColorScheme( QPalette::Active, cs ).
                                   foreground( KColorScheme::LinkText ).color();
    painter->setPen( unreadColor );
    painter->drawText( unreadRect, Qt::AlignLeft, unread );
    painter->restore();

    if ( option.state & QStyle::State_Selected ) {
      painter->restore();
    }
    return;
  }

  // For the unread/total column, paint the summed up count if the item
  // is collapsed
  if ( ( index.column() == 1 || index.column() == 2 ) ) {

    painter->save();

    QStyleOptionViewItem opt = option;

    QString sumText;
    if ( index.column() == 1 && ( ( !expanded && unreadRecursiveCount > 0 ) || ( expanded && unreadCount > 0 ) ) ) {
      QFont font = painter->font();
      font.setBold( true );
      painter->setFont( font );
      sumText = QString::number( expanded ? unreadCount : unreadRecursiveCount );
    } else {

      qint64 totalCount = statistics.unreadCount();
      qint64 totalRecursiveCount = d->getCountRecursive<TotalCount>( index.sibling( index.row(), 0 ) );
      if (index.column() == 2 && ( ( !expanded && totalRecursiveCount > 0 ) || ( expanded && totalCount > 0 ) ) ) {
        sumText = QString::number( expanded ? totalCount : totalRecursiveCount );
      }
    }

    painter->drawText( textRect, Qt::AlignRight, sumText );
    painter->restore();

    if ( option.state & QStyle::State_Selected ) {
      painter->restore();
    }
    return;
  }

  painter->drawText( textRect, option4.displayAlignment, text );

  if ( option.state & QStyle::State_Selected ) {
    painter->restore();
  }
}

#include "collectionstatisticsdelegate.moc"
