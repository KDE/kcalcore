/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#include "entity.h"
#include "entity_p.h"

using namespace Akonadi;

Entity::Entity( const Entity &other )
  : d_ptr( other.d_ptr )
{
}

Entity::Entity( EntityPrivate *dd )
  : d_ptr( dd )
{
}

Entity::~Entity()
{
}

void Entity::setId( Id id )
{
  d_ptr->mId = id;
}

Entity::Id Entity::id() const
{
  return d_ptr->mId;
}

void Entity::setRemoteId( const QString& id )
{
  d_ptr->mRemoteId = id;
}

QString Entity::remoteId() const
{
  return d_ptr->mRemoteId;
}

bool Entity::isValid() const
{
  return ( d_ptr->mId >= 0 );
}

bool Entity::operator==( const Entity &other ) const
{
  return ( d_ptr->mId == other.d_ptr->mId );
}

bool Akonadi::Entity::operator!=(const Entity & other) const
{
  return d_ptr->mId != other.d_ptr->mId;
}

Entity& Entity ::operator=( const Entity &other )
{
  if ( this != &other )
    d_ptr = other.d_ptr;

  return *this;
}

void Entity::addAttribute(Attribute * attr)
{
  if ( d_ptr->mAttributes.contains( attr->type() ) )
    delete d_ptr->mAttributes.take( attr->type() );
  d_ptr->mAttributes.insert( attr->type(), attr );
  d_ptr->mDeletedAttributes.remove( attr->type() );
}

void Entity::removeAttribute( const QByteArray &type )
{
  if ( d_ptr->mAttributes.contains( type ) ) {
    d_ptr->mDeletedAttributes.insert( type );
    delete d_ptr->mAttributes.take( type );
  }
}

bool Entity::hasAttribute(const QByteArray & type) const
{
  return d_ptr->mAttributes.contains( type );
}

QList< Attribute * > Entity::attributes() const
{
  return d_ptr->mAttributes.values();
}

void Akonadi::Entity::clearAttributes()
{
  foreach ( Attribute *attr, d_ptr->mAttributes ) {
    d_ptr->mDeletedAttributes.insert( attr->type() );
    delete attr;
  }
  d_ptr->mAttributes.clear();
}

Attribute * Entity::attribute(const QByteArray & type) const
{
  if ( d_ptr->mAttributes.contains( type ) )
    return d_ptr->mAttributes.value( type );
  return 0;
}

uint qHash( const Akonadi::Entity &entity )
{
  return qHash( entity.id() );
}

AKONADI_DEFINE_PRIVATE( Entity )
