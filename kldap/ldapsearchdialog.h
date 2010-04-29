/*
 * This file is part of libkldap.
 *
 * Copyright (C) 2002 Klarälvdalens Datakonsult AB
 *
 * Author: Steffen Hansen <hansen@kde.org>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef KLDAP_LDAPSEARCHDIALOG_H
#define KLDAP_LDAPSEARCHDIALOG_H

#include "kldap_export.h"

#include <kabc/addressee.h>
#include <kdialog.h>

class QCloseEvent;

namespace KLDAP {

class LdapClient;
class LdapObject;

/**
 * @short A dialog to search contacts in a LDAP directory.
 *
 * This dialog allows the user to search for contacts inside
 * a LDAP directory.
 *
 * @author Steffen Hansen <hansen@kde.org>
 * @since 4.5
 */
class KLDAP_EXPORT LdapSearchDialog : public KDialog
{
  Q_OBJECT

  public:
    /**
     * Creates a new ldap search dialog.
     *
     * @param parent The parent widget.
     */
    LdapSearchDialog( QWidget *parent = 0 );

    /**
     * Destroys the ldap search dialog.
     */
    ~LdapSearchDialog();

    /**
     * Sets the @p text in the search line edit.
     */
    void setSearchText( const QString &text );

    /**
     * Returns a list of contacts that have been selected
     * in the LDAP search.
     */
    KABC::Addressee::List selectedContacts() const;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the user clicked the
     * 'Add Selected' button.
     */
    void contactsAdded();

  protected Q_SLOTS:
    virtual void slotUser1();
    virtual void slotUser2();

  protected:
    virtual void closeEvent( QCloseEvent* );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void slotAddResult( const KLDAP::LdapClient&, const KLDAP::LdapObject& ) )
    Q_PRIVATE_SLOT( d, void slotSetScope( bool ) )
    Q_PRIVATE_SLOT( d, void slotStartSearch() )
    Q_PRIVATE_SLOT( d, void slotStopSearch() )
    Q_PRIVATE_SLOT( d, void slotSearchDone() )
    Q_PRIVATE_SLOT( d, void slotError( const QString& ) )
    Q_PRIVATE_SLOT( d, void slotSelectAll() )
    Q_PRIVATE_SLOT( d, void slotUnselectAll() )
    Q_PRIVATE_SLOT( d, void slotSelectionChanged() )
    //@endcond
};

}

#endif
