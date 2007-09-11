/*
  This file is part of the kblog library.

  Copyright (c) 2006-2007 Christian Weilbach <christian_weilbach@web.de>

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

#include "data.h"

#include "kblog/movabletype.h"
#include "kblog/blogposting.h"
#include "kblog/blogmedia.h"

#include <qtest_kde.h>

#include <unistd.h>
#include <ktimezone.h>
#include <kdatetime.h>

#define TIMEOUT 20000
#define GLOBALTIMEOUT 140000
#define DOWNLOADCOUNT 5

using namespace KBlog;

class TestMovableType : public QObject
{
  Q_OBJECT

  public Q_SLOTS:
    // use this functions as a chain to go through network traffic.
    void fetchUserInfo( const QMap<QString,QString>& );
    void listBlogs( const QList<QMap<QString,QString> >& );
    void listCategories( const QList<QMap<QString,QString> >& categories );
    void listRecentPostings( const QList<KBlog::BlogPosting>& postings );
    void createPosting( KBlog::BlogPosting* posting );
    void modifyPosting( KBlog::BlogPosting* posting );
    void fetchPosting( KBlog::BlogPosting* posting );
    void removePosting( KBlog::BlogPosting* posting );
    // end chain
    void error( KBlog::Blog::ErrorType type, const QString &errStr, KBlog::BlogPosting* );
  private Q_SLOTS:
    void testValidity();
    void testNetwork();
  private:
    void dumpPosting( const KBlog::BlogPosting* );
    KBlog::MovableType *b;
    KBlog::BlogPosting *p;
    QEventLoop *eventLoop;
    QTimer *fetchUserInfoTimer;
    QTimer *listBlogsTimer;
    QTimer *listRecentPostingsTimer;
    QTimer *listCategoriesTimer;
    QTimer *fetchPostingTimer;
    QTimer *modifyPostingTimer;
    QTimer *createPostingTimer;
    QTimer *removePostingTimer;
};

class TestMovableTypeWarnings : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void fetchUserInfoTimeoutWarning();
    void listBlogsTimeoutWarning();
    void listRecentPostingsTimeoutWarning();
    void listCategoriesTimeoutWarning();
    void fetchPostingTimeoutWarning();
    void modifyPostingTimeoutWarning();
    void createPostingTimeoutWarning();
    void removePostingTimeoutWarning();

};

#include "testmovabletype.moc"

void TestMovableType::dumpPosting( const BlogPosting* posting )
{
  qDebug() << "########### posting ############";
  qDebug() << "# postingId: " << posting->postingId();
  qDebug() << "# title: " << posting->title();
  qDebug() << "# content: " << posting->content();
  qDebug() << "# private: " << posting->isPrivate();
  qDebug() << "# categories: " << posting->categories().join( " " );
  qDebug() << "# error: " << posting->error();
  qDebug() << "# journalId: " << posting->journalId();
  qDebug() << "# allowTrackBack: " << posting->isTrackBackAllowed();
  qDebug() << "# allowComment: " << posting->isCommentAllowed();
  qDebug() << "# summary: " << posting->summary();
  qDebug() << "# tags: " << posting->tags();
  qDebug() << "# link: " << posting->link().url();
  qDebug() << "# permalink: " << posting->permaLink().url();
  switch ( posting->status() ){
    case BlogPosting::New:
      qDebug() << "# status: New"; break;
    case BlogPosting::Fetched:
      qDebug() << "# status: Fetched"; break;
    case BlogPosting::Created:
      qDebug() << "# status: Created"; break;
    case BlogPosting::Modified:
      qDebug() << "# status: Modified"; break;
    case BlogPosting::Removed:
      qDebug() << "# status: Removed"; break;
    case BlogPosting::Error:
      qDebug() << "# status: Error"; break;
  };
  qDebug() << "# creationDateTime(UTC): " <<
      posting->creationDateTime().toUtc().toString();
  qDebug() << "# modificationDateTime(UTC): " <<
      posting->modificationDateTime().toUtc().toString();
  qDebug() << "###########################";
}

// the chain starts here

void TestMovableType::fetchUserInfo( const QMap<QString,QString>& userInfo )
{
  fetchUserInfoTimer->stop();
  qDebug() << "########### fetchUserInfo ###########";
  qDebug() << "# nickname: " << userInfo["nickname"];
  qDebug() << "# userid: "  << userInfo["userid"];
  qDebug() << "# url: " <<  userInfo["url"];
  qDebug() << "# email: " <<  userInfo["email"];
  qDebug() << "# lastname: " << userInfo["lastname"];
  qDebug() << "# firstname: " <<  userInfo["firstname"];
  qDebug() << "##############################\n";

  connect( b, SIGNAL( listedBlogs( const QList<QMap<QString,QString> >& ) ),
           this, SLOT( listBlogs( const QList<QMap<QString,QString> >& ) ) );
  b->listBlogs();
  listBlogsTimer->start( TIMEOUT );
}

void TestMovableType::listBlogs( const QList<QMap<QString,QString> >& listedBlogs )
{
  listBlogsTimer->stop();
  qDebug() << "########### listBlogs ###########";
  QList<QMap<QString,QString> >::ConstIterator it = listedBlogs.begin();
  QList<QMap<QString,QString> >::ConstIterator end = listedBlogs.end();
  for ( ; it != end; ++it ) {
    qDebug() << "# " << ( *it ).keys().first() << ": " << ( *it ).values().first();
  }
  qDebug() << "###########################\n";

  connect( b, SIGNAL( listedRecentPostings(const QList<KBlog::BlogPosting>&) ),
           this, SLOT( listRecentPostings(const QList<KBlog::BlogPosting>&) ) );
  b->listRecentPostings( DOWNLOADCOUNT );
  listRecentPostingsTimer->start( TIMEOUT );
}

void TestMovableType::listRecentPostings(
           const QList<KBlog::BlogPosting>& postings )
{
  listRecentPostingsTimer->stop();
  qDebug() << "########### listRecentPostings ###########";
  QList<KBlog::BlogPosting>::ConstIterator it = postings.begin();
  QList<KBlog::BlogPosting>::ConstIterator end = postings.end();
  for ( ; it != end; ++it ) {
    dumpPosting( &( *it ) );
  }
  qDebug() << "#################################\n";

  connect( b, SIGNAL( listedCategories( const QList<QMap<QString,QString> >& ) ),
           this, SLOT( listCategories( const QList<QMap<QString,QString> >&) ) );
  b->listCategories(); // start chain
  listCategoriesTimer->start( TIMEOUT );
}

void TestMovableType::listCategories(
           const QList<QMap<QString,QString> >& categories )
{
  listCategoriesTimer->stop();
  qDebug() << "########### listCategories ###########";
  QList<QMap<QString,QString> >::ConstIterator it = categories.begin();
  QList<QMap<QString,QString> >::ConstIterator end = categories.end();
  for ( ; it != end; ++it ) {
    qDebug() << "# category name: " << ( *it )["name"];
  }
  qDebug() << "###############################\n";

  connect( b, SIGNAL( createdPosting( KBlog::BlogPosting* ) ),
           this, SLOT( createPosting( KBlog::BlogPosting* ) ) );
  b->createPosting( p ); // start chain
  createPostingTimer->start( TIMEOUT );
}

void TestMovableType::createPosting( KBlog::BlogPosting *posting )
{
  createPostingTimer->stop();
  qDebug() << "########### createPosting ############";
  dumpPosting( posting );
  qDebug() << "################################\n";
  QVERIFY( posting->status() == BlogPosting::Created );

  connect( b, SIGNAL( modifiedPosting( KBlog::BlogPosting* ) ),
           this, SLOT( modifyPosting( KBlog::BlogPosting* ) ) );
  p->setContent( mModifiedContent );
  b->modifyPosting( p );
  modifyPostingTimer->start( TIMEOUT );
}

void TestMovableType::modifyPosting( KBlog::BlogPosting *posting )
{
  modifyPostingTimer->stop();
  qDebug() << "########### modifyPosting ############";
  dumpPosting( posting );
  qDebug() << "################################\n";
  QVERIFY( posting->status() == BlogPosting::Modified );

  connect( b, SIGNAL( fetchedPosting( KBlog::BlogPosting* ) ),
           this, SLOT( fetchPosting( KBlog::BlogPosting* ) ) );
  p->setContent( "TestMovableType: created content." );
  b->fetchPosting( p );
  fetchPostingTimer->start( TIMEOUT );
}

void TestMovableType::fetchPosting( KBlog::BlogPosting *posting )
{
  fetchPostingTimer->stop();
  qDebug() << "########### fetchPosting ############";
  dumpPosting( posting );
  qDebug() << "###############################\n";
  QVERIFY( posting->status() == BlogPosting::Fetched );
//   QVERIFY( posting->content() == mModifiedContent );

  connect( b, SIGNAL( removedPosting( KBlog::BlogPosting* ) ),
           this, SLOT( removePosting( KBlog::BlogPosting* ) ) );
  b->removePosting( p );
  removePostingTimer->start( TIMEOUT );
}

void TestMovableType::removePosting( KBlog::BlogPosting *posting )
{
  removePostingTimer->stop();
  qDebug() << "########### removePosting ###########";
  dumpPosting( posting );
  qDebug() << "################################\n";
  QVERIFY( posting->status() == BlogPosting::Removed );
  eventLoop->quit();
}

void TestMovableType::error( KBlog::Blog::ErrorType type, const QString &errStr,
        KBlog::BlogPosting* posting )
{
  qDebug() << "############ error #############";
  switch ( type ){
    case Blog::Atom: qDebug() << "type: Atom"; break;
    case Blog::XmlRpc: qDebug() << "type: xmlRpc"; break;
    case Blog::ParsingError: qDebug() << "type: ParsingError"; break;
    case Blog::AuthenticationError: qDebug() << "type: AuthenticationError"; break;
    case Blog::NotSupported: qDebug() << "type: NotSupported"; break;
    case Blog::Other: qDebug() << "type: Other"; break;
  };
  qDebug() << "error: " << errStr;
  if( posting!=0 ) dumpPosting( posting );
  qDebug() << "#############################\n";
}

// Warnings for Timouts:

void TestMovableTypeWarnings::fetchUserInfoTimeoutWarning()
{
  QWARN( "fetchUserInfo() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestMovableTypeWarnings::listBlogsTimeoutWarning()
{
  QWARN( "listBlogs()  timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestMovableTypeWarnings::listRecentPostingsTimeoutWarning()
{
  QWARN( "listRecentPostings() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestMovableTypeWarnings::listCategoriesTimeoutWarning()
{
  QWARN( "listCategories() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestMovableTypeWarnings::fetchPostingTimeoutWarning()
{
  QWARN( "fetchPosting() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestMovableTypeWarnings::modifyPostingTimeoutWarning()
{
  QWARN( "modifyPosting() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestMovableTypeWarnings::createPostingTimeoutWarning()
{
  QWARN( "createPosting() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestMovableTypeWarnings::removePostingTimeoutWarning()
{
  QWARN( "removePosting() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestMovableType::testValidity()
{
  eventLoop = new QEventLoop( this );

  // no need to delete later ;-):
  b = new MovableType( KUrl( "http://wrong.url.org/somegateway" ) );
  QVERIFY( b->url() == KUrl( "http://wrong.url.org/somegateway" ) );
  KTimeZone mTimeZone( KTimeZone( "UTC" ) );
  b->setUrl( mUrl );
  b->setUsername( mUsername );
  b->setPassword( mPassword );
  b->setBlogId( mBlogId );
  b->setTimeZone( mTimeZone );
  QVERIFY( b->url() == mUrl );
  QVERIFY( b->blogId() == mBlogId );
  QVERIFY( b->username() == mUsername );
  QVERIFY( b->password() == mPassword );
  QVERIFY( b->interfaceName() == "Movable Type" );
  QVERIFY( b->timeZone().name() == mTimeZone.name() );
}

void TestMovableType::testNetwork()
{
  KDateTime mCDateTime( mCreationDateTime );
  KDateTime mMDateTime( mModificationDateTime );
  p = new BlogPosting(); // no need to delete later ;-)
  p->setTitle( mTitle );
  p->setContent( mContent );
  p->setPrivate( mPrivate );
  p->setPostingId( mPostingId );
  p->setCreationDateTime( mCDateTime );
  p->setModificationDateTime( mMDateTime );
  p->setCommentAllowed( mCommentAllowed );
  p->setTrackBackAllowed( mTrackBackAllowed );
  p->setSummary( mSummary );
  p->setTags( mTags );

  BlogMedia *m = new BlogMedia();
  m->setName( "testMovableType.txt" );
  m->setMimetype( "text/plain" );
  m->setData( QString( "YTM0NZomIzI2OTsmIzM0NTueYQ==" ).toAscii() );
  QVERIFY( m->mimetype() == "text/plain" );
  QVERIFY( m->data() == QString( "YTM0NZomIzI2OTsmIzM0NTueYQ==" ).toAscii() );
  QVERIFY( m->name() == QString( "testMovableType.txt" ) );

  connect( b, SIGNAL( error( KBlog::Blog::ErrorType, const QString&, KBlog::BlogPosting* ) ),
           this, SLOT( error( KBlog::Blog::ErrorType, const QString&, KBlog::BlogPosting* ) ) );

  TestMovableTypeWarnings *warnings = new TestMovableTypeWarnings();

  fetchUserInfoTimer = new QTimer( this );
  fetchUserInfoTimer->setSingleShot( true );
  connect( fetchUserInfoTimer, SIGNAL( timeout() ),
           warnings, SLOT( fetchUserInfoTimeoutWarning() ) );

  listBlogsTimer = new QTimer( this );
  listBlogsTimer->setSingleShot( true );
  connect( listBlogsTimer, SIGNAL( timeout() ),
           warnings, SLOT( listBlogsTimeoutWarning() ) );

  listRecentPostingsTimer = new QTimer( this );
  listRecentPostingsTimer->setSingleShot( true );
  connect( listRecentPostingsTimer, SIGNAL( timeout() ),
           warnings, SLOT( listRecentPostingsTimeoutWarning() ) );

  listCategoriesTimer = new QTimer( this );
  listCategoriesTimer->setSingleShot( true );
  connect( listCategoriesTimer, SIGNAL( timeout() ),
           warnings, SLOT( listCategoriesTimeoutWarning() ) );

  fetchPostingTimer = new QTimer( this );
  fetchPostingTimer->setSingleShot( true );
  connect( fetchPostingTimer, SIGNAL( timeout() ),
           warnings, SLOT( fetchPostingTimeoutWarning() ) );

  modifyPostingTimer = new QTimer( this );
  modifyPostingTimer->setSingleShot( true );
  connect( modifyPostingTimer, SIGNAL( timeout() ),
           warnings, SLOT( modifyPostingTimeoutWarning() ) );

  createPostingTimer = new QTimer( this );
  createPostingTimer->setSingleShot( true );
  connect( createPostingTimer, SIGNAL( timeout() ),
           warnings, SLOT( createPostingTimeoutWarning() ) );

  removePostingTimer = new QTimer( this );
  removePostingTimer->setSingleShot( true );
  connect( removePostingTimer, SIGNAL( timeout() ),
           warnings, SLOT( removePostingTimeoutWarning() ) );

  // start the chain
  connect( b, SIGNAL( fetchedUserInfo( const QMap<QString,QString>& ) ),
          this, SLOT( fetchUserInfo( const QMap<QString,QString>&) ) );
  b->fetchUserInfo();
  fetchUserInfoTimer->start( TIMEOUT );

// wait for all jobs to finish

  QTimer::singleShot( GLOBALTIMEOUT, eventLoop, SLOT(quit()));
  eventLoop->exec();
  delete b;
  delete p;
}

QTEST_KDEMAIN_CORE(TestMovableType)
