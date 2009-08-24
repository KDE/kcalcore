/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "itemserializerplugin.h"
#include "item.h"

#include <QtCore/QBuffer>

using namespace Akonadi;

ItemSerializerPlugin::~ItemSerializerPlugin()
{
}

QSet<QByteArray> ItemSerializerPlugin::parts( const Item & item ) const
{
  QSet<QByteArray> set;
  if ( item.hasPayload() )
    set.insert( Item::FullPayload );

  return set;
}

ItemSerializerPluginV2::~ItemSerializerPluginV2()
{
}

QSet<QByteArray> ItemSerializerPluginV2::availableParts( const Item & item ) const
{
  if ( item.hasPayload() )
    return QSet<QByteArray>();

  return QSet<QByteArray>() << Item::FullPayload;
}

void ItemSerializerPluginV2::merge( Item &item, const Item &other )
{
  QBuffer buffer;
  buffer.setBuffer( &other.payloadData() );
  buffer.open( QIODevice::ReadOnly );

  foreach ( const QByteArray part, other.loadedPayloadParts() ) {
    buffer.seek( 0 );
    deserialize( item, part, buffer, 0 );
  }

  buffer.close();
}
