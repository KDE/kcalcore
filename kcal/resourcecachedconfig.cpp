/*
    This file is part of the kcal library.

    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

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

#include "resourcecachedconfig.moc"

#include "resourcecached.h"

#include <QtGui/QLayout>
#include <QtGui/QRadioButton>
#include <QtGui/QSpinBox>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QButtonGroup>
#include <QtGui/QGroupBox>

#include <khbox.h>
#include <klocale.h>
#include <kdebug.h>

using namespace KCal;

ResourceCachedReloadConfig::ResourceCachedReloadConfig( QWidget *parent )
  : QWidget( parent ), d( 0 )
{
  QBoxLayout *topLayout = new QVBoxLayout( this );

  QGroupBox* groupBox = new QGroupBox( i18n("Automatic Reload"), this );
  topLayout->addWidget( groupBox );
  QRadioButton* noAutomaticReload = new QRadioButton( i18n("Never"), groupBox );
  QRadioButton* automaticReloadOnStartup = new QRadioButton( i18n("On startup"), groupBox );
  QRadioButton* intervalRadio = new QRadioButton( i18n("Regular interval"),
                                     groupBox );
  mGroup = new QButtonGroup( this );
  mGroup->addButton( noAutomaticReload, 0 );
  mGroup->addButton( automaticReloadOnStartup, 1 );
  mGroup->addButton( intervalRadio, 2 );

  connect( intervalRadio, SIGNAL( stateChanged( int ) ),
           SLOT( slotIntervalStateChanged( int ) ) );
  KHBox *intervalBox = new KHBox;
  new QLabel( i18n("Interval in minutes"), intervalBox );
  mIntervalSpin = new QSpinBox( intervalBox );
  mIntervalSpin->setRange( 1, 900 );
  mIntervalSpin->setEnabled( false );

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget(noAutomaticReload);
  vbox->addWidget(automaticReloadOnStartup);
  vbox->addWidget(intervalRadio);
  vbox->addWidget(intervalBox);
  vbox->addStretch(1);
  groupBox->setLayout(vbox);


}

void ResourceCachedReloadConfig::loadSettings( ResourceCached *resource )
{
  mGroup->button( resource->reloadPolicy() )->setChecked( true );
  mIntervalSpin->setValue( resource->reloadInterval() );
}

void ResourceCachedReloadConfig::saveSettings( ResourceCached *resource )
{
  resource->setReloadPolicy( mGroup->checkedId() );
  resource->setReloadInterval( mIntervalSpin->value() );
}

void ResourceCachedReloadConfig::slotIntervalStateChanged( int state )
{
  if ( state == Qt::Checked ) mIntervalSpin->setEnabled( true );
  else mIntervalSpin->setEnabled( false );
}


ResourceCachedSaveConfig::ResourceCachedSaveConfig( QWidget *parent )
  : QWidget( parent ), d( 0 )
{
  QBoxLayout *topLayout = new QVBoxLayout( this );

  QGroupBox* groupBox = new QGroupBox( i18n("Automatic Save"), this );
  mGroup = new QButtonGroup( this );
  topLayout->addWidget( groupBox );
  QRadioButton* never = new QRadioButton( i18n("Never"), groupBox );
  QRadioButton* onExit = new QRadioButton( i18n("On exit"), groupBox );

  QRadioButton *intervalRadio = new QRadioButton( i18n("Regular interval"),
                                                  groupBox );

  mGroup = new QButtonGroup( this );
  mGroup->addButton( never, 0 );
  mGroup->addButton( onExit, 1 );
  mGroup->addButton( intervalRadio, 2 );


  connect( intervalRadio, SIGNAL( stateChanged( int ) ),
           SLOT( slotIntervalStateChanged( int ) ) );
  KHBox *intervalBox = new KHBox;
  new QLabel( i18n("Interval in minutes"), intervalBox );
  mIntervalSpin = new QSpinBox( intervalBox );
  mIntervalSpin->setRange( 1, 900 );
  mIntervalSpin->setEnabled( false );

  QRadioButton* delay = new QRadioButton( i18n("Delayed after changes"), groupBox );
  QRadioButton* every = new QRadioButton( i18n("On every change"), groupBox );

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget(never);
  vbox->addWidget(onExit);
  vbox->addWidget(intervalRadio);
  vbox->addWidget(intervalBox);
  vbox->addWidget(delay);
  vbox->addWidget(every);
  vbox->addStretch(1);
  groupBox->setLayout(vbox);

}

void ResourceCachedSaveConfig::loadSettings( ResourceCached *resource )
{
  mGroup->button( resource->savePolicy() )->setChecked( true );
  mIntervalSpin->setValue( resource->saveInterval() );
}

void ResourceCachedSaveConfig::saveSettings( ResourceCached *resource )
{
  resource->setSavePolicy( mGroup->checkedId() );
  resource->setSaveInterval( mIntervalSpin->value() );
}

void ResourceCachedSaveConfig::slotIntervalStateChanged( int state )
{
  if ( state == Qt::Checked ) mIntervalSpin->setEnabled( true );
  else mIntervalSpin->setEnabled( false );
}
