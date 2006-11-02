/*  -*- c++ -*-
    kmime_codec_uuencode.h

    This file is part of KMime, the KDE internet mail/usenet news message library.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

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

#ifndef __KMIME_CODEC_UUENCODE_H__
#define __KMIME_CODEC_UUENCODE_H__

#include "kmime_codecs.h"

namespace KMime {

class KMIME_EXPORT UUCodec : public Codec {
protected:
  friend class Codec;
  UUCodec() : Codec() {}

public:
  virtual ~UUCodec() {}

  const char * name() const {
    return "x-uuencode";
  }

  int maxEncodedSizeFor( int insize, bool withCRLF=false ) const {
    (void)withCRLF;
    return insize; // we have no encoder!
  }

  int maxDecodedSizeFor( int insize, bool withCRLF=false ) const {
    // assuming all characters are part of the uuencode stream (which
    // does almost never hold due to required linebreaking; but
    // additional non-uu chars don't affect the output size), each
    // 4-tupel of them becomes a 3-tupel in the decoded octet
    // stream. So:
    int result = ( ( insize + 3 ) / 4 ) * 3;
    // but all of them may be \n, so
    if ( withCRLF )
      result *= 2; // :-o

    return result;
  }

  Encoder * makeEncoder( bool withCRLF=false ) const;
  Decoder * makeDecoder( bool withCRLF=false ) const;
};

} // namespace KMime

#endif // __KMIME_CODEC_UUENCODE_H__
