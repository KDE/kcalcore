/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMFETCHJOB_H
#define AKONADI_ITEMFETCHJOB_H

#include <akonadi/item.h>
#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class ItemFetchJobPrivate;
class ItemFetchScope;

/**
  Fetches item data from the backend.
*/
class AKONADI_EXPORT ItemFetchJob : public Job
{
    Q_OBJECT
  public:
    /**
     * Creates an empty item fetch job.
     */
    explicit ItemFetchJob( QObject *parent = 0 );

    /**
      Create a new item list job to retrieve envelope parts of all
      items in the given collection.
      @param collection The collection to list.
      @param parent The parent object.
    */
    explicit ItemFetchJob( const Collection &collection, QObject *parent = 0 );

    /**
      Creates a new item fetch job to retrieve all parts of the item
      with the given id in @p item.
      @param parent The parent object.
    */
    explicit ItemFetchJob( const Item &item, QObject *parent = 0 );

    /**
      Destroys this job.
    */
    virtual ~ItemFetchJob();

    /**
      Returns the fetched item objects. Invalid before the result(KJob*)
      signal has been emitted or if an error occurred.
    */
    Item::List items() const;

    /**
      Sets the collection that should be listed.
      @param collection The collection that should be listed.
    */
    void setCollection( const Collection &collection );

    /**
      Sets the unique identifier of the item that should be fetched.
      @param item The item with the unique identifier.
    */
    void setItem( const Item &item );

    void setFetchScope( ItemFetchScope &fetchScope );

    ItemFetchScope &fetchScope();

  Q_SIGNALS:
    /**
     Emitted when items are received.
    */
    void itemsReceived( const Akonadi::Item::List &items );

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( ItemFetchJob )

    Q_PRIVATE_SLOT( d_func(), void selectDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void timeout() )
};

}

#endif
