/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#include "collectiondialog.h"

#include "asyncselectionhandler_p.h"

#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/entityrightsfiltermodel.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/entitytreeview.h>
#include <akonadi/session.h>

#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

using namespace Akonadi;

class CollectionDialog::Private
{
  public:
    Private( QAbstractItemModel *customModel, CollectionDialog *parent )
      : mParent( parent ),
        mMonitor( 0 ),
        mModel( 0 )
    {
      // setup GUI
      QWidget *widget = mParent->mainWidget();
      QVBoxLayout *layout = new QVBoxLayout( widget );

      mTextLabel = new QLabel;
      layout->addWidget( mTextLabel );
      mTextLabel->hide();

      mView = new EntityTreeView;
      mView->header()->hide();
      layout->addWidget( mView );


      mParent->enableButton( KDialog::Ok, false );

      // setup models
      QAbstractItemModel *baseModel;

      if ( customModel ) {
        baseModel = customModel;
      } else {
        mMonitor = new Akonadi::ChangeRecorder( mParent );
        mMonitor->fetchCollection( true );
        mMonitor->setCollectionMonitored( Akonadi::Collection::root() );

        mModel = new EntityTreeModel( Session::defaultSession(), mMonitor, mParent );
        mModel->setItemPopulationStrategy( EntityTreeModel::NoItemPopulation );
        baseModel = mModel;
      }

      mMimeTypeFilterModel = new CollectionFilterProxyModel( mParent );
      mMimeTypeFilterModel->setSourceModel( baseModel );

      mRightsFilterModel = new EntityRightsFilterModel( mParent );
      mRightsFilterModel->setSourceModel( mMimeTypeFilterModel );

      mSelectionHandler = new AsyncSelectionHandler( mRightsFilterModel, mParent );
      mParent->connect( mSelectionHandler, SIGNAL( collectionAvailable( const QModelIndex& ) ),
                        mParent, SLOT( slotCollectionAvailable( const QModelIndex& ) ) );
      mView->setModel( mRightsFilterModel );

      mParent->connect( mView->selectionModel(), SIGNAL( selectionChanged( QItemSelection, QItemSelection ) ),
                        mParent, SLOT( slotSelectionChanged() ) );
    }

    ~Private()
    {
    }

    void slotCollectionAvailable( const QModelIndex &index )
    {
      mView->expandAll();
      mView->setCurrentIndex( index );
    }

    CollectionDialog *mParent;

    ChangeRecorder *mMonitor;
    EntityTreeModel *mModel;
    CollectionFilterProxyModel *mMimeTypeFilterModel;
    EntityRightsFilterModel *mRightsFilterModel;
    EntityTreeView *mView;
    AsyncSelectionHandler *mSelectionHandler;
    QLabel *mTextLabel;

    void slotSelectionChanged();
};

void CollectionDialog::Private::slotSelectionChanged()
{
  mParent->enableButton( KDialog::Ok, mView->selectionModel()->selectedIndexes().count() > 0 );
}

CollectionDialog::CollectionDialog( QWidget *parent )
  : KDialog( parent ),
    d( new Private( 0, this ) )
{
}

CollectionDialog::CollectionDialog( QAbstractItemModel *model, QWidget *parent )
  : KDialog( parent ),
    d( new Private( model, this ) )
{
}

CollectionDialog::~CollectionDialog()
{
  delete d;
}

Akonadi::Collection CollectionDialog::selectedCollection() const
{
  if ( selectionMode() == QAbstractItemView::SingleSelection ) {
    const QModelIndex index = d->mView->currentIndex();
    if ( index.isValid() )
      return index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  }

  return Collection();
}

Akonadi::Collection::List CollectionDialog::selectedCollections() const
{
  Collection::List collections;
  const QItemSelectionModel *selectionModel = d->mView->selectionModel();
  const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
  foreach ( const QModelIndex &index, selectedIndexes ) {
    if ( index.isValid() ) {
      const Collection collection = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
      if ( collection.isValid() )
        collections.append( collection );
    }
  }

  return collections;
}

void CollectionDialog::setMimeTypeFilter( const QStringList &mimeTypes )
{
  d->mMimeTypeFilterModel->clearFilters();
  d->mMimeTypeFilterModel->addMimeTypeFilters( mimeTypes );

  if ( d->mMonitor )
    foreach( const QString &mimetype, mimeTypes )
      d->mMonitor->setMimeTypeMonitored( mimetype );
}

QStringList CollectionDialog::mimeTypeFilter() const
{
  return d->mMimeTypeFilterModel->mimeTypeFilters();
}

void CollectionDialog::setAccessRightsFilter( Collection::Rights rights )
{
  d->mRightsFilterModel->setAccessRights( rights );
}

Collection::Rights CollectionDialog::accessRightsFilter() const
{
  return d->mRightsFilterModel->accessRights();
}

void CollectionDialog::setDescription( const QString &text )
{
  d->mTextLabel->setText( text );
  d->mTextLabel->show();
}

void CollectionDialog::setDefaultCollection( const Collection &collection )
{
  d->mSelectionHandler->waitForCollection( collection );
}

void CollectionDialog::setSelectionMode( QAbstractItemView::SelectionMode mode )
{
  d->mView->setSelectionMode( mode );
}

QAbstractItemView::SelectionMode CollectionDialog::selectionMode() const
{
  return d->mView->selectionMode();
}

#include "collectiondialog.moc"
