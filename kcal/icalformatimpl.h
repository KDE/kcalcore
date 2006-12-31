/*
    This file is part of the kcal library.

    Copyright (c) 2001-2003 Cornelius Schumacher <schumacher@kde.org>
    Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
    Copyright (c) 2006 David Jarvie <software@astrojar.org.uk>

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
#ifndef KCAL_ICALFORMATIMPL_H
#define KCAL_ICALFORMATIMPL_H

#include <QString>
#include <QList>

#include <kdatetime.h>

#include "scheduler.h"
#include "freebusy.h"


extern "C" {
  #include <ical.h>
}

namespace KCal {

class Compat;
class ICalTimeZones;

/**
  @internal

  This class provides the libical dependent functions for ICalFormat.
*/
class ICalFormatImpl
{
  public:
    /** Create new iCal format for calendar object */
    explicit ICalFormatImpl( ICalFormat *parent );
    virtual ~ICalFormatImpl();

    bool populate( Calendar *, icalcomponent *fs);

    icalcomponent *writeIncidence(IncidenceBase *incidence, Scheduler::Method method = Scheduler::Request);
    icalcomponent *writeTodo(Todo *todo, ICalTimeZones *tzlist = 0, ICalTimeZones *tzUsedList = 0);
    icalcomponent *writeEvent(Event *event, ICalTimeZones *tzlist = 0, ICalTimeZones *tzUsedList = 0);
    icalcomponent *writeFreeBusy(FreeBusy *freebusy,
                                 Scheduler::Method method = Scheduler::Publish );
    icalcomponent *writeJournal(Journal *journal, ICalTimeZones *tzlist = 0, ICalTimeZones *tzUsedList = 0);
    void writeIncidence(icalcomponent *parent, Incidence *incidence, ICalTimeZones *tzlist = 0, ICalTimeZones *tzUsedList = 0);
    icalproperty *writeAttendee(Attendee *attendee);
    icalproperty *writeOrganizer( const Person &organizer );
    icalproperty *writeAttachment(Attachment *attach);
    icalproperty *writeRecurrenceRule(Recurrence *);
    icalrecurrencetype writeRecurrenceRule( RecurrenceRule *recur );
    icalcomponent *writeAlarm(Alarm *alarm);

    QString extractErrorProperty(icalcomponent *);
    Todo *readTodo(icalcomponent *vtodo, ICalTimeZones *tzlist);
    Event *readEvent(icalcomponent *vevent, ICalTimeZones *tzlist);
    FreeBusy *readFreeBusy(icalcomponent *vfreebusy);
    Journal *readJournal(icalcomponent *vjournal, ICalTimeZones *tzlist);
    Attendee *readAttendee(icalproperty *attendee);
    Person readOrganizer( icalproperty *organizer );
    Attachment *readAttachment(icalproperty *attach);
    void readIncidence(icalcomponent *parent, Incidence *incidence, ICalTimeZones *tzlist);
    void readRecurrenceRule(icalproperty *rrule,Incidence *event );
    void readExceptionRule( icalproperty *rrule, Incidence *incidence );
    void readRecurrence( const struct icalrecurrencetype &r, RecurrenceRule* recur );
    void readAlarm(icalcomponent *alarm, Incidence *incidence, ICalTimeZones *tzlist);
    /** Return the PRODID string loaded from calendar file */
    const QString &loadedProductId()  { return mLoadedProductId; }

    static icaltimetype writeICalDate(const QDate &);
    static QDate readICalDate(icaltimetype);
    static icaltimetype writeICalDateTime( const KDateTime & );
    static icaltimetype writeICalUtcDateTime(const KDateTime & );
    /** Create an ical property from a date/time value.
     *  If a time zone is specified for the value, a TZID parameter is inserted into
     *  the ical property, @p tzlist and @p tzUsedList are updated to include the time
     *  zone. Note that while @p tzlist owns any time zone instances in its collection,
     *  @p tzUsedList does not.
     *
     *  @param kind   kind of property
     *  @param dt     date/time value
     *  @param tzlist time zones collection
     *  @param tzUsedList time zones collection, only updated if @p tzlist is also specified
     *  @return property, or null if error. It is the caller's responsibility to free the returned property.
     */
    static icalproperty *writeICalDateTimeProperty( const icalproperty_kind kind, const KDateTime &dt, ICalTimeZones *tzlist = 0, ICalTimeZones *tzUsedList = 0 );

    /** Convert a date/time from ICal format.
     *  If the property @p p specifies a time zone using the TZID parameter, a match is
     *  searched for in @p tzlist. If no match is found, the time zone is added to @p tzlist.
     *
     *  @param p      property from which @p t has been obtained
     *  @param t      ICal format date/time
     *  @param tzlist time zones collection
     *  @param utc    UTC date/time is expected
     *  @return date/time, converted to UTC if @p utc is @c true
     */
    static KDateTime readICalDateTime(icalproperty *p, const icaltimetype &t, ICalTimeZones *tzlist, bool utc = false);

    /** Convert a UTC date/time from ICal format.
     *  If @p t is not a UTC date/time, it is treated as invalid.
     *
     *  @param t ICal format date/time
     *  @return date/time, or invalid if @p t is not UTC
     */
    static KDateTime readICalUtcDateTime( icalproperty *p, icaltimetype &t, ICalTimeZones *tzlist = 0 )
                   { return readICalDateTime( p, t, tzlist, true ); }
    /** Read a date or date/time value from a property.
     *
     *  @param p      ical parameter to read from
     *  @param tzlist time zones collection
     *  @param utc    true to read a UTC value, false to allow time zone to be specified
     *  @return date or date/time, or invalid if property doesn't contain a time value
     */
    static KDateTime readICalDateTimeProperty( icalproperty *p, ICalTimeZones *tzlist, bool utc = false );
    /** Read a UTC date/time value from a property. */
    static KDateTime readICalUtcDateTimeProperty( icalproperty *p )
                   { return readICalDateTimeProperty( p, 0, true ); }
    static icaldurationtype writeICalDuration(int seconds);
    static int readICalDuration(icaldurationtype);
    static icaldatetimeperiodtype writeICalDatePeriod( const QDate &date );

    icalcomponent *createCalendarComponent(Calendar * = 0);
    icalcomponent *createScheduleComponent(IncidenceBase *,Scheduler::Method);

  protected:
    void dumpIcalRecurrence(icalrecurrencetype);
  private:
    void writeIncidenceBase(icalcomponent *parent,IncidenceBase *);
    void readIncidenceBase(icalcomponent *parent, IncidenceBase *);
    void writeCustomProperties(icalcomponent *parent,CustomProperties *);
    void readCustomProperties(icalcomponent *parent,CustomProperties *);
    void readTimezone(icalcomponent *vtimezone);

    ICalFormat *mParent;
    Calendar *mCalendar;

    QString mLoadedProductId;         // PRODID string loaded from calendar file

    Event::List mEventsRelate;           // events with relations
    Todo::List mTodosRelate;             // todos with relations

    static const int mSecondsPerWeek;
    static const int mSecondsPerDay;
    static const int mSecondsPerHour;
    static const int mSecondsPerMinute;

    Compat *mCompat;

    class ToComponentVisitor;
    class Private;
    Private *d;
};

}

#endif
