/*
  This file is part of the kblog library.

  Copyright (c) 2007 Christian Weilbach <christian_weilbach@web.de>

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

#ifndef KBLOG_TEST_GDATA_H_
#define KBLOG_TEST_GDATA_H_

#include <QtCore/QObject>

enum ErrorType {
  XmlRpc,
  Atom,
  ParsingError,
  AuthenticationError,
  NotSupported,
  Other
};

class TestGData : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void testValidity();

};

class TestGDataWarnings : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void userInfoTimeoutWarning();
    void listBlogsTimeoutWarning();
    void listPostingsTimeoutWarning();
    void fetchPostingTimeoutWarning();
    void modifyPostingTimeoutWarning();
    void createPostingTimeoutWarning();
    void error( const ErrorType type, const QString &errStr );
};

#endif

