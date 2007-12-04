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

#ifndef KBLOG_WORDPRESSBUGGY_H
#define KBLOG_WORDPRESSBUGGY_H

#include <kblog/movabletype.h>

class KUrl;

/**
  @file
  This file is part of the  for accessing Blog Servers
  and defines the WordpressBuggy class.

  @author Christian Weilbach \<christian_weilbach\@web.de\>
*/

namespace KBlog {

    class WordpressBuggyPrivate;
/**
  @brief
  A class that can be used for access to Wordpress blogs. This is needed because
  of an ugly DateTime bug in Wordpress. Hopefully the bug will be fixed in the
  near feature. This class reimplements createPost and modifyPost from scratch
  to send the DateTime in a format like yyyyMMddThh:mm:ss which is not ISO
  conformant. The date is not extended, where the time is extended.

  The rest of the code is used from MovableType, as Wordpress implements
  MovableType. You should not use thiscode on non-Wordpress blogs.

  @code
  Blog* myblog = new WordpressBuggy("http://example.com/xmlrpc/gateway.php");
  myblog->setUsername( "some_user_id" );
  myblog->setPassword( "YoURFunnyPAsSwoRD" );
  KBlog::BlogPost *post = new BlogPost();
  post->setTitle( "This is the title." );
  post->setContent( "Here is some the content..." );
  myblog->createPost( post );
  @endcode

  @author Christian Weilbach \<christian_weilbach\@web.de\>
*/
class KBLOG_EXPORT WordpressBuggy : public MovableType
{
  Q_OBJECT
  public:
    /**
      Create an object for WordpressBuggy
      @param server is the url for the xmlrpc gateway.
      @param parent is the parent object.
    */
    explicit WordpressBuggy( const KUrl &server, QObject *parent = 0 );

    /**
      Destroy the object.
    */
    virtual ~WordpressBuggy();

    /**
      Create a new post on server.
      @param post is send to the server.
    */
    void createPost( KBlog::BlogPost *post );

    /**
      Modify a post on server.
      @param post The post to be modified on the server.
      You need to set its id correctly.

      @see BlogPost::setPostId( const QString& )
      @see modifiedPost( KBlog::BlogPost* )
    */
    void modifyPost( KBlog::BlogPost *post );

    /**
      Returns the  of the inherited object.
    */
    QString interfaceName() const;

  protected:
    /**
      Constructor needed for private inheritance.
    */
    WordpressBuggy( const KUrl &server, WordpressBuggyPrivate &dd, QObject *parent = 0 );

  private:
    Q_DECLARE_PRIVATE( WordpressBuggy )
    Q_PRIVATE_SLOT( d_func(), void slotCreatePost( KJob * ) )
    Q_PRIVATE_SLOT( d_func(), void slotCreatePostData( KIO::Job *,const QByteArray & ) )
    Q_PRIVATE_SLOT( d_func(), void slotModifyPost( KJob * ) )
    Q_PRIVATE_SLOT( d_func(), void slotModifyPostData( KIO::Job *,const QByteArray & ) )
};

} //namespace KBlog
#endif
