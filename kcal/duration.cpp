/*
    This file is part of the kcal library.

    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2007 David Jarvie <software@astrojar.org.uk>

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
  defines the Duration class.

  @brief
  Represents a span of time measured in seconds.

  @author Cornelius Schumacher
*/

#include "duration.h"

#include <kdatetime.h>

using namespace KCal;

/**
  Private class that helps to provide binary compatibility between releases.
  @internal
*/
//@cond PRIVATE
class KCal::Duration::Private
{
  public:
    int seconds() const { return mDaily ? mDuration * 86400 : mDuration; }
    int mDuration; // number of seconds or days in the duration
    bool mDaily;   // specified in terms of days rather than seconds
};
//@endcond

Duration::Duration()
  : d( new KCal::Duration::Private() )
{
}

Duration::Duration( const KDateTime &start, const KDateTime &end )
  : d( new KCal::Duration::Private() )
{
  if ( start.time() == end.time() && start.timeSpec() == end.timeSpec() ) {
    d->mDuration = start.daysTo( end );
    d->mDaily = true;
  } else {
    d->mDuration = start.secsTo( end );
    d->mDaily = false;
  }
}

Duration::Duration( const KDateTime &start, const KDateTime &end, Type type )
  : d( new KCal::Duration::Private() )
{
  if ( type == Days ) {
    KDateTime endSt( end.toTimeSpec( start ) );
    d->mDuration = start.daysTo( endSt );
    if ( d->mDuration ) {
      // Round down to whole number of days if necessary
      if ( start < endSt ) {
        if ( endSt.time() < start.time() ) {
          --d->mDuration;
        }
      } else {
        if ( endSt.time() > start.time() ) {
          ++d->mDuration;
        }
      }
    }
    d->mDaily = true;
  } else {
    d->mDuration = start.secsTo( end );
    d->mDaily = false;
  }
}

Duration::Duration( int duration, Type type )
  : d( new KCal::Duration::Private() )
{
  d->mDuration = duration;
  d->mDaily = ( type == Days );
}

Duration::Duration( const Duration &duration )
  : d( new KCal::Duration::Private( *duration.d ) )
{
}

Duration::~Duration()
{
  delete d;
}

Duration &Duration::operator=( const Duration &duration )
{
  *d = *duration.d;
  return *this;
}

Duration::operator bool() const
{
  return d->mDuration;
}

bool Duration::operator<( const Duration &other ) const
{
  if ( d->mDaily == other.d->mDaily ) {
    // guard against integer overflow for two daily durations
    return d->mDuration < other.d->mDuration;
  }
  return d->seconds() < other.d->seconds();
}

bool Duration::operator>=( const Duration &other ) const
{
  return !operator<( other );
}

bool Duration::operator>( const Duration &other ) const
{
  return other.operator<( *this );
}

bool Duration::operator<=( const Duration &other ) const
{
  return !other.operator<( *this );
}

bool Duration::operator==( const Duration &other ) const
{
  if ( d->mDaily == other.d->mDaily ) {
    return d->mDuration == other.d->mDuration;
  } else if ( d->mDaily ) {
    return d->mDuration * 86400 == other.d->mDuration;
  } else {
    return d->mDuration == other.d->mDuration * 86400;
  }
}

bool Duration::operator!=( const Duration &other ) const
{
  return !operator==( other );
}

Duration &Duration::operator+=( const Duration &other )
{
  if ( d->mDaily == other.d->mDaily ) {
    d->mDuration += other.d->mDuration;
  } else if ( d->mDaily ) {
    d->mDuration = d->mDuration * 86400 + other.d->mDuration;
    d->mDaily = false;
  } else {
    d->mDuration += other.d->mDuration + 86400;
  }
  return *this;
}

Duration Duration::operator+( const Duration &other ) const
{
  return Duration( *this ) += other;
}

Duration Duration::operator-() const
{
  return Duration( -d->mDuration, ( d->mDaily ? Days : Seconds ) );
}

Duration &Duration::operator-=( const Duration &duration )
{
  return operator+=( -duration );
}

Duration Duration::operator-( const Duration &other ) const
{
  return Duration( *this ) -= other;
}

KDateTime Duration::end( const KDateTime &start ) const
{
  return d->mDaily ? start.addDays( d->mDuration )
                   : start.addSecs( d->mDuration );
}

Duration::Type Duration::type() const
{
  return d->mDaily ? Days : Seconds;
}

bool Duration::isDaily() const
{
  return d->mDaily;
}

int Duration::asSeconds() const
{
  return d->seconds();
}

int Duration::asDays() const
{
  return d->mDaily ? d->mDuration : d->mDuration / 86400;
}

int Duration::value() const
{
  return d->mDuration;
}
