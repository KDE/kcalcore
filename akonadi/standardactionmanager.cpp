/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "standardactionmanager.h"

#include "agentmanager.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionmodel.h"
#include "collectionutils_p.h"
#include "collectionpropertiesdialog.h"
#include "favoritecollectionsmodel.h"
#include "itemdeletejob.h"
#include "itemmodel.h"
#include "pastehelper_p.h"
#include "subscriptiondialog_p.h"

#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KDebug>
#include <KInputDialog>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>

#include <QtCore/QMimeData>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QItemSelectionModel>

#include <boost/static_assert.hpp>

Q_DECLARE_METATYPE(QModelIndex)

using namespace Akonadi;

//@cond PRIVATE

static const struct {
  const char *name;
  const char *label;
  const char *icon;
  int shortcut;
  const char* slot;
  bool isActionMenu;
} actionData[] = {
  { "akonadi_collection_create", I18N_NOOP("&New Folder..."), "folder-new", 0, SLOT(slotCreateCollection()), false },
  { "akonadi_collection_copy", 0, "edit-copy", 0, SLOT(slotCopyCollections()), false },
  { "akonadi_collection_delete", I18N_NOOP("&Delete Folder"), "edit-delete", 0, SLOT(slotDeleteCollection()), false },
  { "akonadi_collection_sync", I18N_NOOP("&Synchronize Folder"), "view-refresh", Qt::Key_F5, SLOT(slotSynchronizeCollection()), false },
  { "akonadi_collection_properties", I18N_NOOP("Folder &Properties"), "configure", 0, SLOT(slotCollectionProperties()), false },
  { "akonadi_item_copy", 0, "edit-copy", 0, SLOT(slotCopyItems()), false },
  { "akonadi_paste", I18N_NOOP("&Paste"), "edit-paste", Qt::CTRL + Qt::Key_V, SLOT(slotPaste()), false },
  { "akonadi_item_delete", 0, "edit-delete", Qt::Key_Delete, SLOT(slotDeleteItems()), false },
  { "akonadi_manage_local_subscriptions", I18N_NOOP("Manage Local &Subscriptions..."), 0, 0, SLOT(slotLocalSubscription()), false },
  { "akonadi_collection_add_to_favorites", I18N_NOOP("Add to Favorite Folders"), "bookmark-new", 0, SLOT(slotAddToFavorites()), false },
  { "akonadi_collection_remove_from_favorites", I18N_NOOP("Remove from Favorite Folders"), "edit-delete", 0, SLOT(slotRemoveFromFavorites()), false },
  { "akonadi_collection_rename_favorite", I18N_NOOP("Rename Favorite..."), "edit-rename", 0, SLOT(slotRenameFavorite()), false },
  { "akonadi_collection_copy_to_menu", I18N_NOOP("Copy Folder To..."), "edit-copy", 0, SLOT(slotCopyCollectionTo(QAction*)), true },
  { "akonadi_item_copy_to_menu", I18N_NOOP("Copy Item To..."), "edit-copy", 0, SLOT(slotCopyItemTo(QAction*)), true }
};
static const int numActionData = sizeof actionData / sizeof *actionData;

BOOST_STATIC_ASSERT( numActionData == StandardActionManager::LastType );

static bool canCreateCollection( const Collection &collection )
{
  if ( !( collection.rights() & Collection::CanCreateCollection ) )
    return false;

  if ( !collection.contentMimeTypes().contains( Collection::mimeType() ) )
    return false;

  return true;
}

/**
 * @internal
 */
class StandardActionManager::Private
{
  public:
    Private( StandardActionManager *parent ) :
      q( parent ),
      collectionSelectionModel( 0 ),
      itemSelectionModel( 0 ),
      favoritesModel( 0 ),
      favoriteSelectionModel( 0 )
    {
      actions.fill( 0, StandardActionManager::LastType );

      pluralLabels.insert( StandardActionManager::CopyCollections, ki18np( "&Copy Folder", "&Copy %1 Folders" ) );
      pluralLabels.insert( StandardActionManager::CopyItems, ki18np( "&Copy Item", "&Copy %1 Items" ) );
      pluralLabels.insert( StandardActionManager::DeleteItems, ki18np( "&Delete Item", "&Delete %1 Items" ) );
    }

    void enableAction( StandardActionManager::Type type, bool enable )
    {
      Q_ASSERT( type >= 0 && type < StandardActionManager::LastType );
      if ( actions[type] )
        actions[type]->setEnabled( enable );

      // Update the action menu
      KActionMenu *actionMenu = qobject_cast<KActionMenu*>( actions[type] );
      if ( actionMenu ) {
        actionMenu->menu()->clear();
        if ( enable ) {
          fillFoldersMenu( type,
                           actionMenu->menu(),
                           collectionSelectionModel->model(),
                           QModelIndex() );
        }
      }
    }

    void updatePluralLabel( StandardActionManager::Type type, int count )
    {
      Q_ASSERT( type >= 0 && type < StandardActionManager::LastType );
      if ( actions[type] && pluralLabels.contains( type ) && !pluralLabels.value( type ).isEmpty() ) {
        actions[type]->setText( pluralLabels.value( type ).subs( qMax( count, 1 ) ).toString() );
      }
    }

    void copy( QItemSelectionModel* selModel )
    {
      Q_ASSERT( selModel );
      if ( selModel->selectedRows().count() <= 0 )
        return;
      QMimeData *mimeData = selModel->model()->mimeData( selModel->selectedRows() );
      QApplication::clipboard()->setMimeData( mimeData );
    }

    void updateActions()
    {
      bool singleColSelected = false;
      bool multiColSelected = false;
      int colCount = 0;
      QModelIndex selectedIndex;
      if ( collectionSelectionModel ) {
        colCount = collectionSelectionModel->selectedRows().count();
        singleColSelected = colCount == 1;
        multiColSelected = colCount > 0;
        if ( singleColSelected )
          selectedIndex = collectionSelectionModel->selectedRows().first();
      }

      enableAction( CopyCollections, multiColSelected );
      enableAction( CollectionProperties, singleColSelected );

      Collection col;
      if ( singleColSelected && selectedIndex.isValid() ) {
        col = selectedIndex.data( CollectionModel::CollectionRole ).value<Collection>();
        enableAction( CreateCollection, canCreateCollection( col ) );
        enableAction( DeleteCollections, col.rights() & Collection::CanDeleteCollection );
        enableAction( CopyCollections, multiColSelected && (col != Collection::root()) );
        enableAction( CollectionProperties, singleColSelected && (col != Collection::root()) );
        enableAction( SynchronizeCollections, CollectionUtils::isResource( col ) || CollectionUtils::isFolder( col ) );
        enableAction( Paste, PasteHelper::canPaste( QApplication::clipboard()->mimeData(), col ) );
        enableAction( AddToFavoriteCollections, (favoritesModel!=0) && (!favoritesModel->collections().contains(col))
                                             && singleColSelected && (col != Collection::root()) );
        enableAction( RemoveFromFavoriteCollections, (favoritesModel!=0) && (favoritesModel->collections().contains(col))
                                                  && singleColSelected && (col != Collection::root()) );
        enableAction( RenameFavoriteCollection, (favoritesModel!=0) && (favoritesModel->collections().contains(col))
                                             && singleColSelected && (col != Collection::root()) );
        enableAction( CopyCollectionToMenu, multiColSelected && (col != Collection::root()) );
      } else {
        enableAction( CreateCollection, false );
        enableAction( DeleteCollections, false );
        enableAction( SynchronizeCollections, false );
        enableAction( Paste, false );
        enableAction( AddToFavoriteCollections, false );
        enableAction( RemoveFromFavoriteCollections, false );
        enableAction( RenameFavoriteCollection, false );
      }

      bool multiItemSelected = false;
      int itemCount = 0;
      if ( itemSelectionModel ) {
        itemCount = itemSelectionModel->selectedRows().count();
        multiItemSelected = itemCount > 0;
      }

      enableAction( CopyItems, multiItemSelected );
      const bool canDeleteItem = !col.isValid() || (col.rights() & Collection::CanDeleteItem);
      enableAction( DeleteItems, multiItemSelected && canDeleteItem );

      enableAction( CopyItemToMenu, multiItemSelected );

      updatePluralLabel( CopyCollections, colCount );
      updatePluralLabel( CopyItems, itemCount );
      updatePluralLabel( DeleteItems, itemCount );

      emit q->actionStateUpdated();
    }

    void clipboardChanged( QClipboard::Mode mode )
    {
      if ( mode == QClipboard::Clipboard )
        updateActions();
    }

    QItemSelection mapToEntityTreeModel( const QAbstractItemModel *model, const QItemSelection &selection )
    {
      const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>( model );
      if ( proxy ) {
        return mapToEntityTreeModel( proxy->sourceModel(), proxy->mapSelectionToSource( selection ) );
      } else {
        return selection;
      }
    }

    QItemSelection mapFromEntityTreeModel( const QAbstractItemModel *model, const QItemSelection &selection )
    {
      const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>( model );
      if ( proxy ) {
        QItemSelection select = mapFromEntityTreeModel( proxy->sourceModel(), selection );
        return proxy->mapSelectionFromSource( select );
      } else {
        return selection;
      }
    }

    void collectionSelectionChanged()
    {
      q->blockSignals(true);

      QItemSelection selection = collectionSelectionModel->selection();
      selection = mapToEntityTreeModel( collectionSelectionModel->model(), selection );
      selection = mapFromEntityTreeModel( favoritesModel, selection );

      if ( favoriteSelectionModel )
        favoriteSelectionModel->select( selection, QItemSelectionModel::ClearAndSelect );

      q->blockSignals(false);

      updateActions();
    }

    void favoriteSelectionChanged()
    {
      q->blockSignals(true);

      QItemSelection selection = favoriteSelectionModel->selection();
      if ( selection.indexes().isEmpty() ) return;
      selection = mapToEntityTreeModel( favoritesModel, selection );
      selection = mapFromEntityTreeModel( collectionSelectionModel->model(), selection );
      collectionSelectionModel->select( selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );

      q->blockSignals(false);

      updateActions();
    }

    void slotCreateCollection()
    {
      Q_ASSERT( collectionSelectionModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      if ( !canCreateCollection( collection ) )
        return;

      const QString name = KInputDialog::getText( i18nc( "@title:window", "New Folder"),
                                                  i18nc( "@label:textbox, name of a thing", "Name"),
                                                  QString(), 0, parentWidget );
      if ( name.isEmpty() )
        return;
      Collection::Id parentId = index.data( CollectionModel::CollectionIdRole ).toLongLong();
      if ( parentId <= 0 )
        return;

      Collection col;
      col.setName( name );
      col.setParent( parentId );
      CollectionCreateJob *job = new CollectionCreateJob( col );
      q->connect( job, SIGNAL(result(KJob*)), q, SLOT(collectionCreationResult(KJob*)) );
    }

    void slotCopyCollections()
    {
      copy( collectionSelectionModel );
    }

    void slotDeleteCollection()
    {
      Q_ASSERT( collectionSelectionModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      QString text = i18n( "Do you really want to delete folder '%1' and all its sub-folders?", index.data().toString() );
      if ( CollectionUtils::isVirtual( collection ) )
        text = i18n( "Do you really want to delete the search view '%1'?", index.data().toString() );

      if ( KMessageBox::questionYesNo( parentWidget, text,
           i18n("Delete folder?"), KStandardGuiItem::del(), KStandardGuiItem::cancel(),
           QString(), KMessageBox::Dangerous ) != KMessageBox::Yes )
        return;
      const Collection::Id colId = index.data( CollectionModel::CollectionIdRole ).toLongLong();
      if ( colId <= 0 )
        return;

      CollectionDeleteJob *job = new CollectionDeleteJob( Collection( colId ), q );
      q->connect( job, SIGNAL(result(KJob*)), q, SLOT(collectionDeletionResult(KJob*)) );
    }

    void slotSynchronizeCollection()
    {
      Q_ASSERT( collectionSelectionModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection col = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( col.isValid() );

      AgentManager::self()->synchronizeCollection( col );
    }

    void slotCollectionProperties()
    {
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;
      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      Collection col = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( col.isValid() );

      CollectionPropertiesDialog* dlg = new CollectionPropertiesDialog( col, parentWidget );
      dlg->show();
    }

    void slotCopyItems()
    {
      copy( itemSelectionModel );
    }

    void slotPaste()
    {
      Q_ASSERT( collectionSelectionModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection col = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( col.isValid() );

      KJob *job = PasteHelper::paste( QApplication::clipboard()->mimeData(), col );
      q->connect( job, SIGNAL(result(KJob*)), q, SLOT(pasteResult(KJob*)) );
    }

    void slotDeleteItems()
    {
      if ( KMessageBox::questionYesNo( parentWidget,
           i18n( "Do you really want to delete all selected items?" ),
           i18n("Delete?"), KStandardGuiItem::del(), KStandardGuiItem::cancel(),
           QString(), KMessageBox::Dangerous ) != KMessageBox::Yes )
        return;

      Q_ASSERT( itemSelectionModel );

      // TODO: fix this once ItemModifyJob can handle item lists
      foreach ( const QModelIndex &index, itemSelectionModel->selectedRows() ) {
        bool ok;
        qlonglong id = index.data( ItemModel::IdRole ).toLongLong(&ok);
        Q_ASSERT(ok);
        new ItemDeleteJob( Item( id ), q );
      }
    }

    void slotLocalSubscription()
    {
      SubscriptionDialog* dlg = new SubscriptionDialog( parentWidget );
      dlg->show();
    }

    void slotAddToFavorites()
    {
      Q_ASSERT( collectionSelectionModel );
      Q_ASSERT( favoritesModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      favoritesModel->addCollection( collection );
    }

    void slotRemoveFromFavorites()
    {
      Q_ASSERT( collectionSelectionModel );
      Q_ASSERT( favoritesModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      favoritesModel->removeCollection( collection );
    }

    void slotRenameFavorite()
    {
      Q_ASSERT( collectionSelectionModel );
      Q_ASSERT( favoritesModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      bool ok;
      QString label = KInputDialog::getText( i18n( "Rename Favorite" ),
                                             i18nc( "@label:textbox New name of the folder.", "Name:" ),
                                             index.data().toString(), &ok, parentWidget );
      if ( !ok )
        return;

      favoritesModel->setFavoriteLabel( collection, label );
    }

    void slotCopyCollectionTo( QAction *action )
    {
      copyTo( collectionSelectionModel, action );
    }

    void slotCopyItemTo( QAction *action )
    {
      copyTo( itemSelectionModel, action );
    }

    void copyTo( QItemSelectionModel *selectionModel, QAction *action )
    {
      Q_ASSERT( selectionModel );
      if ( selectionModel->selectedRows().count() <= 0 )
        return;

      QMimeData *mimeData = selectionModel->model()->mimeData( selectionModel->selectedRows() );

      Q_ASSERT( collectionSelectionModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection col = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( col.isValid() );

      KJob *job = PasteHelper::paste( mimeData, col );
      q->connect( job, SIGNAL(result(KJob*)), q, SLOT(copyToResult(KJob*)) );
    }

    void collectionCreationResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( parentWidget, i18n("Could not create folder: %1", job->errorString()),
                            i18n("Folder creation failed") );
      }
    }

    void collectionDeletionResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( parentWidget, i18n("Could not delete folder: %1", job->errorString()),
                            i18n("Folder deletion failed") );
      }
    }

    void pasteResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( parentWidget, i18n("Could not paste data: %1", job->errorString()),
                            i18n("Paste failed") );
      }
    }

    void copyToResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( parentWidget, i18n("Could not copy data: %1", job->errorString()),
                            i18n("Copy failed") );
      }
    }

    void fillFoldersMenu( StandardActionManager::Type type, QMenu *menu,
                          const QAbstractItemModel *model, QModelIndex parentIndex )
    {
      int rowCount = model->rowCount( parentIndex );

      for ( int row = 0; row < rowCount; row++ ) {
        QModelIndex index = model->index( row, 0, parentIndex );
        Collection collection = model->data( index, CollectionModel::CollectionRole ).value<Collection>();

        if ( CollectionUtils::isVirtual( collection ) ) {
          continue;
        }

        QString label = model->data( index ).toString();
        label.replace( QString::fromUtf8( "&" ), QString::fromUtf8( "&&" ) );
        QIcon icon = model->data( index, Qt::DecorationRole ).value<QIcon>();

        bool readOnly = CollectionUtils::isStructural( collection )
                     || ( type == CopyItemToMenu && !( collection.rights() & Collection::CanCreateItem ) )
                     || ( type == CopyCollectionToMenu && !( collection.rights() & Collection::CanCreateCollection ) );

        if ( model->rowCount( index ) > 0 ) {
          // new level
          QMenu* popup = new QMenu( menu );

          popup->setObjectName( QString::fromUtf8( "subMenu" ) );
          popup->setTitle( label );
          popup->setIcon( icon );

          fillFoldersMenu( type, popup, model, index );

          if ( !readOnly ) {
            popup->addSeparator();

            QAction *act = popup->addAction( i18n("Copy to This Folder") );
            act->setData( QVariant::fromValue<QModelIndex>( index ) );
          }

          menu->addMenu( popup );

        } else {

          // insert an item
          QAction* act = menu->addAction( icon, label );
          act->setData( QVariant::fromValue<QModelIndex>( index ) );
          act->setEnabled( !readOnly );
        }
      }
    }

    void checkModelsConsistency()
    {
      if ( favoritesModel==0 || favoriteSelectionModel==0 ) {
        // No need to check when the favorite collections feature is not used
        return;
      }

      // Check that the collection selection model maps to the same
      // EntityTreeModel than favoritesModel
      if ( collectionSelectionModel!=0 ) {
        const QAbstractItemModel *model = collectionSelectionModel->model();
        while ( const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>( model ) ) {
          model = proxy->sourceModel();
        }
        Q_ASSERT( model == favoritesModel->sourceModel() );
      }

      // Check that the favorite selection model maps to favoritesModel
      const QAbstractItemModel *model = favoriteSelectionModel->model();
      while ( const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>( model ) ) {
        model = proxy->sourceModel();
      }
      Q_ASSERT( model == favoritesModel->sourceModel() );
    }

    StandardActionManager *q;
    KActionCollection *actionCollection;
    QWidget *parentWidget;
    QItemSelectionModel *collectionSelectionModel;
    QItemSelectionModel *itemSelectionModel;
    FavoriteCollectionsModel *favoritesModel;
    QItemSelectionModel *favoriteSelectionModel;
    QVector<KAction*> actions;
    AgentManager *agentManager;
    QHash<StandardActionManager::Type, KLocalizedString> pluralLabels;
};

//@endcond

StandardActionManager::StandardActionManager( KActionCollection * actionCollection,
                                              QWidget * parent) :
    QObject( parent ),
    d( new Private( this ) )
{
  d->parentWidget = parent;
  d->actionCollection = actionCollection;
  connect( QApplication::clipboard(), SIGNAL(changed(QClipboard::Mode)), SLOT(clipboardChanged(QClipboard::Mode)) );
}

StandardActionManager::~ StandardActionManager()
{
  delete d;
}

void StandardActionManager::setCollectionSelectionModel( QItemSelectionModel * selectionModel )
{
  d->collectionSelectionModel = selectionModel;
  connect( selectionModel, SIGNAL(selectionChanged( const QItemSelection&, const QItemSelection& )),
           SLOT(collectionSelectionChanged()) );

  d->checkModelsConsistency();
}

void StandardActionManager::setItemSelectionModel( QItemSelectionModel * selectionModel )
{
  d->itemSelectionModel = selectionModel;
  connect( selectionModel, SIGNAL(selectionChanged( const QItemSelection&, const QItemSelection& )),
           SLOT(updateActions()) );
}

void StandardActionManager::setFavoriteCollectionsModel( FavoriteCollectionsModel *favoritesModel )
{
  d->favoritesModel = favoritesModel;
  d->checkModelsConsistency();
}

void StandardActionManager::setFavoriteSelectionModel( QItemSelectionModel *selectionModel )
{
  d->favoriteSelectionModel = selectionModel;
  connect( selectionModel, SIGNAL(selectionChanged( const QItemSelection&, const QItemSelection& )),
           SLOT(favoriteSelectionChanged()) );
  d->checkModelsConsistency();
}

KAction* StandardActionManager::createAction( Type type )
{
  Q_ASSERT( type >= 0 && type < LastType );
  Q_ASSERT( actionData[type].name );
  if ( d->actions[type] )
    return d->actions[type];
  KAction *action;
  if ( !actionData[type].isActionMenu ) {
    action = new KAction( d->parentWidget );
  } else {
    action = new KActionMenu( d->parentWidget );
  }

  if ( d->pluralLabels.contains( type ) && !d->pluralLabels.value( type ).isEmpty() )
    action->setText( d->pluralLabels.value( type ).subs( 1 ).toString() );
  else if ( actionData[type].label )
    action->setText( i18n( actionData[type].label ) );

  if ( actionData[type].icon )
    action->setIcon( KIcon( QString::fromLatin1( actionData[type].icon ) ) );

  action->setShortcut( actionData[type].shortcut );

  if ( actionData[type].slot && !actionData[type].isActionMenu ) {
    connect( action, SIGNAL(triggered()), actionData[type].slot );
  } else if ( actionData[type].slot ) {
    KActionMenu *actionMenu = qobject_cast<KActionMenu*>( action );
    connect( actionMenu->menu(), SIGNAL(triggered(QAction*)), actionData[type].slot );
  }

  d->actionCollection->addAction( QString::fromLatin1(actionData[type].name), action );
  d->actions[type] = action;
  d->updateActions();
  return action;
}

void StandardActionManager::createAllActions()
{
  for ( int i = 0; i < LastType; ++i )
    createAction( (Type)i );
}

KAction * StandardActionManager::action( Type type ) const
{
  Q_ASSERT( type >= 0 && type < LastType );
  return d->actions[type];
}

void StandardActionManager::setActionText( Type type, const KLocalizedString & text )
{
  Q_ASSERT( type >= 0 && type < LastType );
  d->pluralLabels.insert( type, text );
  d->updateActions();
}

#include "standardactionmanager.moc"
