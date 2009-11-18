/*
    This file is part of the kcal library.

    Copyright (c) 1998 Preston Brown <pbrown@kde.org>
    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2003 David Jarvie <software@astrojar.org.uk>

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
  defines the Alarm class.

  @brief
  Represents an alarm notification.

  @author Cornelius Schumacher \<schumacher@kde.org\>
*/

#include "alarm.h"
#include "incidence.h"
#include "todo.h"

#include <kdebug.h>

using namespace KCal;

/**
  Private class that helps to provide binary compatibility between releases.
  @internal
*/
//@cond PRIVATE
class KCal::Alarm::Private
{
  public:
    Private()
      : mParent( 0 ),
        mType( Alarm::Invalid ),
        mAlarmSnoozeTime( 5 ),
        mAlarmRepeatCount( 0 ),
        mEndOffset( false ),
        mHasTime( false ),
        mAlarmEnabled( false )
    {}
    Private( const Private &other )
      : mParent( other.mParent ),
        mType( other.mType ),
        mDescription( other.mDescription ),
        mFile( other.mFile ),
        mMailSubject( other.mMailSubject ),
        mMailAttachFiles( other.mMailAttachFiles ),
        mMailAddresses( other.mMailAddresses ),
        mAlarmTime( other.mAlarmTime ),
        mAlarmSnoozeTime( other.mAlarmSnoozeTime ),
        mAlarmRepeatCount( other.mAlarmRepeatCount ),
        mOffset( other.mOffset ),
        mEndOffset( other.mEndOffset ),
        mHasTime( other.mHasTime ),
        mAlarmEnabled( other.mAlarmEnabled )
    {}

    Incidence *mParent;  // the incidence which this alarm belongs to

    Type mType;          // type of alarm
    QString mDescription;// text to display/email body/procedure arguments
    QString mFile;       // program to run/optional audio file to play
    QString mMailSubject;// subject of email
    QStringList mMailAttachFiles; // filenames to attach to email
    QList<Person> mMailAddresses; // who to mail for reminder

    KDateTime mAlarmTime;// time at which to trigger the alarm
    Duration mAlarmSnoozeTime; // how long after alarm to snooze before
                               // triggering again
    int mAlarmRepeatCount;// number of times for alarm to repeat
                          // after the initial time

    Duration mOffset;    // time relative to incidence DTSTART
                         // to trigger the alarm
    bool mEndOffset;     // if true, mOffset relates to DTEND, not DTSTART
    bool mHasTime;       // use mAlarmTime, not mOffset
    bool mAlarmEnabled;
};
//@endcond

Alarm::Alarm( Incidence *parent ) : d( new KCal::Alarm::Private )
{
  d->mParent = parent;
}

Alarm::Alarm( const Alarm &other ) :
  CustomProperties( other ), d( new KCal::Alarm::Private( *other.d ) )
{
}

Alarm::~Alarm()
{
  delete d;
}

bool Alarm::operator==( const Alarm &rhs ) const
{
  if ( d->mType != rhs.d->mType ||
       d->mAlarmSnoozeTime != rhs.d->mAlarmSnoozeTime ||
       d->mAlarmRepeatCount != rhs.d->mAlarmRepeatCount ||
       d->mAlarmEnabled != rhs.d->mAlarmEnabled ||
       d->mHasTime != rhs.d->mHasTime ) {
    return false;
  }

  if ( d->mHasTime ) {
    if ( d->mAlarmTime != rhs.d->mAlarmTime ) {
      return false;
    }
  } else {
    if ( d->mOffset != rhs.d->mOffset || d->mEndOffset != rhs.d->mEndOffset ) {
      return false;
    }
  }

  switch ( d->mType ) {
    case Display:
      return d->mDescription == rhs.d->mDescription;

    case Email:
      return d->mDescription == rhs.d->mDescription &&
             d->mMailAttachFiles == rhs.d->mMailAttachFiles &&
             d->mMailAddresses == rhs.d->mMailAddresses &&
             d->mMailSubject == rhs.d->mMailSubject;

    case Procedure:
      return d->mFile == rhs.d->mFile &&
             d->mDescription == rhs.d->mDescription;

    case Audio:
      return d->mFile == rhs.d->mFile;

    case Invalid:
      break;
  }
  return false;
}

void Alarm::setType( Alarm::Type type )
{
  if ( type == d->mType ) {
    return;
  }

  switch ( type ) {
    case Display:
      d->mDescription = "";
      break;
    case Procedure:
      d->mFile = d->mDescription = "";
      break;
    case Audio:
      d->mFile = "";
      break;
    case Email:
      d->mMailSubject = d->mDescription = "";
      d->mMailAddresses.clear();
      d->mMailAttachFiles.clear();
      break;
    case Invalid:
      break;
    default:
      return;
  }
  d->mType = type;
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

Alarm::Type Alarm::type() const
{
  return d->mType;
}

void Alarm::setAudioAlarm( const QString &audioFile )
{
  d->mType = Audio;
  d->mFile = audioFile;
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

void Alarm::setAudioFile( const QString &audioFile )
{
  if ( d->mType == Audio ) {
    d->mFile = audioFile;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

QString Alarm::audioFile() const
{
  return ( d->mType == Audio ) ? d->mFile : QString();
}

void Alarm::setProcedureAlarm( const QString &programFile,
                               const QString &arguments )
{
  d->mType = Procedure;
  d->mFile = programFile;
  d->mDescription = arguments;
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

void Alarm::setProgramFile( const QString &programFile )
{
  if ( d->mType == Procedure ) {
    d->mFile = programFile;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

QString Alarm::programFile() const
{
  return ( d->mType == Procedure ) ? d->mFile : QString();
}

void Alarm::setProgramArguments( const QString &arguments )
{
  if ( d->mType == Procedure ) {
    d->mDescription = arguments;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

QString Alarm::programArguments() const
{
  return ( d->mType == Procedure ) ? d->mDescription : QString();
}

void Alarm::setEmailAlarm( const QString &subject, const QString &text,
                           const QList<Person> &addressees,
                           const QStringList &attachments )
{
  d->mType = Email;
  d->mMailSubject = subject;
  d->mDescription = text;
  d->mMailAddresses = addressees;
  d->mMailAttachFiles = attachments;
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

void Alarm::setMailAddress( const Person &mailAddress )
{
  if ( d->mType == Email ) {
    d->mMailAddresses.clear();
    d->mMailAddresses += mailAddress;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

void Alarm::setMailAddresses( const QList<Person> &mailAddresses )
{
  if ( d->mType == Email ) {
    d->mMailAddresses = mailAddresses;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

void Alarm::addMailAddress( const Person &mailAddress )
{
  if ( d->mType == Email ) {
    d->mMailAddresses += mailAddress;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

QList<Person> Alarm::mailAddresses() const
{
  return ( d->mType == Email ) ? d->mMailAddresses : QList<Person>();
}

void Alarm::setMailSubject( const QString &mailAlarmSubject )
{
  if ( d->mType == Email ) {
    d->mMailSubject = mailAlarmSubject;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

QString Alarm::mailSubject() const
{
  return ( d->mType == Email ) ? d->mMailSubject : QString();
}

void Alarm::setMailAttachment( const QString &mailAttachFile )
{
  if ( d->mType == Email ) {
    d->mMailAttachFiles.clear();
    d->mMailAttachFiles += mailAttachFile;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

void Alarm::setMailAttachments( const QStringList &mailAttachFiles )
{
  if ( d->mType == Email ) {
    d->mMailAttachFiles = mailAttachFiles;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

void Alarm::addMailAttachment( const QString &mailAttachFile )
{
  if ( d->mType == Email ) {
    d->mMailAttachFiles += mailAttachFile;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

QStringList Alarm::mailAttachments() const
{
  return ( d->mType == Email ) ? d->mMailAttachFiles : QStringList();
}

void Alarm::setMailText( const QString &text )
{
  if ( d->mType == Email ) {
    d->mDescription = text;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

QString Alarm::mailText() const
{
  return ( d->mType == Email ) ? d->mDescription : QString();
}

void Alarm::setDisplayAlarm( const QString &text )
{
  d->mType = Display;
  if ( !text.isNull() ) {
    d->mDescription = text;
  }
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

void Alarm::setText( const QString &text )
{
  if ( d->mType == Display ) {
    d->mDescription = text;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

QString Alarm::text() const
{
  return ( d->mType == Display ) ? d->mDescription : QString();
}

void Alarm::setTime( const KDateTime &alarmTime )
{
  d->mAlarmTime = alarmTime;
  d->mHasTime = true;

  if ( d->mParent ) {
    d->mParent->updated();
  }
}

KDateTime Alarm::time() const
{
  if ( hasTime() ) {
    return d->mAlarmTime;
  } else if ( d->mParent ) {
    if ( d->mEndOffset ) {
      if ( d->mParent->type() == "Todo" ) {
        Todo *t = static_cast<Todo*>( d->mParent );
        return d->mOffset.end( t->dtDue() );
      } else {
        return d->mOffset.end( d->mParent->dtEnd() );
      }
    } else {
      return d->mOffset.end( d->mParent->dtStart() );
    }
  } else {
    return KDateTime();
  }
}

bool Alarm::hasTime() const
{
  return d->mHasTime;
}

void Alarm::shiftTimes( const KDateTime::Spec &oldSpec,
                        const KDateTime::Spec &newSpec )
{
  d->mAlarmTime = d->mAlarmTime.toTimeSpec( oldSpec );
  d->mAlarmTime.setTimeSpec( newSpec );
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

void Alarm::setSnoozeTime( const Duration &alarmSnoozeTime )
{
  if ( alarmSnoozeTime.value() > 0 ) {
    d->mAlarmSnoozeTime = alarmSnoozeTime;
    if ( d->mParent ) {
      d->mParent->updated();
    }
  }
}

Duration Alarm::snoozeTime() const
{
  return d->mAlarmSnoozeTime;
}

void Alarm::setRepeatCount( int alarmRepeatCount )
{
  d->mAlarmRepeatCount = alarmRepeatCount;
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

int Alarm::repeatCount() const
{
  return d->mAlarmRepeatCount;
}

Duration Alarm::duration() const
{
  return Duration( d->mAlarmSnoozeTime.value() * d->mAlarmRepeatCount,
                   d->mAlarmSnoozeTime.type() );
}

KDateTime Alarm::nextRepetition( const KDateTime &preTime ) const
{
  KDateTime at = time();
  if ( at > preTime ) {
    return at;
  }
  if ( !d->mAlarmRepeatCount ) {
    // there isn't an occurrence after the specified time
    return KDateTime();
  }
  qint64 repetition;
  int interval = d->mAlarmSnoozeTime.value();
  bool daily = d->mAlarmSnoozeTime.isDaily();
  if ( daily ) {
    int daysTo = at.daysTo( preTime );
    if ( !preTime.isDateOnly() && preTime.time() <= at.time() ) {
      --daysTo;
    }
    repetition = daysTo / interval + 1;
  } else {
    repetition = at.secsTo_long( preTime ) / interval + 1;
  }
  if ( repetition > d->mAlarmRepeatCount ) {
    // all repetitions have finished before the specified time
    return KDateTime();
  }
  return daily ? at.addDays( int( repetition * interval ) )
               : at.addSecs( repetition * interval );
}

KDateTime Alarm::previousRepetition( const KDateTime &afterTime ) const
{
  KDateTime at = time();
  if ( at >= afterTime ) {
    // alarm's first/only time is at/after the specified time
    return KDateTime();
  }
  if ( !d->mAlarmRepeatCount ) {
    return at;
  }
  qint64 repetition;
  int interval = d->mAlarmSnoozeTime.value();
  bool daily = d->mAlarmSnoozeTime.isDaily();
  if ( daily ) {
    int daysTo = at.daysTo( afterTime );
    if ( afterTime.isDateOnly() || afterTime.time() <= at.time() ) {
      --daysTo;
    }
    repetition = daysTo / interval;
  } else {
    repetition = ( at.secsTo_long( afterTime ) - 1 ) / interval;
  }
  if ( repetition > d->mAlarmRepeatCount ) {
    repetition = d->mAlarmRepeatCount;
  }
  return daily ? at.addDays( int( repetition * interval ) )
               : at.addSecs( repetition * interval );
}

KDateTime Alarm::endTime() const
{
  if ( !d->mAlarmRepeatCount ) {
    return time();
  }
  if ( d->mAlarmSnoozeTime.isDaily() ) {
    return time().addDays( d->mAlarmRepeatCount * d->mAlarmSnoozeTime.asDays() );
  } else {
    return time().addSecs( d->mAlarmRepeatCount * d->mAlarmSnoozeTime.asSeconds() );
  }
}

void Alarm::toggleAlarm()
{
  d->mAlarmEnabled = !d->mAlarmEnabled;
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

void Alarm::setEnabled( bool enable )
{
  d->mAlarmEnabled = enable;
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

bool Alarm::enabled() const
{
  return d->mAlarmEnabled;
}

void Alarm::setStartOffset( const Duration &offset )
{
  d->mOffset = offset;
  d->mEndOffset = false;
  d->mHasTime = false;
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

Duration Alarm::startOffset() const
{
  return ( d->mHasTime || d->mEndOffset ) ? Duration( 0 ) : d->mOffset;
}

bool Alarm::hasStartOffset() const
{
  return !d->mHasTime && !d->mEndOffset;
}

bool Alarm::hasEndOffset() const
{
  return !d->mHasTime && d->mEndOffset;
}

void Alarm::setEndOffset( const Duration &offset )
{
  d->mOffset = offset;
  d->mEndOffset = true;
  d->mHasTime = false;
  if ( d->mParent ) {
    d->mParent->updated();
  }
}

Duration Alarm::endOffset() const
{
  return ( d->mHasTime || !d->mEndOffset ) ? Duration( 0 ) : d->mOffset;
}

void Alarm::setParent( Incidence *parent )
{
  d->mParent = parent;
}

Incidence *Alarm::parent() const
{
  return d->mParent;
}

void Alarm::customPropertyUpdated()
{
  if ( d->mParent ) {
    d->mParent->updated();
  }
}
