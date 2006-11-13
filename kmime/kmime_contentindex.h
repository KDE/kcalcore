/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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
/**
  @file
  This file is part of the API for handling @ref MIME data and
  defines the ContentIndex class.

  @brief
  Defines the ContentIndex class.

  @authors Volker Krause \<volker.krause@rwth-aachen.de\>

  @glossary @anchor RFC3501 @anchor rfc3501 @b RFC @b 3501:
  RFC that defines the <a href="http://tools.ietf.org/html/rfc3501">
  Internet Message Access Protocol (IMAP)</a>.
*/

#ifndef KMIME_CONTENTINDEX_H
#define KMIME_CONTENTINDEX_H

#include "kmime.h"

#include <QtCore/QList>
#include <QtCore/QString>

namespace KMime {

/**
  @brief
  A class to uniquely identify message parts (Content) in a hierarchy.

  Based on @ref RFC3501 section 6.4.5 and thus compatible with @acronym IMAP.
*/
class KMIME_EXPORT ContentIndex
{
  public:
    /**
      Creates an empty content index.
    */
    ContentIndex();

    /**
      Creates a content index based on the specified string representation.

      @param index is a string representation of a message part index according
      to @ref RFC3501 section 6.4.5.
    */
    explicit ContentIndex( const QString &index );

    /**
      Returns true if this index is non-empty (valid).
    */
    bool isValid() const;

    /**
      Removes and returns the top-most index. Used to recursively
      descend into the message part hierarchy.

      @see push().
    */
    unsigned int pop();

    /**
      Adds @p index to the content index. Used when ascending the message
      part hierarchy.

      @param index is the top-most content index part.

      @see pop().
    */
    void push( unsigned int index );

    /**
      Returns a string representation of this content index according
      to @ref RFC3501 section 6.4.5.
    */
    QString toString() const;

    /**
      Compares this with @p index for equality.

      @param index is the content index to compare.
    */
    bool operator==( const ContentIndex &index ) const;

    /**
      Compares this with @p index for inequality.

      @param index is the content index to compare.
    */
    bool operator!=( const ContentIndex &index ) const;

  private:
    QList<unsigned int> mIndex;
};

}  //namespace KMime

#endif
