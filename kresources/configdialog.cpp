/*
    This file is part of libkresources.

    Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2002 Jan-Pascal van Best <janpascal@vanbest.org>
    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

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

#include <klocale.h>
#include <klineedit.h>
#include <kmessagebox.h>

#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QCheckBox>

#include "factory.h"
#include "configdialog.h"

using namespace KRES;

ConfigDialog::ConfigDialog( QWidget *parent, const QString& resourceFamily,
    /*const QString& type,*/ Resource* resource, /*KConfig *config, */const char *name )
  : KDialog( parent )/*, mConfig( config )*/, mResource( resource )
{
  setObjectName( name );
  setModal( true );
  setCaption( i18n( "Resource Configuration" ) );
  setButtons( Ok | Cancel );
  setDefaultButton( Ok );
  enableButtonSeparator( false );

  Factory *factory = Factory::self( resourceFamily );

  QFrame *main = new QFrame( this );
  setMainWidget( main );

  QVBoxLayout *mainLayout = new QVBoxLayout( main );
  mainLayout->setSpacing( spacingHint() );

  QGroupBox *generalGroupBox = new QGroupBox( main );
  QGridLayout *gbLayout = new QGridLayout;
  gbLayout->setSpacing( spacingHint() );
  generalGroupBox->setLayout( gbLayout );
  
  generalGroupBox->setTitle( i18n( "General Settings" ) );

  gbLayout->addWidget( new QLabel( i18n( "Name:" ), generalGroupBox ), 0, 0 );

  mName = new KLineEdit();
  gbLayout->addWidget( mName, 0, 1 );

  mReadOnly = new QCheckBox( i18n( "Read-only" ), generalGroupBox );
  gbLayout->addWidget( mReadOnly, 1, 0 );

  mName->setText( mResource->resourceName() );
  mReadOnly->setChecked( mResource->readOnly() );

  mainLayout->addWidget( generalGroupBox );

  QGroupBox *resourceGroupBox = new QGroupBox( main );
  QGridLayout *resourceLayout = new QGridLayout;
  resourceLayout->setSpacing( spacingHint() );
  resourceGroupBox->setLayout( resourceLayout );

  resourceGroupBox->setTitle( i18n( "%1 Resource Settings" ,
                                factory->typeName( resource->type() ) ) );
  mainLayout->addWidget( resourceGroupBox );

  mainLayout->addStretch();

  mConfigWidget = factory->configWidget( resource->type(), resourceGroupBox );
  if ( mConfigWidget ) {
    resourceLayout->addWidget( mConfigWidget );
    mConfigWidget->setInEditMode( false );
    mConfigWidget->loadSettings( mResource );
    mConfigWidget->show();
    connect( mConfigWidget, SIGNAL( setReadOnly( bool ) ),
        SLOT( setReadOnly( bool ) ) );
  }

  connect( mName, SIGNAL( textChanged(const QString &)),
      SLOT( slotNameChanged(const QString &)));

  slotNameChanged( mName->text() );
  setMinimumSize( sizeHint() );
}

void ConfigDialog::setInEditMode( bool value )
{
  if ( mConfigWidget )
    mConfigWidget->setInEditMode( value );
}

void ConfigDialog::slotNameChanged( const QString &text)
{
  enableButtonOk( !text.isEmpty() );
}

void ConfigDialog::setReadOnly( bool value )
{
  mReadOnly->setChecked( value );
}

void ConfigDialog::accept()
{
  if ( mName->text().isEmpty() ) {
    KMessageBox::sorry( this, i18n( "Please enter a resource name." ) );
    return;
  }

  mResource->setResourceName( mName->text() );
  mResource->setReadOnly( mReadOnly->isChecked() );

  if ( mConfigWidget ) {
    // First save generic information
    // Also save setting of specific resource type
    mConfigWidget->saveSettings( mResource );
  }

  KDialog::accept();
}

#include "configdialog.moc"
