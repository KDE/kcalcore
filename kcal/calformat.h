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
/**
  @file
  This file is part of the API for handling calendar data and
  defines the CalFormat base class.

  @brief
  Base class providing an interface to various calendar formats.

  @author Cornelius Schumacher \<schumacher@kde.org\>
*/

#ifndef KCAL_CALFORMAT_H
#define KCAL_CALFORMAT_H

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QEvent>

#include "exceptions.h"
#include "event.h"
#include "kcal_export.h"

namespace KCal {

class Calendar;

/**
  This is the base class for calendar formats. It provides an interface for the
  generation/interpretation of a textual representation of a calendar.
*/
class KCAL_EXPORT CalFormat
{
  public:
    /**
      Constructs a new Calendar Format object.
    */
    CalFormat();

    /**
      Destructor.
    */
    virtual ~CalFormat();

    /**
      Loads a calendar on disk into the calendar associated with this format.

      @param calendar is the Calendar to be loaded.
      @param fileName is the name of the disk file containing the Calendar data.

      @return true if successful; false otherwise.
    */
    virtual bool load( Calendar *calendar, const QString &fileName ) = 0;

    /**
      Writes the calendar to disk.

      @param calendar is the Calendar containing the data to be saved.
      @param fileName is the name of the file to write the calendar data.

      @return true if successful; false otherwise.
    */
    virtual bool save( Calendar *calendar, const QString &fileName ) = 0;

    /**
      Loads a calendar from a string

      @param calendar is the Calendar to be loaded.
      @param string is the QString containing the Calendar data.

      @return true if successful; false otherwise.
      @see toString().
    */
    virtual bool fromString( Calendar *calendar, const QString &string ) = 0;

    /**
      Returns the calendar as a string.
      @param calendar is the Calendar containing the data to be saved.

      @return a QString containing the Calendar data if successful;
      an empty string otherwise.
      @see fromString().
    */
    virtual QString toString( Calendar *calendar ) = 0;

    /**
      Clears the exception status.
    */
    void clearException();

    /**
      Returns an exception, if there is any, containing information about the
      last error that occurred.
    */
    ErrorFormat *exception();

    /**
      Sets the application name for use in unique IDs and error messages,
      and product ID for incidence PRODID property

      @param application is a string containing the application name.
      @param productID is a string containing the product identifier.
    */
    static void setApplication( const QString &application,
                                const QString &productID );

    /**
      Returns the application name used in unique IDs and error messages.
    */
    static const QString &application(); //krazy:exclude=constref

    /**
      Returns the PRODID string to write into calendar files.
    */
    static const QString &productId(); //krazy:exclude=constref

    /**
      Returns the PRODID string loaded from calendar file.
    */
    const QString &loadedProductId(); //krazy:exclude=constref

    /**
      Creates a unique id string.
    */
    static QString createUniqueId();

    /**
      Sets an exception that is to be used by the functions of this class
      to report errors.

      @param error is a pointer to an ErrorFormat which contains the exception.
    */
    void setException( ErrorFormat *error );

  protected:
    /** PRODID string loaded from calendar file. */
    QString mLoadedProductId;

  private:
    /** Name of application, for creating unique ID strings. */
    static QString mApplication;
    /** PRODID string to write to calendar files. */
    static QString mProductId;

    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
