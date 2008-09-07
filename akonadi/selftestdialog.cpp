/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "selftestdialog.h"
#include "agentmanager.h"

#include <akonadi/private/xdgbasedirs_p.h>

#include <KIcon>
#include <KFileDialog>
#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs>

#include <QtDBus>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStandardItemModel>
#include <QSqlDatabase>
#include <QTextStream>

#define AKONADI_CONTROL_SERVICE QLatin1String("org.freedesktop.Akonadi.Control")
#define AKONADI_SERVER_SERVICE QLatin1String("org.freedesktop.Akonadi")

using namespace Akonadi;

SelfTestDialog::SelfTestDialog(QWidget * parent) :
    KDialog( parent )
{
  setCaption( i18n( "Akonadi Server Self-Test" ) );
  setButtons( Close | User1 );
  setButtonText( User1, i18n( "Save Resport" ) );
  setButtonIcon( User1, KIcon( "document-save" ) );
  showButtonSeparator( true );
  ui.setupUi( mainWidget() );

  mTestModel = new QStandardItemModel( this );
  ui.testView->setModel( mTestModel );
  connect( ui.testView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
           SLOT(selectionChanged(QModelIndex)) );

  connect( this, SIGNAL(user1Clicked()), SLOT(saveReport()) );

  runTests();
}

void SelfTestDialog::reportSuccess(const QString & summary, const QString & details)
{
  QStandardItem *item = new QStandardItem( KIcon( "dialog-ok" ), summary );
  item->setEditable( false );
  item->setWhatsThis( details );
  mTestModel->appendRow( item );
}

void SelfTestDialog::reportError(const QString & summary, const QString & details)
{
  QStandardItem *item = new QStandardItem( KIcon( "dialog-error" ), summary );
  item->setEditable( false );
  item->setWhatsThis( details );
  mTestModel->appendRow( item );
}

void SelfTestDialog::selectionChanged(const QModelIndex &index )
{
  if ( index.isValid() ) {
    ui.detailsLabel->setText( index.data( Qt::WhatsThisRole ).toString() );
    ui.detailsGroup->setEnabled( true );
  } else {
    ui.detailsLabel->setText( QString() );
    ui.detailsGroup->setEnabled( false );
  }
}

void SelfTestDialog::runTests()
{
  testSQLDriver();
  testMySQLServer();
  testAkonadiCtl();
  testServerStatus();
  testResources();
}

QVariant SelfTestDialog::serverSetting(const QString & group, const QString & key, const QVariant &def ) const
{
  const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
  QSettings settings( serverConfigFile, QSettings::IniFormat );
  settings.beginGroup( group );
  return settings.value( key, def );
}

bool SelfTestDialog::runProcess(const QString & app, const QStringList & args, QString & result) const
{
  QProcess proc;
  proc.start( app, args );
  const bool rv = proc.waitForFinished();
  result.clear();
  result += QString::fromLocal8Bit( proc.readAllStandardError() );
  result += QString::fromLocal8Bit( proc.readAllStandardOutput() );
  return rv;
}

void SelfTestDialog::testSQLDriver()
{
  const QString driver = serverSetting( "General", "Driver", "QMYSQL" ).toString();
  const QStringList availableDrivers = QSqlDatabase::drivers();
  const QString details = i18n( "The QtSQL driver '%1' is required by your current Akonadi server configuration.\n"
      "The following drivers are installed: %2.\n"
      "Make sure the required driver is installed.", driver, availableDrivers.join( QLatin1String(", ") ) );
  if ( availableDrivers.contains( driver ) )
    reportSuccess( i18n( "Database driver found." ), details );
  else
    reportError( i18n( "Database driver not found." ), details );
}

void SelfTestDialog::testMySQLServer()
{
  const QString driver = serverSetting( "General", "Driver", "QMYSQL" ).toString();
  if ( driver != QLatin1String( "QMYSQL" ) )
    return;
  const bool startServer = serverSetting( driver, "StartServer", true ).toBool();
  if ( !startServer )
    return;
  const QString serverPath = serverSetting( driver,  "ServerPath", "" ).toString(); // ### default?

  const QString details = i18n( "You currently have configured Akonadi to use the MySQL server '%1'.\n"
      "Make sure you have the MySQL server installed, set the correct path and ensure you have the "
      "necessary read and execution rights on the server executable.", serverPath );

  QFileInfo info( serverPath );
  if ( !info.exists() )
    reportError( i18n( "MySQL server not found." ), details );
  else if ( !info.isReadable() )
    reportError( i18n( "MySQL server not readable." ), details );
  else if ( !info.isExecutable() )
    reportError( i18n( "MySQL server not executable." ), details );
  else
    reportSuccess( i18n( "MySQL server found." ), details );

  // be extra sure and get the server version while we are at it
  QString result;
  if ( runProcess( serverPath, QStringList() << QLatin1String( "--version" ), result ) ) {
    const QString details = i18n( "MySQL server found: %1", result );
    reportSuccess( i18n( "MySQL server is executable." ), details );
  } else {
    const QString details = i18n( "Executing the MySQL server '%1' failed with the following error message: '%2'",
                                  result );
    reportError( i18n( "Executing the MySQL server failed." ), details );
  }
}

void SelfTestDialog::testAkonadiCtl()
{
  const QString path = KStandardDirs::findExe( QLatin1String("akonadictl") );
  if ( path.isEmpty() ) {
    reportError( i18n( "akonadictl not found" ),
                 i18n( "The program 'akonadictl' needs to be accessible in $PATH. "
                       "Make sure you have the Akonadi server installed." ) );
    return;
  }
  QString result;
  if ( runProcess( path, QStringList() << QLatin1String( "status" ), result ) ) {
    reportSuccess( i18n( "akonadictl found and usable" ),
                   i18n( "The program '%1' to control the Akonadi server was found "
                         "and could be executed successfully.\nResult:\n%2", path, result ) );
  } else {
    reportError( i18n( "akonadictl found but not usable" ),
                 i18n( "The program '%1' to control the Akonadi server was found "
                       "but could not be executed successfully.\nResult:\n%2\n"
                       "Make sure the Akonadi server is installed correctly.", path, result ) );
  }
}

void SelfTestDialog::testServerStatus()
{
  if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_CONTROL_SERVICE ) ) {
    reportSuccess( i18n( "Akonadi control process registered at D-Bus." ),
                   i18n( "The Akonadi control process is registed at D-Bus which typically indicates it is operational." ) );
  } else {
    reportError( i18n( "Akonadi control process not registed at D-Bus." ),
                 i18n( "The Akonadi control process is not registed at D-Bus which typically means it was not started "
                       "or encountered a fatal error during startup."  ) );
  }

  if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_SERVER_SERVICE ) ) {
    reportSuccess( i18n( "Akonadi server process registered at D-Bus." ),
                   i18n( "The Akonadi server process is registed at D-Bus which typically indicates it is operational." ) );
  } else {
    reportError( i18n( "Akonadi server process not registed at D-Bus." ),
                 i18n( "The Akonadi server process is not registed at D-Bus which typically means it was not started "
                       "or encountered a fatal error during startup."  ) );
  }
}

void SelfTestDialog::testResources()
{
  AgentType::List agentTypes = AgentManager::self()->types();
  bool resourceFound = false;
  foreach ( const AgentType &type, agentTypes ) {
    if ( type.capabilities().contains( "Resource" ) ) {
      resourceFound = true;
      break;
    }
  }

  if ( resourceFound ) {
    reportSuccess( i18n( "Resource agents found." ), i18n( "At least one resource agent has been found." ) );
  } else {
    // TODO: add details on XDG_WHATEVER env var needed to find agents
    reportError( i18n( "No resource agents found." ),
                 i18n( "No resource agents have been found, Akonadi is not usable without at least one. "
                       "This usually means that no resource agents are installed or that there is a setup problem." ) );
  }
}


void SelfTestDialog::saveReport()
{
  const QString fileName =  KFileDialog::getSaveFileName( KUrl(), QString(), this, i18n("Save Test Report") );
  QFile file( fileName );
  if ( !file.open( QFile::ReadWrite ) ) {
    KMessageBox::error( this, i18n( "Could not open file '%1'", fileName ) );
    return;
  }

  QTextStream s( &file );
  s << "Akonadi Server Self-Test Report" << endl;

  for ( int i = 0; i < mTestModel->rowCount(); ++i ) {
    QStandardItem *item = mTestModel->item( i );
    s << endl;
    s << "Test " << i << ": " << item->text() << endl;
    s << item->whatsThis() << endl;
  }

  s.flush();
  file.close();
}

#include "selftestdialog.moc"
