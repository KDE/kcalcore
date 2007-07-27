/*
  This file is part of the kblog library.

  Copyright (c) 2007 Christian Weilbach <christian@whiletaker.homeip.net>
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

#include "metaweblog_p.h"
#include "blogposting.h"

#include <kxmlrpcclient/client.h>

#include <KDebug>
#include <KDateTime>
#include <KLocale>

using namespace KBlog;

MetaWeblogPrivate::MetaWeblogPrivate()
{
  mXmlRpcClient = 0;
}

MetaWeblogPrivate::~MetaWeblogPrivate()
{
  delete mXmlRpcClient;
}

QList<QVariant> MetaWeblogPrivate::defaultArgs( const QString &id )
{
  Q_Q(MetaWeblog);
  QList<QVariant> args;

  if ( id.toInt() ) {
    args << QVariant( id.toInt() );
  }
  if ( !id.toInt() && !id.isNull() ){
    args << QVariant( id );
  }
  args << QVariant( q->username() )
       << QVariant( q->password() );
  return args;
}

void MetaWeblogPrivate::slotListCategories( const QList<QVariant> &result,
                                                              const QVariant &id )
{
  Q_UNUSED( result );
  Q_UNUSED( id );
//
//   kDebug(5323) << "MetaWeblogPrivate::slotListCategories" << endl;
//   kDebug(5323) << "TOP: " << result[0].typeName() << endl;
//   if ( result[0].type() != QVariant::Map &&
//        result[0].type() != QVariant::List ) {
//     // include fix for not metaweblog standard compatible apis with
//     // array of structs instead of struct of structs, e.g. wordpress
//     kDebug(5323) << "Could not list categories out of the result from the server." << endl;
//     emit q->error( ParsingError,
//                         i18n( "Could not list categories out of the result "
//                               "from the server." ) );
//   } else {
//     if ( result[0].type() == QVariant::Map ) {
//       const QMap<QString, QVariant> categories = result[0].toMap();
//       const QList<QString> categoryNames = categories.keys();
//
//       QList<QString>::ConstIterator it = categoryNames.begin();
//       QList<QString>::ConstIterator end = categoryNames.end();
//       for ( ; it != end; ++it ) {
//         kDebug(5323) << "MIDDLE: " << ( *it ) << endl;
//         const QString name( *it );
//         const QMap<QString, QVariant> category = categories[*it].toMap();
//         const QString description( category["description"].toString() );
//         if ( !name.isEmpty() ) {
//           emit q->categoryInfoRetrieved(); //FIXME set the data
//           kDebug(5323) << "Emitting categorieInfoRetrieved( name=" << name
//                        << " description=" << description << " ); " << endl;
//         }
//       }
//     }
//     if ( result[0].type() == QVariant::List ) {
//       // include fix for not metaweblog standard compatible apis with
//       // array of structs instead of struct of structs, e.g. wordpress
//       const QList<QVariant> categories = result[0].toList();
//       QList<QVariant>::ConstIterator it = categories.begin();
//       QList<QVariant>::ConstIterator end = categories.end();
//       for ( ; it != end; ++it ) {
//         kDebug(5323) << "MIDDLE: " << ( *it ).typeName() << endl;
//         const QMap<QString, QVariant> category = ( *it ).toMap();
//         const QString description( category["description"].toString() );
//         const QString name( category["categoryName"].toString() );
//         if ( !name.isEmpty() ) {
//           emit q->categoryInfoRetrieved();//FIXME set the data
//           kDebug(5323) << "Emitting categorieInfoRetrieved( name=" << name
//                        << " description=" << description << " ); " << endl;
//         }
//       }
//       kDebug(5323) << "Emitting listCategoriesFinished()" << endl;
//       emit q->listCategoriesFinished();
//     }
//   }
}

void MetaWeblogPrivate::slotListRecentPostings( const QList<QVariant> &result,
                                                            const QVariant &id )
{
  Q_UNUSED( result );
  Q_UNUSED( id );
/*
  kDebug(5323) << "MetaWeblog::slotListRecentPostings" << endl;
  kDebug(5323) << "TOP: " << result[0].typeName() << endl;
  if ( result[0].type() != QVariant::List ) {
    kDebug(5323) << "Could not fetch list of postings out of the "
                 << "result from the server." << endl;
    emit q->error( ParsingError,
                        i18n( "Could not fetch list of postings out of the "
                              "result from the server." ) );
  } else {
    const QList<QVariant> postReceived = result[0].toList();
    QList<QVariant>::ConstIterator it = postReceived.begin();
    QList<QVariant>::ConstIterator end = postReceived.end();
    for ( ; it != end; ++it ) {
      BlogPosting* posting = new BlogPosting;
      kDebug(5323) << "MIDDLE: " << ( *it ).typeName() << endl;
      const QMap<QString, QVariant> postInfo = ( *it ).toMap();
      if ( readPostingFromMap( posting, postInfo ) ) {
        kDebug(5323) << "Emitting listedPosting( posting.postingId()="
                     << posting->postingId() << "); " << endl;
        emit q->listedPosting( posting ); // KUrl( posting.postingId() ) );
      } else {
        kDebug(5323) << "d->readPostingFromMap failed! " << endl;
        emit q->error( ParsingError, i18n( "Could not read posting." ) );
      }
    }
  } //FIXME should we emit here? (see below, too)
  kDebug(5323) << "Emitting listRecentPostingsFinished()" << endl;
  emit q->listRecentPostingsFinished();*/
}

void MetaWeblogPrivate::slotFetchPosting( const QList<QVariant> &result,
                                                            const QVariant &id )
{
  Q_UNUSED( id );
  Q_Q(MetaWeblog);
  kDebug(5323) << "MetaWeblog::slotFetchPosting" << endl;
  //array of structs containing ISO.8601
  // dateCreated, String userid, String postid, String content;
  // TODO: Time zone for the dateCreated!
  kDebug(5323) << "TOP: " << result[0].typeName() << endl;
  if ( result[0].type() != QVariant::Map ) {
    kDebug(5323) << "Could not fetch posting out of the result from the server." << endl;
    //emit q->error( KBlog::Blog::ParsingError,
    //                    i18n( "Could not fetch posting out of the "
    //                          "result from the server." ) );
  } else {
//     const QList<QVariant> postReceived = result[0].toList();
//     QList<QVariant>::ConstIterator it = postReceived.begin();
    BlogPosting posting;
    const QMap<QString, QVariant> postInfo = result[0].toMap();
    if ( readPostingFromMap( &posting, postInfo ) ) {
      kDebug(5323) << "Emitting fetchedPosting( posting.postingId()="
                   << posting.postingId() << "); " << endl;
//       emit q->fetchedPosting( posting ); // KUrl( posting.posingtId() ) );
    } else {
      kDebug(5323) << "d->readPostingFromMap failed! " << endl;
      //emit q->error( KBlog::Blog::ParsingError,
      //                    i18n( "Could not read posting." ) );
    }
  }
}

void MetaWeblogPrivate::slotCreatePosting( const QList<QVariant> &result,
                                                             const QVariant &id )
{
  Q_UNUSED( id );
  Q_Q(MetaWeblog);
  kDebug(5323) << "MetaWeblog::slotCreatePosting" << endl;
  //array of structs containing ISO.8601
  // dateCreated, String userid, String postid, String content;
  // TODO: Time zone for the dateCreated!
  kDebug(5323) << "TOP: " << result[0].typeName() << endl;
  if ( result[0].type() != QVariant::String ) {
    kDebug(5323) << "Could not read the postingId, not a string." << endl;
    //emit q->error( KBlog::Blog::ParsingError,
    //                    i18n( "Could not read the postingId, not a string." ) );
  } else {
//     emit q->createdPosting( result[0].toString() );
    kDebug(5323) << "emitting createdPosting( " << result[0].toString() << " )" << endl;
  }
}

void MetaWeblogPrivate::slotModifyPosting( const QList<QVariant> &result,
                                                             const QVariant &id )
{
  Q_UNUSED( id );
  Q_Q(MetaWeblog);
  kDebug(5323) << "MetaWeblog::slotModifyPosting" << endl;
  //array of structs containing ISO.8601
  // dateCreated, String userid, String postid, String content;
  // TODO: Time zone for the dateCreated!
  kDebug(5323) << "TOP: " << result[0].typeName() << endl;
  if ( result[0].type() != QVariant::Bool ) {
    kDebug(5323) << "Could not read the result, not a boolean." << endl;
    //emit q->error( KBlog::Blog::ParsingError,
    //                    i18n( "Could not read the result, not a boolean." ) );
  } else {
//     emit q->modifiedPosting( result[0].toBool() );
    kDebug(5323) << "emitting modifiedPosting( " << result[0].toBool() << " )" << endl;
  }
}

void MetaWeblogPrivate::slotCreateMedia( const QList<QVariant> &result,
                                                           const QVariant &id )
{
  Q_UNUSED( id );
  Q_Q(MetaWeblog);
  kDebug(5323) << "MetaWeblogPrivate::slotCreateMedia, no error!" << endl;
  kDebug(5323) << "TOP: " << result[0].typeName() << endl;
  if ( result[0].type() != 8 ) {
    kDebug(5323) << "Could not read the result, not a map." << endl;
    //emit q->error( KBlog::Blog::ParsingError,
    //                    i18n( "Could not read the result, not a map." ) );
  } else {
    const QMap<QString, QVariant> resultStruct = result[0].toMap();
    const QString url = resultStruct["url"].toString();
    kDebug(5323) << "MetaWeblog::slotCreateMedia url=" << url << endl;

    if ( !url.isEmpty() ) {
//       emit q->createdMedia( url );
      kDebug(5323) << "Emitting createdMedia( url=" << url  << " ); " << endl;
    }
  }
}

void MetaWeblogPrivate::slotError( int number,
                                                     const QString &errorString,
                                                     const QVariant &id )
{
  Q_UNUSED( number );
  Q_UNUSED( id );

  //emit q->error( KBlog::Blog::XmlRpc, errorString );
}

bool MetaWeblogPrivate::readPostingFromMap( BlogPosting *post,
                                                        const QMap<QString, QVariant> &postInfo )
{
  // FIXME: integrate error handling
  if ( !post ) {
    return false;
  }
  QStringList mapkeys = postInfo.keys();
  kDebug(5323) << endl << "Keys: " << mapkeys.join( ", " ) << endl << endl;

  KDateTime dt =
    KDateTime( postInfo["dateCreated"].toDateTime(), KDateTime::UTC );
  if ( dt.isValid() && !dt.isNull() ) {
    post->setCreationDateTime( dt );
  }

  dt =
    KDateTime( postInfo["lastModified"].toDateTime(), KDateTime::UTC );
  if ( dt.isValid() && !dt.isNull() ) {
    post->setModificationDateTime( dt );
  }

  post->setPostingId( postInfo["postid"].toString() );

  QString title( postInfo["title"].toString() );
  QString description( postInfo["description"].toString() );
  QStringList categories( postInfo["categories"].toStringList() );

  post->setTitle( title );
  post->setContent( description );
  if ( !categories.isEmpty() ){
    kDebug(5323) << "Categories: " << categories << endl;
    post->setCategories( categories );
  }
  return true;
}

#include "metaweblog_p.moc"
