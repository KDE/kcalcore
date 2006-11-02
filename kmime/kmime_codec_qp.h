/*  -*- c++ -*-
    kmime_codec_qp.h

    This file is part of KMime, the KDE internet mail/usenet news message library.
    Copyright (c) 2001-2002 Marc Mutz <mutz@kde.org>

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

#ifndef __KMIME_CODEC_QP__
#define __KMIME_CODEC_QP__

#include "kmime_codecs.h"

namespace KMime {


class KMIME_EXPORT QuotedPrintableCodec : public Codec {
protected:
  friend class Codec;
  QuotedPrintableCodec() : Codec() {}

public:
  virtual ~QuotedPrintableCodec() {}

  const char * name() const {
    return "quoted-printable";
  }

  int maxEncodedSizeFor( int insize, bool withCRLF=false ) const {
    // all chars encoded:
    int result = 3*insize;
    // then after 25 hexchars comes a soft linebreak: =(\r)\n
    result += (withCRLF ? 3 : 2) * (insize/25);

    return result;
  }

  int maxDecodedSizeFor( int insize, bool withCRLF=false ) const;

  Encoder * makeEncoder( bool withCRLF=false ) const;
  Decoder * makeDecoder( bool withCRLF=false ) const;
};


class KMIME_EXPORT Rfc2047QEncodingCodec : public Codec {
protected:
  friend class Codec;
  Rfc2047QEncodingCodec() : Codec() {}

public:
  virtual ~Rfc2047QEncodingCodec() {}

  const char * name() const {
    return "q";
  }

  int maxEncodedSizeFor( int insize, bool withCRLF=false ) const {
    (void)withCRLF; // keep compiler happy
    // this one is simple: We don't do linebreaking, so all that can
    // happen is that every char needs encoding, so:
    return 3*insize;
  }

  int maxDecodedSizeFor( int insize, bool withCRLF=false ) const;

  Encoder * makeEncoder( bool withCRLF=false ) const;
  Decoder * makeDecoder( bool withCRLF=false ) const;
};


class KMIME_EXPORT Rfc2231EncodingCodec : public Codec {
protected:
  friend class Codec;
  Rfc2231EncodingCodec() : Codec() {}

public:
  virtual ~Rfc2231EncodingCodec() {}

  const char * name() const {
    return "x-kmime-rfc2231";
  }

  int maxEncodedSizeFor( int insize, bool withCRLF=false ) const {
    (void)withCRLF; // keep compiler happy
    // same as for "q" encoding:
    return 3*insize;
  }

  int maxDecodedSizeFor( int insize, bool withCRLF=false ) const;

  Encoder * makeEncoder( bool withCRLF=false ) const;
  Decoder * makeDecoder( bool withCRLF=false ) const;
};


} // namespace KMime

#endif // __KMIME_CODEC_QP__
