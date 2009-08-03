/*
    kmime_content.h

    KMime, the KDE internet mail/usenet news message library.
    Copyright (c) 2001 the KMime authors.
    See file AUTHORS for details
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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
/**
  @file
  This file is part of the API for handling @ref MIME data and
  defines the Content class.

  @brief
  Defines the Content class.

  @authors the KMime authors (see AUTHORS file),
  Volker Krause \<vkrause@kde.org\>

TODO: possible glossary terms:
 content
   encoding, transfer type, disposition, description
 header
 body
 attachment
 charset
 article
*/

#ifndef __KMIME_CONTENT_H__
#define __KMIME_CONTENT_H__

#include <QtCore/QTextStream>
#include <QtCore/QByteArray>
#include <QtCore/QList>

#include "kmime_export.h"
#include "kmime_contentindex.h"
#include "kmime_util.h"
#include "kmime_headers.h"

namespace KMime {

class ContentPrivate;

/**
  @brief
  A class that encapsulates @ref MIME encoded Content.

  It parses the given data and creates a tree-like structure that
  represents the structure of the message.
*/
/*
  KDE5:
  * Do not convert singlepart <-> multipart automatically.
  * A bunch of methods probably don't need to be virtual (since they're not needed
    in either Message or NewsArticle).
*/
class KMIME_EXPORT Content
{
  public:

    /**
      Describes a list of Content objects.
    */
    typedef QList<KMime::Content*> List;

    /**
      Creates an empty Content object.
    */
    Content();

    /**
      Creates an empty Content object with a specified parent.
      @param parent the parent Content object
      @since 4.3
    */
    explicit Content( Content* parent ); // KDE5: Merge with the above.

    /**
      Creates a Content object containing the given raw data.

      @param head is a QByteArray containing the header data.
      @param body is a QByteArray containing the body data.
    */
    Content( const QByteArray &head, const QByteArray &body );

    /**
      Creates a Content object containing the given raw data.

      @param head is a QByteArray containing the header data.
      @param body is a QByteArray containing the body data.
      @param parent the parent Content object
      @since 4.3
    */
    // KDE5: Merge with the above.
    Content( const QByteArray &head, const QByteArray &body, Content *parent );

    /**
      Destroys this Content object.
    */
    virtual ~Content();

    /**
      Returns true if this Content object is not empty.
    */
    bool hasContent() const;

    /**
      Sets the Content to the given raw data, containing the Content head and
      body separated by two linefeeds.

      @param l is a line-splitted list of the raw Content data.
    */
    void setContent( const QList<QByteArray> &l );

    /**
      Sets the Content to the given raw data, containing the Content head and
      body separated by two linefeeds.

      @param s is a QByteArray containing the raw Content data.
    */
    void setContent( const QByteArray &s );

    /**
      Parses the Content.
      This means the broken-down object representation of the Content is
      updated from the string representation of the Content.
      Call this if you want to access or change headers or sub-Contents.
    */
    virtual void parse();

    /**
      Returns whether this Content is frozen.
      A frozen content is immutable, i.e. calling assemble() will never modify
      its head or body, and encodedContent() will return the same data before
      and after parsing.

      @since 4.4.
      @see setFrozen().
    */
    bool isFrozen() const;

    /**
      Freezes this Content if @p frozen is true; otherwise unfreezes it.

      @since 4.4
      @see isFrozen().
    */
    void setFrozen( bool frozen = true );

    /**
      Generates the MIME message.
      This means the string representation of this Content is updated from the
      broken-down object representation.
      Call this if you have made changes to the message, and want
      encodedContent() to reflect those changes.

      @note assemble() has no effect if the Content isFrozen().  You may want
      to freeze, for instance, signed sub-Contents, to make sure they are kept
      unmodified.

      @warning assemble() may change the order of the headers, and other
      details such as where folding occurs.  This may break things like
      signature verification, so you should *ONLY* call assemble() when you
      have actually modified the message.
    */
    virtual void assemble();

    /**
      Clears the complete message and deletes all sub-Contents.
    */
    // KDE5: make non-virtual.
    virtual void clear();

    /**
      Removes all sub-Contents from this message.  Deletes them if @p del is true.
      This is different from calling removeContent() on each sub-Content, because
      removeContent() will convert this to a single-part Content if only one
      sub-Content is left.  Calling clearContents() does NOT make this Content
      single-part.

      @param del Whether to delete the sub-Contents.
      @see removeContent()
      @since 4.4
    */
    void clearContents( bool del = true );

    /**
      Returns the Content header raw data.

      @see setHead().
    */
    QByteArray head() const;

    /**
      Sets the Content header raw data.

      @param head is a QByteArray containing the header data.

      @see head().
    */
    void setHead( const QByteArray &head );

    /**
      Extracts and removes the next header from @p head.
      The caller is responsible for deleting the returned header.

      @deprecated Use nextHeader( QByteArray )
      @param head is a QByteArray containing the header data.
    */
    KDE_DEPRECATED Headers::Generic *getNextHeader( QByteArray &head );

    /**
      Extracts and removes the next header from @p head.
      The caller is responsible for deleting the returned header.
      @since 4.2
      @deprecated Use KMime::extractFirstHeader().
      @param head is a QByteArray containing the header data.
    */
    // KDE5: Remove this. This method has nothing to do with *this object.
    KDE_DEPRECATED Headers::Generic *nextHeader( QByteArray &head );

    /**
      Tries to find a @p type header in the message and returns it.
      @deprecated Use headerByType( const char * )
    */
    // KDE5: Make non-virtual.
    KDE_DEPRECATED virtual Headers::Base *getHeaderByType( const char *type );

    /**
      Returns the first header of type @p type, if it exists.  Otherwise returns 0.
      Note that the returned header may be empty.
      @since 4.2
    */
    // KDE5: Make non-virtual.
    virtual Headers::Base *headerByType( const char *type );

    /**
      Returns the first header of type T, if it exists.
      If the header does not exist and @p create is true, creates an empty header
      and returns it. Otherwise returns 0.
      Note that the returned header may be empty.
      @param create Whether to create the header if it does not exist.
      @since 4.4.
    */
    template <typename T> T *header( bool create = false );

    /**
      Tries to find all the @p type headers in the message and returns it.
      Take care that this result is not cached, so could be slow.
      @since 4.2
    */
    virtual QList<Headers::Base*> headersByType( const char *type );

    /**
      Sets the specified header to this Content.
      Any previous header of the same type is removed.
      If you need multiple headers of the same type, use appendHeader() or
      prependHeader().

      @param h The header to set.
      @see appendHeader()
      @see removeHeader()
      @since 4.4
    */
    // KDE5: make non-virtual.
    virtual void setHeader( Headers::Base *h );

    /**
      Appends the specified header to the headers of this Content.
      @param h The header to append.
      @since 4.4
    */
    void appendHeader( Headers::Base *h );

    /**
      Prepends the specified header to the headers of this Content.
      @param h The header to prepend.
      @since 4.4
    */
    void prependHeader( Headers::Base *h );

    /**
      Searches for the first header of type @p type, and deletes it, removing
      it from this Content.
      @param type The type of the header to look for.
      @return true if a header was found and removed.
    */
    // TODO probably provide removeHeader<T>() too.
    // KDE5: make non-virtual.
    virtual bool removeHeader( const char *type );

    /**
      @return true if this Content has a header of type @p type.
      @param type The type of the header to look for.
    */
    // TODO probably provide hasHeader<T>() too.
    bool hasHeader( const char *type );

    /**
      Returns the Content type header.

      @param create if true, create the header if it doesn't exist yet.
    */
    Headers::ContentType *contentType( bool create=true );

    /**
      Returns the Content transfer encoding.

      @param create if true, create the header if it doesn't exist yet.
    */
    Headers::ContentTransferEncoding *contentTransferEncoding( bool create=true );

    /**
      Returns the Content disposition.

      @param create if true, create the header if it doesn't exist yet.
    */
    Headers::ContentDisposition *contentDisposition( bool create=true );

    /**
      Returns the Content description.

      @param create if true, create the header if it doesn't exist yet.
    */
    Headers::ContentDescription *contentDescription( bool create=true );

    /**
      Returns the Content location.

      @param create if true, create the header if it doesn't exist yet.
      @since 4.2
    */
    Headers::ContentLocation *contentLocation( bool create=true );

    /**
      Returns the Content ID.
      @param create If true, create the header if it does not exist yet.
      @since 4.4
    */
    Headers::ContentID *contentID( bool create = true );

    /**
      Returns the size of the Content body after encoding.
      (If the encoding is quoted-printable, this is only an approximate size.)
    */
    int size();

    /**
      Returns the size of this Content and all sub-Contents.
    */
    int storageSize() const;

    /**
      Line count of this Content and all sub-Contents.
    */
    int lineCount() const;

    /**
      Returns the Content body raw data.

      @see setBody().
    */
    QByteArray body() const;

    /**
      Sets the Content body raw data.

      @param body is a QByteArray containing the body data.

      @see body().
    */
    void setBody( const QByteArray &body );

    /**
      Returns a QByteArray containing the encoded Content, including the
      Content header and all sub-Contents.

      @param useCrLf if true, use @ref CRLF instead of @ref LF for linefeeds.
    */
    QByteArray encodedContent( bool useCrLf = false );

    /**
      Returns the decoded Content body.
    */
    QByteArray decodedContent();

    /**
      Returns the decoded text. Additional to decodedContent(), this also
      applies charset decoding. If this is not a text Content, decodedText()
      returns an empty QString.

      @param trimText if true, then the decoded text will have all trailing
      whitespace removed.
      @param removeTrailingNewlines if true, then the decoded text will have
      all consecutive trailing newlines removed.

      The last trailing new line of the decoded text is always removed.

    */
    QString decodedText( bool trimText = false,
                         bool removeTrailingNewlines = false );

    /**
      Sets the Content body to the given string using the current charset.

      @param s Unicode-encoded string.
    */
    void fromUnicodeString( const QString &s );

    /**
      Returns the first Content with mimetype text/.
    */
    Content *textContent();

    /**
      Returns a list of attachments.

      @param incAlternatives if true, include multipart/alternative parts.
    */
    List attachments( bool incAlternatives = false );

    /**
      Returns a list of sub-Contents.
    */
    List contents() const;

    /**
      Adds a new sub-Content. If the sub-Content is already part of another
      Content object, it is removed from there and its parent is updated.
      If the current Content object is single-part, it is converted to
      multipart/mixed first.

      @warning If the single-part to multipart conversion happens, all
      pointers you may have into this object (such as headers) will become
      invalid!

      @param content The new sub-Content.
      @param prepend If true, prepend to the Content list; otherwise append.
      to the Content list.

      @see removeContent().
    */
    // KDE5: Do not convert single-part->multipart automatically.
    void addContent( Content *content, bool prepend = false );

    /**
      Removes the given sub-Content. If only one sub-Content is left, the
      current Content object is converted into a single-part Content.

      @warning If the multipart to single-part conversion happens, the head
      and body of the single remaining sub-Content are copied over, and the
      sub-Content is deleted.  All pointers to it or into it (such as headers)
      will become invalid!

      @param content The Content to remove.
      @param del If true, delete the removed Content object. Otherwise set its
      parent to 0.

      @see addContent().
      @see clearContents().
    */
    // KDE5: Do not convert multipart->single-part automatically.
    void removeContent( Content *content, bool del = false );

    /**
      Changes the encoding of this Content to @p e.  If the Content is binary,
      this actually re-encodes the data to use the new encoding.

      @param e The new encoding to use.
    */
    void changeEncoding( Headers::contentEncoding e );

    /**
      Saves the encoded Content to the given textstream

      @param ts is the stream where the Content should be written to.
      @param scrambleFromLines: if true, replace "\nFrom " with "\n>From "
      in the stream. This is needed to avoid problem with mbox-files
    */
    void toStream( QTextStream &ts, bool scrambleFromLines = false );

    // NOTE: The charset methods below are accessed by the headers which
    // have this Content as a parent.

    /**
      Returns the charset that is used for all headers and the body
      if the charset is not declared explictly.

      @see setDefaultCharset()
    */
    QByteArray defaultCharset() const;

    /**
      Sets the default charset.

      @param cs is a QByteArray containing the new default charset.

      @see defaultCharset().
    */
    void setDefaultCharset( const QByteArray &cs );

    /**
      Use the default charset even if a different charset is
      declared in the article.

      @see setForceDefaultCharset().
    */
    bool forceDefaultCharset() const;

    /**
      Enables/disables the force mode, housekeeping.
      works correctly only when the article is completely empty or
      completely loaded.

      @param b if true, force the default charset to be used.

      @see forceDefaultCharset().
    */
    virtual void setForceDefaultCharset( bool b );

    /**
      Returns the Content specified by the given index.
      If the index does not point to a Content, 0 is returned. If the index
      is invalid (empty), this Content is returned.

      @param index The Content index.
    */
    Content *content( const ContentIndex &index ) const;

    /**
      Returns the ContentIndex for the given Content, or an invalid index
      if the Content is not found within the hierarchy.
      @param content the Content object to search.
    */
    ContentIndex indexForContent( Content *content ) const;

    /**
      Returns true if this is the top-level node in the MIME tree, i.e. if this
      is actually a message or news article.
    */
    virtual bool isTopLevel() const;

    /**
     * Sets a new parent to the Content and add to its contents list. If it already had a parent, it is removed from the
     * old parents contents list.
     * @param parent the new parent
     * @since 4.3
     */
    void setParent( Content *parent );

    /**
     * Returns the parent content object, or NULL if the content doesn't have a parent.
     * @since 4.3
     */
    Content* parent() const;

    /**
     * Returns the toplevel content object, NULL if there is no such object.
     * @since 4.3
     */
    Content* topLevel() const;

    /**
     * Returns the index of this Content based on the topLevel() object.
     * @since 4.3
     */
    ContentIndex index() const;

  protected:
    /**
      Reimplement this method if you need to assemble additional headers in a
      derived class. Don't forget to call the implementation of the base class.
      @return The raw, assembled headers.
    */
    virtual QByteArray assembleHeaders();

    /**
      Returns the raw string representing the header of type @p name.
      @deprecated Use KMime::extractHeader() directly instead.
    */
    KDE_DEPRECATED QByteArray rawHeader( const char *name ) const;

    /**
      Returns a list of raw strings representing all header of type @p name.
      @deprecated Use KMime::extractHeaders() directly instead.
    */
    KDE_DEPRECATED QList<QByteArray> rawHeaders( const char *name ) const;

    /**
      Returns whether this object holds text content.
    */
    // KDE5: Not needed outside. Move to Private class.
    bool decodeText();

    /**
      @deprecated Use header() instead.
    */
    template <class T> KDE_DEPRECATED T *headerInstance( T *ptr, bool create );

    // KDE5: Move to Private class.
    Headers::Base::List h_eaders;

    //@cond PRIVATE
    ContentPrivate *d_ptr;
    explicit Content( ContentPrivate *d );
    //@endcond

  private:
    Q_DECLARE_PRIVATE( Content )
    Q_DISABLE_COPY( Content )
};

// some compilers (for instance Compaq C++) need template inline functions
// here rather than in the *.cpp file

template <class T> T *Content::headerInstance( T *ptr, bool create )
{
  return header<T>( create );
}

template <typename T> T *Content::header( bool create )
{
  T dummy;
  Headers::Base *h = headerByType( dummy.type() );
  if( h ) {
    // Make sure the header is actually of the right type.
    Q_ASSERT( dynamic_cast<T*>( h ) );
  } else if( create ) {
    h = new T( this );
    setHeader( h );
  }
  return static_cast<T*>( h );
}

} // namespace KMime

#endif // __KMIME_CONTENT_H__
