/*
    kmime_charfreq.h

    KMime, the KDE internet mail/usenet news message library.
    Copyright (c) 2001 the KMime authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, US
*/
#ifndef __KMIME_CHARFREQ_H__
#define __KMIME_CHARFREQ_H__

#include <qcstring.h>
#undef None

namespace KMime {

class CharFreq {
public:
  CharFreq( const QByteArray & buf );
  CharFreq( const char * buf, size_t len );

  enum Type { None = 0, EightBitData, Binary = EightBitData,
	      SevenBitData, EightBitText, SevenBitText };

  Type type() const;
  bool isEightBitData() const;
  bool isEightBitText() const;
  bool isSevenBitData() const;
  bool isSevenBitText() const;
  /** Returns the percentage of printable characters: printable/total.
      If total == 0, the result is undefined. */
  float printableRatio() const;

protected:
  bool lastWasCR;
  uint NUL;       // count of NUL chars
  uint CTL;       // count of CTLs (incl. DEL, excl. CR, LF, HT)
  uint CR, LF;    // count of CRs and LFs
  uint CRLF;      // count of LFs, preceded by CRs
  uint printable; // count of printable US-ASCII chars (SPC..~)
  uint eightBit;  // count of other latin1 chars (those with 8th bit set)
  uint total;
  uint lineMin;
  uint lineMax;

private:
  void count( const char * buf, size_t len );
};

} // namespace KMime

#endif /* __KMIME_CHARFREQ_H__ */