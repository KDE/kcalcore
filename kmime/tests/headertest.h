/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef KMIME_HEADERTEST_H
#define KMIME_HEADERTEST_H

#include <QtCore/QObject>

class HeaderTest : public QObject
{
  Q_OBJECT
  private slots:
    void testIdentHeader();
    void testAddressListHeader();
    void testMailCopiesToHeader();
    void testParametrizedHeader();
    void testContentDispositionHeader();
    void testContentTypeHeader();
    void testTokenHeader();
    void testContentTransferEncoding();
    void testPhraseListHeader();
    void testDotAtomHeader();
    void testDateHeader();

    // makes sure we don't accidently have an abstract header class that's not
    // meant to be abstract
    void noAbstractHeaders();
};


#endif
