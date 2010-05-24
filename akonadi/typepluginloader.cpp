/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "typepluginloader_p.h"

#include "item.h"
#include "itemserializer_p.h"
#include "itemserializerplugin.h"

// KDE core
#include <kdebug.h>
#include <kmimetype.h>

// Qt
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

// temporary
#include "pluginloader_p.h"

namespace Akonadi {

K_GLOBAL_STATIC( DefaultItemSerializerPlugin, s_defaultItemSerializerPlugin )

class PluginEntry
{
  public:
    PluginEntry()
      : mPlugin( 0 )
    {
    }

    explicit PluginEntry( const QString &identifier, QObject *plugin = 0 )
      : mIdentifier( identifier ), mPlugin( plugin )
    {
    }

    inline QObject* plugin() const
    {
      if ( mPlugin )
        return mPlugin;

      QObject *object = PluginLoader::self()->createForName( mIdentifier );
      if ( !object ) {
        kWarning() << "ItemSerializerPluginLoader: "
                   << "plugin" << mIdentifier << "is not valid!" << endl;

        // we try to use the default in that case
        mPlugin = s_defaultItemSerializerPlugin;
      }

      mPlugin = object;
      if ( !qobject_cast<ItemSerializerPlugin*>( mPlugin ) ) {
        kWarning() << "ItemSerializerPluginLoader: "
                   << "plugin" << mIdentifier << "doesn't provide interface ItemSerializerPlugin!" << endl;

        // we try to use the default in that case
        mPlugin = s_defaultItemSerializerPlugin;
      }

      Q_ASSERT( mPlugin );

      return mPlugin;
    }

    QString type() const
    {
      return mIdentifier;
    }

    bool operator<( const PluginEntry &other ) const
    {
      return mIdentifier < other.mIdentifier;
    }

    bool operator<( const QString &type ) const
    {
      return mIdentifier < type;
    }

  private:
    QString mIdentifier;
    mutable QObject *mPlugin;
};

static bool operator<( const QString &type, const PluginEntry &entry )
{
  return type < entry.type();
}


class PluginRegistry
{
  public:
    PluginRegistry()
      : mDefaultPlugin( PluginEntry( QLatin1String( "application/octet-stream" ), s_defaultItemSerializerPlugin ) )
    {
      const PluginLoader* pl = PluginLoader::self();
      if ( !pl ) {
        kWarning() << "Cannot instantiate plugin loader!" << endl;
        return;
      }
      const QStringList types = pl->types();
      kDebug() << "ItemSerializerPluginLoader: "
               << "found" << types.size() << "plugins." << endl;
      allPlugins.reserve( types.size() + 1 );
      foreach ( const QString &type, types )
        allPlugins.append( PluginEntry( type ) );
      allPlugins.append( mDefaultPlugin );
      std::sort( allPlugins.begin(), allPlugins.end() );
    }

    const PluginEntry& findBestMatch( const QString &type )
    {
      KMimeType::Ptr mimeType = KMimeType::mimeType( type, KMimeType::ResolveAliases );
      if ( mimeType.isNull() )
        return mDefaultPlugin;

      // step 1: find all plugins that match at all
      QVector<int> matchingIndexes;
      for ( int i = 0, end = allPlugins.size(); i < end; ++i ) {
        if ( mimeType->is( allPlugins[i].type() ) )
          matchingIndexes.append( i );
      }

      // 0 matches: no luck (shouldn't happend though, as application/octet-stream matches everything)
      if ( matchingIndexes.isEmpty() )
        return mDefaultPlugin;
      // 1 match: we are done
      if ( matchingIndexes.size() == 1 )
        return allPlugins[matchingIndexes.first()];

      // step 2: if we have more than one match, find the most specific one using topological sort
      boost::adjacency_list<> graph( matchingIndexes.size() );
      for ( int i = 0, end = matchingIndexes.size() ; i != end ; ++i ) {
        KMimeType::Ptr mimeType = KMimeType::mimeType( allPlugins[matchingIndexes[i]].type(), KMimeType::ResolveAliases );
        if ( mimeType.isNull() )
          continue;
        for ( int j = 0; j != end; ++j ) {
          if ( i != j && mimeType->is( allPlugins[matchingIndexes[j]].type() ) )
            boost::add_edge( j, i, graph );
        }
      }

      QVector<int> order;
      order.reserve( allPlugins.size() );
      try {
        boost::topological_sort( graph, std::back_inserter( order ) );
      } catch ( boost::not_a_dag &e ) {
        kWarning() << "Mimetype tree is not a DAG!";
        return mDefaultPlugin;
      }

      return allPlugins[matchingIndexes[order.first()]];
    }

    QVector<PluginEntry> allPlugins;
    QHash<QString, QObject*> cachedPlugins;

  private:
    PluginEntry mDefaultPlugin;
};

K_GLOBAL_STATIC( PluginRegistry, s_pluginRegistry )

QObject* TypePluginLoader::objectForMimeType( const QString &mimetype )
{
  // plugin cached, so let's take that one
  if ( s_pluginRegistry->cachedPlugins.contains( mimetype ) )
    return s_pluginRegistry->cachedPlugins.value( mimetype );

  QObject *plugin = 0;

  // check if we have one that matches exactly
  const QVector<PluginEntry>::const_iterator it
    = qBinaryFind( s_pluginRegistry->allPlugins.constBegin(), s_pluginRegistry->allPlugins.constEnd(), mimetype );
  if ( it != s_pluginRegistry->allPlugins.constEnd() ) {
    plugin = ( *it ).plugin();
  } else {
    // check if we have a more generic plugin
    const PluginEntry &entry = s_pluginRegistry->findBestMatch( mimetype );
    kDebug() << "Did not find exactly matching serializer plugin for type" << mimetype
             << ", taking" << entry.type() << "as the closest match";
    plugin = entry.plugin();
  }

  Q_ASSERT(plugin);
  s_pluginRegistry->cachedPlugins.insert( mimetype, plugin );
  return plugin;
}

ItemSerializerPlugin* TypePluginLoader::pluginForMimeType( const QString &mimetype )
{
  return qobject_cast<ItemSerializerPlugin*>( objectForMimeType( mimetype ) );
}

}
