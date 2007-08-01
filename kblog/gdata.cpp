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

#include "gdata.h"
#include "gdata_p.h"
#include "blogposting.h"
#include "blogpostingcomment.h"

#include <syndication/loader.h>
#include <syndication/item.h>

#include <kio/netaccess.h>
#include <kio/http.h>
#include <kio/job.h>
#include <KDebug>
#include <KLocale>
#include <KDateTime>

#include <QByteArray>
#include <QRegExp>

#define TIMEOUT 600

using namespace KBlog;

GData::GData( const KUrl &server, QObject *parent )
  : Blog( server, *new GDataPrivate, parent )
{
  Q_D(GData);
  setUrl( server );
}

GData::~GData()
{
}

QString GData::interfaceName() const
{
  return QLatin1String( "Google Blogger Data" );
}

QString GData::fullName() const
{
  return d_func()->mFullName;
}

void GData::setFullName( const QString &fullName )
{
  Q_D(GData);
  d->mFullName = fullName;
}

QString GData::profileId() const
{
  return d_func()->mProfileId;
}

void GData::setProfileId( const QString& pid )
{
  Q_D(GData);
  d->mProfileId = pid;
}

void GData::fetchProfileId()
{
  Q_D(GData);
  kDebug() << "fetchProfileId()" << endl;
  QByteArray data;
  KIO::Job *job = KIO::get( url(), false, false );
  KUrl blogUrl = url();
  connect( job, SIGNAL(data(KIO::Job*,const QByteArray&)),
                  this,SLOT(slotFetchProfileIdData(KIO::Job*,const QByteArray&)));
  connect( job, SIGNAL(result(KIO::Job*)),
                  this,SLOT(slotFetchProfileId(KIO::Job*)));
}

void GData::listBlogs()
{
  Q_D(GData);
  kDebug() << "listBlogs()" << endl;
  Syndication::Loader *loader = Syndication::Loader::create();
  connect( loader, SIGNAL(loadingComplete(Syndication::Loader*,
                          Syndication::FeedPtr, Syndication::ErrorCode)),
                          this, SLOT(slotListedBlogs(Syndication::Loader*,
                  Syndication::FeedPtr, Syndication::ErrorCode)) );
  loader->loadFrom( QString( "http://www.blogger.com/feeds/" ) + profileId()
      + QString( "/blogs" ) );
}

void GData::listRecentPostings( const QString &label, const int number, 
                const KDateTime &minTime, const KDateTime &maxTime, 
                const listRecentPostingsOptions &opts )
{

}

void GData::listRecentPostings( const int number )
{
  Q_D(GData);
  kDebug() << "listRecentPostings()" << endl;
  Syndication::Loader *loader = Syndication::Loader::create();
  connect( loader, SIGNAL(loadingComplete(Syndication::Loader*,
                          Syndication::FeedPtr, Syndication::ErrorCode)),
                          this, SLOT(slotListedRecentPostings(
                                  Syndication::Loader*,
                  Syndication::FeedPtr, Syndication::ErrorCode)) );
  loader->loadFrom( QString( "http://www.blogger.com/feeds/" ) + blogId()
      + QString( "/posts/default" ) );
}

void GData::listComments( KBlog::BlogPosting *posting )
{
  Q_D(GData);
  kDebug() << "listComments()" << endl;
  Syndication::Loader *loader = Syndication::Loader::create();
  connect( loader, SIGNAL(loadingComplete(Syndication::Loader*,
                          Syndication::FeedPtr, Syndication::ErrorCode)),
                          this, SLOT(slotListedComments(
                                  Syndication::Loader*,
                  Syndication::FeedPtr, Syndication::ErrorCode)) );
  loader->loadFrom( QString( "http://www.blogger.com/feeds/" ) + blogId()
      + posting->postingId() + QString( "/comments/default" ) );
}

void GData::listAllComments()
{
  Q_D(GData);
  kDebug() << "listRecentPostings()" << endl;
  Syndication::Loader *loader = Syndication::Loader::create();
  connect( loader, SIGNAL(loadingComplete(Syndication::Loader*,
                          Syndication::FeedPtr, Syndication::ErrorCode)),
                          this, SLOT(slotListedAllComments(
                                  Syndication::Loader*,
                  Syndication::FeedPtr, Syndication::ErrorCode)) );
  loader->loadFrom( QString( "http://www.blogger.com/feeds/" ) + blogId()
      + QString( "/comments/default" ) );
}

void GData::fetchPosting( KBlog::BlogPosting *posting )
{
  Q_D(GData);
  kDebug() << "fetchPosting()" << endl;
  Syndication::Loader *loader = Syndication::Loader::create();
//   d->setFetchPostingId( postingId );
  connect( loader, SIGNAL(loadingComplete(Syndication::Loader*,
                   Syndication::FeedPtr, Syndication::ErrorCode)),
                   this, SLOT(slotFetchedPosting(Syndication::Loader*,
                   Syndication::FeedPtr, Syndication::ErrorCode)));
  loader->loadFrom( QString( "http://www.blogger.com/feeds/" ) + blogId()
      + QString( "/posts/default" ) );
}

void GData::modifyPosting( KBlog::BlogPosting* posting )
{
  Q_D(GData);
  Q_UNUSED( posting );
  kDebug() << "modifyPosting()" << endl;
  d->authenticate();
}

void GData::createPosting( KBlog::BlogPosting* posting )
{
  Q_D(GData);
  Q_UNUSED( posting );
    kDebug() << "createPosting()" << endl;
    d->authenticate();

    QString atomMarkup = "<entry xmlns='http://www.w3.org/2005/Atom'>";
    atomMarkup += "<title type='text'>"+posting->title() +"</title>";
    if( !posting->isPublished() )
    {
      atomMarkup += "<app:control xmlns:app=*http://purl.org/atom/app#'>";
      atomMarkup += "<app:draft>yes</app:draft></app:control>";
    }
    atomMarkup += "<content type='xhtml'>";
    atomMarkup += "<div xmlns='http://www.w3.org/1999/xhtml'>";
    atomMarkup += posting->content();
    atomMarkup += "</div></content>";
    atomMarkup += "<author>";
    atomMarkup += "<name>" + fullName() + "</name>"; //FIXME user's name
    atomMarkup += "<email>" + username() + "</email>";
    atomMarkup += "</author>";
    atomMarkup += "</entry>";

    QByteArray postData;
    QDataStream stream( &postData, QIODevice::WriteOnly );
    stream.writeRawData( atomMarkup.toUtf8(), atomMarkup.toUtf8().length() );

    KIO::TransferJob *job = KIO::http_post(
        KUrl( "http://www.blogger.com/feeds/" + blogId() + "/posts/default" ),
        postData, false );

    if ( !job ) {
      kWarning() << "Unable to create KIO job for http://www.blogger.com/feeds/"
          << blogId() <<"/posts/default" << endl;
    }

    job->addMetaData( "content-type", "Content-Type: text/xml; charset=utf-8" );
    job->addMetaData( "ConnectTimeout", "50" );

//     connect( job, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
//              d, SLOT( slotData( KIO::Job *, const QByteArray & ) ) );
//     connect( job, SIGNAL( result( KJob * ) ),
//              d, SLOT( slotCreatePosting( KJob * ) ) );
}

void GData::removePosting( KBlog::BlogPosting *posting )
{
  Q_D(GData);
    Q_UNUSED( posting );
    kDebug() << "deletePosting()" << endl;
    d->authenticate();
}

void GData::createComment( KBlog::BlogPosting *posting, KBlog::BlogPostingComment *comment )
{
  Q_D(GData);
  return; //FIXME
}

void GData::deleteComment( KBlog::BlogPosting *posting, KBlog::BlogPostingComment *comment )
{
  Q_D(GData);
  return; //FIXME
}

GDataPrivate::GDataPrivate():
  mAuthenticationString(),mAuthenticationTime(){

}

GDataPrivate::~GDataPrivate(){

}

QString GDataPrivate::authenticate(){
  Q_Q(GData);
  QByteArray data;
  KUrl authGateway( "https://www.google.com/accounts/ClientLogin" );
  authGateway.addQueryItem( "Email", q->username() );
  authGateway.addQueryItem( "Passwd", q->password() );
  authGateway.addQueryItem( "source" , q->userAgent() );
  authGateway.addQueryItem( "service", "blogger" );
  if( !mAuthenticationTime.isValid() ||
      QDateTime::currentDateTime().toTime_t() - mAuthenticationTime.toTime_t()
       > TIMEOUT || mAuthenticationString.isEmpty() ){
    KIO::Job *job = KIO::http_post( authGateway, QByteArray(), false );
    if ( KIO::NetAccess::synchronousRun(
         job, (QWidget*)0, &data, &authGateway ) ) {
      kDebug(5323) << "Fetched authentication result for " <<
          authGateway.prettyUrl() << ". " << endl;
      kDebug(5323) << "Authentication response: " << data << endl;
      QRegExp rx( "Auth=(.+)" );
      if( rx.indexIn( data )!=-1 ){
        kDebug(5323)<<"RegExp got authentication string: " << rx.cap(1) << endl;
        mAuthenticationString = rx.cap(1);
        mAuthenticationTime = QDateTime::currentDateTime();
        return mAuthenticationString;
      }
    }
    return QString();
  }
  return mAuthenticationString;
}

void GDataPrivate::slotFetchProfileIdData( KIO::Job *job, const QByteArray &data )
{
  unsigned int oldSize = mFetchProfileIdBuffer[ job ].size();
  mFetchProfileIdBuffer[ job ].resize( oldSize + data.size() );
  memcpy( mFetchProfileIdBuffer[ job ].data() + oldSize, data.data(), data.size() );
}

void GDataPrivate::slotFetchProfileId(KIO::Job* job)
{
  Q_Q(GData);
  if ( !job->error() ) {
    QRegExp pid( "http://www.blogger.com/profile/(\\d+)" );
    if( pid.indexIn( mFetchProfileIdBuffer[ job ] )!=-1 ){
       q->setProfileId( pid.cap(1) );
       emit q->fetchedProfileId( pid.cap(1) );
    }
    else
      emit q->error( GData::Other, i18n( "Could not regexp the Profile ID." ) );
      emit q->fetchedProfileId( QString() );
    kDebug(5323)<<"QRegExp bid( 'http://www.blogger.com/profile/(\\d+)' matches "
        << pid.cap(1) << endl;
  }
  else {
    emit q->error( GData::Other, i18n( "Could not fetch the homepage data." ) );
    emit q->fetchedProfileId( QString() );
    kDebug(5323)<< "Could not fetch the homepage data." << endl;
  }
  mFetchProfileIdBuffer[ job ].resize( 0 );
  mFetchProfileIdBuffer.remove( job );
}

void GDataPrivate::slotListBlogs(
    Syndication::Loader* loader, Syndication::FeedPtr feed,
    Syndication::ErrorCode status ) {
  Q_Q(GData);
  Q_UNUSED( loader );
  if (status != Syndication::Success){
    emit q->error( GData::Atom, i18n( "Could not get blogs." ) );
    return;
  }

  QMap <QString,QMap<QString,QString> > blogMap;

  QList<Syndication::ItemPtr> items = feed->items();
  QList<Syndication::ItemPtr>::ConstIterator it = items.begin();
  QList<Syndication::ItemPtr>::ConstIterator end = items.end();
  for( ; it!=end; ++it ){
      QRegExp rx( "blog-(\\d+)" );
      if( rx.indexIn( ( *it )->id() )!=-1 ){
        kDebug(5323)<<"QRegExp rx( 'blog-(\\d+)' matches "<< rx.cap(1) << endl;
        blogMap[ rx.cap(1) ]["title"] = ( *it )->title();
        blogMap[ rx.cap(1) ]["summary"] = ( *it )->description(); //TODO fix/add more
      }
      else{
        emit q->error( GData::Other,
                            i18n( "Could not regexp the blog id path." ) );
        kDebug(5323)<<"QRegExp rx( 'blog-(\\d+)' does not match anything in: "
            << ( *it )->id() << endl;
      }

    }
    emit q->listedBlogs( blogMap );
    kDebug(5323) << "Emitting listedBlogs(); " << endl;
}


void GDataPrivate::slotListComments(
    Syndication::Loader* loader, Syndication::FeedPtr feed,
    Syndication::ErrorCode status )
{

}

void GDataPrivate::slotListAllComments(
    Syndication::Loader* loader, Syndication::FeedPtr feed,
    Syndication::ErrorCode status )
{

}

void GDataPrivate::slotListRecentPostings(
    Syndication::Loader* loader, Syndication::FeedPtr feed,
    Syndication::ErrorCode status ) {
  Q_Q(GData);
  Q_UNUSED( loader );

  if (status != Syndication::Success){
    emit q->error( GData::Atom, i18n( "Could not get postings." ) );
    return;
  }

  QList<KBlog::BlogPosting*> postingList;

  QList<Syndication::ItemPtr> items = feed->items();
  QList<Syndication::ItemPtr>::ConstIterator it = items.begin();
  QList<Syndication::ItemPtr>::ConstIterator end = items.end();
  for( ; it!=end; ++it ){
      BlogPosting* posting = new BlogPosting;
      QRegExp rx( "post-(\\d+)" );
      if( rx.indexIn( ( *it )->id() )==-1 ){
        kDebug(5323)<<
        "QRegExp rx( 'post-(\\d+)' does not match "<< rx.cap(1) << endl;
        emit q->error( GData::Other,
        i18n( "Could not regexp the posting id path." ) );
        return;
      }

      kDebug(5323)<<"QRegExp rx( 'post-(\\d+)' matches "<< rx.cap(1) << endl;
      posting->setPostingId( rx.cap(1) );
      posting->setTitle( ( *it )->title() );
      posting->setContent( ( *it )->content() );
//       FIXME: assuming UTC for now
      posting->setCreationDateTime( KDateTime( QDateTime::fromTime_t(
  ( *it )->datePublished() ), KDateTime::Spec::UTC() ) );
      posting->setModificationDateTime( KDateTime( QDateTime::fromTime_t(
  ( *it )->dateUpdated() ), KDateTime::Spec::UTC() ) );
      postingList.append( posting );
  }
  kDebug(5323) << "Emitting listedRecentPostings()" << endl;
  emit q->listedRecentPostings( postingList );
}

void GDataPrivate::slotFetchPosting(
    Syndication::Loader* loader, Syndication::FeedPtr feed,
    Syndication::ErrorCode status ){
  Q_Q(GData);

  bool success = false;

  if (status != Syndication::Success){
    emit q->error( GData::Atom, i18n( "Could not get postings." ) );
    return;
  }
  QList<Syndication::ItemPtr> items = feed->items();
  QList<Syndication::ItemPtr>::ConstIterator it = items.begin();
  QList<Syndication::ItemPtr>::ConstIterator end = items.end();
  for( ; it!=end; ++it ){
      BlogPosting* posting = new BlogPosting();
      QRegExp rx( "post-(\\d+)" );
      if( rx.indexIn( ( *it )->id() )!=-1 && rx.cap(1)==mFetchPostingsMap[ loader ]->postingId() ){
        kDebug(5323)<<"QRegExp rx( 'post-(\\d+)' matches "<< rx.cap(1) << endl;
        posting->setPostingId( rx.cap(1) );
        posting->setTitle( ( *it )->title() );
        posting->setContent( ( *it )->content() );
//         FIXME: assuming UTC for now
        posting->setCreationDateTime( KDateTime( QDateTime::fromTime_t(
                                     ( *it )->datePublished() ),
                                        KDateTime::Spec::UTC() ) );
        posting->setModificationDateTime( KDateTime( QDateTime::fromTime_t(
                                         ( *it )->dateUpdated() ),
                                            KDateTime::Spec::UTC() ) );
        emit q->fetchedPosting( posting );
        success = true;
        kDebug(5323) << "Emitting fetchedPosting( postingId="
            << posting->postingId() << " ); " << endl;
      }
  }
  if(!success){
    emit q->error( GData::Other, i18n( "Could not regexp the blog id path." ) );
    kDebug(5323) << "QRegExp rx( 'post-(\\d+)' does not match "
        << mFetchPostingsMap[ loader ]->postingId() << ". " << endl;
  }
  mFetchPostingsMap.remove( loader );
}

void GDataPrivate::slotCreatePostingData( KIO::Job *job, const QByteArray &data )
{
  unsigned int oldSize = mCreatePostingBuffer[ job ].size();
  mCreatePostingBuffer[ job ].resize( oldSize + data.size() );
  memcpy( mCreatePostingBuffer[ job ].data() + oldSize, data.data(), data.size() );
}

void GDataPrivate::slotCreatePosting( KIO::Job *job )
{
  Q_Q(GData);
  if ( job->error() != 0 ) {
    kDebug(5323) << "slotCreatePosting error: " << job->errorString() << endl;
    emit q->error( GData::Atom, job->errorString() );
    mCreatePostingBuffer[ job ].resize( 0 );
    mCreatePostingBuffer.remove( job );
    return;
  }

  const QString data = QString::fromUtf8( mCreatePostingBuffer[ job ].data(), mCreatePostingBuffer[ job ].size() );

  kDebug(5323) << "Response: " << data << endl;
}

#include "gdata.moc"
