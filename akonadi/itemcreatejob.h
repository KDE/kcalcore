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

#ifndef AKONADI_ITEMCREATEJOB_H
#define AKONADI_ITEMCREATEJOB_H

#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class Item;
class ItemCreateJobPrivate;

/**
  Creates a new PIM item on the backend.
*/
class AKONADI_EXPORT ItemCreateJob : public Job
{
  Q_OBJECT
  public:
    /**
      Create a new item append job.
      @param item The item to append. It must have a mimetype set.
      @param collection Parent collection.
      @param parent The parent object.
    */
    ItemCreateJob( const Item &item, const Collection &collection, QObject *parent = 0 );

    /**
      Deletes this job.
    */
    ~ItemCreateJob();

    /**
      Returns the item with the new unique id, or an invalid item if the job failed.
    */
    Item item() const;

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( ItemCreateJob )
};

}

#endif
