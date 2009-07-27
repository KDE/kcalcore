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

#include "servermanager.h"
#include "servermanager_p.h"

#include "agenttype.h"
#include "agentbase.h"
#include "agentmanager.h"
#include "selftestdialog_p.h"
#include "session_p.h"

#include <KDebug>
#include <KGlobal>

#include <QtDBus>

#define AKONADI_CONTROL_SERVICE QLatin1String("org.freedesktop.Akonadi.Control")
#define AKONADI_SERVER_SERVICE QLatin1String("org.freedesktop.Akonadi")

using namespace Akonadi;

class Akonadi::ServerManagerPrivate
{
  public:
    ServerManagerPrivate() :
      instance( new ServerManager( this ) )
    {
      operational = instance->isRunning();
    }

    ~ServerManagerPrivate()
    {
      delete instance;
    }

    void serviceOwnerChanged( const QString &service, const QString &oldOwner, const QString &newOwner )
    {
      Q_UNUSED( oldOwner );
      Q_UNUSED( newOwner );
      if ( service != AKONADI_SERVER_SERVICE && service != AKONADI_CONTROL_SERVICE )
        return;
      serverProtocolVersion = -1,
      checkStatusChanged();
    }

    void checkStatusChanged()
    {
      const bool status = instance->isRunning();
      if ( status == operational )
        return;
      operational = status;
      if ( operational )
        emit instance->started();
      else
        emit instance->stopped();
    }

    ServerManager *instance;
    static int serverProtocolVersion;
    bool operational;
};

int ServerManagerPrivate::serverProtocolVersion = -1;

K_GLOBAL_STATIC( ServerManagerPrivate, sInstance )

ServerManager::ServerManager(ServerManagerPrivate * dd ) :
    d( dd )
{
  connect( QDBusConnection::sessionBus().interface(),
           SIGNAL(serviceOwnerChanged(QString,QString,QString)),
           SLOT(serviceOwnerChanged(QString,QString,QString)) );

  // HACK see if we are a agent ourselves and skip AgentManager creation since that can cause deadlocks
  QObject *obj = QDBusConnection::sessionBus().objectRegisteredAt( QLatin1String("/") );
  if ( obj && dynamic_cast<AgentBase*>( obj ) )
    return;
  connect( AgentManager::self(), SIGNAL(typeAdded(Akonadi::AgentType)), SLOT(checkStatusChanged()) );
  connect( AgentManager::self(), SIGNAL(typeRemoved(Akonadi::AgentType)), SLOT(checkStatusChanged()) );
}

ServerManager * Akonadi::ServerManager::self()
{
  return sInstance->instance;
}

bool ServerManager::start()
{
  const bool ok = QProcess::startDetached( QLatin1String("akonadi_control") );
  if ( !ok ) {
    kWarning() << "Unable to execute akonadi_control, falling back to D-Bus auto-launch";
    QDBusReply<void> reply = QDBusConnection::sessionBus().interface()->startService( AKONADI_CONTROL_SERVICE );
    if ( !reply.isValid() ) {
      kDebug() << "Akonadi server could not be started via D-Bus either: "
               << reply.error().message();
      return false;
    }
  }
  return true;
}

bool ServerManager::stop()
{
  QDBusInterface iface( AKONADI_CONTROL_SERVICE,
                        QString::fromLatin1("/ControlManager"),
                        QString::fromLatin1("org.freedesktop.Akonadi.ControlManager") );
  if ( !iface.isValid() )
    return false;
  iface.call( QDBus::NoBlock, QString::fromLatin1("shutdown") );
  return true;
}

void ServerManager::showSelfTestDialog( QWidget *parent )
{
  Akonadi::SelfTestDialog dlg( parent );
  dlg.hideIntroduction();
  dlg.exec();
}

bool ServerManager::isRunning()
{
  if ( !QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_CONTROL_SERVICE ) ||
       !QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_SERVER_SERVICE ) ) {
    return false;
  }

  // check if the server protocol is recent enough
  if ( sInstance.exists() ) {
    if ( Internal::serverProtocolVersion() >= 0 &&
         Internal::serverProtocolVersion() < SessionPrivate::minimumProtocolVersion() )
      return false;
  }

  // HACK see if we are a agent ourselves and skip the test below which can in some cases deadlock the server
  // and is not really needed in this case anyway since we happen to know at least one agent is available
  QObject *obj = QDBusConnection::sessionBus().objectRegisteredAt( QLatin1String("/") );
  if ( obj && dynamic_cast<AgentBase*>( obj ) )
    return true;

  // besides the running server processes we also need at least one resource to be operational
  AgentType::List agentTypes = AgentManager::self()->types();
  foreach ( const AgentType &type, agentTypes ) {
    if ( type.capabilities().contains( QLatin1String("Resource") ) )
      return true;
  }
  return false;
}

int Internal::serverProtocolVersion()
{
  return ServerManagerPrivate::serverProtocolVersion;
}

void Internal::setServerProtocolVersion( int version )
{
  ServerManagerPrivate::serverProtocolVersion = version;
  if ( sInstance.exists() )
    sInstance->checkStatusChanged();
}

#include "servermanager.moc"
