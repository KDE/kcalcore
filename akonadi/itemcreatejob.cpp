/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2007        Robert Zwerus <arzie@dds.nl>

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

#include "itemcreatejob.h"

#include "collection.h"
#include "imapparser_p.h"
#include "item.h"
#include "itemserializer.h"
#include "job_p.h"

#include <kdebug.h>

#include <QtCore/QStringList>

using namespace Akonadi;

class Akonadi::ItemCreateJobPrivate : public JobPrivate
{
  public:
    ItemCreateJobPrivate( ItemCreateJob *parent )
      : JobPrivate( parent )
    {
    }

    Collection mCollection;
    Item mItem;
    QStringList mParts;
    Item::Id mUid;
    QByteArray mData;
};

ItemCreateJob::ItemCreateJob( const Item &item, const Collection &collection, QObject * parent )
  : Job( new ItemCreateJobPrivate( this ), parent )
{
  Q_D( ItemCreateJob );

  Q_ASSERT( !item.mimeType().isEmpty() );
  d->mItem = item;
  d->mParts = d->mItem.loadedPayloadParts();
  foreach ( const Attribute *attr, item.attributes() )
    d->mParts << QString::fromLatin1( attr->type() );
  d->mCollection = collection;
}

ItemCreateJob::~ItemCreateJob()
{
}

void ItemCreateJob::doStart()
{
  Q_D( ItemCreateJob );

  QByteArray remoteId;

  if ( !d->mItem.remoteId().isEmpty() )
    remoteId = ' ' + ImapParser::quote( "\\RemoteId[" + d->mItem.remoteId().toUtf8() + ']' );
  // switch between a normal APPEND and a multipart X-AKAPPEND, based on the number of parts
  if ( d->mParts.isEmpty() || (d->mParts.size() == 1 && d->mParts.first() == Item::PartBody) ) {
    if ( d->mItem.hasPayload() )
      ItemSerializer::serialize( d->mItem, Item::PartBody, d->mData );
    int dataSize = d->mData.size();

    writeData( newTag() + " APPEND " + QByteArray::number( d->mCollection.id() )
        + " (\\MimeType[" + d->mItem.mimeType().toLatin1() + ']' + remoteId + ") {"
        + QByteArray::number( dataSize ) + "}\n" );
  }
  else { // do a multipart X-AKAPPEND
    QByteArray command = newTag() + " X-AKAPPEND " + QByteArray::number( d->mCollection.id() )
        + " (\\MimeType[" + d->mItem.mimeType().toLatin1() + ']' + remoteId + ") ";

    QList<QByteArray> partSpecs;
    int totalSize = 0;
    foreach( const QString partName, d->mParts ) {
      QByteArray partData;
      ItemSerializer::serialize( d->mItem, partName, partData );
      totalSize += partData.size();
      partSpecs.append( ImapParser::quote( partName.toLatin1() ) + ':' +
        QByteArray::number( partData.size() ) );
      d->mData += partData;
    }
    command += '(' + ImapParser::join( partSpecs, "," ) + ") " +
      '{' + QByteArray::number( totalSize ) + "}\n";

    writeData( command );
  }
}

void ItemCreateJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  Q_D( ItemCreateJob );

  if ( tag == "+" ) { // ready for literal data
    writeData( d->mData );
    if ( !d->mData.endsWith( '\n' ) )
      writeData( "\n" );
    return;
  }
  if ( tag == this->tag() ) {
    if ( int pos = data.indexOf( "UIDNEXT" ) ) {
      bool ok = false;
      ImapParser::parseNumber( data, d->mUid, &ok, pos + 7 );
      if ( !ok ) {
        kDebug( 5250 ) << "Invalid UIDNEXT response to APPEND command: "
                       << tag << data;
      }
    }
  }
}

Item ItemCreateJob::item() const
{
  Q_D( const ItemCreateJob );

  if ( d->mUid == 0 )
    return Item();

  Item item( d->mItem );
  item.setId( d->mUid );

  return item;
}

#include "itemcreatejob.moc"
