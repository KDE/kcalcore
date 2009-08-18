/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "collectionmodifyjob.h"
#include "imapparser_p.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "collectionstatistics.h"
#include "collection_p.h"

using namespace Akonadi;

class Akonadi::CollectionModifyJobPrivate : public JobPrivate
{
  public:
    CollectionModifyJobPrivate( CollectionModifyJob *parent )
      : JobPrivate( parent )
    {
    }

    Collection mCollection;
};

CollectionModifyJob::CollectionModifyJob( const Collection &collection, QObject * parent )
  : Job( new CollectionModifyJobPrivate( this ), parent )
{
  Q_D( CollectionModifyJob );
  d->mCollection = collection;
}

CollectionModifyJob::~CollectionModifyJob()
{
}

void CollectionModifyJob::doStart()
{
  Q_D( CollectionModifyJob );
  if ( !d->mCollection.isValid() && d->mCollection.remoteId().isEmpty() ) {
    setError( Unknown );
    setErrorText( QLatin1String( "Invalid collection" ) );
    emitResult();
    return;
  }

  QByteArray command = d->newTag();
  if ( d->mCollection.isValid() )
    command += " MODIFY " + QByteArray::number( d->mCollection.id() );
  else
    command += " RID MODIFY " + ImapParser::quote( d->mCollection.remoteId().toUtf8() );
  QByteArray changes;
  if ( d->mCollection.d_func()->contentTypesChanged ) {
    QList<QByteArray> bList;
    foreach( const QString &s, d->mCollection.contentMimeTypes() ) bList << s.toLatin1();
    changes += " MIMETYPE (" + ImapParser::join( bList, " " ) + ')';
  }
  if ( d->mCollection.parentCollection().id() >= 0 )
    changes += " PARENT " + QByteArray::number( d->mCollection.parentCollection().id() );
  if ( !d->mCollection.name().isEmpty() )
    changes += " NAME " + ImapParser::quote( d->mCollection.name().toUtf8() );
  if ( !d->mCollection.remoteId().isNull() )
    changes += " REMOTEID " + ImapParser::quote( d->mCollection.remoteId().toUtf8() );
  if ( d->mCollection.d_func()->cachePolicyChanged )
    changes += ' ' + ProtocolHelper::cachePolicyToByteArray( d->mCollection.cachePolicy() );
  if ( d->mCollection.attributes().count() > 0 )
    changes += ' ' + ProtocolHelper::attributesToByteArray( d->mCollection );
  foreach ( const QByteArray &b, d->mCollection.d_func()->mDeletedAttributes )
    changes += " -" + b;
  if ( changes.isEmpty() ) {
    emitResult();
    return;
  }
  command += changes + '\n';
  d->writeData( command );
}

Collection CollectionModifyJob::collection() const
{
  const Q_D( CollectionModifyJob );
  return d->mCollection;
}

#include "collectionmodifyjob.moc"
