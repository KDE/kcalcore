/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

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

#include "favoritecollectionsview.h"

#include "dragdropmanager_p.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QDragMoveEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>

#include <KAction>
#include <KLocale>
#include <KMessageBox>
#include <KUrl>
#include <KXMLGUIFactory>

#include <kdebug.h>
#include <kxmlguiclient.h>

#include <akonadi/collection.h>
#include <akonadi/control.h>
#include <akonadi/item.h>
#include <akonadi/entitytreemodel.h>

using namespace Akonadi;

/**
 * @internal
 */
class FavoriteCollectionsView::Private
{
public:
  Private( FavoriteCollectionsView *parent )
      : mParent( parent ), mDragDropManager( new DragDropManager( mParent ) ), mXmlGuiClient( 0 )
  {
  }

  void init();
  void itemClicked( const QModelIndex& );
  void itemDoubleClicked( const QModelIndex& );
  void itemCurrentChanged( const QModelIndex& );

  FavoriteCollectionsView *mParent;
  DragDropManager *mDragDropManager;
  KXMLGUIClient *mXmlGuiClient;
};

void FavoriteCollectionsView::Private::init()
{
  mParent->setEditTriggers( QAbstractItemView::EditKeyPressed );
  mParent->setAcceptDrops( true );
  mParent->setDropIndicatorShown( true );
  mParent->setDragDropMode( DragDrop );
  mParent->setDragEnabled( true );
  mParent->setRootIsDecorated(false);

  mParent->connect( mParent, SIGNAL( clicked( const QModelIndex& ) ),
                    mParent, SLOT( itemClicked( const QModelIndex& ) ) );
  mParent->connect( mParent, SIGNAL( doubleClicked( const QModelIndex& ) ),
                    mParent, SLOT( itemDoubleClicked( const QModelIndex& ) ) );

  Control::widgetNeedsAkonadi( mParent );
}

void FavoriteCollectionsView::Private::itemClicked( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection collection = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() ) {
    emit mParent->clicked( collection );
  }
}

void FavoriteCollectionsView::Private::itemDoubleClicked( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection collection = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() ) {
    emit mParent->doubleClicked( collection );
  }
}

void FavoriteCollectionsView::Private::itemCurrentChanged( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection collection = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() ) {
    emit mParent->currentChanged( collection );
  }
}

FavoriteCollectionsView::FavoriteCollectionsView( QWidget * parent )
  : QTreeView( parent ),
    d( new Private( this ) )
{
  setSelectionMode( QAbstractItemView::SingleSelection );
  d->init();
}

FavoriteCollectionsView::FavoriteCollectionsView( KXMLGUIClient *xmlGuiClient, QWidget * parent )
  : QTreeView( parent ),
    d( new Private( this ) )
{
  d->mXmlGuiClient = xmlGuiClient;
  d->init();
}

FavoriteCollectionsView::~FavoriteCollectionsView()
{
  delete d->mDragDropManager;
  delete d;
}

void FavoriteCollectionsView::setModel( QAbstractItemModel * model )
{
  if ( selectionModel() ) {
    disconnect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( itemCurrentChanged( const QModelIndex& ) ) );
  }

  QTreeView::setModel( model );

  connect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
           SLOT( itemCurrentChanged( const QModelIndex& ) ) );
}

void FavoriteCollectionsView::dragMoveEvent( QDragMoveEvent * event )
{
  if ( d->mDragDropManager->dropAllowed( event ) ) {
    // All urls are supported. process the event.
    QTreeView::dragMoveEvent( event );
    return;
  }

  event->setDropAction( Qt::IgnoreAction );
}

void FavoriteCollectionsView::dropEvent( QDropEvent * event )
{
  if ( d->mDragDropManager->processDropEvent( event ) ) {
    QTreeView::dropEvent( event );
  }
}

void FavoriteCollectionsView::contextMenuEvent( QContextMenuEvent * event )
{
  if ( !d->mXmlGuiClient )
    return;

  const QModelIndex index = indexAt( event->pos() );

  QMenu *popup = 0;

  // check if the index under the cursor is a collection or item
  const Collection collection = model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() )
    popup = static_cast<QMenu*>( d->mXmlGuiClient->factory()->container(
                                 QLatin1String( "akonadi_favoriteview_contextmenu" ), d->mXmlGuiClient ) );
  if ( popup )
    popup->exec( event->globalPos() );
}

void FavoriteCollectionsView::setXmlGuiClient( KXMLGUIClient *xmlGuiClient )
{
  d->mXmlGuiClient = xmlGuiClient;
}

void FavoriteCollectionsView::startDrag( Qt::DropActions supportedActions )
{
  d->mDragDropManager->startDrag( supportedActions );
}

#include "favoritecollectionsview.moc"
