/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "kmime_charfreq_test.h"
#include <qtest_kde.h>

#include <KDebug>

#include <kmime_charfreq.h>
using namespace KMime;

QTEST_KDEMAIN( KMimeCharFreqTest, NoGUI )

void KMimeCharFreqTest::test8bitData()
{
  {
    // If it has NUL then it's Binary (equivalent to EightBitData in CharFreq).
    QByteArray data( "123" );
    data += char( 0 );
    data += "test";
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::Binary );
  }

  {
    // If it has lines longer than 998, it's EightBitData.
    QByteArray data;
    for( int i = 0; i < 999; i++ ) {
      data += char( 169 );
    }
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::EightBitData );
  }

  {
    // If #CR != #CRLF then it's EightBitData.
    QByteArray data( "©line1\r\nline2\r" );
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::EightBitData );
  }

  {
    // If #LF != #CRLF then it's EightBitData.
    QByteArray data( "©line1\r\nline2\n" );
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::EightBitData );
  }

  {
    // If it has a lot of control chars, it's EightBitData.
    QByteArray data( "©test\a\a\a\a\a\a\a" );
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::EightBitData );
  }
}

void KMimeCharFreqTest::test8bitText()
{
  {
    // If it has no NULs, few CTLs, no stray CRs or LFs, it's EightBitText.
    QByteArray data( "©beware the beast but enjoy the feast he offers...\r\n" );
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::EightBitText );
  }
}

void KMimeCharFreqTest::test7bitData()
{
  {
    // If it has lines longer than 998, it's SevenBitData.
    QByteArray data;
    for( int i = 0; i < 999; i++ ) {
      data += 'a';
    }
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::SevenBitData );
  }

  {
    // If #CR != #CRLF then it's SevenBitData.
    QByteArray data( "line1\r\nline2\r" );
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::SevenBitData );
  }

  {
    // If #LF != #CRLF then it's SevenBitData.
    QByteArray data( "line1\r\nline2\n" );
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::SevenBitData );
  }

  {
    // If it has a lot of control chars, it's SevenBitData.
    QByteArray data( "test\a\a\a\a\a\a\a" );
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::SevenBitData );
  }
}

void KMimeCharFreqTest::test7bitText()
{
  {
    // If it has no NULs, few CTLs, no stray CRs or LFs, it's SevenBitText.
    QByteArray data( "beware the beast but enjoy the feast he offers...\r\n" );
    kDebug() << data;
    CharFreq cf( data );
    QCOMPARE( cf.type(), CharFreq::SevenBitText );
  }
}

void KMimeCharFreqTest::testTrailingWhitespace()
{
  QByteArray data( "test " );
  kDebug() << data;
  CharFreq cf( data );
  QVERIFY( cf.hasTrailingWhitespace() );
}

void KMimeCharFreqTest::testLeadingFrom()
{
  QByteArray data( "From here thither" );
  kDebug() << data;
  CharFreq cf( data );
  QVERIFY( cf.hasLeadingFrom() );
}

#include "kmime_charfreq_test.moc"
