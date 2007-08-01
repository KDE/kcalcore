/*
    This file is part of the kblog library.

    Copyright (c) 2007 Christian Weilbach <christian_weilbach@web.de>

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

#ifndef KBLOG_GDATA_P_H
#define KBLOG_GDATA_P_H

#include "gdata.h"
#include "blog_p.h"

#include <syndication/loader.h>

class KJob;
class QDateTime;
class QByteArray;
template <class T,class S>class QMap;

namespace KIO
{
  class Job;
}

namespace KBlog {

class GDataPrivate : public BlogPrivate
{
  public:
    QString mAuthenticationString;
    QDateTime mAuthenticationTime;
    QMap<KIO::Job*,QByteArray> mCreatePostingBuffer;
    QMap<KIO::Job*,KBlog::BlogPosting*> mCreatePostingMap;
    QMap<KIO::Job*,QByteArray> mFetchProfileIdBuffer;
    QMap<Syndication::Loader*,KBlog::BlogPosting*> mFetchPostingsMap;
    QString mFullName;
    QString mProfileId;
    GDataPrivate();
    ~GDataPrivate();
    QString authenticate();
    virtual void slotFetchProfileIdData(KIO::Job*,const QByteArray&);
    virtual void slotFetchProfileId(KIO::Job*);
    virtual void slotListBlogs(Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode);
    virtual void slotListComments(Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode);
    virtual void slotListAllComments(Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode);
    virtual void slotListRecentPostings(Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode);
    virtual void slotFetchPosting(Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode);
    virtual void slotCreatePosting(KIO::Job*);
    virtual void slotCreatePostingData( KIO::Job *, const QByteArray& );
    Q_DECLARE_PUBLIC(GData)
};

}
#endif
