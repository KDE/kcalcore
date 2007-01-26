/*
    This file is part of the kcal library.

    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
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

#include <QString>
#include <QFile>
#include <QList>
#include <QByteArray>
#include <cstdlib>

#include <kdebug.h>
#include <kdatetime.h>
#include <ktzfiletimezone.h>
#include <ksystemtimezone.h>
#include <klocale.h>

extern "C" {
  #include <ical.h>
  #include <icalparser.h>
  #include <icalrestriction.h>
}

#include "calendar.h"
#include "journal.h"
#include "icalformat.h"
#include "icalformatimpl.h"
#include "icaltimezones.h"
#include "compat.h"

#define _ICAL_VERSION "2.0"

using namespace KCal;

/* Static helpers */
/*static void _dumpIcaltime( const icaltimetype& t)
{
  kDebug(5800) << "--- Y: " << t.year << " M: " << t.month << " D: " << t.day
      << endl;
  kDebug(5800) << "--- H: " << t.hour << " M: " << t.minute << " S: " << t.second
      << endl;
  kDebug(5800) << "--- isUtc: " << icaltime_is_utc( t )<< endl;
  kDebug(5800) << "--- zoneId: " << icaltimezone_get_tzid( const_cast<icaltimezone*>( t.zone ) )<< endl;
}*/

const int gSecondsPerMinute = 60;
const int gSecondsPerHour   = gSecondsPerMinute * 60;
const int gSecondsPerDay    = gSecondsPerHour   * 24;
const int gSecondsPerWeek   = gSecondsPerDay    * 7;


inline icaltimetype ICalFormatImpl::writeICalUtcDateTime(const KDateTime &dt )
{
  return writeICalDateTime( dt.toUtc() );
}

ICalFormatImpl::ICalFormatImpl( ICalFormat *parent ) :
  mParent( parent ), mCompat( new Compat )
{
}

ICalFormatImpl::~ICalFormatImpl()
{
  delete mCompat;
}

class KCal::ICalFormatImpl::ToComponentVisitor : public IncidenceBase::Visitor
{
  public:
    ToComponentVisitor( ICalFormatImpl *impl, Scheduler::Method m ) : mImpl( impl ), mComponent( 0 ), mMethod( m ) {}

    bool visit( Event *e ) { mComponent = mImpl->writeEvent( e ); return true; }
    bool visit( Todo *e ) { mComponent = mImpl->writeTodo( e ); return true; }
    bool visit( Journal *e ) { mComponent = mImpl->writeJournal( e ); return true; }
    bool visit( FreeBusy *fb ) { mComponent = mImpl->writeFreeBusy( fb, mMethod ); return true; }

    icalcomponent *component() { return mComponent; }

  private:
    ICalFormatImpl *mImpl;
    icalcomponent *mComponent;
    Scheduler::Method mMethod;
};

icalcomponent *ICalFormatImpl::writeIncidence( IncidenceBase *incidence, Scheduler::Method method )
{
  ToComponentVisitor v( this, method );
  if ( incidence->accept(v) )
    return v.component();
  else return 0;
}

icalcomponent *ICalFormatImpl::writeTodo(Todo *todo, ICalTimeZones *tzlist, ICalTimeZones *tzUsedList)
{
  QString tmpStr;
  QStringList tmpStrList;

  icalcomponent *vtodo = icalcomponent_new(ICAL_VTODO_COMPONENT);

  writeIncidence(vtodo, todo, tzlist, tzUsedList);

  // due date
  icalproperty *prop;
  if (todo->hasDueDate()) {
    icaltimetype due;
    if (todo->floats()) {
      due = writeICalDate(todo->dtDue(true).date());
      prop = icalproperty_new_due(due);
    } else {
      prop = writeICalDateTimeProperty( ICAL_DUE_PROPERTY, todo->dtDue(true), tzlist, tzUsedList );
    }
    icalcomponent_add_property(vtodo, prop);
  }

  // start time
  if ( todo->hasStartDate() || todo->doesRecur() ) {
    icaltimetype start;
    if (todo->floats()) {
//      kDebug(5800) << " Incidence " << todo->summary() << " floats." << endl;
      start = writeICalDate(todo->dtStart(true).date());
      prop = icalproperty_new_dtstart(start);
    } else {
//      kDebug(5800) << " incidence " << todo->summary() << " has time." << endl;
      prop = writeICalDateTimeProperty( ICAL_DTSTART_PROPERTY, todo->dtStart(true), tzlist, tzUsedList );
    }
    icalcomponent_add_property(vtodo, prop);
  }

  // completion date (UTC)
  if (todo->isCompleted()) {
    if (!todo->hasCompletedDate()) {
      // If todo was created by KOrganizer <2.2 it has no correct completion
      // date. Set it to now.
      todo->setCompleted(KDateTime::currentUtcDateTime());
    }
    icaltimetype completed = writeICalUtcDateTime(todo->completed());
    icalcomponent_add_property(vtodo, icalproperty_new_completed(completed));
  }

  icalcomponent_add_property(vtodo,
      icalproperty_new_percentcomplete(todo->percentComplete()));

  if( todo->doesRecur() ) {
    icalcomponent_add_property(vtodo,
        writeICalDateTimeProperty( ICAL_RECURRENCEID_PROPERTY, todo->dtDue(), tzlist, tzUsedList ));
  }

  return vtodo;
}

icalcomponent *ICalFormatImpl::writeEvent(Event *event, ICalTimeZones *tzlist, ICalTimeZones *tzUsedList)
{
#if 0
  kDebug(5800) << "Write Event '" << event->summary() << "' (" << event->uid()
                << ")" << endl;
#endif

  icalcomponent *vevent = icalcomponent_new(ICAL_VEVENT_COMPONENT);

  writeIncidence(vevent, event, tzlist, tzUsedList);

  // start time
  icalproperty *prop;
  icaltimetype start;
  if (event->floats()) {
//    kDebug(5800) << " Incidence " << event->summary() << " floats." << endl;
    start = writeICalDate(event->dtStart().date());
    prop = icalproperty_new_dtstart(start);
  } else {
//    kDebug(5800) << " incidence " << event->summary() << " has time." << endl;
    prop = writeICalDateTimeProperty( ICAL_DTSTART_PROPERTY, event->dtStart(), tzlist, tzUsedList );
  }
  icalcomponent_add_property(vevent, prop);

  if (event->hasEndDate()) {
    // End time.
    // RFC2445 says that if DTEND is present, it has to be greater than DTSTART.
    icaltimetype end;
    KDateTime dt = event->dtEnd();
    if (event->floats()) {
//      kDebug(5800) << " Event " << event->summary() << " floats." << endl;
      // +1 day because end date is non-inclusive.
      end = writeICalDate( dt.date().addDays( 1 ) );
      icalcomponent_add_property( vevent, icalproperty_new_dtend(end) );
    } else {
//      kDebug(5800) << " Event " << event->summary() << " has time." << endl;
      if (dt != event->dtStart()) {
        icalcomponent_add_property( vevent,
                writeICalDateTimeProperty( ICAL_DTEND_PROPERTY, dt, tzlist, tzUsedList ) );
      }
    }
  }

// TODO: resources
#if 0
  // resources
  QStringList tmpStrList = anEvent->resources();
  QString tmpStr = tmpStrList.join(";");
  if (!tmpStr.isEmpty())
    addPropValue(vevent, VCResourcesProp, tmpStr.toUtf8());

#endif

  // Transparency
  switch( event->transparency() ) {
  case Event::Transparent:
    icalcomponent_add_property(
      vevent,
      icalproperty_new_transp( ICAL_TRANSP_TRANSPARENT ) );
    break;
  case Event::Opaque:
    icalcomponent_add_property(
      vevent,
      icalproperty_new_transp( ICAL_TRANSP_OPAQUE ) );
    break;
  }

  return vevent;
}

icalcomponent *ICalFormatImpl::writeFreeBusy(FreeBusy *freebusy,
                                             Scheduler::Method method)
{
#if QT_VERSION >= 300
  kDebug(5800) << "icalformatimpl: writeFreeBusy: startDate: "
    << freebusy->dtStart().toString("ddd MMMM d yyyy: h:m:s ap") << " End Date: "
    << freebusy->dtEnd().toString("ddd MMMM d yyyy: h:m:s ap") << endl;
#endif

  icalcomponent *vfreebusy = icalcomponent_new(ICAL_VFREEBUSY_COMPONENT);

  writeIncidenceBase(vfreebusy,freebusy);

  icalcomponent_add_property(vfreebusy, icalproperty_new_dtstart(
      writeICalUtcDateTime(freebusy->dtStart())));

  icalcomponent_add_property(vfreebusy, icalproperty_new_dtend(
      writeICalUtcDateTime(freebusy->dtEnd())));

  if (method == Scheduler::Request) {
    icalcomponent_add_property(vfreebusy,icalproperty_new_uid(
       freebusy->uid().toUtf8()));
  }

  //Loops through all the periods in the freebusy object
  QList<Period> list = freebusy->busyPeriods();
  QList<Period>::Iterator it;
  icalperiodtype period;
  for (it = list.begin(); it!= list.end(); ++it) {
    period.start = writeICalUtcDateTime((*it).start());
    if ( (*it).hasDuration() ) {
      period.duration = writeICalDuration( (*it).duration().asSeconds() );
    } else {
      period.end = writeICalUtcDateTime((*it).end());
    }
    icalcomponent_add_property(vfreebusy, icalproperty_new_freebusy(period) );
  }

  return vfreebusy;
}

icalcomponent *ICalFormatImpl::writeJournal(Journal *journal, ICalTimeZones *tzlist, ICalTimeZones *tzUsedList)
{
  icalcomponent *vjournal = icalcomponent_new(ICAL_VJOURNAL_COMPONENT);

  writeIncidence(vjournal, journal, tzlist, tzUsedList);

  // start time
  icalproperty *prop;
  KDateTime dt = journal->dtStart();
  if (dt.isValid()) {
    icaltimetype start;
    if (journal->floats()) {
//      kDebug(5800) << " Incidence " << event->summary() << " floats." << endl;
      start = writeICalDate(dt.date());
      prop = icalproperty_new_dtstart(start);
    } else {
//      kDebug(5800) << " incidence " << event->summary() << " has time." << endl;
      prop = writeICalDateTimeProperty( ICAL_DTSTART_PROPERTY, dt, tzlist, tzUsedList );
    }
    icalcomponent_add_property(vjournal, prop);
  }

  return vjournal;
}

void ICalFormatImpl::writeIncidence(icalcomponent *parent, Incidence *incidence, ICalTimeZones *tzlist, ICalTimeZones *tzUsedList)
{
  if ( incidence->schedulingID() != incidence->uid() )
    // We need to store the UID in here. The rawSchedulingID will
    // go into the iCal UID component
    incidence->setCustomProperty( "LIBKCAL", "ID", incidence->uid() );
  else
    incidence->removeCustomProperty( "LIBKCAL", "ID" );

  // pilot sync stuff
// TODO: move this application-specific code to kpilot
  if (incidence->pilotId()) {
    incidence->setNonKDECustomProperty("X-PILOTID", QString::number(incidence->pilotId()));
    incidence->setNonKDECustomProperty("X-PILOTSTAT", QString::number(incidence->syncStatus()));
  }

  writeIncidenceBase(parent, incidence);

  // creation date
  icalcomponent_add_property( parent,
      writeICalDateTimeProperty( ICAL_CREATED_PROPERTY, incidence->created() ) );

  // unique id
  // If the scheduling ID is different from the real UID, the real
  // one is stored on X-REALID above
  if ( !incidence->schedulingID().isEmpty() ) {
    icalcomponent_add_property(parent,icalproperty_new_uid(
        incidence->schedulingID().utf8()));
  }

  // revision
  if ( incidence->revision() > 0 ) { // 0 is default, so don't write that out
    icalcomponent_add_property(parent,icalproperty_new_sequence(
        incidence->revision()));
  }

  // last modification date
  if ( incidence->lastModified().isValid() ) {
    icalcomponent_add_property( parent,
        writeICalDateTimeProperty( ICAL_LASTMODIFIED_PROPERTY, incidence->lastModified() ) );
  }
  
  // description
  if (!incidence->description().isEmpty()) {
    icalcomponent_add_property(parent, icalproperty_new_description(
        incidence->description().toUtf8()));
  }

  // summary
  if (!incidence->summary().isEmpty()) {
    icalcomponent_add_property(parent, icalproperty_new_summary(
        incidence->summary().toUtf8()));
  }

  // location
  if (!incidence->location().isEmpty()) {
    icalcomponent_add_property(parent, icalproperty_new_location(
        incidence->location().toUtf8()));
  }

  // status
  icalproperty_status status = ICAL_STATUS_NONE;
  switch (incidence->status()) {
    case Incidence::StatusTentative:    status = ICAL_STATUS_TENTATIVE;  break;
    case Incidence::StatusConfirmed:    status = ICAL_STATUS_CONFIRMED;  break;
    case Incidence::StatusCompleted:    status = ICAL_STATUS_COMPLETED;  break;
    case Incidence::StatusNeedsAction:  status = ICAL_STATUS_NEEDSACTION;  break;
    case Incidence::StatusCanceled:     status = ICAL_STATUS_CANCELLED;  break;
    case Incidence::StatusInProcess:    status = ICAL_STATUS_INPROCESS;  break;
    case Incidence::StatusDraft:        status = ICAL_STATUS_DRAFT;  break;
    case Incidence::StatusFinal:        status = ICAL_STATUS_FINAL;  break;
    case Incidence::StatusX: {
      icalproperty* p = icalproperty_new_status(ICAL_STATUS_X);
      icalvalue_set_x(icalproperty_get_value(p), incidence->statusStr().toUtf8());
      icalcomponent_add_property(parent, p);
      break;
    }
    case Incidence::StatusNone:
    default:
      break;
  }
  if (status != ICAL_STATUS_NONE)
    icalcomponent_add_property(parent, icalproperty_new_status(status));

  // secrecy
  icalproperty_class secClass;
  switch (incidence->secrecy()) {
    case Incidence::SecrecyPublic:
      secClass = ICAL_CLASS_PUBLIC;
      break;
    case Incidence::SecrecyConfidential:
      secClass = ICAL_CLASS_CONFIDENTIAL;
      break;
    case Incidence::SecrecyPrivate:
    default:
      secClass = ICAL_CLASS_PRIVATE;
      break;
  }
  if ( secClass != ICAL_CLASS_PUBLIC ) {
    icalcomponent_add_property(parent,icalproperty_new_class(secClass));
  }

  // priority
  if ( incidence->priority() > 0 ) { // 0 is undefined priority
    icalcomponent_add_property(parent,icalproperty_new_priority(
        incidence->priority()));
  }

  // categories
  QStringList categories = incidence->categories();
  QStringList::Iterator it;
  for(it = categories.begin(); it != categories.end(); ++it ) {
    icalcomponent_add_property(parent, icalproperty_new_categories((*it).toUtf8()));
  }

  // related event
  if ( !incidence->relatedToUid().isEmpty() ) {
    icalcomponent_add_property(parent, icalproperty_new_relatedto(
        incidence->relatedToUid().toUtf8()));
  }

//   kdDebug(5800) << "Write recurrence for '" << incidence->summary() << "' (" << incidence->uid()
//             << ")" << endl;

  RecurrenceRule::List rrules( incidence->recurrence()->rRules() );
  RecurrenceRule::List::ConstIterator rit;
  for ( rit = rrules.begin(); rit != rrules.end(); ++rit ) {
    icalcomponent_add_property( parent, icalproperty_new_rrule(
                                writeRecurrenceRule( (*rit) ) ) );
  }

  RecurrenceRule::List exrules( incidence->recurrence()->exRules() );
  RecurrenceRule::List::ConstIterator exit;
  for ( exit = exrules.begin(); exit != exrules.end(); ++exit ) {
    icalcomponent_add_property( parent, icalproperty_new_rrule(
                                writeRecurrenceRule( (*exit) ) ) );
  }

  DateList dateList = incidence->recurrence()->exDates();
  DateList::ConstIterator exIt;
  for(exIt = dateList.begin(); exIt != dateList.end(); ++exIt) {
    icalcomponent_add_property(parent, icalproperty_new_exdate(
        writeICalDate(*exIt)));
  }
  DateTimeList dateTimeList = incidence->recurrence()->exDateTimes();
  DateTimeList::ConstIterator extIt;
  for(extIt = dateTimeList.begin(); extIt != dateTimeList.end(); ++extIt) {
    icalcomponent_add_property(parent,
        writeICalDateTimeProperty( ICAL_EXDATE_PROPERTY, *extIt, tzlist, tzUsedList ));
  }


  dateList = incidence->recurrence()->rDates();
  DateList::ConstIterator rdIt;
  for( rdIt = dateList.begin(); rdIt != dateList.end(); ++rdIt) {
     icalcomponent_add_property( parent, icalproperty_new_rdate(
         writeICalDatePeriod(*rdIt) ) );
  }
  dateTimeList = incidence->recurrence()->rDateTimes();
  DateTimeList::ConstIterator rdtIt;
  for( rdtIt = dateTimeList.begin(); rdtIt != dateTimeList.end(); ++rdtIt) {
    icalcomponent_add_property( parent,
         writeICalDateTimeProperty( ICAL_RDATE_PROPERTY, *rdtIt, tzlist, tzUsedList ));
  }

  // attachments
  Attachment::List attachments = incidence->attachments();
  Attachment::List::ConstIterator atIt;
  for ( atIt = attachments.begin(); atIt != attachments.end(); ++atIt ) {
    icalcomponent_add_property( parent, writeAttachment( *atIt ) );
  }

  // alarms
  Alarm::List::ConstIterator alarmIt;
  for ( alarmIt = incidence->alarms().begin();
        alarmIt != incidence->alarms().end(); ++alarmIt ) {
    if ( (*alarmIt)->enabled() ) {
//      kDebug(5800) << "Write alarm for " << incidence->summary() << endl;
      icalcomponent_add_component( parent, writeAlarm( *alarmIt ) );
    }
  }

  // duration
  if (incidence->hasDuration()) {
    icaldurationtype duration;
    duration = writeICalDuration( incidence->duration() );
    icalcomponent_add_property(parent, icalproperty_new_duration(duration));
  }
}

void ICalFormatImpl::writeIncidenceBase( icalcomponent *parent,
                                         IncidenceBase * incidenceBase )
{
  icalcomponent_add_property( parent,
      writeICalDateTimeProperty( ICAL_DTSTAMP_PROPERTY, KDateTime::currentUtcDateTime() ) );

  // organizer stuff
  if ( !incidenceBase->organizer().isEmpty() ) {
    icalcomponent_add_property( parent, writeOrganizer( incidenceBase->organizer() ) );
  }

  // attendees
  if ( incidenceBase->attendeeCount() > 0 ) {
    Attendee::List::ConstIterator it;
    for( it = incidenceBase->attendees().begin();
         it != incidenceBase->attendees().end(); ++it ) {
      icalcomponent_add_property( parent, writeAttendee( *it ) );
    }
  }

  // comments
  QStringList comments = incidenceBase->comments();
  for (QStringList::Iterator it=comments.begin(); it!=comments.end(); ++it) {
    icalcomponent_add_property(parent, icalproperty_new_comment((*it).toUtf8()));
  }

  // custom properties
  writeCustomProperties( parent, incidenceBase );
}

void ICalFormatImpl::writeCustomProperties(icalcomponent *parent,CustomProperties *properties)
{
  QMap<QByteArray, QString> custom = properties->customProperties();
  for (QMap<QByteArray, QString>::Iterator c = custom.begin();  c != custom.end();  ++c) {
    icalproperty *p = icalproperty_new_x(c.value().toUtf8());
    icalproperty_set_x_name(p,c.key());
    icalcomponent_add_property(parent,p);
  }
}

icalproperty *ICalFormatImpl::writeOrganizer( const Person &organizer )
{
  icalproperty *p = icalproperty_new_organizer("MAILTO:" + organizer.email().toUtf8());

  if (!organizer.name().isEmpty()) {
    icalproperty_add_parameter( p, icalparameter_new_cn(organizer.name().toUtf8()) );
  }
  // TODO: Write dir, sent-by and language

  return p;
}


icalproperty *ICalFormatImpl::writeAttendee(Attendee *attendee)
{
  icalproperty *p = icalproperty_new_attendee("mailto:" + attendee->email().toUtf8());

  if (!attendee->name().isEmpty()) {
    icalproperty_add_parameter(p, icalparameter_new_cn(attendee->name().toUtf8()));
  }


  icalproperty_add_parameter(p,icalparameter_new_rsvp(
          attendee->RSVP() ? ICAL_RSVP_TRUE : ICAL_RSVP_FALSE ));

  icalparameter_partstat status = ICAL_PARTSTAT_NEEDSACTION;
  switch (attendee->status()) {
    default:
    case Attendee::NeedsAction:
      status = ICAL_PARTSTAT_NEEDSACTION;
      break;
    case Attendee::Accepted:
      status = ICAL_PARTSTAT_ACCEPTED;
      break;
    case Attendee::Declined:
      status = ICAL_PARTSTAT_DECLINED;
      break;
    case Attendee::Tentative:
      status = ICAL_PARTSTAT_TENTATIVE;
      break;
    case Attendee::Delegated:
      status = ICAL_PARTSTAT_DELEGATED;
      break;
    case Attendee::Completed:
      status = ICAL_PARTSTAT_COMPLETED;
      break;
    case Attendee::InProcess:
      status = ICAL_PARTSTAT_INPROCESS;
      break;
  }
  icalproperty_add_parameter(p, icalparameter_new_partstat(status));

  icalparameter_role role = ICAL_ROLE_REQPARTICIPANT;
  switch (attendee->role()) {
    case Attendee::Chair:
      role = ICAL_ROLE_CHAIR;
      break;
    default:
    case Attendee::ReqParticipant:
      role = ICAL_ROLE_REQPARTICIPANT;
      break;
    case Attendee::OptParticipant:
      role = ICAL_ROLE_OPTPARTICIPANT;
      break;
    case Attendee::NonParticipant:
      role = ICAL_ROLE_NONPARTICIPANT;
      break;
  }
  icalproperty_add_parameter(p, icalparameter_new_role(role));

  if (!attendee->uid().isEmpty()) {
    icalparameter* icalparameter_uid = icalparameter_new_x(attendee->uid().toUtf8());
    icalparameter_set_xname(icalparameter_uid,"X-UID");
    icalproperty_add_parameter(p, icalparameter_uid);
  }

  return p;
}

icalproperty *ICalFormatImpl::writeAttachment(Attachment *att)
{
  icalattach *attach;
  if (att->isUri())
      attach = icalattach_new_from_url( att->uri().toUtf8().data());
  else
      attach = icalattach_new_from_data ( (unsigned char *)att->data(), 0, 0);
  icalproperty *p = icalproperty_new_attach(attach);

  if ( !att->mimeType().isEmpty() ) {
    icalproperty_add_parameter( p,
        icalparameter_new_fmttype( att->mimeType().toUtf8().data() ) );
  }

  if ( att->isBinary() ) {
    icalproperty_add_parameter( p,
        icalparameter_new_value( ICAL_VALUE_BINARY ) );
    icalproperty_add_parameter( p,
        icalparameter_new_encoding( ICAL_ENCODING_BASE64 ) );
  }

  if ( att->showInline() ) {
    icalparameter* icalparameter_inline = icalparameter_new_x( "inline" );
    icalparameter_set_xname( icalparameter_inline, "X-CONTENT-DISPOSITION" );
    icalproperty_add_parameter( p, icalparameter_inline );
  }

  if ( !att->label().isEmpty() ) {
    icalparameter* icalparameter_label = icalparameter_new_x( att->label().toUtf8() );
    icalparameter_set_xname( icalparameter_label, "X-LABEL" );
    icalproperty_add_parameter( p, icalparameter_label );
  }

  if ( att->isLocal() ) {
    icalparameter* icalparameter_inline = icalparameter_new_x( "local" );
    icalparameter_set_xname( icalparameter_inline, "X-KONTACT-TYPE" );
    icalproperty_add_parameter( p, icalparameter_inline );
  }

  return p;
}

icalrecurrencetype ICalFormatImpl::writeRecurrenceRule( RecurrenceRule *recur )
{
//  kDebug(5800) << "ICalFormatImpl::writeRecurrenceRule()" << endl;

  icalrecurrencetype r;
  icalrecurrencetype_clear(&r);

  switch( recur->recurrenceType() ) {
    case RecurrenceRule::rSecondly:
      r.freq = ICAL_SECONDLY_RECURRENCE;
      break;
    case RecurrenceRule::rMinutely:
      r.freq = ICAL_MINUTELY_RECURRENCE;
      break;
    case RecurrenceRule::rHourly:
      r.freq = ICAL_HOURLY_RECURRENCE;
      break;
    case RecurrenceRule::rDaily:
      r.freq = ICAL_DAILY_RECURRENCE;
      break;
    case RecurrenceRule::rWeekly:
      r.freq = ICAL_WEEKLY_RECURRENCE;
      break;
    case RecurrenceRule::rMonthly:
      r.freq = ICAL_MONTHLY_RECURRENCE;
      break;
    case RecurrenceRule::rYearly:
      r.freq = ICAL_YEARLY_RECURRENCE;
      break;
    default:
      r.freq = ICAL_NO_RECURRENCE;
      kDebug(5800) << "ICalFormatImpl::writeRecurrence(): no recurrence" << endl;
      break;
  }

  int index = 0;
  QList<int> bys;
  QList<int>::ConstIterator it;

  // Now write out the BY* parts:
  bys = recur->bySeconds();
  index = 0;
  for ( it = bys.begin(); it != bys.end(); ++it ) {
    r.by_second[index++] = *it;
  }

  bys = recur->byMinutes();
  index = 0;
  for ( it = bys.begin(); it != bys.end(); ++it ) {
    r.by_minute[index++] = *it;
  }

  bys = recur->byHours();
  index = 0;
  for ( it = bys.begin(); it != bys.end(); ++it ) {
    r.by_hour[index++] = *it;
  }

  bys = recur->byMonthDays();
  index = 0;
  for ( it = bys.begin(); it != bys.end(); ++it ) {
    r.by_month_day[index++] = icalrecurrencetype_day_position( (*it) * 8 );
  }

  bys = recur->byYearDays();
  index = 0;
  for ( it = bys.begin(); it != bys.end(); ++it ) {
    r.by_year_day[index++] = *it;
  }

  bys = recur->byWeekNumbers();
  index = 0;
  for ( it = bys.begin(); it != bys.end(); ++it ) {
     r.by_week_no[index++] = *it;
  }

  bys = recur->byMonths();
  index = 0;
  for ( it = bys.begin(); it != bys.end(); ++it ) {
    r.by_month[index++] = *it;
  }

  bys = recur->bySetPos();
  index = 0;
  for ( it = bys.begin(); it != bys.end(); ++it ) {
     r.by_set_pos[index++] = *it;
  }


  QList<RecurrenceRule::WDayPos> byd = recur->byDays();
  int day;
  index = 0;
  for ( QList<RecurrenceRule::WDayPos>::ConstIterator dit = byd.begin();
        dit != byd.end(); ++dit ) {
    day = (*dit).day() % 7 + 1;     // convert from Monday=1 to Sunday=1
    if ( (*dit).pos() < 0 ) {
      day += (-(*dit).pos())*8;
      day = -day;
    } else {
      day += (*dit).pos()*8;
    }
    r.by_day[index++] = day;
  }

  r.week_start = static_cast<icalrecurrencetype_weekday>(
                                             recur->weekStart()%7 + 1);

  if ( recur->frequency() > 1 ) {
    // Dont' write out INTERVAL=1, because that's the default anyway
    r.interval = recur->frequency();
  }

  if ( recur->duration() > 0 ) {
    r.count = recur->duration();
  } else if ( recur->duration() == -1 ) {
    r.count = 0;
  } else {
    if ( recur->doesFloat() )
      r.until = writeICalDate(recur->endDt().date());
    else
      r.until = writeICalUtcDateTime(recur->endDt());
  }

// Debug output
#if 0
  const char *str = icalrecurrencetype_as_string(&r);
  if (str) {
    kDebug(5800) << " String: " << str << endl;
  } else {
    kDebug(5800) << " No String" << endl;
  }
#endif

  return r;
}


icalcomponent *ICalFormatImpl::writeAlarm(Alarm *alarm)
{
//kDebug(5800) << " ICalFormatImpl::writeAlarm" << endl;
  icalcomponent *a = icalcomponent_new(ICAL_VALARM_COMPONENT);

  icalproperty_action action;
  icalattach *attach = 0;

  switch (alarm->type()) {
    case Alarm::Procedure:
      action = ICAL_ACTION_PROCEDURE;
      attach = icalattach_new_from_url(QFile::encodeName(alarm->programFile()).data());
      icalcomponent_add_property(a, icalproperty_new_attach(attach));
      if (!alarm->programArguments().isEmpty()) {
        icalcomponent_add_property(a, icalproperty_new_description(alarm->programArguments().toUtf8()));
      }
      break;
    case Alarm::Audio:
      action = ICAL_ACTION_AUDIO;
//kDebug(5800) << " It's an audio action, file: " << alarm->audioFile() << endl;
      if (!alarm->audioFile().isEmpty()) {
        attach = icalattach_new_from_url(QFile::encodeName( alarm->audioFile() ).data());
        icalcomponent_add_property(a, icalproperty_new_attach(attach));
      }
      break;
    case Alarm::Email: {
      action = ICAL_ACTION_EMAIL;
      QList<Person> addresses = alarm->mailAddresses();
      for (QList<Person>::Iterator ad = addresses.begin();  ad != addresses.end();  ++ad) {
        icalproperty *p = icalproperty_new_attendee("MAILTO:" + (*ad).email().toUtf8());
        if (!(*ad).name().isEmpty()) {
          icalproperty_add_parameter(p, icalparameter_new_cn((*ad).name().toUtf8()));
        }
        icalcomponent_add_property(a, p);
      }
      icalcomponent_add_property(a, icalproperty_new_summary(alarm->mailSubject().toUtf8()));
      icalcomponent_add_property(a, icalproperty_new_description(alarm->mailText().toUtf8()));
      QStringList attachments = alarm->mailAttachments();
      if (attachments.count() > 0) {
        for (QStringList::Iterator at = attachments.begin();  at != attachments.end();  ++at) {
          attach = icalattach_new_from_url(QFile::encodeName( *at ).data());
          icalcomponent_add_property(a, icalproperty_new_attach(attach));
        }
      }
      break;
    }
    case Alarm::Display:
      action = ICAL_ACTION_DISPLAY;
      icalcomponent_add_property(a, icalproperty_new_description(alarm->text().toUtf8()));
      break;
    case Alarm::Invalid:
    default:
      kDebug(5800) << "Unknown type of alarm" << endl;
      action = ICAL_ACTION_NONE;
      break;
  }
  icalcomponent_add_property(a, icalproperty_new_action(action));

  // Trigger time
  icaltriggertype trigger;
  if ( alarm->hasTime() ) {
    trigger.time = writeICalUtcDateTime(alarm->time());
    trigger.duration = icaldurationtype_null_duration();
  } else {
    trigger.time = icaltime_null_time();
    Duration offset;
    if ( alarm->hasStartOffset() )
      offset = alarm->startOffset();
    else
      offset = alarm->endOffset();
    trigger.duration = icaldurationtype_from_int( offset.asSeconds() );
  }
  icalproperty *p = icalproperty_new_trigger(trigger);
  if ( alarm->hasEndOffset() )
    icalproperty_add_parameter(p, icalparameter_new_related(ICAL_RELATED_END));
  icalcomponent_add_property(a, p);

  // Repeat count and duration
  if (alarm->repeatCount()) {
    icalcomponent_add_property(a, icalproperty_new_repeat(alarm->repeatCount()));
    icalcomponent_add_property(a, icalproperty_new_duration(
                             icaldurationtype_from_int(alarm->snoozeTime()*60)));
  }

  // Custom properties
  QMap<QByteArray, QString> custom = alarm->customProperties();
  for (QMap<QByteArray, QString>::Iterator c = custom.begin();  c != custom.end();  ++c) {
    icalproperty *p = icalproperty_new_x(c.value().toUtf8());
    icalproperty_set_x_name(p,c.key());
    icalcomponent_add_property(a,p);
  }

  return a;
}

Todo *ICalFormatImpl::readTodo(icalcomponent *vtodo, ICalTimeZones *tzlist)
{
  Todo *todo = new Todo;

  readIncidence(vtodo, todo, tzlist);

  icalproperty *p = icalcomponent_get_first_property(vtodo,ICAL_ANY_PROPERTY);

  QStringList categories;

  while (p) {
    icalproperty_kind kind = icalproperty_isa(p);
    switch (kind) {

      case ICAL_DUE_PROPERTY: { // due date/time
        KDateTime kdt = readICalDateTimeProperty( p, tzlist );
        if ( kdt.isDateOnly() ) {
          todo->setDtDue(KDateTime(kdt.date(), todo->dtStart().timeSpec()), true);
        } else {
          todo->setDtDue( kdt, true );
          todo->setFloats(false);
        }
        todo->setHasDueDate(true);
        break;
      }
      case ICAL_COMPLETED_PROPERTY:  // completion date/time
        todo->setCompleted( readICalDateTimeProperty( p, tzlist ) );
        break;

      case ICAL_PERCENTCOMPLETE_PROPERTY:  // Percent completed
        todo->setPercentComplete(icalproperty_get_percentcomplete(p));
        break;

      case ICAL_RELATEDTO_PROPERTY:  // related todo (parent)
        todo->setRelatedToUid(QString::fromUtf8(icalproperty_get_relatedto(p)));
        mTodosRelate.append(todo);
        break;

      case ICAL_DTSTART_PROPERTY:
        // Flag that todo has start date. Value is read in by readIncidence().
        if ( todo->comments().filter("NoStartDate").count() )
          todo->setHasStartDate( false );
        else
          todo->setHasStartDate( true );
        break;

      case ICAL_RECURRENCEID_PROPERTY:
        todo->setDtRecurrence( readICalDateTimeProperty( p, tzlist ) );
        break;

      default:
//        kDebug(5800) << "ICALFormat::readTodo(): Unknown property: " << kind
//                  << endl;
        break;
    }

    p = icalcomponent_get_next_property(vtodo,ICAL_ANY_PROPERTY);
  }

  if (mCompat) mCompat->fixEmptySummary( todo );

  return todo;
}

Event *ICalFormatImpl::readEvent( icalcomponent *vevent, ICalTimeZones *tzlist)
{
  Event *event = new Event;

  readIncidence( vevent, event, tzlist);

  icalproperty *p = icalcomponent_get_first_property(vevent,ICAL_ANY_PROPERTY);

  QStringList categories;
  icalproperty_transp transparency;

  bool dtEndProcessed = false;

  while (p) {
    icalproperty_kind kind = icalproperty_isa(p);
    switch (kind) {

      case ICAL_DTEND_PROPERTY: { // end date and time
        KDateTime kdt = readICalDateTimeProperty( p, tzlist );
        if ( kdt.isDateOnly() ) {
          // End date is non-inclusive
          QDate endDate = kdt.date().addDays( -1 );
          if ( mCompat ) mCompat->fixFloatingEnd( endDate );
          if ( endDate < event->dtStart().date() ) {
            endDate = event->dtStart().date();
          }
          event->setDtEnd( KDateTime( endDate, event->dtStart().timeSpec() ) );
        } else {
          event->setDtEnd( kdt );
          event->setFloats( false );
        }
        dtEndProcessed = true;
        break;
      }
      case ICAL_RELATEDTO_PROPERTY:  // related event (parent)
        event->setRelatedToUid(QString::fromUtf8(icalproperty_get_relatedto(p)));
        mEventsRelate.append(event);
        break;


      case ICAL_TRANSP_PROPERTY:  // Transparency
        transparency = icalproperty_get_transp(p);
        if( transparency == ICAL_TRANSP_TRANSPARENT )
          event->setTransparency( Event::Transparent );
        else
          event->setTransparency( Event::Opaque );
        break;

      default:
//        kDebug(5800) << "ICALFormat::readEvent(): Unknown property: " << kind
//                  << endl;
        break;
    }

    p = icalcomponent_get_next_property(vevent,ICAL_ANY_PROPERTY);
  }

  // according to rfc2445 the dtend shouldn't be written when it equals
  // start date. so assign one equal to start date.
  if ( !dtEndProcessed && !event->hasDuration() ) {
    event->setDtEnd( event->dtStart() );
  }

  QString msade = event->nonKDECustomProperty("X-MICROSOFT-CDO-ALLDAYEVENT");
  if (!msade.isNull()) {
    bool floats = (msade == QLatin1String("TRUE"));
//    kDebug(5800) << "ICALFormat::readEvent(): all day event: " << floats << endl;
    event->setFloats(floats);
    if (floats) {
      KDateTime endDate = event->dtEnd();
      event->setDtEnd(endDate.addDays(-1));
    }
  }

  if ( mCompat ) mCompat->fixEmptySummary( event );

  return event;
}

FreeBusy *ICalFormatImpl::readFreeBusy(icalcomponent *vfreebusy)
{
  FreeBusy *freebusy = new FreeBusy;

  readIncidenceBase(vfreebusy, freebusy);

  icalproperty *p = icalcomponent_get_first_property(vfreebusy, ICAL_ANY_PROPERTY);

  PeriodList periods;

  while (p) {
    icalproperty_kind kind = icalproperty_isa(p);
    switch (kind) {

      case ICAL_DTSTART_PROPERTY:  // start date and time (UTC)
        freebusy->setDtStart( readICalUtcDateTimeProperty( p ) );
        break;

      case ICAL_DTEND_PROPERTY:  // end Date and Time (UTC)
        freebusy->setDtEnd( readICalUtcDateTimeProperty( p ) );
        break;

      case ICAL_FREEBUSY_PROPERTY: { //Any FreeBusy Times (UTC)
        icalperiodtype icalperiod = icalproperty_get_freebusy(p);
        KDateTime period_start = readICalUtcDateTime(p, icalperiod.start);
        if ( !icaltime_is_null_time(icalperiod.end) ) {
          KDateTime period_end = readICalUtcDateTime(p, icalperiod.end);
          periods.append( Period( period_start, period_end ) );
        } else {
          Duration duration ( readICalDuration( icalperiod.duration ) );
          periods.append( Period( period_start, duration ) );
        }
        break;}

      default:
//        kDebug(5800) << "ICalFormatImpl::readFreeBusy(): Unknown property: "
//                      << kind << endl;
      break;
    }
    p = icalcomponent_get_next_property(vfreebusy, ICAL_ANY_PROPERTY);
  }
  freebusy->addPeriods( periods );

  return freebusy;
}

Journal *ICalFormatImpl::readJournal(icalcomponent *vjournal, ICalTimeZones *tzlist)
{
  Journal *journal = new Journal;

  readIncidence(vjournal, journal, tzlist);

  return journal;
}

Attendee *ICalFormatImpl::readAttendee(icalproperty *attendee)
{
  icalparameter *p = 0;

  QString email = QString::fromUtf8(icalproperty_get_attendee(attendee));

  QString name;
  QString uid = QString();
  p = icalproperty_get_first_parameter(attendee, ICAL_CN_PARAMETER);
  if (p) {
    name = QString::fromUtf8(icalparameter_get_cn(p));
  } else {
  }

  bool rsvp=false;
  p = icalproperty_get_first_parameter(attendee, ICAL_RSVP_PARAMETER);
  if (p) {
    icalparameter_rsvp rsvpParameter = icalparameter_get_rsvp(p);
    if (rsvpParameter == ICAL_RSVP_TRUE) rsvp = true;
  }

  Attendee::PartStat status = Attendee::NeedsAction;
  p = icalproperty_get_first_parameter(attendee,ICAL_PARTSTAT_PARAMETER);
  if (p) {
    icalparameter_partstat partStatParameter = icalparameter_get_partstat(p);
    switch(partStatParameter) {
      default:
      case ICAL_PARTSTAT_NEEDSACTION:
        status = Attendee::NeedsAction;
        break;
      case ICAL_PARTSTAT_ACCEPTED:
        status = Attendee::Accepted;
        break;
      case ICAL_PARTSTAT_DECLINED:
        status = Attendee::Declined;
        break;
      case ICAL_PARTSTAT_TENTATIVE:
        status = Attendee::Tentative;
        break;
      case ICAL_PARTSTAT_DELEGATED:
        status = Attendee::Delegated;
        break;
      case ICAL_PARTSTAT_COMPLETED:
        status = Attendee::Completed;
        break;
      case ICAL_PARTSTAT_INPROCESS:
        status = Attendee::InProcess;
        break;
    }
  }

  Attendee::Role role = Attendee::ReqParticipant;
  p = icalproperty_get_first_parameter(attendee, ICAL_ROLE_PARAMETER);
  if (p) {
    icalparameter_role roleParameter = icalparameter_get_role(p);
    switch(roleParameter) {
      case ICAL_ROLE_CHAIR:
        role = Attendee::Chair;
        break;
      default:
      case ICAL_ROLE_REQPARTICIPANT:
        role = Attendee::ReqParticipant;
        break;
      case ICAL_ROLE_OPTPARTICIPANT:
        role = Attendee::OptParticipant;
        break;
      case ICAL_ROLE_NONPARTICIPANT:
        role = Attendee::NonParticipant;
        break;
    }
  }

  p = icalproperty_get_first_parameter(attendee, ICAL_X_PARAMETER);
  uid = icalparameter_get_xvalue(p);
  // This should be added, but there seems to be a libical bug here.
  // TODO: does this work now in libical-0.24 or greater?
  /*while (p) {
   // if (icalparameter_get_xname(p) == "X-UID") {
    uid = icalparameter_get_xvalue(p);
    p = icalproperty_get_next_parameter(attendee, ICAL_X_PARAMETER);
  } */

  return new Attendee( name, email, rsvp, status, role, uid );
}

Person ICalFormatImpl::readOrganizer( icalproperty *organizer )
{
  QString email = QString::fromUtf8(icalproperty_get_organizer(organizer));
  if ( email.startsWith("mailto:", Qt::CaseInsensitive ) ) {
    email = email.mid( 7 );
  }
  QString cn;

  icalparameter *p = icalproperty_get_first_parameter(
             organizer, ICAL_CN_PARAMETER );

  if ( p ) {
    cn = QString::fromUtf8( icalparameter_get_cn( p ) );
  }
  Person org( cn, email );
  // TODO: Treat sent-by, dir and language here, too
  return org;
}

Attachment *ICalFormatImpl::readAttachment( icalproperty *attach )
{
  Attachment *attachment = 0;

  icalvalue_kind value_kind = icalvalue_isa( icalproperty_get_value( attach ) );

  if ( value_kind == ICAL_ATTACH_VALUE ) {
    icalattach *a = icalproperty_get_attach( attach );

    int isurl = icalattach_get_is_url( a );
    if ( isurl == 0 )
      attachment = new Attachment( (const char* )icalattach_get_data( a ) );
    else {
      attachment = new Attachment( QString( icalattach_get_url( a ) ) );
    }
  } else if ( value_kind == ICAL_URI_VALUE ) {
    attachment =
      new Attachment( QString( icalvalue_get_uri( icalproperty_get_value( attach ) ) ) );
  }

  icalparameter *p =
    icalproperty_get_first_parameter( attach, ICAL_FMTTYPE_PARAMETER );
  if ( p && attachment )
    attachment->setMimeType( QString( icalparameter_get_fmttype( p ) ) );

  p = icalproperty_get_first_parameter( attach, ICAL_X_PARAMETER );
  while ( p ) {
    QString xname = QString( icalparameter_get_xname( p ) ).toUpper();
    QString xvalue = QString::fromUtf8( icalparameter_get_xvalue( p ) );
    if ( xname == "X-CONTENT-DISPOSITION" )
      attachment->setShowInline( xvalue.toLower() == "inline" );
    if ( xname == "X-LABEL" )
      attachment->setLabel( xvalue );
    if ( xname == "X-KONTACT-TYPE" )
      attachment->setLocal( xvalue.toLower() == "local" );
    p = icalproperty_get_next_parameter( attach, ICAL_X_PARAMETER );
  }

  return attachment;
}

void ICalFormatImpl::readIncidence(icalcomponent *parent, Incidence *incidence, ICalTimeZones *tzlist)
{
  readIncidenceBase(parent,incidence);

  icalproperty *p = icalcomponent_get_first_property(parent, ICAL_ANY_PROPERTY);

  const char *text;
  int intvalue, inttext;
  icaldurationtype icalduration;
  KDateTime kdt;

  QStringList categories;

  while (p) {
    icalproperty_kind kind = icalproperty_isa(p);
    switch (kind) {

      case ICAL_CREATED_PROPERTY:
        incidence->setCreated( readICalDateTimeProperty( p, tzlist ) );
        break;

      case ICAL_SEQUENCE_PROPERTY:  // sequence
        intvalue = icalproperty_get_sequence(p);
        incidence->setRevision(intvalue);
        break;

      case ICAL_LASTMODIFIED_PROPERTY:  // last modification UTC date/time
        incidence->setLastModified( readICalDateTimeProperty( p, tzlist ) );
        break;

      case ICAL_DTSTART_PROPERTY:  // start date and time
        kdt = readICalDateTimeProperty( p, tzlist );
        incidence->setDtStart( kdt );
        incidence->setFloats( kdt.isDateOnly() );
        break;

      case ICAL_DURATION_PROPERTY:  // start date and time
        icalduration = icalproperty_get_duration(p);
        incidence->setDuration(readICalDuration(icalduration));
        break;

      case ICAL_DESCRIPTION_PROPERTY:  // description
        text = icalproperty_get_description(p);
        incidence->setDescription(QString::fromUtf8(text));
        break;

      case ICAL_SUMMARY_PROPERTY:  // summary
        text = icalproperty_get_summary(p);
        incidence->setSummary(QString::fromUtf8(text));
        break;

      case ICAL_LOCATION_PROPERTY:  // location
        text = icalproperty_get_location(p);
        incidence->setLocation(QString::fromUtf8(text));
        break;

      case ICAL_STATUS_PROPERTY: {  // status
        Incidence::Status stat;
        switch (icalproperty_get_status(p)) {
          case ICAL_STATUS_TENTATIVE:   stat = Incidence::StatusTentative; break;
          case ICAL_STATUS_CONFIRMED:   stat = Incidence::StatusConfirmed; break;
          case ICAL_STATUS_COMPLETED:   stat = Incidence::StatusCompleted; break;
          case ICAL_STATUS_NEEDSACTION: stat = Incidence::StatusNeedsAction; break;
          case ICAL_STATUS_CANCELLED:   stat = Incidence::StatusCanceled; break;
          case ICAL_STATUS_INPROCESS:   stat = Incidence::StatusInProcess; break;
          case ICAL_STATUS_DRAFT:       stat = Incidence::StatusDraft; break;
          case ICAL_STATUS_FINAL:       stat = Incidence::StatusFinal; break;
          case ICAL_STATUS_X:
            incidence->setCustomStatus(QString::fromUtf8(icalvalue_get_x(icalproperty_get_value(p))));
            stat = Incidence::StatusX;
            break;
          case ICAL_STATUS_NONE:
          default:                      stat = Incidence::StatusNone; break;
        }
        if (stat != Incidence::StatusX)
          incidence->setStatus(stat);
        break;
      }

      case ICAL_PRIORITY_PROPERTY:  // priority
        intvalue = icalproperty_get_priority( p );
        if ( mCompat )
          intvalue = mCompat->fixPriority( intvalue );
        incidence->setPriority( intvalue );
        break;

      case ICAL_CATEGORIES_PROPERTY:  // categories
        text = icalproperty_get_categories(p);
        categories.append(QString::fromUtf8(text));
        break;

      case ICAL_RRULE_PROPERTY:
        readRecurrenceRule( p, incidence );
        break;

      case ICAL_RDATE_PROPERTY:
        kdt = readICalDateTimeProperty( p, tzlist );
        if ( kdt.isValid() ) {
          if ( kdt.isDateOnly() ) {
            incidence->recurrence()->addRDate( kdt.date() );
          } else {
            incidence->recurrence()->addRDateTime( kdt );
          }
        } else {
          // TODO: RDates as period are not yet implemented!
        }
        break;

      case ICAL_EXRULE_PROPERTY:
        readExceptionRule( p, incidence );
        break;

      case ICAL_EXDATE_PROPERTY:
        kdt = readICalDateTimeProperty( p, tzlist );
        if ( kdt.isDateOnly() ) {
          incidence->recurrence()->addExDate( kdt.date() );
        } else {
          incidence->recurrence()->addExDateTime( kdt );
        }
        break;

      case ICAL_CLASS_PROPERTY:
        inttext = icalproperty_get_class(p);
        if (inttext == ICAL_CLASS_PUBLIC ) {
          incidence->setSecrecy(Incidence::SecrecyPublic);
        } else if (inttext == ICAL_CLASS_CONFIDENTIAL ) {
          incidence->setSecrecy(Incidence::SecrecyConfidential);
        } else {
          incidence->setSecrecy(Incidence::SecrecyPrivate);
        }
        break;

      case ICAL_ATTACH_PROPERTY:  // attachments
        incidence->addAttachment(readAttachment(p));
        break;

      default:
//        kDebug(5800) << "ICALFormat::readIncidence(): Unknown property: " << kind
//                  << endl;
        break;
    }

    p = icalcomponent_get_next_property(parent,ICAL_ANY_PROPERTY);
  }

  // Set the scheduling ID
  const QString uid = incidence->customProperty( "LIBKCAL", "ID" );
  if ( !uid.isNull() ) {
    // The UID stored in incidencebase is actually the scheduling ID
    // It has to be stored in the iCal UID component for compatibility
    // with other iCal applications
    incidence->setSchedulingID( incidence->uid() );
    incidence->setUid( uid );
  }

  // kpilot stuff
// TODO: move this application-specific code to kpilot
  QString kp = incidence->nonKDECustomProperty("X-PILOTID");
  if (!kp.isNull()) {
    incidence->setPilotId(kp.toInt());
  }
  kp = incidence->nonKDECustomProperty("X-PILOTSTAT");
  if (!kp.isNull()) {
    incidence->setSyncStatus(kp.toInt());
  }

  // Now that recurrence and exception stuff is completely set up,
  // do any backwards compatibility adjustments.
  if ( incidence->doesRecur() && mCompat )
      mCompat->fixRecurrence( incidence );

  // add categories
  incidence->setCategories(categories);

  // iterate through all alarms
  for (icalcomponent *alarm = icalcomponent_get_first_component(parent, ICAL_VALARM_COMPONENT);
       alarm;
       alarm = icalcomponent_get_next_component(parent, ICAL_VALARM_COMPONENT)) {
    readAlarm(alarm, incidence, tzlist);
  }
  // Fix incorrect alarm settings by other applications (like outloook 9)
  if ( mCompat ) mCompat->fixAlarms( incidence );
}

void ICalFormatImpl::readIncidenceBase(icalcomponent *parent, IncidenceBase *incidenceBase)
{
  icalproperty *p = icalcomponent_get_first_property(parent, ICAL_ANY_PROPERTY);

  while (p) {
    icalproperty_kind kind = icalproperty_isa(p);
    switch (kind) {

      case ICAL_UID_PROPERTY:  // unique id
        incidenceBase->setUid(QString::fromUtf8(icalproperty_get_uid(p)));
        break;

      case ICAL_ORGANIZER_PROPERTY:  // organizer
        incidenceBase->setOrganizer( readOrganizer(p));
        break;

      case ICAL_ATTENDEE_PROPERTY:  // attendee
        incidenceBase->addAttendee(readAttendee(p));
        break;

      case ICAL_COMMENT_PROPERTY:
        incidenceBase->addComment(
            QString::fromUtf8(icalproperty_get_comment(p)));
        break;

      default:
        break;
    }

    p = icalcomponent_get_next_property(parent, ICAL_ANY_PROPERTY);
  }

  // custom properties
  readCustomProperties(parent, incidenceBase);
}

void ICalFormatImpl::readCustomProperties(icalcomponent *parent, CustomProperties *properties)
{
  QMap<QByteArray, QString> customProperties;
  QString lastProperty=QString();

  icalproperty *p = icalcomponent_get_first_property(parent, ICAL_X_PROPERTY);

  while (p) {

    QString value = QString::fromUtf8(icalproperty_get_x(p));
    const char *name = icalproperty_get_x_name(p);
    if ( lastProperty != name )
    {
      customProperties[name] = value;
      //kDebug(5800) << "Set custom property [" << name << '=' << value << ']' << endl;
    }
    else
    {
      customProperties[name] = customProperties[name].append(",").append(value);
    }
    p = icalcomponent_get_next_property(parent,ICAL_X_PROPERTY);
    lastProperty=name;
  }

  properties->setCustomProperties(customProperties);
}



void ICalFormatImpl::readRecurrenceRule(icalproperty *rrule, Incidence *incidence )
{
//  kDebug(5800) << "Read recurrence for " << incidence->summary() << endl;

  Recurrence *recur = incidence->recurrence();

  struct icalrecurrencetype r = icalproperty_get_rrule(rrule);
  // dumpIcalRecurrence(r);

  RecurrenceRule *recurrule = new RecurrenceRule( /*incidence*/ );
  recurrule->setStartDt( incidence->dtStart() );
  readRecurrence( r, recurrule );
  recur->addRRule( recurrule );
}

void ICalFormatImpl::readExceptionRule( icalproperty *rrule, Incidence *incidence )
{
//  kDebug(5800) << "Read recurrence for " << incidence->summary() << endl;

  struct icalrecurrencetype r = icalproperty_get_exrule(rrule);
  // dumpIcalRecurrence(r);

  RecurrenceRule *recurrule = new RecurrenceRule( /*incidence*/ );
  recurrule->setStartDt( incidence->dtStart() );
  readRecurrence( r, recurrule );

  Recurrence *recur = incidence->recurrence();
  recur->addExRule( recurrule );
}

void ICalFormatImpl::readRecurrence( const struct icalrecurrencetype &r, RecurrenceRule* recur )
{
  // Generate the RRULE string
  recur->mRRule = QString( icalrecurrencetype_as_string( const_cast<struct icalrecurrencetype*>(&r) ) );
  // Period
  switch ( r.freq ) {
    case ICAL_SECONDLY_RECURRENCE: recur->setRecurrenceType( RecurrenceRule::rSecondly ); break;
    case ICAL_MINUTELY_RECURRENCE: recur->setRecurrenceType( RecurrenceRule::rMinutely ); break;
    case ICAL_HOURLY_RECURRENCE: recur->setRecurrenceType( RecurrenceRule::rHourly ); break;
    case ICAL_DAILY_RECURRENCE: recur->setRecurrenceType( RecurrenceRule::rDaily ); break;
    case ICAL_WEEKLY_RECURRENCE: recur->setRecurrenceType( RecurrenceRule::rWeekly ); break;
    case ICAL_MONTHLY_RECURRENCE: recur->setRecurrenceType( RecurrenceRule::rMonthly ); break;
    case ICAL_YEARLY_RECURRENCE: recur->setRecurrenceType( RecurrenceRule::rYearly ); break;
    case ICAL_NO_RECURRENCE:
    default:
        recur->setRecurrenceType( RecurrenceRule::rNone );
  }
  // Frequency
  recur->setFrequency( r.interval );

  // Duration & End Date
  if ( !icaltime_is_null_time( r.until ) ) {
    icaltimetype t = r.until;
    recur->setEndDt( readICalUtcDateTime(0, t) );
  } else {
    if (r.count == 0)
      recur->setDuration( -1 );
    else
      recur->setDuration( r.count );
  }

  // Week start setting
  int wkst = (r.week_start + 5)%7 + 1;
  recur->setWeekStart( wkst );

  // And now all BY*
  QList<int> lst;
  int i;
  int index = 0;

#define readSetByList(rrulecomp,setfunc) \
  index = 0; \
  lst.clear(); \
  while ( (i = r.rrulecomp[index++] ) != ICAL_RECURRENCE_ARRAY_MAX ) \
    lst.append( i ); \
  if ( !lst.isEmpty() ) recur->setfunc( lst );

  // BYSECOND, MINUTE and HOUR, MONTHDAY, YEARDAY, WEEKNUMBER, MONTH
  // and SETPOS are standard int lists, so we can treat them with the
  // same macro
  readSetByList( by_second, setBySeconds );
  readSetByList( by_minute, setByMinutes );
  readSetByList( by_hour, setByHours );
  readSetByList( by_month_day, setByMonthDays );
  readSetByList( by_year_day, setByYearDays );
  readSetByList( by_week_no, setByWeekNumbers );
  readSetByList( by_month, setByMonths );
  readSetByList( by_set_pos, setBySetPos );
#undef readSetByList

  // BYDAY is a special case, since it's not an int list
  QList<RecurrenceRule::WDayPos> wdlst;
  short day;
  index=0;
  while((day = r.by_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
    RecurrenceRule::WDayPos pos;
    pos.setDay( ( icalrecurrencetype_day_day_of_week( day ) + 5 )%7 + 1 );
    pos.setPos( icalrecurrencetype_day_position( day ) );
//     kDebug(5800)<< "    o) By day, index="<<index-1<<", pos="<<pos.Pos<<", day="<<pos.Day<<endl;
    wdlst.append( pos );
  }
  if ( !wdlst.isEmpty() ) recur->setByDays( wdlst );


  // TODO Store all X- fields of the RRULE inside the recurrence (so they are
  // preserved
}


void ICalFormatImpl::readAlarm(icalcomponent *alarm, Incidence *incidence, ICalTimeZones *tzlist)
{
  //kDebug(5800) << "Read alarm for " << incidence->summary() << endl;

  Alarm* ialarm = incidence->newAlarm();
  ialarm->setRepeatCount(0);
  ialarm->setEnabled(true);

  // Determine the alarm's action type
  icalproperty *p = icalcomponent_get_first_property(alarm,ICAL_ACTION_PROPERTY);
  Alarm::Type type = Alarm::Display;
  icalproperty_action action = ICAL_ACTION_DISPLAY;
  if ( !p ) {
    kDebug(5800) << "Unknown type of alarm, using default" << endl;
//    return;
  } else {

    action = icalproperty_get_action(p);
    switch ( action ) {
      case ICAL_ACTION_DISPLAY:   type = Alarm::Display;  break;
      case ICAL_ACTION_AUDIO:     type = Alarm::Audio;  break;
      case ICAL_ACTION_PROCEDURE: type = Alarm::Procedure;  break;
      case ICAL_ACTION_EMAIL:     type = Alarm::Email;  break;
      default:
        kDebug(5800) << "Unknown type of alarm: " << action << endl;
//        type = Alarm::Invalid;
    }
  }
  ialarm->setType(type);
  //kDebug(5800) << " alarm type =" << type << endl;

  p = icalcomponent_get_first_property(alarm,ICAL_ANY_PROPERTY);
  while (p) {
    icalproperty_kind kind = icalproperty_isa(p);

    switch (kind) {

      case ICAL_TRIGGER_PROPERTY: {
        icaltriggertype trigger = icalproperty_get_trigger(p);
        if (icaltime_is_null_time(trigger.time)) {
          if (icaldurationtype_is_null_duration(trigger.duration)) {
            kDebug(5800) << "ICalFormatImpl::readAlarm(): Trigger has no time and no duration." << endl;
          } else {
            Duration duration ( icaldurationtype_as_int( trigger.duration ) );
            icalparameter *param = icalproperty_get_first_parameter(p, ICAL_RELATED_PARAMETER);
            if (param && icalparameter_get_related(param) == ICAL_RELATED_END)
              ialarm->setEndOffset(duration);
            else
              ialarm->setStartOffset(duration);
          }
        } else {
          ialarm->setTime(readICalUtcDateTime(p, trigger.time, tzlist));
        }
        break;
      }
      case ICAL_DURATION_PROPERTY: {
        icaldurationtype duration = icalproperty_get_duration(p);
        ialarm->setSnoozeTime(icaldurationtype_as_int(duration)/60);
        break;
      }
      case ICAL_REPEAT_PROPERTY:
        ialarm->setRepeatCount(icalproperty_get_repeat(p));
        break;

      // Only in DISPLAY and EMAIL and PROCEDURE alarms
      case ICAL_DESCRIPTION_PROPERTY: {
        QString description = QString::fromUtf8(icalproperty_get_description(p));
        switch ( action ) {
          case ICAL_ACTION_DISPLAY:
            ialarm->setText( description );
            break;
          case ICAL_ACTION_PROCEDURE:
            ialarm->setProgramArguments( description );
            break;
          case ICAL_ACTION_EMAIL:
            ialarm->setMailText( description );
            break;
          default:
            break;
        }
        break;
      }
      // Only in EMAIL alarm
      case ICAL_SUMMARY_PROPERTY:
        ialarm->setMailSubject(QString::fromUtf8(icalproperty_get_summary(p)));
        break;

      // Only in EMAIL alarm
      case ICAL_ATTENDEE_PROPERTY: {
        QString email = QString::fromUtf8(icalproperty_get_attendee(p));
        QString name;
        icalparameter *param = icalproperty_get_first_parameter(p, ICAL_CN_PARAMETER);
        if (param) {
          name = QString::fromUtf8(icalparameter_get_cn(param));
        }
        ialarm->addMailAddress(Person(name, email));
        break;
      }
      // Only in AUDIO and EMAIL and PROCEDURE alarms
      case ICAL_ATTACH_PROPERTY: {
        Attachment *attach = readAttachment( p );
        if ( attach && attach->isUri() ) {
          switch ( action ) {
            case ICAL_ACTION_AUDIO:
              ialarm->setAudioFile( attach->uri() );
              break;
            case ICAL_ACTION_PROCEDURE:
              ialarm->setProgramFile( attach->uri() );
              break;
            case ICAL_ACTION_EMAIL:
              ialarm->addMailAttachment( attach->uri() );
              break;
            default:
              break;
          }
        } else {
          kDebug() << "Alarm attachments currently only support URIs, but "
                       "no binary data" << endl;
        }
        delete attach;
        break;
      }
      default:
        break;
    }

    p = icalcomponent_get_next_property(alarm, ICAL_ANY_PROPERTY);
  }

  // custom properties
  readCustomProperties(alarm, ialarm);

  // TODO: check for consistency of alarm properties
}

icaldatetimeperiodtype ICalFormatImpl::writeICalDatePeriod( const QDate &date )
{
  icaldatetimeperiodtype t;
  t.time = writeICalDate( date );
  t.period = icalperiodtype_null_period();
  return t;
}

icaltimetype ICalFormatImpl::writeICalDate(const QDate &date)
{
  icaltimetype t = icaltime_null_time();

  t.year = date.year();
  t.month = date.month();
  t.day = date.day();

  t.hour = 0;
  t.minute = 0;
  t.second = 0;

  t.is_date = 1;

  t.is_utc = 0;

  t.zone = 0;

  return t;
}

icaltimetype ICalFormatImpl::writeICalDateTime( const KDateTime &datetime )
{
  icaltimetype t = icaltime_null_time();

  t.year = datetime.date().year();
  t.month = datetime.date().month();
  t.day = datetime.date().day();

  t.hour = datetime.time().hour();
  t.minute = datetime.time().minute();
  t.second = datetime.time().second();

  t.is_date = 0;
  t.zone = 0;   // zone is NOT set
  t.is_utc = datetime.isUtc() ? 1 : 0;

 // _dumpIcaltime( t );

  return t;
}

icalproperty *ICalFormatImpl::writeICalDateTimeProperty(const icalproperty_kind type,
                     const KDateTime &dt, ICalTimeZones *tzlist, ICalTimeZones *tzUsedList)
{
  icaltimetype t;
  switch ( type ) {
    case ICAL_DTSTAMP_PROPERTY:
    case ICAL_CREATED_PROPERTY:
    case ICAL_LASTMODIFIED_PROPERTY:
      t = writeICalDateTime( dt.toUtc() );
      break;
    default:
      t = writeICalDateTime( dt );
      break;
  }
  icalproperty *p;
  switch ( type ) {
    case ICAL_DTSTAMP_PROPERTY:
      p = icalproperty_new_dtstamp( t );
      break;
    case ICAL_CREATED_PROPERTY:
      p = icalproperty_new_created( t );
      break;
    case ICAL_LASTMODIFIED_PROPERTY:
      p = icalproperty_new_lastmodified( t );
      break;
    case ICAL_DTSTART_PROPERTY:  // start date and time
      p = icalproperty_new_dtstart( t );
      break;
    case ICAL_DTEND_PROPERTY:    // end date and time
      p = icalproperty_new_dtend( t );
      break;
    case ICAL_DUE_PROPERTY:
      p = icalproperty_new_due( t );
      break;
    case ICAL_RECURRENCEID_PROPERTY:
      p = icalproperty_new_recurrenceid( t );
      break;
    case ICAL_EXDATE_PROPERTY:
      p = icalproperty_new_exdate( t );
      break;
    default: {
      icaldatetimeperiodtype tp;
      tp.time = t;
      tp.period = icalperiodtype_null_period();
      switch ( type ) {
        case ICAL_RDATE_PROPERTY:
          p = icalproperty_new_rdate( tp );
          break;
       default:
         return 0;
      }
    }
  }
  const KTimeZone *ktz = t.is_utc ? 0 : dt.timeZone();
  if ( ktz ) {
    if ( tzlist ) {
      const ICalTimeZone *tz = tzlist->zone( ktz->name() );
      if ( !tz ) {
        // The time zone isn't in the list of known zones for the calendar
        // - add it to the calendar's zone list
        ICalTimeZone *tznew = new ICalTimeZone( *ktz );
        tzlist->add( tznew );
	tz = tznew;
      }
      if ( tzUsedList )
        tzUsedList->addConst( tz );   // 'tz' belongs to tzlist
    }
    icalproperty_add_parameter(p, icalparameter_new_tzid( ktz->name().toUtf8() ));
  }
  return p;
}

KDateTime ICalFormatImpl::readICalDateTime( icalproperty *p, const icaltimetype& t, ICalTimeZones *tzlist, bool utc)
{
//  kDebug(5800) << "ICalFormatImpl::readICalDateTime()" << endl;
//  _dumpIcaltime( t );
  KDateTime::Spec timeSpec;
  if ( t.is_utc  ||  t.zone == icaltimezone_get_utc_timezone() ) {
    timeSpec = KDateTime::UTC;   // the time zone is UTC
    utc = false;    // no need to convert to UTC
  }
  else {
    if ( !tzlist ) {
      utc = true;   // should be UTC, but it isn't
    }
    icalparameter *param = p ? icalproperty_get_first_parameter(p, ICAL_TZID_PARAMETER) : 0;
    const char *tzid = param ? icalparameter_get_tzid(param) : 0;
    if ( !tzid )
      timeSpec = KDateTime::ClockTime;
    else {
      QString tzidStr = QString::fromUtf8( tzid );
      const ICalTimeZone *tz = tzlist ? tzlist->zone( tzidStr ) : 0;
      if ( !tz ) {
        // The time zone is not in the existing list for the calendar.
        // Try to read it from the system or libical databases.
        ICalTimeZoneSource tzsource;
        ICalTimeZone *newtz = tzsource.standardZone( tzidStr );
        if ( newtz && tzlist )
          tzlist->add( newtz );
        tz = newtz;
      }
      timeSpec = tz ? KDateTime::Spec( tz ) : KDateTime::LocalZone;
//      kDebug(5800) << "--- Time zone: " << (tz ? timeSpec.timeZone()->name() : QString()) << endl;
    }
  }
  KDateTime result( QDate(t.year,t.month,t.day), QTime(t.hour,t.minute,t.second), timeSpec );
  return utc ? result.toUtc() : result;
}

QDate ICalFormatImpl::readICalDate(icaltimetype t)
{
  return QDate(t.year,t.month,t.day);
}

KDateTime ICalFormatImpl::readICalDateTimeProperty( icalproperty *p, ICalTimeZones *tzlist, bool utc )
{
  icaldatetimeperiodtype tp;
  icalproperty_kind kind = icalproperty_isa( p );
  switch ( kind ) {
    case ICAL_CREATED_PROPERTY:   // UTC date/time
      tp.time = icalproperty_get_created( p );
      utc = true;
      break;
    case ICAL_LASTMODIFIED_PROPERTY:  // last modification UTC date/time
      tp.time = icalproperty_get_lastmodified( p );
      utc = true;
      break;
    case ICAL_DTSTART_PROPERTY:  // start date and time (UTC for freebusy)
      tp.time = icalproperty_get_dtstart( p );
      break;
    case ICAL_DTEND_PROPERTY:    // end date and time (UTC for freebusy)
      tp.time = icalproperty_get_dtend( p );
      break;
    case ICAL_DUE_PROPERTY:      // due date/time
      tp.time = icalproperty_get_due( p );
      break;
    case ICAL_COMPLETED_PROPERTY:  // UTC completion date/time
      tp.time = icalproperty_get_completed( p );
      utc = true;
      break;
    case ICAL_RECURRENCEID_PROPERTY:
      tp.time = icalproperty_get_recurrenceid( p );
      break;
    case ICAL_EXDATE_PROPERTY:
      tp.time = icalproperty_get_exdate( p );
      break;
    default:
      switch ( kind ) {
        case ICAL_RDATE_PROPERTY:
          tp = icalproperty_get_rdate( p );
          break;
       default:
         return KDateTime();
      }
      if ( !icaltime_is_valid_time( tp.time ) )
        return KDateTime();   // a time period was found (not implemented yet)
      break;
  }
  if ( tp.time.is_date ) {
    return KDateTime( readICalDate(tp.time), KDateTime::Spec::ClockTime );
  } else {
    return readICalDateTime( p, tp.time, tzlist, utc );
  }
}

icaldurationtype ICalFormatImpl::writeICalDuration(int seconds)
{
  icaldurationtype d;

  d.is_neg  = (seconds<0)?1:0;
  if (seconds<0) seconds = -seconds;

  d.weeks    = seconds / gSecondsPerWeek;
  seconds   %= gSecondsPerWeek;
  d.days     = seconds / gSecondsPerDay;
  seconds   %= gSecondsPerDay;
  d.hours    = seconds / gSecondsPerHour;
  seconds   %= gSecondsPerHour;
  d.minutes  = seconds / gSecondsPerMinute;
  seconds   %= gSecondsPerMinute;
  d.seconds  = seconds;

  return d;
}

int ICalFormatImpl::readICalDuration(icaldurationtype d)
{
  int result = 0;

  result += d.weeks   * gSecondsPerWeek;
  result += d.days    * gSecondsPerDay;
  result += d.hours   * gSecondsPerHour;
  result += d.minutes * gSecondsPerMinute;
  result += d.seconds;

  if (d.is_neg) result *= -1;

  return result;
}

icalcomponent *ICalFormatImpl::createCalendarComponent(Calendar *cal)
{
  icalcomponent *calendar;

  // Root component
  calendar = icalcomponent_new(ICAL_VCALENDAR_COMPONENT);

  icalproperty *p;

  // Product Identifier
  p = icalproperty_new_prodid(CalFormat::productId().toUtf8());
  icalcomponent_add_property(calendar,p);

  // TODO: Add time zone

  // iCalendar version (2.0)
  p = icalproperty_new_version(const_cast<char *>(_ICAL_VERSION));
  icalcomponent_add_property(calendar,p);

  // Custom properties
  if( cal != 0 )
    writeCustomProperties(calendar, cal);

  return calendar;
}



// take a raw vcalendar (i.e. from a file on disk, clipboard, etc. etc.
// and break it down from its tree-like format into the dictionary format
// that is used internally in the ICalFormatImpl.
bool ICalFormatImpl::populate( Calendar *cal, icalcomponent *calendar)
{
  // this function will populate the caldict dictionary and other event
  // lists. It turns vevents into Events and then inserts them.

    if (!calendar) return false;

// TODO: check for METHOD

  icalproperty *p;

  p = icalcomponent_get_first_property(calendar,ICAL_PRODID_PROPERTY);
  if (!p) {
    kDebug(5800) << "No PRODID property found" << endl;
    mLoadedProductId = "";
  } else {
    mLoadedProductId = QString::fromUtf8(icalproperty_get_prodid(p));
//    kDebug(5800) << "VCALENDAR prodid: '" << mLoadedProductId << "'" << endl;

    delete mCompat;
    mCompat = CompatFactory::createCompat( mLoadedProductId );
  }

  p = icalcomponent_get_first_property(calendar,ICAL_VERSION_PROPERTY);
  if (!p) {
    kDebug(5800) << "No VERSION property found" << endl;
    mParent->setException(new ErrorFormat(ErrorFormat::CalVersionUnknown));
    return false;
  } else {
    const char *version = icalproperty_get_version(p);
//    kDebug(5800) << "VCALENDAR version: '" << version << "'" << endl;

    if (strcmp(version,"1.0") == 0) {
      kDebug(5800) << "Expected iCalendar, got vCalendar" << endl;
      mParent->setException(new ErrorFormat(ErrorFormat::CalVersion1,
                            i18n("Expected iCalendar format")));
      return false;
    } else if (strcmp(version,"2.0") != 0) {
      kDebug(5800) << "Expected iCalendar, got unknown format" << endl;
      mParent->setException(new ErrorFormat(ErrorFormat::CalVersionUnknown));
      return false;
    }
  }

  // Populate the calendar's time zone collection with all VTIMEZONE components
  ICalTimeZones *tzlist = cal->timeZones();
  tzlist->clear();
  ICalTimeZoneSource tzs;
  tzs.parse(calendar, *tzlist);
kDebug(5800)<<"populate(): count="<<tzlist->zones().count()<<endl;

  // custom properties
  readCustomProperties(calendar, cal);

  // Store all events with a relatedTo property in a list for post-processing
  mEventsRelate.clear();
  mTodosRelate.clear();
  // TODO: make sure that only actually added events go to this lists.

  icalcomponent *c;

  // Iterate through all todos
  c = icalcomponent_get_first_component(calendar,ICAL_VTODO_COMPONENT);
  while (c) {
//    kDebug(5800) << "----Todo found" << endl;
    Todo *todo = readTodo(c, tzlist);
    if (todo && !cal->todo(todo->uid())) cal->addTodo(todo);
    c = icalcomponent_get_next_component(calendar,ICAL_VTODO_COMPONENT);
  }

  // Iterate through all events
  c = icalcomponent_get_first_component(calendar,ICAL_VEVENT_COMPONENT);
  while (c) {
//    kDebug(5800) << "----Event found" << endl;
    Event *event = readEvent(c, tzlist);
    if (event && !cal->event(event->uid())) cal->addEvent(event);
    c = icalcomponent_get_next_component(calendar,ICAL_VEVENT_COMPONENT);
  }

  // Iterate through all journals
  c = icalcomponent_get_first_component(calendar,ICAL_VJOURNAL_COMPONENT);
  while (c) {
//    kDebug(5800) << "----Journal found" << endl;
    Journal *journal = readJournal(c, tzlist);
    if (journal && !cal->journal(journal->uid())) cal->addJournal(journal);
    c = icalcomponent_get_next_component(calendar,ICAL_VJOURNAL_COMPONENT);
  }

  // Post-Process list of events with relations, put Event objects in relation
  Event::List::ConstIterator eIt;
  for ( eIt = mEventsRelate.begin(); eIt != mEventsRelate.end(); ++eIt ) {
    (*eIt)->setRelatedTo( cal->incidence( (*eIt)->relatedToUid() ) );
  }
  Todo::List::ConstIterator tIt;
  for ( tIt = mTodosRelate.begin(); tIt != mTodosRelate.end(); ++tIt ) {
    (*tIt)->setRelatedTo( cal->incidence( (*tIt)->relatedToUid() ) );
   }

  return true;
}

QString ICalFormatImpl::extractErrorProperty(icalcomponent *c)
{
//  kDebug(5800) << "ICalFormatImpl:extractErrorProperty: "
//            << icalcomponent_as_ical_string(c) << endl;

  QString errorMessage;

  icalproperty *error;
  error = icalcomponent_get_first_property(c,ICAL_XLICERROR_PROPERTY);
  while(error) {
    errorMessage += icalproperty_get_xlicerror(error);
    errorMessage += '\n';
    error = icalcomponent_get_next_property(c,ICAL_XLICERROR_PROPERTY);
  }

//  kDebug(5800) << "ICalFormatImpl:extractErrorProperty: " << errorMessage << endl;

  return errorMessage;
}

void ICalFormatImpl::dumpIcalRecurrence(icalrecurrencetype r)
{
  int i;

  kDebug(5800) << " Freq: " << r.freq << endl;
  kDebug(5800) << " Until: " << icaltime_as_ical_string(r.until) << endl;
  kDebug(5800) << " Count: " << r.count << endl;
  if (r.by_day[0] != ICAL_RECURRENCE_ARRAY_MAX) {
    int index = 0;
    QString out = " By Day: ";
    while((i = r.by_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
      out.append(QString::number(i) + ' ');
    }
    kDebug(5800) << out << endl;
  }
  if (r.by_month_day[0] != ICAL_RECURRENCE_ARRAY_MAX) {
    int index = 0;
    QString out = " By Month Day: ";
    while((i = r.by_month_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
      out.append(QString::number(i) + ' ');
    }
    kDebug(5800) << out << endl;
  }
  if (r.by_year_day[0] != ICAL_RECURRENCE_ARRAY_MAX) {
    int index = 0;
    QString out = " By Year Day: ";
    while((i = r.by_year_day[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
      out.append(QString::number(i) + ' ');
    }
    kDebug(5800) << out << endl;
  }
  if (r.by_month[0] != ICAL_RECURRENCE_ARRAY_MAX) {
    int index = 0;
    QString out = " By Month: ";
    while((i = r.by_month[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
      out.append(QString::number(i) + ' ');
    }
    kDebug(5800) << out << endl;
  }
  if (r.by_set_pos[0] != ICAL_RECURRENCE_ARRAY_MAX) {
    int index = 0;
    QString out = " By Set Pos: ";
    while((i = r.by_set_pos[index++]) != ICAL_RECURRENCE_ARRAY_MAX) {
      kDebug(5800) << "========= " << i << endl;
      out.append(QString::number(i) + ' ');
    }
    kDebug(5800) << out << endl;
  }
}

icalcomponent *ICalFormatImpl::createScheduleComponent(IncidenceBase *incidence,
                                                   Scheduler::Method method)
{
  icalcomponent *message = createCalendarComponent();

  icalproperty_method icalmethod = ICAL_METHOD_NONE;

  switch (method) {
    case Scheduler::Publish:
      icalmethod = ICAL_METHOD_PUBLISH;
      break;
    case Scheduler::Request:
      icalmethod = ICAL_METHOD_REQUEST;
      break;
    case Scheduler::Refresh:
      icalmethod = ICAL_METHOD_REFRESH;
      break;
    case Scheduler::Cancel:
      icalmethod = ICAL_METHOD_CANCEL;
      break;
    case Scheduler::Add:
      icalmethod = ICAL_METHOD_ADD;
      break;
    case Scheduler::Reply:
      icalmethod = ICAL_METHOD_REPLY;
      break;
    case Scheduler::Counter:
      icalmethod = ICAL_METHOD_COUNTER;
      break;
    case Scheduler::Declinecounter:
      icalmethod = ICAL_METHOD_DECLINECOUNTER;
      break;
    default:
      kDebug(5800) << "ICalFormat::createScheduleMessage(): Unknow method" << endl;
      return message;
  }

  icalcomponent_add_property(message,icalproperty_new_method(icalmethod));

  icalcomponent *inc = writeIncidence( incidence, method );
  /*
   * RFC 2446 states in section 3.4.3 ( REPLY to a VTODO ), that
   * a REQUEST-STATUS property has to be present. For the other two, event and
   * free busy, it can be there, but is optional. Until we do more
   * fine grained handling, assume all is well. Note that this is the
   * status of the _request_, not the attendee. Just to avoid confusion.
   * - till
   */
  if ( icalmethod == ICAL_METHOD_REPLY ) {
    struct icalreqstattype rst;
    rst.code = ICAL_2_0_SUCCESS_STATUS;
    rst.desc = 0;
    rst.debug = 0;
    icalcomponent_add_property( inc, icalproperty_new_requeststatus( rst ) );
  }
  icalcomponent_add_component( message, inc );

  return message;
}
