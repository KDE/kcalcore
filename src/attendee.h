/*
  This file is part of the kcalcore library.

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
  defines the Attendee class.

  @author Cornelius Schumacher \<schumacher@kde.org\>
*/

#ifndef KCALCORE_ATTENDEE_H
#define KCALCORE_ATTENDEE_H

#include <QMetaType>
#include <QSharedDataPointer>

#include "kcalcore_export.h"
#include "customproperties.h"

namespace KCalCore
{

/**
  @brief
  Represents information related to an attendee of an Calendar Incidence,
  typically a meeting or task (to-do).

  Attendees are people with a name and (optional) email address who are
  invited to participate in some way in a meeting or task.  This class
  also tracks that status of the invitation: accepted; tentatively accepted;
  declined; delegated to another person; in-progress; completed.

  Attendees may optionally be asked to @acronym RSVP ("Respond Please") to
  the invitation.

  Note that each attendee be can optionally associated with a @acronym UID
  (unique identifier) derived from a Calendar Incidence, Email Message,
  or any other thing you want.
*/
class KCALCORE_EXPORT Attendee
{
public:
    /**
      The different types of participant status.
      The meaning is specific to the incidence type in context.
    */
    enum PartStat {
        NeedsAction,     /**< Event, to-do or journal needs action (default) */
        Accepted,        /**< Event, to-do or journal accepted */
        Declined,        /**< Event, to-do or journal declined */
        Tentative,       /**< Event or to-do tentatively accepted */
        Delegated,       /**< Event or to-do delegated */
        Completed,       /**< To-do completed */
        InProcess,       /**< To-do in process of being completed */
        None
    };

    /**
      The different types of participation roles.
    */
    enum Role {
        ReqParticipant,  /**< Participation is required (default) */
        OptParticipant,  /**< Participation is optional */
        NonParticipant,  /**< Non-Participant; copied for information purposes */
        Chair            /**< Chairperson */
    };

    /**
     * The different types of a participant.
     *
     * @since 4.14
     */
    enum CuType {
        Individual,       /**< An individual (default) */
        Group,            /**< A group of individuals */
        Resource,         /**< A physical resource */
        Room,             /**< A room resource */
        Unknown           /**< Otherwise not known */
        /**
         * Parameters that have to set via the QString variant of @setCuType() and @cuType()
         * x-name         ; Experimental cuType
         * iana-token     ; Other IANA-registered
         */
    };

    /**
      List of attendees.
    */
    typedef QVector<Attendee> List;

    /** Create a null Attendee. */
    Attendee();

    /**
      Constructs an attendee consisting of a person name (@p name) and
      email address (@p email); invitation status and #Role;
      an optional @acronym RSVP flag and @acronym UID.

      @param name is person name of the attendee.
      @param email is person email address of the attendee.
      @param rsvp if true, the attendee is requested to reply to invitations.
      @param status is the #PartStat status of the attendee.
      @param role is the #Role of the attendee.
      @param uid is the @acronym UID of the attendee.
    */
    Attendee(const QString &name, const QString &email,
             bool rsvp = false, PartStat status = None,
             Role role = ReqParticipant, const QString &uid = QString());

    /**
      Constructs an attendee by copying another attendee.

      @param attendee is the attendee to be copied.
    */
    Attendee(const Attendee &attendee);

    /**
      Destroys the attendee.
    */
    ~Attendee();

    /**
     * Returns @c true if this is a default-constructed Attendee instance.
     */
    bool isNull() const;

    /**
      Returns the name of the attendee.
    */
    Q_REQUIRED_RESULT QString name() const;
    /**
      Sets the name of the attendee to @p name.
    */
    void setName(const QString &name);

    /**
      Returns the full name and email address of this attendee
      @return A QString containing the person's full name in the form
        "FirstName LastName \<mail@domain\>".
    */
    Q_REQUIRED_RESULT QString fullName() const;

    /**
      Returns the email address for this attendee.
    */
    Q_REQUIRED_RESULT QString email() const;
    /**
      Sets the email address for this attendee to @p email.
    */
    void setEmail(const QString &email);

    /**
      Sets the Role of the attendee to @p role.

      @param role is the Role to use for the attendee.

      @see role()
    */
    void setRole(Role role);

    /**
      Returns the Role of the attendee.

      @see setRole()
    */
    Q_REQUIRED_RESULT Role role() const;

    /**
      Sets the @acronym UID of the attendee to @p uid.

      @param uid is the @acronym UID to use for the attendee.

      @see uid()
    */
    void setUid(const QString &uid);

    /**
      Returns the @acronym UID of the attendee.

      @see setUid()
    */
    Q_REQUIRED_RESULT QString uid() const;

    /**
      Sets the #PartStat of the attendee to @p status.

      @param status is the #PartStat to use for the attendee.

      @see status()
    */
    void setStatus(PartStat status);

    /**
      Returns the #PartStat of the attendee.

      @see setStatus()
    */
    Q_REQUIRED_RESULT PartStat status() const;

    /**
      Sets the #CuType of the attendee to @p cuType.

      @param cuType is the #CuType to use for the attendee.

      @see cuType()

      @since 4.14
    */
    void setCuType(CuType cuType);

    /**
      Sets the #CuType of the attendee to @p cuType.

      @param cuType is the #CuType to use for the attendee.

      @see cuType()

      @since 4.14
    */
    void setCuType(const QString &cuType);

    /**
      Returns the #CuType of the attendee.

      @see setCuType()

      @since 4.14
    */
    Q_REQUIRED_RESULT CuType cuType() const;

    /**
      Returns the #CuType of the attendee.

      @see setCuType()

      @since 4.14
    */
    Q_REQUIRED_RESULT QString cuTypeStr() const;

    /**
      Sets the @acronym RSVP flag of the attendee to @p rsvp.

      @param rsvp if set (true), the attendee is requested to reply to
      invitations.

      @see RSVP()
    */
    void setRSVP(bool rsvp);

    /**
      Returns the attendee @acronym RSVP flag.

      @see setRSVP()
    */
    Q_REQUIRED_RESULT bool RSVP() const;

    /**
      Compares this with @p attendee for equality.

      @param attendee the attendee to compare.
    */
    bool operator==(const Attendee &attendee) const;

    /**
      Compares this with @p attendee for inequality.

      @param attendee the attendee to compare.
    */
    bool operator!=(const Attendee &attendee) const;

    /**
      Sets the delegate.
      @param delegate is a string containing a MAILTO URI of those delegated
      to attend the meeting.
      @see delegate(), setDelegator().
    */
    void setDelegate(const QString &delegate);

    /**
      Returns the delegate.
      @see setDelegate().
    */
    Q_REQUIRED_RESULT QString delegate() const;

    /**
      Sets the delegator.
      @param delegator is a string containing a MAILTO URI of those who
      have delegated their meeting attendance.
      @see delegator(), setDelegate().
    */
    void setDelegator(const QString &delegator);

    /**
      Returns the delegator.
      @see setDelegator().
    */
    Q_REQUIRED_RESULT QString delegator() const;

    /**
      Adds a custom property. If the property already exists it will be overwritten.
      @param xname is the name of the property.
      @param xvalue is its value.
    */
    void setCustomProperty(const QByteArray &xname, const QString &xvalue);

    /**
      Returns a reference to the CustomProperties object
    */
    Q_REQUIRED_RESULT CustomProperties &customProperties();

    /**
      Returns a const reference to the CustomProperties object
    */
    const CustomProperties &customProperties() const;

    /**
      Sets this attendee equal to @p attendee.

      @param attendee is the attendee to copy.
    */
    Attendee &operator=(const Attendee &attendee);

private:
    //@cond PRIVATE
    class Private;
    QSharedDataPointer<Private> d;
    //@endcond

    friend KCALCORE_EXPORT QDataStream &operator<<(QDataStream &s,
            const KCalCore::Attendee &attendee);
    friend KCALCORE_EXPORT QDataStream &operator>>(QDataStream &s,
            KCalCore::Attendee &attendee);
};

/**
  Serializes an Attendee object into a data stream.
  @param stream is a QDataStream.
  @param attendee is a pointer to a Attendee object to be serialized.
*/
KCALCORE_EXPORT QDataStream &operator<<(QDataStream &stream,
                                        const KCalCore::Attendee &attendee);

/**
  Initializes an Attendee object from a data stream.
  @param stream is a QDataStream.
  @param attendee is a pointer to a Attendee object to be initialized.
*/
KCALCORE_EXPORT QDataStream &operator>>(QDataStream &stream,
                                        KCalCore::Attendee &attendee);
}

//@cond PRIVATE
Q_DECLARE_TYPEINFO(KCalCore::Attendee, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(KCalCore::Attendee)
//@endcond

#endif
