/*
  This file is part of the kcal library.

  Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

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
/*
  @file
  This file is part of the API for handling calendar data and
  defines the CalendarNull class.

  @author Cornelius Schumacher
*/
#ifndef KCAL_CALENDARNULL_H
#define KCAL_CALENDARNULL_H

#include "calendar.h"
#include "kcal.h"


namespace KCal {

/**
   @brief
   Represents a null calendar class; that is, a calendar which contains
   no information and provides no capabilities.

   The null calendar can be passed to functions which need a calendar object
   when there is no real calendar available yet.

   CalendarNull can be used to implement the null object design pattern:
   pass a CalendarNull object instead of passing a 0 pointer and checking
   for 0 with each access.
*/
class KCAL_EXPORT CalendarNull : public Calendar
{
  public:
    /**
      Construct Calendar object using a time specification (time zone, etc.).
      The time specification is used for creating or modifying incidences
      in the Calendar. It is also used for viewing incidences (see
      setViewTimeSpec()). The time specification does not alter existing
      incidences.

      @param timeSpec time specification
    */
    explicit CalendarNull( const KDateTime::Spec &timeSpec );

    /**
      Constructs a null calendar with a specified time zone @p timeZoneId.

      @param timeZoneId is a string containing a time zone ID, which is
      assumed to be valid.  If no time zone is found, the viewing time
      specification is set to local clock time.
      @e Example: "Europe/Berlin"
    */
    explicit CalendarNull( const QString &timeZoneId );

    /**
      Destroys the null calendar.
    */
    ~CalendarNull();

    /**
      Returns a pointer to the CalendarNull object, of which there can
      be only one.  The object is constructed if necessary.
    */
    static CalendarNull *self();

    /**
      @copydoc
      Calendar::close()
    */
    void close();

    /**
      @copydoc
      Calendar::save()
    */
    bool save();

    /**
      @copydoc
      Calendar::reload()
    */
    bool reload();

  // Event Specific Methods //

    /**
      @copydoc
      Calendar::addEvent()
    */
    bool addEvent( Event *event );

    /**
      @copydoc
      Calendar::deleteEvent()
    */
    bool deleteEvent( Event *event );

    /**
      @copydoc
      Calendar::deleteAllEvents()
    */
    void deleteAllEvents();

    /**
      @copydoc
      Calendar::rawEvents(EventSortField, SortDirection)
    */
    Event::List rawEvents( EventSortField sortField,
                           SortDirection sortDirection );

    /**
      @copydoc
      Calendar::rawEvents(const QDate &, const QDate &, bool)
    */
    Event::List rawEvents( const QDate &start, const QDate &end,
                           bool inclusive );

    /**
      @copydoc
      Calendar::rawEventsForDate(const QDate &, EventSortField, SortDirection)
    */
    Event::List rawEventsForDate( const QDate &date,
                                  EventSortField sortField=EventSortUnsorted,
                                  SortDirection sortDirection=SortDirectionAscending );

    /**
      @copydoc
      Calendar::rawEventsForDate(const KDateTime &)
    */
    Event::List rawEventsForDate( const KDateTime &dt );

    /**
      @copydoc
      Calendar::event()
    */
    Event *event( const QString &uid );

  // To-do Specific Methods //

    /**
      @copydoc
      Calendar::addTodo()
    */
    bool addTodo( Todo *todo );

    /**
      @copydoc
      Calendar::deleteTodo()
    */
    bool deleteTodo( Todo *todo );

    /**
      @copydoc
      Calendar::deleteAllTodos()
    */
    void deleteAllTodos();

    /**
      @copydoc
      Calendar::rawTodos()
    */
    Todo::List rawTodos( TodoSortField sortField,
                         SortDirection sortDirection );

    /**
      @copydoc
      Calendar::rawTodosForDate()
    */
    Todo::List rawTodosForDate( const QDate &date );

    /**
      @copydoc
      Calendar::todo()
    */
    Todo *todo( const QString &uid );

  // Journal Specific Methods //

    /**
      @copydoc
      Calendar::addJournal()
    */
    bool addJournal( Journal *journal );

    /**
      @copydoc
      Calendar::deleteJournal()
    */
    bool deleteJournal( Journal *journal );

    /**
      @copydoc
      Calendar::deleteAllJournals()
    */
    void deleteAllJournals();

    /**
      @copydoc
      Calendar::rawJournals()
    */
    Journal::List rawJournals( JournalSortField sortField,
                               SortDirection sortDirection );

    /**
      @copydoc
      Calendar::rawJournalsForDate()
    */
    Journal::List rawJournalsForDate( const QDate &date );

    /**
      @copydoc
      Calendar::journal()
    */
    Journal *journal( const QString &uid );

  // Alarm Specific Methods //

    /**
      @copydoc
      Calendar::alarms()
    */
    Alarm::List alarms( const KDateTime &from, const KDateTime &to );

  // Observer Specific Methods //

    /**
      @copydoc
      Calendar::incidenceUpdated()
    */
    void incidenceUpdated( IncidenceBase *incidenceBase );

  private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
