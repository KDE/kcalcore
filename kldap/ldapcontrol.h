/*
  This file is part of libkldap.
  Copyright (c) 2004-2006 Szombathelyi György <gyurco@freemail.hu>
        
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General  Public
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

#ifndef KLDAP_LDAPCONTROL_H
#define KLDAP_LDAPCONTROL_H

#include <QString>
#include <QList>

#include <kldap/kldap.h>

namespace KLDAP {

  /** This class represents an LDAP Control
   */
  class KLDAP_EXPORT LdapControl
  {
    public:
      /**
       * Creates an empty control.
       */
      LdapControl();
      /**
       * Creates a control with the given OID, value and criticality.
       */
      LdapControl( QString &oid, QByteArray &value, bool critical = false );

      LdapControl( const LdapControl &that );
      LdapControl& operator= (const LdapControl &that);
      /**
       * Destroys the control object.
       */
      virtual ~LdapControl();
      /**
       * Sets the control's OID, value and criticality.
       */
      void setControl( const QString &oid, const QByteArray &value, bool critical = false);
      /**
       * Sets the control's OID.
       */
      void setOid( const QString &oid );
      /**
       * Sets the control's value.
       */
      void setValue( const QByteArray &value );
      /**
       * Sets the control's criticality.
       */
      void setCritical( bool critical );
      /**
       * Returns the control's OID.
       */
      QString oid() const;
      /**
       * Returns the control's value.
       */
      QByteArray value() const;
      /**
       * Returns the control's criticality.
       */
      bool critical() const;

    private:
      
      QString mOid;
      QByteArray mValue;
      bool mCritical;
      
      class LdapControlPrivate;
      LdapControlPrivate *d;
  };

  typedef QList<LdapControl> LdapControls;

}
#endif
