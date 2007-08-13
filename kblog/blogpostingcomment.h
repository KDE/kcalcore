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

#ifndef KBLOG_BLOGPOSTINGCOMMENT_H
#define KBLOG_BLOGPOSTINGCOMMENT_H

#include <kblog/kblog_export.h>

#include <QtCore/QString>

class QStringList;

class KDateTime;
class KUrl;

namespace KBlog {

    class BlogPostingCommentPrivate;
/**
  @brief
  A class that represents a blog comment on the blog post.

  @code
  KBlog::BlogPostingComment *comment = new BlogPostingComment();
  comment->setTitle( "This is the title." );
  comment->setContent( "Here is some the content..." );
  @endcode

  @author Mike Arthur \<mike\@mikearthur.co.uk\>
*/

class KBLOG_EXPORT BlogPostingComment
{
  public:

  BlogPostingComment( const BlogPostingComment& comment );
  /**
    Constructor.

    @param commentId The ID of the comment on the server.
  */
  explicit BlogPostingComment( const QString &commentId = QString() );

  /**
    Virtual default destructor.
  */
  virtual ~BlogPostingComment();

  /**
    Returns the title.

    @return title
    @see setTitle()
  */
  QString title() const;

  /**
    Sets the title.

    @param title set the title.
    @see title()
  */
  void setTitle( const QString &title );

  /**
    Returns the content.

    @return content
    @see setContent()
  */
  QString content() const;

  /**
    Sets the content.

    @param content set the content.
    @see content()
  */
  void setContent( const QString &content );

  /**
    Returns the comment's id.

    @return comment id
    @see setCommentId()
  */
  QString commentId() const;

  /**
    Sets the comment's id.

    @param id The comment's id.
    @see commentId()
  */
  void setCommentId( const QString& id );

  /**
    Returns the email.

    @return email
    @see setContent()
  */
  QString email() const;

  /**
    Sets the email.

    @param email set the email.
    @see content()
  */
  void setName( const QString &name );

  /**
    Returns the email.

    @return email
    @see setContent()
  */
  QString name() const;

  /**
    Sets the email.

    @param email set the email.
    @see content()
  */
  void setEmail( const QString &email );

  /**
    Returns the poster's homepage URL.

    @return content
    @see setContent()
  */
  KUrl url() const;

  /**
  Sets the poster's homepage URL.

    @param content set the homepage.
    @see url()
  */
  void setUrl( const KUrl &url );

  /**
    Returns the creation date time.

    @return dateTime
    @see setDateTime()
  */
  KDateTime modificationDateTime() const;

  /**
    Sets the creation time.

    @param datetime set the time the comment has been created.
    @see creationTime()
  */
  void setModificationDateTime( const KDateTime &datetime );

  /**
    Returns the creation date time.

    @return dateTime
    @see setDateTime()
  */
  KDateTime creationDateTime() const;

  /**
    Sets the creation time.

    @param datetime set the time the comment has been created.
    @see creationTime()
  */
  void setCreationDateTime( const KDateTime &datetime );

  /**
    The enumartion of the different posting status, reflecting the status changes
    on the server.
  */
  enum Status { New, Fetched, Created, Modified, Removed, Error };

  /**
    Returns the status on the server.

    @return status
    @see setStatus(), Status
  */
  Status status() const;

  /**
    Sets the status.

    @param status The status on the server.
    @see status(), Status
  */
  void setStatus( Status status );

  /**
    Returns the last error.

    @returns error
    @see setError(), Error
  */
  QString error() const;

  /**
    Sets the error.

    @param error The error string.
    @see error(), Error
  */
  void setError( const QString& error );

  /**
    The overloaed = operator.
  */
  BlogPostingComment& operator=(const BlogPostingComment &comment );

  protected:
    BlogPostingComment( BlogPostingCommentPrivate &dd );
  private:
    BlogPostingCommentPrivate * const d_ptr;
};


} //namespace KBlog

#endif
