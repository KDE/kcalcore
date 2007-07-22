/*
  This file is part of the kblog library.

  Copyright (c) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2006 Christian Weilbach <christian@whiletaker.homeip.net>
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
#ifndef API_BLOG_H
#define API_BLOG_H

#include <kblog/kblog_export.h>

#include <kurl.h>
#include <kio/job.h>
#include <ktimezone.h>
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
  and defines the BlogPosting, BlogMedia, and APIBlog class.

  @author Reinhold Kainhofer \<reinhold\@kainhofer.com\>
  @author Christian Weilbach \<christian\@whiletaker.homeip.net\>
*/

/** Namespace for blog related classes. */
namespace KBlog {

/**
  @brief
  A class that represents a blog posting on the server.

  @code
  KBlog::BlogPosting *post = new BlogPosting();
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
      @param categories
      @param publish
    */
    BlogPosting( const QString &title, const QString &content,
                 const QStringList &categories = QStringList(),
                 const bool publish = true );

    /**
      Virtual default destructor.
    */
    virtual ~BlogPosting();

    /**
      Returns if the posting is published or not.

      @return bool
      @see setPublish()
    */
    bool publish() const;

    /**
      Sets the publish value.

      @param publish set this to true, if you want to publish immediately.
      @see publish()
    */
    void setPublish( const bool publish );

    /**
      Returns the postId. This is for fetched postings.
      @return postingId
      @see setPostingId()
    */
    QString postingId() const;

    /**
      Sets the post id value. This is important for modifying postings.

      @param postingId set this to the post id on the server.
      @see postingId()
    */
    void setPostingId( const QString &postingId );

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
      Returns the categories.

      @return categories
      @see setCategories()
    */
    QStringList categories() const;

    /**
      Sets the categories.

      @param categories set the categories.
      @see categories()
    */
    void setCategories( const QStringList &categories );

    /**
      Returns the creation date time.

      @return creationdatetime
      @see setCreationDateTime()
    */
    KDateTime creationDateTime() const;

    /**
      Sets the creation time.

      @param datetime set the time the posting has been created.
      @see creationTime()
    */
    void setCreationDateTime( const KDateTime &datetime );

    /**
      Returns the modification date time.

      @return modificationdatetime
      @see setModificationDateTime(), creationDateTime()
    */
    KDateTime modificationDateTime() const;

    /**
      Sets the modification time.

      @param datetime set the time the posting has been modified.
      @see modificationTime(), setCreationDateTime()
    */
    void setModificationDateTime( const KDateTime &datetime );

    /**
      Returns true if the date format is extended

      @see setUseExtendedDateFormat()
     */
    bool useExtendedDateFormat() const;

    /**
      Sets the Date Format
       extended format: YYYY-MM-DD
       basic format: YYYYMMDD

      @param extended  true extended format will be used.
      @see useExtendedDateFormat()
     */
    void setUseExtendedDateFormat( bool extended );

    /**
      Returns true if the time format is extended

      @see setUseExtendedTimeFormat()
     */
    bool useExtendedTimeFormat() const;

    /**
      Sets the Time Format
       extended format: HH:MM:SS
       basic format: HHMMSS

      @param extended  true extended format will be used.
      @see useExtendedTimeFormat()
     */
    void setUseExtendedTimeFormat( bool extended );

  private:
    class BlogPostingPrivate;
    BlogPostingPrivate *const d;
};

/**
  @brief
  A class that represents a media object on the server.

  @code
  KBlog::BlogMedia *media = new BlogMedia();
  post->setMimetype( "some_mimetype" );
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
      Returns the name. This is most likely the filename on the server side
      (at least with wordpress).

      @return name
      @see setName()
    */
    QString name() const;

    /**
      Sets the name. This will most likely be the filename on the server side
      (at least with wordpress).

      @param title set the name.
      @see name()
    */
    void setName( const QString &title );

    /**
      Returns the mimetype.

      @return mimetype of the object
      @see setMimetype()
    */
    QString mimetype() const;

    /**
      Set the mimtype.

      @param mimetype is the mimetype
      @see mimetype()
    */
    void setMimetype( const QString &mimetype );

    /**
      Returns the data of the file.

      @return data
      @see setData()
    */
    QByteArray data() const;

    /**
       Set the data of the file.

       @param data is the data stream
       @see data()
    */
    void setData( const QByteArray &data );

  private:
    class BlogMediaPrivate;
    BlogMediaPrivate *const d;
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
    */
    explicit APIBlog( const KUrl &server, QObject *parent = 0 );

    /**
      Destroys the APIBlog object.
    */
    virtual ~APIBlog();

    /**
      Enumeration for possible errors.
    */
    enum errorType {
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
      Sets the blog id of the Server.

      @param blogId
    */
    virtual void setBlogId( const QString &blogId );

    /**
      Returns the blog id.

      @return blogId
    */
    virtual QString blogId() const;

    /**
      Sets the password for the blog.
      @param pass is a QString containing the blog password.

      @see password();
    */
    virtual void setPassword( const QString &pass );

    /**
      Returns the password of the blog.
      @see setPassword();
    */
    virtual QString password() const;

    /**
      Sets the username for the blog.
      @param uname is a QString containing the blog username.

      @see username()
    */
    virtual void setUsername( const QString &uname );

    /**
       Returns the username of the blog.
       @see setUsername()
    */
    virtual QString username() const;

    /**
      Sets the URL for the blog.
      @param url is the blog URL.

      @see url()
    */
    virtual void setUrl( const KUrl &url );

    /**
      Get the URL for the blog.
      @see setUrl()
    */
    virtual KUrl url() const;

    /**
      Sets the time zone of the blog server.
      @param tz time zone of the server
      @see timeZone()
    */
    virtual void setTimeZone( const KTimeZone &tz );

    /**
      Get the time zone of the blog server.
      @see void setTimeZone()
    */
    virtual KTimeZone timeZone();

    /**
      Sets the number of blog posts to be downloaded.
      @param nr number of posts to download. '0' gets all posts.
      @see listPostings()
      @see downloadCount()
    */
    virtual void setDownloadCount( int nr );

    /**
      Gets the number of blog posts to be downloaded.
      @see listPostings()
      @see setDownloadCount()
    */
    virtual int downloadCount() const;

    /**
      Get information about the user from the blog.
      @see userInfoRetrieved()
    */
    virtual bool userInfo() = 0;

    /**
      List the blogs available for this authentication on the server.
      @see blogInfoRetrieved()
    */
    virtual bool listBlogs() = 0;

    /**
      List recent postings on the server.
      @see listedPosting()
      @see fetchedPosting()
      @see listPostingsFinished()
    */
    virtual bool listPostings() = 0;

    /**
      List the categories of the blog.
      @see categoriesInfoRetrieved(), listCategoriesFinished()
    */
    virtual bool listCategories() = 0;

    /**
      Fetch the Posting with postingId.
      @param postingId is the id of the posting on the server.

      @see fetchedPosting()
    */
    virtual bool fetchPosting( const QString &postingId ) = 0;

    /**
      Modify a posting on server.

      @param posting is used to send the modified posting including the
      correct postingId from it to the server.
    */
    virtual bool modifyPosting( KBlog::BlogPosting *posting ) = 0;

    /**
      Create a new posting on server.

      @param posting is send to the server.
    */
    virtual bool createPosting( KBlog::BlogPosting *posting ) = 0;

    /**
      Create a new media object, e.g. picture, on server.

      @param media is send to the server.
    */
    virtual bool createMedia( KBlog::BlogMedia *media ) = 0;

    /**
      Remove a posting from the server.

      @param postingId is the id of the posting to remove.

      @see removePosting( KBlog::BlogPosting* )
      @see void modifiedPosting( bool modified )
    */
    virtual bool removePosting( const QString &postingId ) = 0;

    /**
      Overloaded function, provided for convenience.

      @param posting is the posting which will be removed. It will also
      be deleted.
    */
    virtual bool removePosting( KBlog::BlogPosting *posting );

  Q_SIGNALS:
    /**
      This signal is emitted when a userInfo() job fetches the user
      information from the blogging server.

      @param nickname is the nickname of the user.
      @param userid is the id of the user on the server.
      @param email is the email address used on the server.

'     @see userInfo()
    */
    virtual void userInfoRetrieved( const QString &nickname, const QString &userid,
                            const QString &email );

    /**
      This signal is emitted when a listBlogs() job fetches the blog
      information from the blogging server.

      @param id is the blog's id on the server.
      @param name is the name of the blog on the server.

      @see listBlogs()
    */
    virtual void blogInfoRetrieved( const QString &id, const QString &name );

    /**
      This signal is emitted when a listCategories() job fetches category
      information from the blogging server.

      @param name is the name of the category on the server.
      @param description is the description of the category on the server.

      @see listCategories()
    */
    virtual void categoryInfoRetrieved( const QString &name,
                                const QString &description );

    /**
      This signal is emitted when a listPostings() job fetches a posting
      from the blogging server.

      @param posting is the fetched posting.

      @see listPostings()
    */
    virtual void listedPosting( KBlog::BlogPosting &posting );

    /**
      This signal is emitted when a fetchPosting() job fetches a posting
      from the blogging server.

      @param posting is the fetched posting.

      @see fetchPosting(KBlog::BlogPosting*)
    */
    virtual void fetchedPosting( KBlog::BlogPosting &posting );

    /**
      This signal is emitted when a createPosting() job successfully creates
      a posting on the server.

      @param id is the id the posting has on the server.

      @see createPosting( KBlog::BlogPosting* )
    */
    virtual void createdPosting( const QString &id );

    /**
      This signal is emitted when a createMedia() job successfully creates
      a posting on the server.

      @param url is the url of the posting on the server. This, depending
      on the server, can only be an id string, too.

      @see createMedia( KBlog::BlogMedia* )
    */
    virtual void createdMedia( const QString &url );

    /**
      This signal is emitted when a modifyPosting() job modifies a posting
      on the server.

      @param modified shows the success of the modification.

      @see modifyPosting( KBlog::BlogPosting* )
    */
    virtual void modifiedPosting( bool modified );

    /**
      This signal is emitted when the last posting of the listPostings()
      job has been fetched.

      @see listPostings()
    */
    virtual void listPostingsFinished();

    /**
      This signal is emitted when the last category of the listCategories()
      job has been fetched.

      @see listCategories()
    */
    virtual void listCategoriesFinished();

    /**
      All xml parsing and all structural problems will emit an error.

      @see errorType
    */
    virtual void error( const errorType &type, const QString &errorMessage );

  protected:
    class APIBlogPrivate;
    APIBlogPrivate *const d;
};

}
#endif
