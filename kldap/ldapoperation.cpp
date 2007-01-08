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

#include "ldapoperation.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <kdebug.h>

#include <kldap_config.h>

#ifdef LDAP_FOUND
#include <ldap.h>
#include <lber.h>
#endif

using namespace KLDAP;

class LdapOperation::LdapOperationPrivate {
  public:
  LdapControls mClientCtrls, mServerCtrls, mControls;
  LdapObject mObject;
  QByteArray mExtOid, mExtData;
  QString mMatchedDn;
  QList<QByteArray> mReferrals;

  LdapConnection *mConnection;
};

LdapOperation::LdapOperation()
  : d( new LdapOperationPrivate )
{
  d->mConnection = 0;
}

LdapOperation::LdapOperation( LdapConnection &conn )
  : d( new LdapOperationPrivate )
{
  setConnection( conn );
}

LdapOperation::~LdapOperation()
{
  delete d;
}

void LdapOperation::setConnection( LdapConnection &conn )
{
  d->mConnection = &conn;
}

LdapConnection &LdapOperation::connection()
{
  return *d->mConnection;
}

void LdapOperation::setClientControls( const LdapControls &ctrls )
{
  d->mClientCtrls = ctrls;
}

void LdapOperation::setServerControls( const LdapControls &ctrls )
{
  d->mServerCtrls = ctrls;
}

const LdapControls &LdapOperation::clientControls() const
{
  return d->mClientCtrls;
}

const LdapControls &LdapOperation::serverControls() const
{
  return d->mServerCtrls;
}

const LdapObject &LdapOperation::object() const
{
  return d->mObject;
}

const LdapControls &LdapOperation::controls() const
{
  return d->mControls;
}

QByteArray LdapOperation::extendedOid() const
{
  return d->mExtOid;
}

QByteArray LdapOperation::extendedData() const
{
  return d->mExtData;
}

QString LdapOperation::matchedDn() const
{
  return d->mMatchedDn;
}

QList<QByteArray> LdapOperation::referrals() const
{
  return d->mReferrals;
}

#ifdef LDAP_FOUND

static void addModOp( LDAPMod ***pmods, int mod_type, const QString &attr,
                      const QByteArray &value )
{
  //  kDebug() << "type: " << mod_type << " attr: " << attr <<
  //    " value: " << QString::fromUtf8(value,value.size()) <<
  //    " size: " << value.size() << endl;
  LDAPMod **mods;

  mods = *pmods;

  uint i = 0;

  if ( mods == 0 ) {
    mods = (LDAPMod **) malloc ( 2 * sizeof( LDAPMod * ) );
    mods[ 0 ] = (LDAPMod *) malloc( sizeof( LDAPMod ) );
    mods[ 1 ] = 0;
    memset( mods[ 0 ], 0, sizeof( LDAPMod ) );
  } else {
    while ( mods[ i ] != 0 &&
            ( strcmp( attr.toUtf8(), mods[i]->mod_type ) != 0 ||
              ( mods[ i ]->mod_op & ~LDAP_MOD_BVALUES ) != mod_type ) ) i++;

    if ( mods[ i ] == 0 ) {
      mods = (LDAPMod **)realloc( mods, ( i + 2 ) * sizeof( LDAPMod * ) );
      if ( mods == 0 ) {
        kError() << "addModOp: realloc" << endl;
        return;
      }
      mods[ i + 1 ] = 0;
      mods[ i ] = (LDAPMod *) malloc( sizeof( LDAPMod ) );
      memset( mods[ i ], 0, sizeof( LDAPMod ) );
    }
  }

  mods[ i ]->mod_op = mod_type | LDAP_MOD_BVALUES;
  if ( mods[ i ]->mod_type == 0 ) {
    mods[ i ]->mod_type = strdup( attr.toUtf8() );
  }

  *pmods = mods;

  int vallen = value.size();
  if ( vallen == 0 ) {
    return;
  }
  BerValue *berval;
  berval = (BerValue*) malloc( sizeof( BerValue ) );
  berval -> bv_val = (char*) malloc( vallen );
  berval -> bv_len = vallen;
  memcpy( berval -> bv_val, value.data(), vallen );

  if ( mods[ i ] -> mod_vals.modv_bvals == 0 ) {
    mods[ i ]->mod_vals.modv_bvals =
      (BerValue**) malloc( sizeof( BerValue * ) * 2 );
    mods[ i ]->mod_vals.modv_bvals[ 0 ] = berval;
    mods[ i ]->mod_vals.modv_bvals[ 1 ] = 0;
    kDebug() << "addModOp: new bervalue struct " << endl;
  } else {
    uint j = 0;
    while ( mods[ i ]->mod_vals.modv_bvals[ j ] != 0 ) {
      j++;
    }
    mods[ i ]->mod_vals.modv_bvals =
      (BerValue **) realloc( mods[ i ]->mod_vals.modv_bvals,
                             ( j + 2 ) * sizeof( BerValue * ) );
    if ( mods[ i ]->mod_vals.modv_bvals == 0 ) {
      kError() << "addModOp: realloc" << endl;
      free( berval );
      return;
    }
    mods[ i ]->mod_vals.modv_bvals[ j ] = berval;
    mods[ i ]->mod_vals.modv_bvals[ j+1 ] = 0;
    kDebug() << j << ". new bervalue " << endl;
  }
}

static void addControlOp( LDAPControl ***pctrls, const QString &oid,
                          const QByteArray &value, bool critical )
{
  LDAPControl **ctrls;
  LDAPControl *ctrl = (LDAPControl *) malloc( sizeof( LDAPControl ) );

  ctrls = *pctrls;

  kDebug() << "addControlOp: oid:'" << oid << "' val: '" << value << "'" << endl;
  int vallen = value.size();
  ctrl->ldctl_value.bv_len = vallen;
  if ( vallen ) {
    ctrl->ldctl_value.bv_val = (char*) malloc( vallen );
    memcpy( ctrl->ldctl_value.bv_val, value.data(), vallen );
  } else {
    ctrl->ldctl_value.bv_val = 0;
  }
  ctrl->ldctl_iscritical = critical;
  ctrl->ldctl_oid = strdup( oid.toUtf8() );

  uint i = 0;

  if ( ctrls == 0 ) {
    ctrls = (LDAPControl **) malloc ( 2 * sizeof( LDAPControl * ) );
    ctrls[ 0 ] = 0;
    ctrls[ 1 ] = 0;
  } else {
    while ( ctrls[ i ] != 0 ) {
      i++;
    }
    ctrls[ i + 1 ] = 0;
    ctrls =
      (LDAPControl **) realloc( ctrls, ( i + 2 ) * sizeof( LDAPControl * ) );
  }
  ctrls[ i ] = ctrl;
  *pctrls = ctrls;
}

static void createControls( LDAPControl ***pctrls, const LdapControls &ctrls )
{
  for ( int i = 0; i< ctrls.count(); ++i ) {
    addControlOp( pctrls, ctrls[i].oid(), ctrls[i].value(), ctrls[i].critical() );
  }
}

static void extractControls( LdapControls &ctrls, LDAPControl **pctrls )
{
  LDAPControl *ctrl;
  LdapControl control;
  int i = 0;

  while ( pctrls[i] ) {
    ctrl = pctrls[ i ];
    control.setOid( QString::fromUtf8( ctrl->ldctl_oid ) );
    control.setValue( QByteArray( ctrl->ldctl_value.bv_val,
                                  ctrl->ldctl_value.bv_len ) );
    control.setCritical( ctrl->ldctl_iscritical );
    ctrls.append( control );
    i++;
  }
}

int LdapOperation::search( const QString &base, LdapUrl::Scope scope,
                           const QString &filter, const QStringList &attributes )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  char **attrs = 0;
  int msgid;

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  int count = attributes.count();
  if ( count > 0 ) {
    attrs = static_cast<char**>( malloc( ( count + 1 ) * sizeof( char * ) ) );
    for ( int i=0; i<count; i++ ) {
      attrs[i] = strdup( attributes.at(i).toUtf8() );
    }
    attrs[count] = 0;
  }

  int lscope = LDAP_SCOPE_BASE;
  switch ( scope ) {
  case LdapUrl::Base:
    lscope = LDAP_SCOPE_BASE;
    break;
  case LdapUrl::One:
    lscope = LDAP_SCOPE_ONELEVEL;
    break;
  case LdapUrl::Sub:
    lscope = LDAP_SCOPE_SUBTREE;
    break;
  }

  kDebug() << "asyncSearch() base=\"" << base << "\" scope=" << scope <<
    " filter=\"" << filter << "\" attrs=" << attributes << endl;
  int retval =
    ldap_search_ext( ld, base.toUtf8(), lscope,
                     filter.isEmpty() ? QByteArray("objectClass=*") : filter.toUtf8(),
                     attrs, 0, serverctrls, clientctrls, 0,
                     d->mConnection->sizeLimit(), &msgid );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  // free the attributes list again
  if ( count > 0 ) {
    for ( int i=0; i<count; i++ ) {
      free( attrs[i] );
    }
    free( attrs );
  }

  if ( retval == 0 ) {
    retval = msgid;
  }
  return retval;
}

int LdapOperation::add( const LdapObject &object )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  int msgid;
  LDAPMod **lmod = 0;

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  for ( LdapAttrMap::ConstIterator it = object.attributes().begin();
        it != object.attributes().end(); ++it ) {
    QString attr = it.key();
    for ( LdapAttrValue::ConstIterator it2 = (*it).begin(); it2 != (*it).end(); ++it2 ) {
      addModOp( &lmod, 0, attr, *it2 );
    }
  }

  int retval =
    ldap_add_ext( ld, object.dn().toString().toUtf8(), lmod, serverctrls,
                  clientctrls, &msgid );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );
  ldap_mods_free( lmod, 1 );
  if ( retval == 0 ) {
    retval = msgid;
  }
  return retval;
}

int LdapOperation::add_s( const LdapObject &object )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  LDAPMod **lmod = 0;

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  for ( LdapAttrMap::ConstIterator it = object.attributes().begin();
        it != object.attributes().end(); ++it ) {
    QString attr = it.key();
    for ( LdapAttrValue::ConstIterator it2 = (*it).begin(); it2 != (*it).end(); ++it2 ) {
      addModOp( &lmod, 0, attr, *it2 );
    }
  }

  int retval =
    ldap_add_ext_s( ld, object.dn().toString().toUtf8(), lmod, serverctrls,
                    clientctrls );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );
  ldap_mods_free( lmod, 1 );
  return retval;
}

int LdapOperation::rename( const QString &dn, const QString &newRdn,
                           const QString &newSuperior, bool deleteold )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  int msgid;

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  int retval = ldap_rename( ld, dn.toUtf8(), newRdn.toUtf8(),
                            newSuperior.isEmpty() ? (char *) 0 : newSuperior.toUtf8().data(),
                            deleteold, serverctrls, clientctrls, &msgid );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  if ( retval == 0 ) {
    retval = msgid;
  }
  return retval;
}

int LdapOperation::rename_s( const QString &dn, const QString &newRdn,
                             const QString &newSuperior, bool deleteold )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  int retval = ldap_rename_s( ld, dn.toUtf8(), newRdn.toUtf8(),
                              newSuperior.isEmpty() ? (char *) 0 : newSuperior.toUtf8().data(),
                              deleteold, serverctrls, clientctrls );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  return retval;
}

int LdapOperation::del( const QString &dn )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  int msgid;

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  int retval = ldap_delete_ext( ld, dn.toUtf8(), serverctrls, clientctrls, &msgid );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  if ( retval == 0 ) {
    retval = msgid;
  }
  return retval;
}

int LdapOperation::del_s( const QString &dn )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  int retval = ldap_delete_ext_s( ld, dn.toUtf8(), serverctrls, clientctrls );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  return retval;
}

int LdapOperation::modify( const QString &dn, const ModOps &ops )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  int msgid;
  LDAPMod **lmod = 0;

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  for ( int i = 0; i < ops.count(); ++i ) {
    int mtype = 0;
    switch ( ops[i].type ) {
    case Mod_None:
      mtype = 0;
      break;
    case Mod_Add:
      mtype = LDAP_MOD_ADD;
      break;
    case Mod_Replace:
      mtype = LDAP_MOD_REPLACE;
      break;
    case Mod_Del:
      mtype = LDAP_MOD_DELETE;
      break;
    }
    for ( int j = 0; j < ops[i].values.count(); ++j ) {
      addModOp( &lmod, mtype, ops[i].attr, ops[i].values[j] );
    }
  }

  int retval = ldap_modify_ext( ld, dn.toUtf8(), lmod, serverctrls, clientctrls, &msgid );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );
  ldap_mods_free( lmod, 1 );
  if ( retval == 0 ) {
    retval = msgid;
  }
  return retval;
}

int LdapOperation::modify_s( const QString &dn, const ModOps &ops )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  LDAPMod **lmod = 0;

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  for ( int i = 0; i < ops.count(); ++i ) {
    int mtype = 0;
    switch ( ops[i].type ) {
    case Mod_None:
      mtype = 0;
      break;
    case Mod_Add:
      mtype = LDAP_MOD_ADD;
      break;
    case Mod_Replace:
      mtype = LDAP_MOD_REPLACE;
      break;
    case Mod_Del:
      mtype = LDAP_MOD_DELETE;
      break;
    }
    for ( int j = 0; j < ops[i].values.count(); ++j ) {
      addModOp( &lmod, mtype, ops[i].attr, ops[i].values[j] );
    }
  }

  int retval = ldap_modify_ext_s( ld, dn.toUtf8(), lmod, serverctrls, clientctrls );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );
  ldap_mods_free( lmod, 1 );
  return retval;
}

int LdapOperation::compare( const QString &dn, const QString &attr, const QByteArray &value )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();
  int msgid;

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  int vallen = value.size();
  BerValue *berval;
  berval = (BerValue*) malloc( sizeof( BerValue ) );
  berval -> bv_val = (char*) malloc( vallen );
  berval -> bv_len = vallen;
  memcpy( berval -> bv_val, value.data(), vallen );

  int retval = ldap_compare_ext( ld, dn.toUtf8(), attr.toUtf8(), berval,
                                 serverctrls, clientctrls, &msgid );

  ber_bvfree( berval );
  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  if ( retval == 0 ) {
    retval = msgid;
  }
  return retval;
}

int LdapOperation::compare_s( const QString &dn, const QString &attr, const QByteArray &value )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  int vallen = value.size();
  BerValue *berval;
  berval = (BerValue*) malloc( sizeof( BerValue ) );
  berval -> bv_val = (char*) malloc( vallen );
  berval -> bv_len = vallen;
  memcpy( berval -> bv_val, value.data(), vallen );

  int retval = ldap_compare_ext_s( ld, dn.toUtf8(), attr.toUtf8(), berval,
                                   serverctrls, clientctrls );

  ber_bvfree( berval );
  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  return retval;
}

int LdapOperation::exop( const QString &oid, const QByteArray &data )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();
  int msgid;

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  int vallen = data.size();
  BerValue *berval;
  berval = (BerValue*) malloc( sizeof( BerValue ) );
  berval -> bv_val = (char*) malloc( vallen );
  berval -> bv_len = vallen;
  memcpy( berval -> bv_val, data.data(), vallen );

  int retval = ldap_extended_operation( ld, oid.toUtf8(), berval,
                                        serverctrls, clientctrls, &msgid );

  ber_bvfree( berval );
  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  if ( retval == 0 ) {
    retval = msgid;
  }
  return retval;
}

int LdapOperation::exop_s( const QString &oid, const QByteArray &data )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();
  BerValue *retdata;
  char *retoid;

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  int vallen = data.size();
  BerValue *berval;
  berval = (BerValue*) malloc( sizeof( BerValue ) );
  berval -> bv_val = (char*) malloc( vallen );
  berval -> bv_len = vallen;
  memcpy( berval -> bv_val, data.data(), vallen );

  int retval = ldap_extended_operation_s( ld, oid.toUtf8(), berval,
                                          serverctrls, clientctrls, &retoid, &retdata );

  ber_bvfree( berval );
  ber_bvfree( retdata );
  free( retoid );
  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  return retval;
}

int LdapOperation::abandon( int id )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  LDAPControl **serverctrls = 0, **clientctrls = 0;
  createControls( &serverctrls, d->mServerCtrls );
  createControls( &serverctrls, d->mClientCtrls );

  int retval = ldap_abandon_ext( ld, id, serverctrls, clientctrls );

  ldap_controls_free( serverctrls );
  ldap_controls_free( clientctrls );

  return retval;
}

int LdapOperation::result( int id )
{
  Q_ASSERT( d->mConnection );
  LDAP *ld = (LDAP*) d->mConnection->handle();

  LDAPMessage *msg;
  int rescode, retval;

  rescode = ldap_result( ld, id, 0, NULL, &msg );
  if ( rescode == -1 ) {
    return -1;
  }

  switch ( rescode ) {
  case RES_SEARCH_ENTRY:
  {
    d->mObject.clear();
    LdapAttrMap attrs;
    char *name;
    struct berval **bvals;
    BerElement     *entry;

    char *dn = ldap_get_dn( ld, msg );
    d->mObject.setDn( QString::fromUtf8( dn ) );
    ldap_memfree( dn );

    // iterate over the attributes
    name = ldap_first_attribute( ld, msg, &entry );
    while ( name != 0 ) {
      // print the values
      bvals = ldap_get_values_len( ld, msg, name );
      LdapAttrValue values;
      if ( bvals ) {
        for ( int i = 0; bvals[i] != 0; i++ ) {
          char *val = bvals[i]->bv_val;
          unsigned long len = bvals[i]->bv_len;
          values.append( QByteArray( val, len ) );
        }
        ldap_value_free_len( bvals );
      }
      attrs[ QString::fromLatin1( name ) ] = values;
      // next attribute
      name = ldap_next_attribute( ld, msg, entry );
    }
    ber_free( entry , 0 );
    d->mObject.setAttributes( attrs );
    break;
  }
  case RES_EXTENDED:
  {
    char *retoid;
    struct berval *retdata;
    retval = ldap_parse_extended_result( ld, msg, &retoid, &retdata, 0 );
    if ( retval != LDAP_SUCCESS ) {
      ldap_msgfree( msg );
      return -1;
    }
    d->mExtOid = retoid ? QByteArray( retoid ) : QByteArray();
    d->mExtData = retdata ? QByteArray( retdata->bv_val, retdata->bv_len ) : QByteArray();
    ldap_memfree( retoid );
    ber_bvfree( retdata );
    break;
  }
  default:
  {
    LDAPControl **serverctrls = 0;
    char *matcheddn = 0, *errmsg = 0;
    char **referralsp;
    int errcodep;
    retval =
      ldap_parse_result( ld, msg, &errcodep, &matcheddn, &errmsg, &referralsp,
                         &serverctrls, 0 );
    kDebug() << "rescode " << rescode << " retval: " << retval
             << " matcheddn: " << matcheddn << " errcode: "
             << errcodep << " errmsg: " << errmsg << endl;
    if ( retval != LDAP_SUCCESS ) {
      ldap_msgfree( msg );
      return -1;
    }
    d->mControls.clear();
    if ( serverctrls ) {
      extractControls( d->mControls, serverctrls );
      ldap_controls_free( serverctrls );
    }
    d->mReferrals.clear();
    if ( referralsp ) {
      char **tmp = referralsp;
      while ( *tmp ) {
        d->mReferrals.append( QByteArray( *tmp ) );
        tmp++;
      }
      ber_memvfree( (void **) referralsp );
    }
    d->mMatchedDn = QString();
    if ( matcheddn ) {
      d->mMatchedDn = QString::fromUtf8( matcheddn );
      ldap_memfree( matcheddn );
    }
    if ( errmsg ) {
      ldap_memfree( errmsg );
    }
  }
  }

  ldap_msgfree( msg );

  return rescode;
}

#else
int LdapOperation::search( const QString &base, LdapUrl::Scope scope,
                           const QString &filter, const QStringList &attributes )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::add( const LdapObject &object )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::add_s( const LdapObject &object )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::rename( const QString &dn, const QString &newRdn,
                           const QString &newSuperior, bool deleteold )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::rename_s( const QString &dn, const QString &newRdn,
                             const QString &newSuperior, bool deleteold )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::del( const QString &dn )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::del_s( const QString &dn )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::modify( const QString &dn, const ModOps &ops )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::modify_s( const QString &dn, const ModOps &ops )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::compare( const QString &dn, const QString &attr, const QByteArray &value )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::exop( const QString &oid, const QByteArray &data )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::compare_s( const QString &dn, const QString &attr, const QByteArray &value )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::exop_s( const QString &oid, const QByteArray &data )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::result( int id )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

int LdapOperation::abandon( int id )
{
  kError() << "LDAP support not compiled" << endl;
  return -1;
}

#endif
