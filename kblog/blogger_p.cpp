/*
    This file is part of the kblog library.

    Copyright (c) 2007 Christian Weilbach <christian@whiletaker.homeip.net>

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

#include <blogger.h>
#include <kxmlrpcclient/client.h>
#include <kdebug.h>
#include <klocale.h>

#include <blogger_p.h>

#include <QtCore/QList>

using namespace KBlog;

APIBlogger::APIBloggerPrivate::APIBloggerPrivate()
{
  mXmlRpcClient = 0;
}

APIBlogger::APIBloggerPrivate::~APIBloggerPrivate()
{
  delete mXmlRpcClient;  
}

QList<QVariant> APIBlogger::APIBloggerPrivate::defaultArgs( const QString &id )
{
  QList<QVariant> args;
  args << QVariant( QString( "0123456789ABCDEF" ) );
  if ( !id.isNull() ) {
    args << QVariant( id );
  }
  args << QVariant( parent->username() )
       << QVariant( parent->password() );
  return args;
}

void APIBlogger::APIBloggerPrivate::slotUserInfo( const QList<QVariant> &result, const QVariant &id )
{
  // TODO: Implement user authentication
  kDebug () << "TOP: " << result[ 0 ].typeName() << endl;
  if( result[ 0 ].type()!=QVariant::Map ){
    kDebug () << "Could not fetch user information out of the result from the server, not a list." << endl;
    emit parent->error( ParsingError, i18n("Could not fetch user information out of the result from the server, not a list." ) );
  }
  else {
    const QMap<QString,QVariant> userInfo= result[ 0 ].toMap();
    const QString nickname = userInfo[ "nickname" ].toString();
    const QString userid = userInfo[ "userid" ].toString();
    const QString email = userInfo[ "email" ].toString();
    kDebug() << "emit userInfoRetrieved( " << nickname << ", " << userid << ", " << email << " )" << endl;
    // FIXME: What about a BlogUserInfo class/struct?
    emit parent->userInfoRetrieved( nickname, userid, email );
  }
}

void APIBlogger::APIBloggerPrivate::slotListBlogs( const QList<QVariant> &result, const QVariant &id )
{
  kDebug() << "APIBlogger::slotListBlogs" << endl;
  kDebug () << "TOP: " << result[ 0 ].typeName() << endl;
  if( result[ 0 ].type()!=QVariant::List ){
    kDebug () << "Could not fetch blogs out of the result from the server, not a list." << endl;
    emit parent->error( ParsingError, i18n("Could not blogs Posting out of the result from the server, not a list." ) );
  }
  else {
    const QList<QVariant> posts = result[ 0 ].toList();
    QList<QVariant>::ConstIterator it = posts.begin();
    QList<QVariant>::ConstIterator end = posts.end();
    for ( ; it != end; ++it ) {
      kDebug () << "MIDDLE: " << ( *it ).typeName() << endl;
      const QMap<QString, QVariant> postInfo = ( *it ).toMap();

      const QString id( postInfo[ "blogid" ].toString() );
      const QString name( postInfo[ "blogName" ].toString() );
      const QString url( postInfo[ "url" ].toString() );

      if ( !id.isEmpty() && !name.isEmpty() ) {
        emit parent->blogInfoRetrieved( id, name );
        kDebug()<< "Emitting blogInfoRetrieved( id=" << id << ", name=" << name << "); " << endl;
      }
    }
  }
}

void APIBlogger::APIBloggerPrivate::slotListPostings( const QList<QVariant> &result, const QVariant &id )
{
  kDebug()<<"APIBlogger::slotListPostings"<<endl;
  kDebug () << "TOP: " << result[ 0 ].typeName() << endl;
  if( result[ 0 ].type()!=QVariant::List ){
    kDebug () << "Could not fetch list of postings out of the result from the server, not a list." << endl;
    emit parent->error( ParsingError, i18n("Could not fetch list of postings out of the result from the server, not a list." ) );
  }
  else {
    const QList<QVariant> postReceived = result[ 0 ].toList();
    QList<QVariant>::ConstIterator it = postReceived.begin();
    QList<QVariant>::ConstIterator end = postReceived.end();
    for ( ; it != end; ++it ) {
      BlogPosting posting;
      kDebug () << "MIDDLE: " << ( *it ).typeName() << endl;
      const QMap<QString, QVariant> postInfo = ( *it ).toMap();
      if ( readPostingFromMap( &posting, postInfo ) ) {
        kDebug() << "Emitting listedPosting( posting.postingId()=" <<posting.postingId() << "); " << endl;
        emit parent->listedPosting( posting ); // KUrl( posting.postingId() ) );
      } else {
        kDebug() << "d->readPostingFromMap failed! " << endl;
        emit parent->error( ParsingError, i18n("Couldn't read posting.") );
      }
    }
  } //FIXME should we emit here? (see below, too)
  kDebug() << "Emitting listPostingsFinished()" << endl;
  emit parent->listPostingsFinished();
}

void APIBlogger::APIBloggerPrivate::slotFetchPosting( const QList<QVariant> &result, const QVariant &id )
{
  kDebug()<<"APIBlogger::slotFetchPosting"<<endl;
  //array of structs containing ISO.8601 dateCreated, String userid, String postid, String content;
  // TODO: Time zone for the dateCreated!
  kDebug () << "TOP: " << result[ 0 ].typeName() << endl;
  if( result[ 0 ].type()!=QVariant::Map ){
    kDebug () << "Could not fetch posting out of the result from the server." << endl;
    emit parent->error( ParsingError, i18n("Could not fetch posting out of the result from the server." ) );
  }
  else {
//     const QList<QVariant> postReceived = result[ 0 ].toList();
//     QList<QVariant>::ConstIterator it = postReceived.begin();
    BlogPosting posting;
    const QMap<QString, QVariant> postInfo = result[ 0 ].toMap();
    if ( readPostingFromMap( &posting, postInfo ) ) {
      kDebug() << "Emitting fetchedPosting( posting.postingId()=" <<posting.postingId() << "); " << endl;
      emit parent->fetchedPosting( posting ); // KUrl( posting.posingtId() ) );
    } else {
      kDebug() << "d->readPostingFromMap failed! " << endl;
      emit parent->error( ParsingError, i18n("Could not read posting.") );
    }
  }
}

void APIBlogger::APIBloggerPrivate::slotCreatePosting( const QList<QVariant> &result, const QVariant &id )
{
  kDebug()<<"APIBlogger::slotCreatePosting"<<endl;
  //array of structs containing ISO.8601 dateCreated, String userid, String postid, String content;
  // TODO: Time zone for the dateCreated!
  kDebug () << "TOP: " << result[ 0 ].typeName() << endl;
  if( result[ 0 ].type()!=QVariant::Int ){
    kDebug () << "Could not read the postingId, not an integer." << endl;
    emit parent->error( ParsingError, i18n( "Could not read the postingId, not an integer." ) );
  }
  else {
    emit parent->createdPosting( QString().setNum( result[ 0 ].toInt() ) );
    kDebug() << "emitting createdPosting( " << result[ 0 ].toInt() << " )" << endl;
  }
}

void APIBlogger::APIBloggerPrivate::slotModifyPosting( const QList<QVariant> &result, const QVariant &id )
{
  kDebug()<<"APIBlogger::slotModifyPosting"<<endl;
  //array of structs containing ISO.8601 dateCreated, String userid, String postid, String content;
  // TODO: Time zone for the dateCreated!
  kDebug () << "TOP: " << result[ 0 ].typeName() << endl;
  if( result[ 0 ].type()!=QVariant::Bool ){
    kDebug () << "Could not read the result, not a boolean." << endl;
    emit parent->error( ParsingError, i18n( "Could not read the result, not a boolean." ) );
  }
  else {
    emit parent->modifiedPosting( result[ 0 ].toBool() );
    kDebug() << "emitting modifiedPosting( " << result[ 0 ].toBool() << " )" << endl;
  }
}

void APIBlogger::APIBloggerPrivate::faultSlot( int number, const QString& errorString, const QVariant& id )
{
  emit parent->error( XmlRpc, errorString );
}

bool APIBlogger::APIBloggerPrivate::readPostingFromMap( BlogPosting *post, const QMap<QString, QVariant> &postInfo )
{
  // FIXME: integrate error handling
  if ( !post ) return false;
  QStringList mapkeys = postInfo.keys();
  kDebug() << endl << "Keys: " << mapkeys.join(", ") << endl << endl;
  
  KDateTime dt( postInfo[ "dateCreated" ].toDateTime() );
  if ( dt.isValid() && !dt.isNull() ) {
    post->setCreationDateTime( dt );
  }
  dt = KDateTime ( postInfo[ "lastModified" ].toDateTime() );
  if ( dt.isValid() && !dt.isNull() ) {
    post->setModificationDateTime( dt );
  }
//  post->setUserId( postInfo[ "userid" ].toString() ); TODO remove if sure that not needed
  post->setPostingId( postInfo[ "postid" ].toString() );

  QString title( postInfo[ "title" ].toString() );
  QString description( postInfo[ "description" ].toString() );
  QString contents( postInfo[ "content" ].toString() );
  QString category;

  post->setTitle( title );
  post->setContent( contents );
  return true;
}

#include "blogger_p.moc"
