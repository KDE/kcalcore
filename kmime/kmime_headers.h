/*  -*- c++ -*-
    kmime_headers.h

    KMime, the KDE internet mail/usenet news message library.
    Copyright (c) 2001-2002 the KMime authors.
    See file AUTHORS for details
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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
#ifndef __KMIME_HEADERS_H__
#define __KMIME_HEADERS_H__

// Content:
//
// - header's base class defining the common interface
// - generic base classes for different types of fields
// - incompatible, Structured-based field classes
// - compatible, Unstructured-based field classes

#include "kmime.h"
#include "kmime_header_parsing.h"

#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QDateTime>
#include <QMap>
#include <QList>
#include <QByteArray>

#include <kdatetime.h>

namespace KMime {

//forward declaration
class Content;

namespace Headers {

enum contentCategory {
  CCsingle,
  CCcontainer,
  CCmixedPart,
  CCalternativePart
};

enum contentEncoding {
  CE7Bit,
  CE8Bit,
  CEquPr,
  CEbase64,
  CEuuenc,
  CEbinary
};

enum contentDisposition {
  CDInvalid,
  CDinline,
  CDattachment,
  CDparallel
};

//often used charset
// TODO: get rid of this!
static const QByteArray Latin1( "ISO-8859-1" );

//@cond PRIVATE
// internal macro to generate default constructors
#define kmime_mk_trivial_ctor( subclass )                               \
  public:                                                               \
  subclass();                                                           \
  subclass( Content *parent );                                          \
  subclass( Content *parent, const QByteArray &s );                     \
  subclass( Content *parent, const QString &s, const QByteArray &charset ); \
  ~subclass();

#define kmime_mk_trivial_ctor_with_name( subclass )     \
  kmime_mk_trivial_ctor( subclass )                     \
    const char *type() const;
//@endcond

//
//
// HEADER'S BASE CLASS. DEFINES THE COMMON INTERFACE
//
//

/** Baseclass of all header-classes. It represents a
    header-field as described in RFC-822.  */
class KMIME_EXPORT Base
{
  public:
    /**
      A list of headers.
    */
    typedef QList<KMime::Headers::Base*> List;

    /**
      Creates an empty header.
    */
    Base();

    /**
      Creates an empty header with a parent-content.
    */
    Base( KMime::Content *parent );

    /**
      Destructor.
    */
    virtual ~Base();

    /**
      Returns the parent of this header.
    */
    KMime::Content *parent() const;

    /**
      Sets the parent for this header to @p parent.
    */
    void setParent( KMime::Content *parent );

    /**
      Parses the given string. Take care of RFC2047-encoded strings.
      @param s The encoded header data.
    */
    virtual void from7BitString( const QByteArray &s ) = 0;

    /**
      Returns the encoded header.
      @param withHeaderType Specifies whether the header-type should be included.
    */
    virtual QByteArray as7BitString( bool withHeaderType = true ) const = 0;

    /**
      Returns the charset that is used for RFC2047-encoding.
    */
    QByteArray rfc2047Charset() const;

    /**
      Sets the charset for RFC2047-encoding.
      @param cs The new charset used for RFC2047 encoding.
    */
    void setRFC2047Charset( const QByteArray &cs );

    /**
      Returns the default charset.
    */
    QByteArray defaultCharset() const;

    /**
      Returns if the default charset is mandatory.
    */
    bool forceDefaultCharset() const;

    /**
      Parses the given string and set the charset.
      @param s The header data as unicode string.
      @param b The charset prefered for encoding.
    */
    virtual void fromUnicodeString( const QString &s, const QByteArray &b ) = 0;

    /**
      Returns the decoded content of the header without the header-type.
    */
    virtual QString asUnicodeString() const = 0;

    /**
      Deletes.
    */
    virtual void clear() = 0;

    /**
      Checks if this header contains any data.
    */
    virtual bool isEmpty() const = 0;

    /**
      Returns the type of this header (e.g. "From").
    */
    virtual const char *type() const
      { return ""; }

    /**
      Checks if this header is of type @p t.
    */
    bool is( const char *t ) const;

    /**
      Checks if this header is a MIME header.
    */
    bool isMimeHeader() const;

    /**
      Checks if this header is a X-Header.
    */
    bool isXHeader() const;

  protected:
    /**
      Helper method, returns the header prefix including ":".
    */
    QByteArray typeIntro() const;

    // TODO: rename and preferably make it private somehow
    QByteArray e_ncCS;

  private:
    KMime::Content *mParent;
};

//
//
// GENERIC BASE CLASSES FOR DIFFERENT TYPES OF FIELDS
//
//

namespace Generics {

/**
  Abstract base class for unstructured header fields
  (e.g. "Subject", "Comment", "Content-description").

  Features: Decodes the header according to RFC2047, incl. RFC2231
  extensions to encoded-words.

  Subclasses need only re-implement @p const @p char* @p type().
*/

// known issues:
// - uses old decodeRFC2047String function, instead of our own...

class KMIME_EXPORT Unstructured : public Base
{
  public:
  Unstructured() : Base() {}
  Unstructured( Content *p ) : Base( p ) {}
  Unstructured( Content *p, const QByteArray &s ) : Base( p )
    { from7BitString( s ); }
  Unstructured( Content *p, const QString &s, const QByteArray &cs ) : Base( p )
    { fromUnicodeString( s, cs ); }
  ~Unstructured() {}

  virtual void from7BitString( const QByteArray &s );
  virtual QByteArray as7BitString( bool withHeaderType=true ) const;

  virtual void fromUnicodeString( const QString &str,
                                  const QByteArray &suggestedCharset );
  virtual QString asUnicodeString() const;

  virtual void clear()
      { d_ecoded.truncate( 0 ); }

  virtual bool isEmpty() const
    { return d_ecoded.isEmpty(); }

  private:
    QString d_ecoded;
};

/**
  @brief
  Base class for structured header fields.

  This is the base class for all structured header fields.
  It contains parsing methods for all basic token types found in rfc2822.

  @section Parsing

  At the basic level, there are tokens & tspecials (rfc2045),
  atoms & specials, quoted-strings, domain-literals (all rfc822) and
  encoded-words (rfc2047).

  As a special token, we have the comment. It is one of the basic
  tokens defined in rfc822, but it's parsing relies in part on the
  basic token parsers (e.g. comments may contain encoded-words).
  Also, most upper-level parsers (notably those for phrase and
  dot-atom) choose to ignore any comment when parsing.

  Then there are the real composite tokens, which are made up of one
  or more of the basic tokens (and semantically invisible comments):
  phrases (rfc822 with rfc2047) and dot-atoms (rfc2822).

  This finishes the list of supported token types. Subclasses will
  provide support for more higher-level tokens, where necessary,
  using these parsers.

  @author Marc Mutz <mutz@kde.org>
*/

class KMIME_EXPORT Structured : public Base
{
  public:
    Structured() : Base() {}
    Structured( Content *p ) : Base( p ) {}
    Structured( Content *p, const QByteArray &s ) : Base( p )
      { from7BitString( s ); }
    Structured( Content *p, const QString &s, const QByteArray &cs ) : Base( p )
      { fromUnicodeString( s, cs ); }
    ~Structured() {}

    virtual void from7BitString( const QByteArray &s );
    virtual QString asUnicodeString() const;
    virtual void fromUnicodeString( const QString &s, const QByteArray &b );

  protected:
    /**
      This method parses the raw header and needs to be implemented in
      every sub-class.

      @param scursor Pointer to the start of the data still to parse.
      @param send Pointer to the end of the data.
      @param isCRLF true if input string is terminated with a CRLF.
    */
    virtual bool parse( const char* &scursor, const char* const send,
                        bool isCRLF = false ) = 0;
};

/**
  Base class for all address related headers.
*/
class KMIME_EXPORT Address : public Structured
{
  public:
    Address() : Structured() {}
    Address( Content *p ) : Structured( p ) {}
    Address( Content *p, const QByteArray &s ) : Structured( p )
      { from7BitString( s ); }
    Address( Content *p, const QString &s, const QByteArray &cs )
      : Structured( p ) { fromUnicodeString( s, cs ); }
    ~Address() {}
};

/**
  Base class for headers that deal with (possibly multiple)
  addresses, but don't allow groups.

  @see RFC 2822, section 3.4
*/
class KMIME_EXPORT MailboxList : public Address
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor( MailboxList )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual void fromUnicodeString( const QString &s, const QByteArray &b );
    virtual QString asUnicodeString() const;

    virtual void clear();
    virtual bool isEmpty() const;

    /**
      Adds an address to this header.

      @param mbox A Mailbox object specifying the address.
    */
    void addAddress( const Types::Mailbox &mbox );

    /**
      Adds an address to this header.
      @param address The actual email address, with or without angle brackets.
      @param displayName An optional name associated with the address.
    */
    void addAddress( const QByteArray &address,
                     const QString &displayName = QString() );

    /**
      Returns a list of all addresses in this header, regardless of groups.
    */
    QList<QByteArray> addresses() const;

    /**
      Returns a list of all display names associated with the addresses in
      this header. An empty entry is added for addresses that do not have
      a display name.
    */
    QStringList displayNames() const;

    /**
      Returns a list of assembled display name / address strings of the following form:
      "Display Name &lt;address&gt;". These are unicode strings without any transport
      encoding, ie. they are only suitable for displaying.
    */
    QStringList prettyAddresses() const;

    /**
      Returns a list of mailboxes listed in this header.
    */
    Types::Mailbox::List mailboxes() const;

  protected:
    bool parse( const char* &scursor, const char * const send, bool isCRLF=false );

    /** The list of mailboxes */
    QList<Types::Mailbox> mMailboxList;
};

/**
   Base class for headers that deal with exactly one mailbox
   (e.g. Sender).
*/
class KMIME_EXPORT SingleMailbox : public MailboxList
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor( SingleMailbox )
  //@endcond
  protected:
    bool parse( const char* &scursor, const char * const send, bool isCRLF=false );
};

/**
  Base class for headers that deal with (possibly multiple)
  addresses, allowing groups.

  Note: Groups are parsed but not represented in the API yet. All addresses in
  groups are listed as if they would not be part of a group.

  @todo Add API for groups?

  @see RFC 2822, section 3.4
*/
class KMIME_EXPORT AddressList : public Address
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor( AddressList )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual void fromUnicodeString( const QString &s, const QByteArray &b );
    virtual QString asUnicodeString() const;

    virtual void clear();
    virtual bool isEmpty() const;

    /**
      Adds an address to this header.

      @param mbox A Mailbox object specifying the address.
    */
    void addAddress( const Types::Mailbox &mbox );

    /**
      Adds an address to this header.
      @param address The actual email address, with or without angle brackets.
      @param displayName An optional name associated with the address.
    */
    void addAddress( const QByteArray &address, const QString &displayName = QString() );

    /**
      Returns a list of all addresses in this header, regardless of groups.
    */
    QList<QByteArray> addresses() const;

    /**
      Returns a list of all display names associated with the addresses in this header.
      An empty entry is added for addresses that don't have a display name.
    */
    QStringList displayNames() const;

    /**
      Returns a list of assembled display name / address strings of the following form:
      "Display Name &lt;address&gt;". These are unicode strings without any transport
      encoding, ie. they are only suitable for displaying.
    */
    QStringList prettyAddresses() const;

    /**
      Returns a list of mailboxes listed in this header.
    */
    Types::Mailbox::List mailboxes() const;

  protected:
    bool parse( const char* & scursor, const char * const send, bool isCRLF=false );

    /** The list of addresses */
    QList<Types::Address> mAddressList;
};

/**
  Base class for headers which deal with a list of msg-id's.

  @see RFC 2822, section 3.6.4
*/
class KMIME_EXPORT Ident : public Address
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor( Ident )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual void clear();
    virtual bool isEmpty() const;

    /**
      Returns the list of identifiers contained in this header.
      Note:
      - Identifiers are not enclosed in angle-brackets.
      - Identifiers are listed in the same order as in the header.
    */
    QList<QByteArray> identifiers() const;

    /**
      Appends a new identifier to this header.
      @param id The identifier to append, with or without angle-brackets.
    */
    void appendIdentifier( const QByteArray &id );

  protected:
    bool parse( const char* & scursor, const char * const send, bool isCRLF=false );

    /** The list of msg-id's */
    QList<Types::AddrSpec> mMsgIdList;
};

/**
  Base class for headers which deal with a single msg-id.

  @see RFC 2822, section 3.6.4
*/
class KMIME_EXPORT SingleIdent : public Ident
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor( SingleIdent )
  //@endcond
  public:
    /**
      Returns the identifier contained in this header.
      Note: The identifiers is not enclosed in angle-brackets.
    */
    QByteArray identifier() const;

    /**
      Sets the identifier.
      @param id The new identifier with or without angle-brackets.
    */
    void setIdentifier( const QByteArray &id );

  protected:
    bool parse( const char* & scursor, const char * const send, bool isCRLF=false );
};

/**
  Base class for headers which deal with a single atom.
*/
class KMIME_EXPORT Token : public Structured
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor( Token )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual void clear();
    virtual bool isEmpty() const;

    /**
      Returns the token.
    */
    QByteArray token() const;

    /**
      Sets the token to @p t,
    */
    void setToken( const QByteArray &t );

  protected:
    bool parse( const char* & scursor, const char * const send, bool isCRLF=false );

  private:
    QByteArray mToken;
};

/**
  Base class for headers containing a list of phrases.
*/
class KMIME_EXPORT PhraseList : public Structured
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor( PhraseList )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual QString asUnicodeString() const;
    virtual void clear();
    virtual bool isEmpty() const;

    /**
      Returns the list of phrases contained in this header.
    */
    QStringList phrases() const;

  protected:
    bool parse( const char* &scursor, const char * const send, bool isCRLF=false );

  private:
    QStringList mPhraseList;
};

/**
  Base class for headers containing a dot atom.
*/
class KMIME_EXPORT DotAtom : public Structured
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor( DotAtom )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual QString asUnicodeString() const;
    virtual void clear();
    virtual bool isEmpty() const;

  protected:
    bool parse( const char* & scursor, const char * const send, bool isCRLF=false );

  private:
    QString mDotAtom;
};

/**
  Base class for headers containing a parameter list such as "Content-Type".
*/
class KMIME_EXPORT Parametrized : public Structured
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor( Parametrized )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;

    virtual bool isEmpty() const;
    virtual void clear();

    /**
      Returns the value of the specified parameter.
      @param key The parameter name.
    */
    QString parameter( const QString &key ) const;

    /**
      Sets the parameter @p key to @p value.
      @param key The parameter name.
      @param value The new value for @p key.
    */
    void setParameter( const QString &key, const QString &value );

  protected:
    virtual bool parse( const char* & scursor, const char * const send, bool isCRLF=false );

  private:
    QMap<QString,QString> mParameterHash;
};

} // namespace Generics

//
//
// INCOMPATIBLE, GSTRUCTURED-BASED FIELDS:
//
//

/**
  Represents the Return-Path header field.

  @see RFC 2822, section 3.6.7
*/
class KMIME_EXPORT ReturnPath : public Generics::Address
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor_with_name( ReturnPath )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual void clear();
    virtual bool isEmpty() const;

  protected:
    bool parse( const char* &scursor, const char * const send, bool isCRLF=false );

  private:
    Types::Mailbox mMailbox;
};

// Address et al.:

// rfc(2)822 headers:
/**
   Represent a "From" header.

   @see RFC 2822, section 3.6.2.
*/
class KMIME_EXPORT From : public Generics::MailboxList
{
  kmime_mk_trivial_ctor_with_name( From )
};

/**
  Represents a "Sender" header.

  @see RFC 2822, section 3.6.2.
*/
class KMIME_EXPORT Sender : public Generics::SingleMailbox
{
  kmime_mk_trivial_ctor_with_name( Sender )
};

/**
  Represents a "To" header.

  @see RFC 2822, section 3.6.3.
*/
class KMIME_EXPORT To : public Generics::AddressList
{
  kmime_mk_trivial_ctor_with_name( To )
};

/**
  Represents a "Cc" header.

  @see RFC 2822, section 3.6.3.
*/
class KMIME_EXPORT Cc : public Generics::AddressList
{
  kmime_mk_trivial_ctor_with_name( Cc )
};

/**
  Represents a "Bcc" header.

  @see RFC 2822, section 3.6.3.
*/
class KMIME_EXPORT Bcc : public Generics::AddressList
{
  kmime_mk_trivial_ctor_with_name( Bcc )
};

/**
  Represents a "ReplyTo" header.

  @see RFC 2822, section 3.6.2.
*/
class KMIME_EXPORT ReplyTo : public Generics::AddressList
{
  kmime_mk_trivial_ctor_with_name( ReplyTo )
};

/**
  Represents a "Mail-Copies-To" header.

  @see http://www.newsreaders.com/misc/mail-copies-to.html
*/
class KMIME_EXPORT MailCopiesTo : public Generics::AddressList
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor_with_name( MailCopiesTo )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual QString asUnicodeString() const;

    virtual void clear();
    virtual bool isEmpty() const;

    /**
      Returns true if a mail copy was explicitly requested.
    */
    bool alwaysCopy() const;

    /**
      Sets the header to "poster".
    */
    void setAlwaysCopy();

    /**
      Returns true if a mail copy was explicitly denied.
    */
    bool neverCopy() const;

    /**
      Sets the header to "never".
    */
    void setNeverCopy();

  protected:
    virtual bool parse( const char* &scursor, const char * const send, bool isCRLF=false );

  private:
    bool mAlwaysCopy;
    bool mNeverCopy;
};

/**
  Represents a "Content-Transfer-Encoding" header.

  @see RFC 2045, section 6.
*/
class KMIME_EXPORT ContentTransferEncoding : public Generics::Token
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor_with_name( ContentTransferEncoding )
  //@endcond
  public:
    virtual void clear();

    /**
      Returns the encoding specified in this header.
    */
    contentEncoding encoding() const;

    /**
      Sets the encoding to @p e.
    */
    void setEncoding( contentEncoding e );

    // TODO: de-inline and document
    bool decoded() const
      { return d_ecoded; }

    void setDecoded( bool d=true )
      { d_ecoded = d; }

    bool needToEncode() const
      { return d_ecoded && (c_te == CEquPr || c_te == CEbase64); }

  protected:
    virtual bool parse( const char* &scursor, const char * const send, bool isCRLF=false );

  private:
    contentEncoding c_te;
    bool d_ecoded;
};

/**
  Represents a "Keywords" header.

  @see RFC 2822, section 3.6.5.
*/
class KMIME_EXPORT Keywords : public Generics::PhraseList
{
  kmime_mk_trivial_ctor_with_name( Keywords )
};

// DotAtom:

/**
  Represents a "MIME-Version" header.

  @see RFC 2045, section 4.
*/
class KMIME_EXPORT MIMEVersion : public Generics::DotAtom
{
  kmime_mk_trivial_ctor_with_name( MIMEVersion )
};

// Ident:

/**
  Represents a "Message-ID" header.

  @see RFC 2822, section 3.6.4.
*/
class KMIME_EXPORT MessageID : public Generics::SingleIdent
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor_with_name( MessageID )
  //@endcond
  public:
    /**
      Generate a message identifer.
      @param fqdn A fully qualified domain name.
    */
    void generate( const QByteArray &fqdn );
};

/**
  Represents a "Content-ID" header.
*/
class KMIME_EXPORT ContentID : public Generics::SingleIdent
{
  kmime_mk_trivial_ctor_with_name( ContentID )
};

/**
  Represents a "Supersedes" header.
*/
class KMIME_EXPORT Supersedes : public Generics::SingleIdent
{
  kmime_mk_trivial_ctor_with_name( Supersedes )
};

/**
  Represents a "In-Reply-To" header.

  @see RFC 2822, section 3.6.4.
*/
class KMIME_EXPORT InReplyTo : public Generics::Ident
{
  kmime_mk_trivial_ctor_with_name( InReplyTo )
};

/**
  Represents a "References" header.

  @see RFC 2822, section 3.6.4.
*/
class KMIME_EXPORT References : public Generics::Ident
{
  kmime_mk_trivial_ctor_with_name( References )
};

/**
  Represents a "Content-Type" header.

  @see RFC 2045, section 5.
*/
class KMIME_EXPORT ContentType : public Generics::Parametrized
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor_with_name( ContentType )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual void clear();
    virtual bool isEmpty() const;

    /**
      Returns the mimetype.
    */
    QByteArray mimeType() const;

    /**
      Returns the media type (first part of the mimetype).
    */

    QByteArray mediaType() const;

    /**
      Returns the mime sub-type (second part of the mimetype).
    */
    QByteArray subType() const;

    /**
      Sets the mimetype and clears already existing parameters.
      @param mimeType The new mimetype.
    */
    void setMimeType( const QByteArray &mimeType );

    /**
      Tests if the media type equals @p mediatype.
    */
    bool isMediatype( const char *mediatype ) const;

    /**
      Tests if the mime sub-type equals @p subtype.
    */
    bool isSubtype( const char *subtype ) const;

    /**
      Returns true if the associated MIME entity is a text.
    */
    bool isText() const;

    /**
      Returns true if the associated MIME entity is a plain text.
    */
    bool isPlainText() const;

    /**
      Returns true if the associated MIME entity is a HTML file.
    */
    bool isHTMLText() const;

    /**
      Returns true if the associated MIME entity is an image.
    */
    bool isImage() const;

    /**
      Returns true if the associated MIME entity is a mulitpart container.
    */
    bool isMultipart() const;

    /**
      Returns true if the associated MIME entity contains partial data.
      @see partialNumber(), partialCount()
    */
    bool isPartial() const;

    /**
      Returns the charset for the associated MIME entity.
    */
    QByteArray charset() const;

    /**
      Sets the charset.
    */
    void setCharset( const QByteArray &s );

    /**
      Returns the boundary (for mulitpart containers).
    */
    QByteArray boundary() const;

    /**
      Sets the mulitpart container boundary.
    */
    void setBoundary( const QByteArray &s );

    /**
      Returns the name of the associated MIME entity.
    */
    QString name() const;

    /**
      Sets the name to @p s using charset @p cs.
    */
    void setName( const QString &s, const QByteArray &cs );

    /**
      Returns the identifier of the associated MIME entity.
    */
    QByteArray id() const;

    /**
      Sets the identifier.
    */
    void setId( const QByteArray &s );

    /**
      Returns the position of this part in a multi-part set.
      @see isPartial(), partialCount()
    */
    int partialNumber() const;

    /**
      Returns the total number of parts in a multi-part set.
      @see isPartial(), partialNumber()
    */
    int partialCount() const;

    /**
      Sets parameters of a partial MIME entity.
      @param total The total number of entities in the multi-part set.
      @param number The number of this entity in a multi-part set.
    */
    void setPartialParams( int total, int number );

    //category
    // TODO: document & de-inline
    contentCategory category() const
      { return c_ategory; }
    void setCategory( contentCategory c )
      { c_ategory=c; }

  protected:
    bool parse( const char* & scursor, const char * const send, bool isCRLF=false );

  private:
    contentCategory c_ategory;
    QByteArray mMimeType;
    QByteArray mMimeSubType;
};

/**
  Represents a "Content-Disposition" header.

  @see RFC 2183
*/
class KMIME_EXPORT ContentDisposition : public Generics::Parametrized
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor_with_name( ContentDisposition )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual bool isEmpty() const;
    virtual void clear();

    /**
      Returns the content disposition.
    */
    contentDisposition disposition() const;

    /**
      Sets the content disposition.
      @param d The new content disposition.
    */
    void setDisposition( contentDisposition d );

    /**
      Returns the suggested filename for the associated MIME part.
      This is just a convenience function, it is equivalent to calling
      parameter( "filename" );
    */
    QString filename() const;

    /**
      Sets the suggested filename for the associated MIME part.
      This is just a convenience function, it is equivalent to calling
      setParameter( "filename", filename );
      @param filename The filename.
    */
    void setFilename( const QString &filename );

  protected:
    bool parse( const char* &scursor, const char * const send, bool isCRLF=false );

  private:
    contentDisposition mDisposition;
};

//
//
// COMPATIBLE GUNSTRUCTURED-BASED FIELDS:
//
//

/**
  Represents an arbitrary header, that can contain any header-field.
  Adds a type over Unstructured.
  @see Unstructured
*/
class KMIME_EXPORT Generic : public Generics::Unstructured
{
  public:
    Generic() : Generics::Unstructured(), t_ype( 0 ) {}

    Generic( const char *t ) : Generics::Unstructured(), t_ype( 0 )
      { setType( t ); }

    Generic( const char *t, Content *p )
      : Generics::Unstructured( p ), t_ype( 0 )
      { setType( t ); }

    Generic( const char *t, Content *p, const QByteArray &s )
      : Generics::Unstructured( p, s ), t_ype( 0 )
      { setType( t ); }

    Generic( const char *t, Content *p, const QString &s, const QByteArray &cs )
      : Generics::Unstructured( p, s, cs ), t_ype( 0 )
      { setType( t ); }

    ~Generic() { delete[] t_ype; }

    virtual void clear()
      { delete[] t_ype; Unstructured::clear(); }

    virtual bool isEmpty() const
      { return t_ype == 0 || Unstructured::isEmpty(); }

    virtual const char *type() const
      { return t_ype; }

    void setType( const char *type );

  private:
    char *t_ype;
};

/**
  Represents a "Subject" header.

  @see RFC 2822, section 3.6.5.
*/
class KMIME_EXPORT Subject : public Generics::Unstructured
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor_with_name( Subject )
  //@endcond
  public:
    bool isReply() const;
};

/**
  Represents a "Organization" header.
*/
class KMIME_EXPORT Organization : public Generics::Unstructured
{
  kmime_mk_trivial_ctor_with_name( Organization )
};

/**
  Represents a "Content-Description" header.
*/
class KMIME_EXPORT ContentDescription : public Generics::Unstructured
{
  kmime_mk_trivial_ctor_with_name( ContentDescription )
};

//
//
// NOT YET CONVERTED STUFF BELOW:
//
//

/**
  Represents a "Control" header.
*/
class KMIME_EXPORT Control : public Base
{
  public:
    Control() : Base() {}
    Control( Content *p ) : Base( p ) {}
    Control( Content *p, const QByteArray &s ) : Base( p )
      { from7BitString( s ); }
    Control( Content *p, const QString &s ) : Base( p )
      { fromUnicodeString( s, Latin1 ); }
    ~Control() {}

    virtual void from7BitString( const QByteArray &s );
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual void fromUnicodeString( const QString &s, const QByteArray &b );
    virtual QString asUnicodeString() const;
    virtual void clear()
      { c_trlMsg.truncate( 0 ); }

    virtual bool isEmpty() const
      { return c_trlMsg.isEmpty(); }

    virtual const char *type() const
      { return "Control"; }

    bool isCancel()
      { return QString::fromLatin1( c_trlMsg ).contains(
        QLatin1String( "cancel" ), Qt::CaseInsensitive ); }

  protected:
    QByteArray c_trlMsg;
};

/**
  Represents a "Date" header.

  @see RFC 2822, section 3.3.
*/
class KMIME_EXPORT Date : public Generics::Structured
{
  //@cond PRIVATE
  kmime_mk_trivial_ctor_with_name( Date )
  //@endcond
  public:
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual void clear();
    virtual bool isEmpty() const;

    /**
      Returns the date contained in this header.
    */
    KDateTime dateTime() const;

    /**
      Sets the date.
    */
    void setDateTime( const KDateTime &dt );

    /**
      Returns the age of the message.
    */
    int ageInDays() const;

  protected:
    bool parse( const char* &scursor, const char * const send, bool isCRLF = false );

  private:
    KDateTime mDateTime;
};

/**
  Represents a "Newsgroups" header.
*/
class KMIME_EXPORT Newsgroups : public Base
{
  public:
    Newsgroups() : Base() {}
    Newsgroups( Content *p ) : Base( p ) {}
    Newsgroups( Content *p, const QByteArray &s ) : Base( p )
      { from7BitString( s ); }
    Newsgroups( Content *p, const QString &s ) : Base( p )
      { fromUnicodeString( s, Latin1 ); }
    ~Newsgroups() {}

    virtual void from7BitString( const QByteArray &s );
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual void fromUnicodeString( const QString &s, const QByteArray &b );
    virtual QString asUnicodeString() const;
    virtual void clear() { g_roups.clear(); }
    virtual bool isEmpty() const { return g_roups.isEmpty(); }
    virtual const char *type() const { return "Newsgroups"; }

    QByteArray firstGroup();
    bool isCrossposted() { return g_roups.contains( ',' ); }
    QStringList getGroups();

  protected:
    QByteArray g_roups;
};

/**
  Represents a "Followup-To" header.
*/
class KMIME_EXPORT FollowUpTo : public Newsgroups
{
  public:
    FollowUpTo() : Newsgroups() {}
    FollowUpTo( Content *p ) : Newsgroups( p ) {}
    FollowUpTo( Content *p, const QByteArray &s ) : Newsgroups( p, s ) {}
    FollowUpTo( Content *p, const QString &s ) : Newsgroups( p, s ) {}
    ~FollowUpTo() {}

    virtual const char *type() const { return "Followup-To"; }
};

/**
  Represents a "Lines" header.
*/
class KMIME_EXPORT Lines : public Base
{
  public:
    Lines() : Base(), l_ines( -1 ) {}
    Lines( Content *p ) : Base( p ), l_ines( -1 ) {}
    Lines( Content *p, unsigned int i ) : Base( p ), l_ines( i ) {}
    Lines( Content *p, const QByteArray &s ) : Base( p )
      { from7BitString( s ); }
    Lines( Content *p, const QString &s ) : Base( p )
      { fromUnicodeString( s, Latin1 ); }
    ~Lines() {}

    virtual void from7BitString( const QByteArray &s );
    virtual QByteArray as7BitString( bool withHeaderType = true ) const;
    virtual void fromUnicodeString( const QString &s, const QByteArray &b );
    virtual QString asUnicodeString() const;
    virtual void clear() { l_ines=-1; }
    virtual bool isEmpty() const { return( l_ines == -1 ); }
    virtual const char *type() const { return "Lines"; }

    int numberOfLines() { return l_ines; }
    void setNumberOfLines( int i ) { l_ines = i; }

  private:
    int l_ines;
};

/**
  Represents a "User-Agent" header.
*/
class KMIME_EXPORT UserAgent : public Generics::Unstructured
{
  kmime_mk_trivial_ctor_with_name( UserAgent )
};

}  //namespace Headers

}  //namespace KMime

// undefine code generation macros again
#undef kmime_mk_trivial_ctor
#undef kmime_mk_trivial_ctor_with_name

#endif // __KMIME_HEADERS_H__
