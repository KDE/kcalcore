/*
    This file is part of the kblog library.

    Copyright (c) 2006-2007 Christian Weilbach <christian_weilbach@web.de>
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

#ifndef METAWEBLOG_P_H
#define METAWEBLOG_P_H

#include "metaweblog.h"
#include "blogger1_p.h"

#include <kxmlrpcclient/client.h>

namespace KBlog {

class MetaWeblogPrivate : public Blogger1Private
{
  public:
    QMap<QString,QString> mCategories;
    unsigned int mCallMediaCounter;
    QMap<unsigned int,KBlog::BlogMedia*> mCallMediaMap;
    MetaWeblogPrivate();
    ~MetaWeblogPrivate();
    virtual void slotListCategories( const QList<QVariant> &result,
                                     const QVariant &id );
    virtual void slotCreateMedia( const QList<QVariant> &result,
                                  const QVariant &id );
    Q_DECLARE_PUBLIC( MetaWeblog )

  private:
    QList<QVariant> defaultArgs( const QString &id = QString() );
    bool readPostFromMap( BlogPost *post, const QMap<QString, QVariant> &postInfo );
    bool readArgsFromPost( QList<QVariant> *args, const BlogPost &post );
    QString getCallFromFunction( FunctionToCall type );
};

}
#endif
