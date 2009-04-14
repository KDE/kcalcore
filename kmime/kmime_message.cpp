/*
    kmime_message.cpp

    KMime, the KDE internet mail/usenet news message library.
    Copyright (c) 2001 the KMime authors.
    See file AUTHORS for details

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

#include "kmime_message.h"
#include "kmime_message_p.h"
#include "kmime_util_p.h"

using namespace KMime;

namespace KMime {

Message::Message() : Content( new MessagePrivate( this ) ) {}

Message::Message(MessagePrivate * d) : Content( d ) {}

Message::~Message()
{}

void Message::parse()
{
  Q_D(Message);
  Content::parse();

  QByteArray raw;
  if ( !( raw = rawHeader( d->subject.type() ) ).isEmpty() )
    d->subject.from7BitString( raw );

  if ( !( raw = rawHeader( d->date.type() ) ).isEmpty() )
    d->date.from7BitString( raw );
}

QByteArray Message::assembleHeaders()
{
  Q_D(Message);
  Headers::Base *h;
  QByteArray newHead;

  //Message-ID
  if ( ( h = messageID( false ) ) != 0 && !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  //From
  h = from(); // "From" is mandatory
  if ( !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  //Subject
  h = subject(); // "Subject" is mandatory
  if ( !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  //To
  if ( ( h = to( false )) != 0 && !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  //Cc
  if ( ( h = cc( false )) != 0 && !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  //Reply-To
  if ( ( h = replyTo( false )) != 0 && !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  //Date
  h = date(); // "Date" is mandatory
  if ( !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  //References
  if ( ( h = references( false )) != 0 && !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  //Organization
  if ( ( h = organization( false )) != 0 && !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  //UserAgent
  if ( ( h = userAgent( false )) != 0 && !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  // In-Reply-To
  if ( ( h = inReplyTo( false ) ) != 0 && !h->isEmpty() ) {
    newHead += h->as7BitString() + '\n';
    KMime::removeHeader( d->head, h->type() );
  }

  //Mime-Version
  newHead += "MIME-Version: 1.0\n";
  KMime::removeHeader( d->head, "MIME-Version" );

  return newHead + Content::assembleHeaders();
}

void Message::clear()
{
  Q_D(Message);
  d->subject.clear();
  d->date.clear();
  Content::clear();
}

Headers::Base *Message::getHeaderByType( const char *type )
{
  Q_D(Message);
  if ( strcasecmp( "Subject", type ) == 0 ) {
    if ( d->subject.isEmpty() ) {
      return 0;
    } else {
      return &d->subject;
    }
  }
  else if ( strcasecmp("Date", type ) == 0 ){
    if ( d->date.isEmpty() ) {
      return 0;
    } else {
      return &d->date;
    }
  } else {
    return Content::getHeaderByType( type );
  }
}

void Message::setHeader( Headers::Base *h )
{
  Q_D(Message);
  bool del = true;
  if ( h->is( "Subject" ) ) {
    d->subject.fromUnicodeString( h->asUnicodeString(), h->rfc2047Charset() );
  } else if ( h->is( "Date" ) ) {
    d->date.setDateTime( (static_cast<Headers::Date*>( h))->dateTime() );
  } else {
    del = false;
    Content::setHeader( h );
  }

  if ( del ) delete h;
}

bool Message::removeHeader( const char *type )
{
  Q_D(Message);
  if ( strcasecmp( "Subject", type ) == 0 ) {
    d->subject.clear();
  } else if ( strcasecmp( "Date", type ) == 0 ) {
    d->date.clear();
  } else {
    return Content::removeHeader( type );
  }

  return true;
}

Headers::Subject *Message::subject( bool create )
{
  Q_D( Message );
  if ( !create && d->subject.isEmpty() ) {
    return 0;
  }
  return &d->subject;
}

Headers::Date *Message::date( bool create )
{
  Q_D( Message );
  if ( !create && d->date.isEmpty() ) {
    return 0;
  }
  return &d->date;
}

bool Message::isTopLevel() const
{
  return true;
}

Content *Message::mainBodyPart( const QByteArray &type )
{
  KMime::Content *c = this;
  while ( c ) {
    // not a multipart message
    if ( !c->contentType()->isMultipart() ) {
      if ( c->contentType()->mimeType() == type || type.isEmpty() ) {
        return c;
      }
      return 0;
    }

    // empty multipart
    if ( c->contents().count() == 0 ) {
      return 0;
    }

    // multipart/alternative
    if ( c->contentType()->subType() == "alternative" ) {
      if ( type.isEmpty() ) {
        return c->contents().first();
      }
      foreach ( Content *c1, c->contents() ) {
        if ( c1->contentType()->mimeType() == type ) {
          return c1;
        }
      }
      return 0;
    }

    c = c->contents().first();
  }

  return 0;
}

// @cond PRIVATE
#define kmime_mk_header_accessor( header, method ) \
Headers::header *Message::method( bool create ) { \
  Headers::header *p = 0; \
  return getHeaderInstance( p, create ); \
}

kmime_mk_header_accessor( MessageID, messageID )
kmime_mk_header_accessor( Organization, organization )
kmime_mk_header_accessor( From, from )
kmime_mk_header_accessor( ReplyTo, replyTo )
kmime_mk_header_accessor( To, to )
kmime_mk_header_accessor( Cc, cc )
kmime_mk_header_accessor( Bcc, bcc )
kmime_mk_header_accessor( References, references )
kmime_mk_header_accessor( UserAgent, userAgent )
kmime_mk_header_accessor( InReplyTo, inReplyTo )
kmime_mk_header_accessor( Sender, sender )

#undef kmime_mk_header_accessor
// @endcond

}

