/*
  This file is part of the kblog library.

  Copyright (c) 2007 Mike Arthur <mike@mikearthur.co.uk>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef MOVABLETYPE_P_H
#define MOVABLETYPE_P_H

#include "movabletype.h"

#include <kxmlrpcclient/client.h>

using namespace KBlog;

class MovableType::MovableTypePrivate : public QObject
{
  Q_OBJECT
  public:
    QString mAppId;
    QMap<QString,QString> mCategories;
    MovableType *parent;
    KXmlRpc::Client *mXmlRpcClient;

    MovableTypePrivate();
    ~MovableTypePrivate();
    QList<QVariant> defaultArgs( const QString &id = QString() );

  private:
    bool readPostingFromMap( BlogPosting *post,
                             const QMap<QString, QVariant> &postInfo );

  public Q_SLOTS:
    void slotCreatePosting( const QList<QVariant> &result, const QVariant &id );
    void slotError( int, const QString&, const QVariant& );
    void slotFetchPosting( const QList<QVariant> &result, const QVariant &id );
    void slotListRecentPostings( const QList<QVariant> &result,
                                 const QVariant &id );
    void slotListTrackbackPings( const QList<QVariant> &result,
                                 const QVariant &id );
    void slotModifyPosting( const QList<QVariant> &result, const QVariant &id );
};

#endif
