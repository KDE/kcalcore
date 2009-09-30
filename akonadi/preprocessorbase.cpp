/******************************************************************************
 *
 *  Copyright (c) 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA, 02110-1301, USA.
 *
 *****************************************************************************/

#include "preprocessorbase.h"

#include "agentbase_p.h"
#include "item.h"
#include "collection.h"
#include "preprocessoradaptor.h"

#include <kdebug.h>

namespace Akonadi
{

class PreprocessorBasePrivate : public AgentBasePrivate
{
  public:
    PreprocessorBasePrivate( PreprocessorBase *parent )
      : AgentBasePrivate( parent ),
        mInDelayedProcessing( false ),
        mDelayedProcessingItemId( 0 )
    {
    }

    Q_DECLARE_PUBLIC( PreprocessorBase )

    void delayedInit()
    {
      if ( !QDBusConnection::sessionBus().registerService( QLatin1String( "org.freedesktop.Akonadi.Preprocessor." ) + mId ) )
        kFatal() << "Unable to register service at D-Bus: " << QDBusConnection::sessionBus().lastError().message();
      AgentBasePrivate::delayedInit();
    }

    bool mInDelayedProcessing;
    qlonglong mDelayedProcessingItemId;
};

} // namespace Akonadi

using namespace Akonadi;

PreprocessorBase::PreprocessorBase( const QString &id )
  : AgentBase( new PreprocessorBasePrivate( this ), id )
{
  new PreprocessorAdaptor( this );
}

PreprocessorBase::~PreprocessorBase()
{
}

void PreprocessorBase::terminateProcessing( ProcessingResult )
{
  Q_D( PreprocessorBase );

  Q_ASSERT_X( result != ProcessingDelayed, "PreprocessorBase::terminateProcessing", "You should never pass ProcessingDelayed to this function" );
  Q_ASSERT_X( d->mInDelayedProcessing, "PreprocessorBase::terminateProcessing", "terminateProcessing() called while not in delayed processing mode" );

  d->mInDelayedProcessing = false;
  emit itemProcessed( d->mDelayedProcessingItemId );
}

void PreprocessorBase::beginProcessItem( qlonglong itemId, qlonglong collectionId, const QString &mimeType )
{
  Q_D( PreprocessorBase );

  kDebug() << "PreprocessorBase: about to process item " << itemId << " in collection " << collectionId << " and mimetype " << mimeType;

  switch( processItem( static_cast< Item::Id >( itemId ), static_cast< Collection::Id >( collectionId ), mimeType ) )
  {
    case ProcessingFailed:
    case ProcessingRefused:
    case ProcessingCompleted:
      kDebug() << "PreprocessorBase: item processed, emitting signal (" << itemId << ")";

      // TODO: Handle the different status codes appropriately

      emit itemProcessed( itemId );

      kDebug() << "PreprocessorBase: item processed, signal emitted (" << itemId << ")";
    break;
    case ProcessingDelayed:
      kDebug() << "PreprocessorBase: item processing delayed (" << itemId << ")";

      d->mInDelayedProcessing = true;
      d->mDelayedProcessingItemId = itemId;
    break;
  }
}

#include "preprocessorbase.moc"
