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

#ifndef AKONADI_COLLECTIONMODEL_H
#define AKONADI_COLLECTIONMODEL_H

#include "akonadi_export.h"

#include <akonadi/collection.h>

#include <QtCore/QAbstractItemModel>

namespace Akonadi {

class CollectionModelPrivate;

/**
  Model to handle a collection tree.

  @todo Split into generic and KDE dependent parts?
*/
class AKONADI_EXPORT CollectionModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    /**
      Extended item roles for collections.
    */
    //FIXME_API: rename to Roles
    enum CollectionItemRole {
      CollectionIdRole = Qt::UserRole, ///< The collection path.
      CollectionRole, ///< The actual collection object.
      ChildCreatableRole, ///< The collection can contain sub-collections.
      CollectionContentTypesRole, ///< Returns the mimetypes supported by the collection
      CollectionViewUserRole = Qt::UserRole + 32 ///< Role for user extensions
    };
    //FIXME_API: rename CollectionViewUserRole to UserRole
    //FIXME_API: remove ChildCreatableRole and CollectionContentTypesRole

    /**
      Create a new collection model.
      @param parent The parent object.
    */
    explicit CollectionModel( QObject *parent = 0 );

    /**
      Destroys this model.
    */
    virtual ~CollectionModel();

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QModelIndex parent( const QModelIndex & index ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual Qt::DropActions supportedDropActions() const;

    /**
      Reimplemented from QAbstractItemModel.
     */
    virtual QMimeData *mimeData( const QModelIndexList &indexes ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
   virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QStringList mimeTypes() const;

  Q_SIGNALS:
    /**
     * emitted when the unread count of the complete model changes.
     */
    //FIXME_API: drop it
    void unreadCountChanged( int );

  protected:
    /**
      Returns the collection for a given collection id.
      @param id A collection id.
    */
    Collection collectionForId( Collection::Id id ) const;

    /**
      Enable fetching of collection statistics information.
      @see CollectionStatistics.
    */
    //FIXME_API: move to public
    void fetchCollectionStatistics( bool enable );

    /**
      Also include unsubscribed collections.
    */
    //FIXME_API: move to public
    void includeUnsubscribed( bool include = true );

    Akonadi::CollectionModelPrivate *d_ptr;

    explicit CollectionModel( CollectionModelPrivate *d, QObject *parent = 0 );

  private:
    /**
      Helper function to generate a model index for a given collection reference.
    */
    //FIXME_API: move to private class
    QModelIndex indexForId( Collection::Id id, int column = 0 );

    /**
      Helper method to remove a single row from the model (not from the Akonadi backend).
      @param row The row index.
      @param parent The parent model index.
    */
    //FIXME_API: move to private class
    bool removeRowFromModel( int row, const QModelIndex & parent = QModelIndex() );

    /**
      Returns true if a new sub-collection for the given parent collection can be created.
      @param parent The parent model index.
    */
    //FIXME_API: move to private class
    bool canCreateCollection( const QModelIndex &parent ) const;

    /**
      Returns whether the specified collection supports <em>any</em> of the given mime-types.
      @param index The model index.
      @param contentTypes The content types to check.
    */
    //FIXME_API: move to private class
    bool supportsContentType( const QModelIndex &index, const QStringList &contentTypes );

  private:
    Q_DECLARE_PRIVATE( CollectionModel )

    Q_PRIVATE_SLOT( d_func(), void startFirstListJob() )
    Q_PRIVATE_SLOT( d_func(), void collectionRemoved( const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d_func(), void collectionChanged( const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d_func(), void updateDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void collectionStatisticsChanged(
                                        Akonadi::Collection::Id,
                                        const Akonadi::CollectionStatistics& ) )
    Q_PRIVATE_SLOT( d_func(), void listDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void editDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void dropResult( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void collectionsChanged( const Akonadi::Collection::List& ) )

};

}

#endif
