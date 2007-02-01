/*
    This file is part of the kcal library.

    Copyright (c) 2001-2003 Cornelius Schumacher <schumacher@kde.org>

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

#ifndef KCAL_EXCEPTIONS_H
#define KCAL_EXCEPTIONS_H
//
// Exception classes for the kcal library.
//
// We don't use actual C++ exceptions right now. These classes are currently
// returned by an error function, but we can build upon them, if we start
// to use C++ exceptions.

#include <QString>
#include "kcal.h"

namespace KCal {

/**
  KOrganizer exceptions base class. This is currently used as a fancy kind of
  error code not as an C++ exception.
*/
class Exception
{
  public:
    /**
      Construct exception with descriptive message @p message.
    */
    explicit Exception( const QString &message = QString() );
    virtual ~Exception();

    /**
      Return descriptive message of exception.
    */
    virtual QString message();

  protected:
    QString mMessage;

  private:
    class Private;
    Private *d;
};

/**
  Calendar format related error class.
*/
class ErrorFormat : public Exception
{
  public:
    enum ErrorCodeFormat { LoadError, SaveError,
                           ParseErrorIcal, ParseErrorKcal,
                           NoCalendar,
                           CalVersion1,CalVersion2,
                           CalVersionUnknown,
                           Restriction };

    /**
      Create format error exception.
    */
    explicit ErrorFormat( ErrorCodeFormat code,
                          const QString &message = QString() );

    /**
      Return format error message.
    */
    QString message();
    /**
      Return format error code.
    */
    ErrorCodeFormat errorCode();

  private:
    ErrorCodeFormat mCode;

    class Private;
    Private *d;
};

}

#endif
