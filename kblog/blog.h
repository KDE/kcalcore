/*
    This file is part of the kblog library.

    Copyright (c) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
    Copyright (c) 2006 Christian Weilbach <christian@whiletaker.homeip.net>

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
#ifndef API_BLOG_H
#define API_BLOG_H

#include <kblog.h>
#include <kurl.h>
#include <kio/job.h>
#include <ktimezones.h>
#include <kdatetime.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>

/**
This is the main interface for blog backends
@author Ian Reinhart Geiser, Reinhold Kainhofer, Christian Weilbach
*/

/**
  @file

  This file is part of the API for accessing Blog Servers
  and defines the #BlogPosting, #BlogMedia, and #APIBlog class.

  @author Reinhold Kainhofer \<reinhold\@kainhofer.com\>
  @author Christian Weilbach \<christian\@whiletaker.homeip.net\>

  \par Maintainer: Christian Weilbach \<christian\@whiletaker.homeip.net\>
 */

/** Namespace for blog related classes. */
namespace KBlog {

/**
  @brief
  A class that represents a blog posting on the server.

  @code
    KBlog::BlogPosting *post = new BlogPosting();
    post->setUserId( "some_user_id" );
    post->setTitle( "This is the title." );
    post->setContent( "Here is some the content..." );
  @endcode

  @author Christian Weilbach \<christian\@whiletaker.homeip.net\>
 */


class KBLOG_EXPORT BlogPosting
{
public:
  /**
    Default constructor. Creates an empty BlogPosting object.
  */
  BlogPosting();

  /**
    Constructor for convenience.

    @param title
    @param content
    @param category
    @param publish
  */
  BlogPosting( const QString& title, const QString& content,
               const QString& category = QString(),
	       const bool publish = true );

  /**
    Virtual default destructor.
  */
  virtual ~BlogPosting();

  /**
    Returns if the posting is published or not.

    @result bool
    @see setPublish( const bool publish )
  */
  bool publish() const;

  /**
    Set the publish value.

    @param publish set this to true, if you want to publish immediately.
    @see publish()
  */
  void setPublish( const bool publish );

  /**
    Returns the postId. This is for fetched postings.
    @result postingId
    @see setPostingId( const QString &postingId )
  */
  QString postingId() const;

  /**
    Set the post id value. This is important for modifying postings.

    @param postingId set this to the post id on the server.
    @see postingId()
  */
  void setPostingId( const QString &postingId );

  /**
    Returns the title.

    @result title
    @see setTitle( const QString &title )
  */
  QString title() const;

  /**
    Set the title.

    @param title set the title.
    @see title()
  */
  void setTitle( const QString &title );

  /**
    Returns the content.

    @result content
    @see setContent( const QString &content )
  */
  QString content() const;

  /**
    Set the content.

    @param content set the content.
    @see content()
  */
  void setContent( const QString &content );

  /**
    Returns the category.

    @result category
    @see setCategory( const QString &category )
  */
  QString category() const;

  /**
    Set the category.

    @param category set the category.
    @see category()
  */
  void setCategory( const QString &category );

  /**
    Returns the creation date time.

    @result creationdatetime
    @see setCreationDateTime( const QString &datetime )
  */
  KDateTime creationDateTime() const;

  /**
    Set the creation time.

    @param datetime set the time the posting has been created.
    @see creationTime()
  */
  void setCreationDateTime( const KDateTime &datetime );

  /**
    Returns the modification date time.

    @result modificationdatetime
    @see setModificationDateTime( const QString &datetime )
    @see creationDateTime()
  */
  KDateTime modificationDateTime() const;

  /**
    Set the modification time.

    @param datetime set the time the posting has been modified.
    @see modificationTime()
    @see setCreationDateTime( const KDateTime &datetime )
  */
  void setModificationDateTime( const KDateTime &datetime );

  /**
    Returns if the post has been deleted on the server. Note: This is
    currently not set automatically on post.

    @result deleted
    @see setDeleted( const bool deleted )
  */
  bool deleted() const; // TODO: set on post

  /**
    Set when the posting has been deleted on the server.

    @param deleted set to the status of the posting.
    @see deleted()
  */
  void setDeleted( const bool deleted );

  /**
    Returns if the post has been uploaded to the server. Note: This ist
    currently not set automatically on post.

    @result uploaded
    @see setUploaded( const bool uploaded )
  */
  bool uploaded() const; // TODO: set on post

  /**
    Set when the posting has been uploaded to the server.

    @param uploaded set the status of the posting.
    @see uploaded()
  */
  void setUploaded( const bool deleted);


protected:
  // Override this method to detect the new postId assigned when adding a new post
  virtual void assignPostId( const QString &/*postId*/ ) {}
private:
  class Private;
  Private* const d;
};

/**
  @brief
  A class that represents a media object on the server.

  @code
    KBlog::BlogMedia *media = new BlogMedia();
    ost->setMimetype( "some_mimetype" );
    post->setData( some_qbytestream );
  @endcode

  @author Christian Weilbach \<christian\@whiletaker.homeip.net\>
 */


class KBLOG_EXPORT BlogMedia {
public:
  /**
    Default constructor. Creates an empty BlogMedia object.
  */
  BlogMedia();

  /**
    Virtual default destructor.
  */
  virtual ~BlogMedia();

  /**
    Returns the name. This is most likely the filename on the server side ( at least with wordpress ).

    @result name
    @see setName( const QString &name )
  */
  QString name() const;

  /**
    Set the name. This will most likely be the filename on the server side ( at least with wordpress ).

    @param title set the name.
    @see name()
  */
  void setName( const QString &title );

  /**
    Return the mimetype.

    @result mimetype of the object
    @see setMimetype()
  */
  QString mimetype() const;
  void setMimetype( const QString& mimetype );

  /**
    Return the data of the file.

    @result data
    @see setData()
  */
  QByteArray data() const;
  void setData( const QByteArray& data );

private:
  class Private;
  Private* const d;
};

/**
  @brief
  A virtual basis class that represents a connection to a blog server.
  This is the main interface to the blog client library.

  @author Christian Weilbach \<christian\@whiletaker.homeip.net\>
  @author Reinhold Kainhofer \<reinhold\@kainhofer.com\>
 */

class KBLOG_EXPORT APIBlog : public QObject
{
  Q_OBJECT
  public:
      /**
                        Construtor used by the API implementations.

      @param server the gateway url of the server.
      @param parent the parent of this object, defaults to NULL.
      @param name  the name of the instance.
     */
    explicit APIBlog( const KUrl &server, QObject *parent = 0,
                      const char *name = 0 );

     /**
       Destroys the APIBlog object.
     */
    virtual ~APIBlog();

    /**
        Enumeration for possible errors.
    */
    enum errorType
   {
      XmlRpc,
      AtomAPI,
      ParsingError,
      AuthenticationError,
      NotSupported,
      Other
    };

     /**
        Returns the API of the inherited object.
     */
    virtual QString interfaceName() const = 0;

     /**
        Set the blod id of the Server.

        @param blogId
     */
    void setBlogId( const QString &blogId );

     /**
        Returns the blog id.

        @result blogId
     */
    QString blogId() const;

     /**
        Sets the password for the blog.
        @see password();
     */
    void setPassword( const QString &pass );
     /**
        Returns the password of the blog.
        @see setPassword();
    */
    QString password() const;

    /**
       Sets the username for the blog.
       @see username()
    */
    void setUsername( const QString &uname );

    /**
       Get the username of the blog.
       @see setUsername()
    */
    QString username() const;

    /**
        Sets the URL for the blog.
        @see url()
    */
    void setUrl( const KUrl& url );

    /**
        Get the URL for the blog.
        @see setUrl()
    */
    KUrl url() const;

    /**
        Sets the time zone of the blog server.
	@param tz time zone of the server
	@see timezone()
    */
    void setTimezone( const KTimeZone& tz );

    /**
        Get the time zone of the blog server.
	@see void setTimezone( const KTimeZone& tz )
    */
    KTimeZone timezone();

    // TODO once again, do we need this?
    void setDownloadCount( int nr );
    int downloadCount() const;

    /**
        Get information about the user from the blog.
	@see void userInfoRetrieved( const QString &nickname, const QString &userid, const QString &email )
    */
    virtual void userInfo() = 0;

    /**
        List the blogs available for this authentication on the server.
	@see void blogInfoRetrieved( const QString &id, const QString &name )
    */
    virtual void listBlogs() = 0;

    /**
        List recent postings on the server..
	@see     void listedPosting( KBlog::BlogPosting &posting )
        @see     void fetchedPosting( KBlog::BlogPosting &posting )
        @see     void listPostingsFinished()
    */
    virtual void listPostings() = 0;

    /**
        List the categories of the blog.
	@see  void categoryInfoRetrieved( const QString &name, const QString &description )
        @see  void listCategoriesFinished()
    */
    virtual void listCategories() = 0;

    /**
        Fetch the Posting with postingId.
        @param postingId is the id of the posting on the server.

        @see  void fetchedPosting( KBlog::BlogPosting &posting )
    */
    virtual void fetchPosting( const QString &postingId ) = 0;

    /**
        Overloaded for convenience.
        @param posting is a posting with the posting id already set. Note: The content is currently not updated on fetch. You will find the posting with @see fetchedPosting( KBlog::BlogPosting &posting ) signal.
    */
    void fetchPosting( KBlog::BlogPosting *posting ); // TODO: either update the *posting inside function or remove

    /**
        Modify a posting on server.

        @param posting is used to send the modified posting including the correct postingId from it to the server.
    */
    virtual void modifyPosting( KBlog::BlogPosting *posting ) = 0;

    /**
        Create a new posting on server.

        @param posting is send to the server.
    */
    virtual void createPosting( KBlog::BlogPosting *posting ) = 0;

    /**
        Create a new media object, e.g. picture, on server.

        @param media is send to the server.
    */
    virtual void createMedia( KBlog::BlogMedia *media ) = 0;

    /**
        Remove a posting from the server.

        @param postingId is the id of the posting to remove.

        @see void removePosting( KBlog::BlogPosting *posting )
    */
    virtual void removePosting( const QString &postingId ) = 0;

    /**
        Overloaded function, provided for convenience.

        @param posting is the posting which will be removed. It will also be deleted.
    */
    void removePosting( KBlog::BlogPosting *posting );

  Q_SIGNALS:
    void userInfoRetrieved( const QString &nickname, const QString &userid, const QString &email );
    void blogInfoRetrieved( const QString &id, const QString &name );
    void categoryInfoRetrieved( const QString &name, const QString &description );

    void listedPosting( KBlog::BlogPosting &posting );
    void fetchedPosting( KBlog::BlogPosting &posting );
    void createdPosting( const QString &id );
    void createdMedia( const QString &url );
    void modifiedPosting( bool );

    void listPostingsFinished();
    void listCategoriesFinished();

    /**
         All xml parsing and all structural problems will emit an error.
    */
    void error( const errorType& type, const QString& errorMessage );

  private:
    class Private;
    Private* const d;
};

}
#endif
