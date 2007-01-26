/*
    This file is part of the kcal library.

    Copyright (c) 1998 Preston Brown <pbrown@kde.org>
    Copyright (c) 2001,2003 Cornelius Schumacher <schumacher@kde.org>

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
  defines the ResourceLocal class.

  @author Preston Brown <pbrown@kde.org>
  @author Cornelius Schumacher <schumacher@kde.org>
*/

#ifndef KCAL_RESOURCELOCAL_H
#define KCAL_RESOURCELOCAL_H

#include <QString>

#include <kdatetime.h>
#include <kurl.h>
#include <kdirwatch.h>

class KConfig;

#include "calendarlocal.h"
#include "kcal.h"
#include "resourcecached.h"

namespace KCal {

class CalFormat;

/**
  @brief Provides a calendar resource stored as a local file.
*/
class KCAL_EXPORT ResourceLocal : public ResourceCached
{
    Q_OBJECT

    friend class ResourceLocalConfig;

  public:

    /**
      Constructs a resource from configuration information
      stored in a KConfig object.

      @param config the KConfig object to store as a resource.
    */
    explicit ResourceLocal( const KConfig *config );

    /**
      Constructs a resource for file named @p fileName.

      @param fileName the file to link to the resource.
    */
    explicit ResourceLocal( const QString &fileName );

    /**
      Destroys the resource.
    **/
    virtual ~ResourceLocal();

    /**
      Writes KConfig @p config to a local file.
    **/
    virtual void writeConfig( KConfig *config );

    /**
      Returns the lock.
    **/
    KABC::Lock *lock();

    /**
      Returns the fileName for this resource.

      @see setFileName()
    **/
    QString fileName() const;

    /**
      Sets the fileName for this resource. This will be the local
      file where the resource data will be stored.

      @see fileName()
    **/
    bool setFileName( const QString &fileName );

    /**
      Sets a value for this resource.

      @param key the distinct name for this value.
      @param value the actual data for this value.
    **/
    bool setValue( const QString &key, const QString &value );

    /**
      Dumps the resource.
    **/
    void dump() const;

  protected slots:

    /**
      Reload the resource data from the local file.
    **/
    void reload();

  protected:

    /**
      Actually loads the data from the local file.
    **/
    virtual bool doLoad( bool syncCache );

    /**
      Actually saves the data to the local file.
    **/
    virtual bool doSave( bool syncCache );

    /**
      Called by reload() to reload the resource, if it is already open.
      @return true if successful, else false. If true is returned,
              reload() will emit a resourceChanged() signal.

      @see doLoad(), doSave()
    */
    virtual bool doReload();

    /**
      Returns the date/time the local file was last modified.

      @see doSave()
    **/
    KDateTime readLastModified();

    /**
      Compares this ResourceLocal and @p other for equality.
      Returns true if they are equal.
    **/
    bool operator==( const ResourceLocal &other );

    /**
      Sets this ResourceLocal equal to @p other.
    **/
    ResourceLocal &operator=( const ResourceLocal &other );

  private:
    void init();
    //@cond PRIVATE
    class Private;
    Private *d;
    //@endcond
};

}

class KCal::ResourceLocal::Private
{
  public:
    KUrl mURL;
    CalFormat *mFormat;
    KDirWatch mDirWatch;
    KABC::Lock *mLock;
    KDateTime mLastModified;
};

#endif
