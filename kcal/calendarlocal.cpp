/*
    This file is part of the kcal library.

    Copyright (c) 1998 Preston Brown <pbrown@kde.org>
    Copyright (c) 2001,2003,2004 Cornelius Schumacher <schumacher@kde.org>
    Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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

#include <QDateTime>
#include <QHash>
#include <QString>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "incidence.h"
#include "event.h"
#include "todo.h"
#include "journal.h"
#include "filestorage.h"

#include "calendarlocal.h"

using namespace KCal;

/**
  Private class that helps to provide binary compatibility between releases.
  @internal
*/
//@cond PRIVATE
class KCal::CalendarLocal::Private
{
  public:
    QString         mFileName;         // filename where the calendar is stored
    QHash<QString, Event*> mEvents;
    //@TODO should there be a hash of to-dos and journals as well?
    Todo::List      mTodoList;         // a list of all to-dos in the calendar
    Journal::List   mJournalList;      // a list of all journals in the calendar
    Incidence::List mDeletedIncidences;// a list of all deleted incidences
};
//@endcond

CalendarLocal::CalendarLocal( const QString &timeZoneId )
  : Calendar( timeZoneId ), d( new KCal::CalendarLocal::Private )
{
  d->mDeletedIncidences.setAutoDelete( true );
  d->mFileName.clear();
}

CalendarLocal::~CalendarLocal()
{
  close();
  delete d;
}

bool CalendarLocal::load( const QString &fileName, CalFormat *format )
{
  d->mFileName = fileName;
  FileStorage storage( this, fileName, format );
  return storage.load();
}

bool CalendarLocal::reload( const QString &tz )
{
  const QString filename = d->mFileName;
  save();
  close();
  d->mFileName = filename;
  setTimeZoneId( tz );
  FileStorage storage( this, d->mFileName );
  return storage.load();
}

bool CalendarLocal::save( const QString &fileName, CalFormat *format )
{
  // Save only if the calendar is either modified, or saved to a
  // different file than it was loaded from
  if ( d->mFileName != fileName || isModified() ) {
    FileStorage storage( this, fileName, format );
    return storage.save();
  } else {
    return true;
  }
}

void CalendarLocal::close()
{
  setObserversEnabled( false );
  d->mFileName.clear();

  deleteAllEvents();
  deleteAllTodos();
  deleteAllJournals();

  d->mDeletedIncidences.clear();
  setModified( false );

  setObserversEnabled( true );
}

bool CalendarLocal::addEvent( Event *event )
{
  insertEvent( event );

  event->registerObserver( this );

  setModified( true );

  notifyIncidenceAdded( event );

  return true;
}

bool CalendarLocal::deleteEvent( Event *event )
{
  if ( d->mEvents.remove( event->uid() ) ) {
    setModified( true );
    notifyIncidenceDeleted( event );
    d->mDeletedIncidences.append( event );
    return true;
  } else {
    kWarning() << "CalendarLocal::deleteEvent(): Event not found." << endl;
    return false;
  }
}

void CalendarLocal::deleteAllEvents()
{
  foreach ( Event *e, d->mEvents ) {
    notifyIncidenceDeleted( e );
  }

  qDeleteAll( d->mEvents );
  d->mEvents.clear();
}

Event *CalendarLocal::event( const QString &uid )
{
  return d->mEvents.value( uid );
}

bool CalendarLocal::addTodo( Todo *todo )
{
  d->mTodoList.append( todo );

  todo->registerObserver( this );

  // Set up subtask relations
  setupRelations( todo );

  setModified( true );

  notifyIncidenceAdded( todo );

  return true;
}

bool CalendarLocal::deleteTodo( Todo *todo )
{
  // Handle orphaned children
  removeRelations( todo );

  if ( d->mTodoList.removeRef( todo ) ) {
    setModified( true );
    notifyIncidenceDeleted( todo );
    d->mDeletedIncidences.append( todo );
    return true;
  } else {
    kWarning() << "CalendarLocal::deleteTodo(): Todo not found." << endl;
    return false;
  }
}

void CalendarLocal::deleteAllTodos()
{
  foreach ( Todo *t, d->mTodoList ) {
    notifyIncidenceDeleted( t );
  }
  d->mTodoList.setAutoDelete( true );
  d->mTodoList.clear();
  d->mTodoList.setAutoDelete( false );
}

Todo::List CalendarLocal::rawTodos( TodoSortField sortField,
                                    SortDirection sortDirection )
{
  return sortTodos( &d->mTodoList, sortField, sortDirection );
}

Todo *CalendarLocal::todo( const QString &uid )
{
  foreach ( Todo *t, d->mTodoList ) {
    if ( t->uid() == uid ) {
      return t;
    }
  }
  return 0;
}

Todo::List CalendarLocal::rawTodosForDate( const QDate &date )
{
  Todo::List todos;
  foreach ( Todo *t, d->mTodoList ) {
    if ( t->hasDueDate() && t->dtDue().date() == date ) {
      todos.append( t );
    }
  }
  return todos;
}

Alarm::List CalendarLocal::alarmsTo( const QDateTime &to )
{
  return alarms( QDateTime( QDate( 1900, 1, 1 ) ), to );
}

Alarm::List CalendarLocal::alarms( const QDateTime &from, const QDateTime &to )
{
  Alarm::List alarms;
  foreach ( Event *e, d->mEvents ) {
    if ( e->doesRecur() ) {
      appendRecurringAlarms( alarms, e, from, to );
    } else {
      appendAlarms( alarms, e, from, to );
    }
  }

  foreach ( Todo *t, d->mTodoList ) {
    if (! t->isCompleted() ) {
      appendAlarms( alarms, t, from, to );
    }
  }

  return alarms;
}

void CalendarLocal::insertEvent( Event *event )
{
  QString uid = event->uid();
  if ( d->mEvents.value( uid ) == 0 ) {
    d->mEvents.insert( uid, event );
  }
#ifndef NDEBUG
  else {
    // if we already have an event with this UID, it has to be the same event,
    // otherwise something's really broken
    Q_ASSERT( d->mEvents.value( uid ) == event );
  }
#endif
}

Event::List CalendarLocal::rawEventsForDate( const QDate &qd,
                                             EventSortField sortField,
                                             SortDirection sortDirection )
{
  Event::List eventList;

  foreach ( Event *event, d->mEvents ) {

    if ( event->doesRecur() ) {
      if ( event->isMultiDay() ) {
        int extraDays = event->dtStart().date().daysTo( event->dtEnd().date() );
        int i;
        for ( i = 0; i <= extraDays; i++ ) {
          if ( event->recursOn( qd.addDays( -i ) ) ) {
            eventList.append( event );
            break;
          }
        }
      } else {
        if ( event->recursOn( qd ) ) {
          eventList.append( event );
        }
      }
    } else {
      if ( event->dtStart().date() <= qd && event->dateEnd() >= qd ) {
        eventList.append( event );
      }
    }
  }

  return sortEvents( &eventList, sortField, sortDirection );
}

Event::List CalendarLocal::rawEvents( const QDate &start, const QDate &end,
                                          bool inclusive )
{
  Event::List eventList;

  // Get non-recurring events
  foreach ( Event *event, d->mEvents ) {
    if ( event->doesRecur() ) {
      QDate rStart = event->dtStart().date();
      bool found = false;
      if ( inclusive ) {
        if ( rStart >= start && rStart <= end ) {
          // Start date of event is in range. Now check for end date.
          // if duration is negative, event recurs forever, so do not include it.
          if ( event->recurrence()->duration() == 0 ) {  // End date set
            QDate rEnd = event->recurrence()->endDate();
            if ( rEnd >= start && rEnd <= end ) {  // End date within range
              found = true;
            }
          } else if ( event->recurrence()->duration() > 0 ) {  // Duration set
            // TODO: Calculate end date from duration. Should be done in Event
            // For now exclude all events with a duration.
          }
        }
      } else {
        if ( rStart <= end ) {  // Start date not after range
          if ( rStart >= start ) {  // Start date within range
            found = true;
          } else if ( event->recurrence()->duration() == -1 ) {  // Recurs forever
            found = true;
          } else if ( event->recurrence()->duration() == 0 ) {  // End date set
            QDate rEnd = event->recurrence()->endDate();
            if ( rEnd >= start && rEnd <= end ) {  // End date within range
              found = true;
            }
          } else {  // Duration set
            // TODO: Calculate end date from duration. Should be done in Event
            // For now include all events with a duration.
            found = true;
          }
        }
      }

      if ( found ) {
        eventList.append( event );
      }

    } else {
      QDate s = event->dtStart().date();
      QDate e = event->dtEnd().date();

      if ( inclusive ) {
        if ( s >= start && e <= end ) {
          eventList.append( event );
        }
      } else {
        if ( ( s >= start && s <= end ) || ( e >= start && e <= end ) ) {
          eventList.append( event );
        }
      }
    }
  }

  return eventList;
}

Event::List CalendarLocal::rawEventsForDate( const QDateTime &qdt )
{
  return rawEventsForDate( qdt.date() );
}

Event::List CalendarLocal::rawEvents( EventSortField sortField, SortDirection sortDirection )
{
  Event::List eventList;
  foreach ( Event *e, d->mEvents ) {
    eventList.append( e );
  }
  return sortEvents( &eventList, sortField, sortDirection );
}

bool CalendarLocal::addJournal( Journal *journal )
{
  if ( journal->dtStart().isValid() ) {
    kDebug(5800) << "Adding Journal on "
                 << journal->dtStart().toString() << endl;
  } else {
    kDebug(5800) << "Adding Journal without a DTSTART" << endl;
  }

  d->mJournalList.append( journal );

  journal->registerObserver( this );

  setModified( true );

  notifyIncidenceAdded( journal );

  return true;
}

bool CalendarLocal::deleteJournal( Journal *journal )
{
  if ( d->mJournalList.removeRef( journal ) ) {
    setModified( true );
    notifyIncidenceDeleted( journal );
    d->mDeletedIncidences.append( journal );
    return true;
  } else {
    kWarning() << "CalendarLocal::deleteJournal(): Journal not found." << endl;
    return false;
  }
}

void CalendarLocal::deleteAllJournals()
{
  foreach ( Journal *j, d->mJournalList ) {
    notifyIncidenceDeleted( j );
  }
  d->mJournalList.setAutoDelete( true );
  d->mJournalList.clear();
  d->mJournalList.setAutoDelete( false );
}

Journal *CalendarLocal::journal( const QString &uid )
{
  foreach ( Journal *j, d->mJournalList ) {
    if ( j->uid() == uid ) {
      return j;
    }
  }
  return 0;
}

Journal::List CalendarLocal::rawJournals( JournalSortField sortField,
                                          SortDirection sortDirection )
{
  return sortJournals( &d->mJournalList, sortField, sortDirection );
}

Journal::List CalendarLocal::rawJournalsForDate( const QDate &date )
{
  Journal::List journals;
  foreach ( Journal *j, d->mJournalList ) {
    if ( j->dtStart().date() == date ) {
      journals.append( j );
    }
  }
  return journals;
}

void CalendarLocal::setTimeZoneIdViewOnly( const QString &tz )
{
  const QString question( i18n("The time zone setting was changed. "
                               "To display the current calendar in the new "
                               "time zone it must first be saved. Do you want "
                               "to save the pending changes now or wait and "
                               "apply the new time zone on the next reload?" ) );
  int rc = KMessageBox::Yes;
  if ( isModified() ) {
    rc = KMessageBox::questionYesNo( 0, question,
                                     i18n("Save before applying time zones?"),
                                     KStdGuiItem::save(),
                                     KGuiItem(i18n("Apply Time Zone Change on Next Reload")),
                                     "calendarLocalSaveBeforeTimeZoneShift");
  }
  if ( rc == KMessageBox::Yes ) {
    reload( tz );
  }
}
