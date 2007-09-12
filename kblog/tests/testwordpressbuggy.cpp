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

#include "kblog/wordpressbuggy.h"
#include "kblog/blogpost.h"
#include "kblog/blogmedia.h"

#include <qtest_kde.h>

#include <unistd.h>
#include <ktimezone.h>
#include <kdatetime.h>

#define TIMEOUT 20000
#define GLOBALTIMEOUT 140000
#define DOWNLOADCOUNT 5

using namespace KBlog;

class TestWordpressBuggy : public QObject
{
  Q_OBJECT

  public Q_SLOTS:
    // use this functions as a chain to go through network traffic.
    void fetchUserInfo( const QMap<QString,QString>& );
    void listBlogs( const QList<QMap<QString,QString> >& );
    void listCategories( const QList<QMap<QString,QString> >& categories );
    void listRecentPostings( const QList<KBlog::BlogPost>& postings );
    void createPosting( KBlog::BlogPost* posting );
    void modifyPosting( KBlog::BlogPost* posting );
    void fetchPosting( KBlog::BlogPost* posting );
    void removePosting( KBlog::BlogPost* posting );
    // end chain
    void error( KBlog::Blog::ErrorType type, const QString &errStr, KBlog::BlogPost* );
  private Q_SLOTS:
    void testValidity();
    void testNetwork();
  private:
    void dumpPosting( const KBlog::BlogPost* );
    KBlog::WordpressBuggy *b;
    KBlog::BlogPost *p;
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

class TestWordpressBuggyWarnings : public QObject
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

#include "testwordpressbuggy.moc"

void TestWordpressBuggy::dumpPosting( const BlogPost* posting )
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
    case BlogPost::New:
      qDebug() << "# status: New"; break;
    case BlogPost::Fetched:
      qDebug() << "# status: Fetched"; break;
    case BlogPost::Created:
      qDebug() << "# status: Created"; break;
    case BlogPost::Modified:
      qDebug() << "# status: Modified"; break;
    case BlogPost::Removed:
      qDebug() << "# status: Removed"; break;
    case BlogPost::Error:
      qDebug() << "# status: Error"; break;
  };
  qDebug() << "# creationDateTime(UTC): " <<
      posting->creationDateTime().toUtc().toString();
  qDebug() << "# modificationDateTime(UTC): " <<
      posting->modificationDateTime().toUtc().toString();
  qDebug() << "###########################";
}

// the chain starts here

void TestWordpressBuggy::fetchUserInfo( const QMap<QString,QString>& userInfo )
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

void TestWordpressBuggy::listBlogs( const QList<QMap<QString,QString> >& listedBlogs )
{
  listBlogsTimer->stop();
  qDebug() << "########### listBlogs ###########";
  QList<QMap<QString,QString> >::ConstIterator it = listedBlogs.begin();
  QList<QMap<QString,QString> >::ConstIterator end = listedBlogs.end();
  for ( ; it != end; ++it ) {
    qDebug() << "# " << ( *it ).keys().first() << ": " << ( *it ).values().first();
  }
  qDebug() << "###########################\n";

  connect( b, SIGNAL( listedRecentPostings(const QList<KBlog::BlogPost>&) ),
           this, SLOT( listRecentPostings(const QList<KBlog::BlogPost>&) ) );
  b->listRecentPostings( DOWNLOADCOUNT );
  listRecentPostingsTimer->start( TIMEOUT );
}

void TestWordpressBuggy::listRecentPostings(
           const QList<KBlog::BlogPost>& postings )
{
  listRecentPostingsTimer->stop();
  qDebug() << "########### listRecentPostings ###########";
  QList<KBlog::BlogPost>::ConstIterator it = postings.begin();
  QList<KBlog::BlogPost>::ConstIterator end = postings.end();
  for ( ; it != end; ++it ) {
    dumpPosting( &( *it ) );
  }
  qDebug() << "#################################\n";

  connect( b, SIGNAL( listedCategories( const QList<QMap<QString,QString> >& ) ),
           this, SLOT( listCategories( const QList<QMap<QString,QString> >&) ) );
  b->listCategories(); // start chain
  listCategoriesTimer->start( TIMEOUT );
}

void TestWordpressBuggy::listCategories(
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

  connect( b, SIGNAL( createdPosting( KBlog::BlogPost* ) ),
           this, SLOT( createPosting( KBlog::BlogPost* ) ) );
  b->createPosting( p ); // start chain
  createPostingTimer->start( TIMEOUT );
}

void TestWordpressBuggy::createPosting( KBlog::BlogPost *posting )
{
  createPostingTimer->stop();
  qDebug() << "########### createPosting ############";
  dumpPosting( posting );
  qDebug() << "################################\n";
  QVERIFY( posting->status() == BlogPost::Created );

  connect( b, SIGNAL( modifiedPosting( KBlog::BlogPost* ) ),
           this, SLOT( modifyPosting( KBlog::BlogPost* ) ) );
  p->setContent( mModifiedContent );
  b->modifyPosting( p );
  modifyPostingTimer->start( TIMEOUT );
}

void TestWordpressBuggy::modifyPosting( KBlog::BlogPost *posting )
{
  modifyPostingTimer->stop();
  qDebug() << "########### modifyPosting ############";
  dumpPosting( posting );
  qDebug() << "################################\n";
  QVERIFY( posting->status() == BlogPost::Modified );

  connect( b, SIGNAL( fetchedPosting( KBlog::BlogPost* ) ),
           this, SLOT( fetchPosting( KBlog::BlogPost* ) ) );
  p->setContent( "TestWordpressBuggy: created content." );
  b->fetchPosting( p );
  fetchPostingTimer->start( TIMEOUT );
}

void TestWordpressBuggy::fetchPosting( KBlog::BlogPost *posting )
{
  fetchPostingTimer->stop();
  qDebug() << "########### fetchPosting ############";
  dumpPosting( posting );
  qDebug() << "###############################\n";
  QVERIFY( posting->status() == BlogPost::Fetched );
//   QVERIFY( posting->content() == mModifiedContent );

  connect( b, SIGNAL( removedPosting( KBlog::BlogPost* ) ),
           this, SLOT( removePosting( KBlog::BlogPost* ) ) );
  b->removePosting( p );
  removePostingTimer->start( TIMEOUT );
}

void TestWordpressBuggy::removePosting( KBlog::BlogPost *posting )
{
  removePostingTimer->stop();
  qDebug() << "########### removePosting ###########";
  dumpPosting( posting );
  qDebug() << "################################\n";
  QVERIFY( posting->status() == BlogPost::Removed );
  eventLoop->quit();
}

void TestWordpressBuggy::error( KBlog::Blog::ErrorType type, const QString &errStr,
        KBlog::BlogPost* posting )
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

void TestWordpressBuggyWarnings::fetchUserInfoTimeoutWarning()
{
  QWARN( "fetchUserInfo() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestWordpressBuggyWarnings::listBlogsTimeoutWarning()
{
  QWARN( "listBlogs()  timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestWordpressBuggyWarnings::listRecentPostingsTimeoutWarning()
{
  QWARN( "listRecentPostings() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestWordpressBuggyWarnings::listCategoriesTimeoutWarning()
{
  QWARN( "listCategories() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestWordpressBuggyWarnings::fetchPostingTimeoutWarning()
{
  QWARN( "fetchPosting() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestWordpressBuggyWarnings::modifyPostingTimeoutWarning()
{
  QWARN( "modifyPosting() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestWordpressBuggyWarnings::createPostingTimeoutWarning()
{
  QWARN( "createPosting() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestWordpressBuggyWarnings::removePostingTimeoutWarning()
{
  QWARN( "removePosting() timeout. This can be caused by an error, too. Any following calls will fail." );
}

void TestWordpressBuggy::testValidity()
{
  eventLoop = new QEventLoop( this );

  // no need to delete later ;-):
  b = new WordpressBuggy( KUrl( "http://wrong.url.org/somegateway" ) );
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

void TestWordpressBuggy::testNetwork()
{
  KDateTime mCDateTime( mCreationDateTime );
  KDateTime mMDateTime( mModificationDateTime );
  p = new BlogPost(); // no need to delete later ;-)
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
  m->setName( "testWordpressBuggy.txt" );
  m->setMimetype( "text/plain" );
  m->setData( QString( "YTM0NZomIzI2OTsmIzM0NTueYQ==" ).toAscii() );
  QVERIFY( m->mimetype() == "text/plain" );
  QVERIFY( m->data() == QString( "YTM0NZomIzI2OTsmIzM0NTueYQ==" ).toAscii() );
  QVERIFY( m->name() == QString( "testWordpressBuggy.txt" ) );

  connect( b, SIGNAL( errorPosting( KBlog::Blog::ErrorType, const QString&, KBlog::BlogPost* ) ),
           this, SLOT( error( KBlog::Blog::ErrorType, const QString&, KBlog::BlogPost* ) ) );

  TestWordpressBuggyWarnings *warnings = new TestWordpressBuggyWarnings();

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

QTEST_KDEMAIN_CORE(TestWordpressBuggy)
