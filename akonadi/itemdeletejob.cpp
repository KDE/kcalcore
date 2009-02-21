/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "itemdeletejob.h"
#include "itemmodifyjob.h"
#include "job_p.h"
#include "transactionsequence.h"
#include "expungejob.h"

using namespace Akonadi;

class Akonadi::ItemDeleteJobPrivate : public JobPrivate
{
  public:
    enum State {
      Begin,
      Store,
      Expunge,
      Commit
    };

    ItemDeleteJobPrivate( ItemDeleteJob *parent )
      : JobPrivate( parent )
    {
    }

    void jobDone( KJob* );

    Q_DECLARE_PUBLIC( ItemDeleteJob )

    Item mItem;
    State mState;
};

void ItemDeleteJobPrivate::jobDone( KJob * job )
{
  Q_Q( ItemDeleteJob );

  if ( !job->error() ) // error is already handled by KCompositeJob
    q->emitResult();
}

ItemDeleteJob::ItemDeleteJob( const Item & item, QObject * parent )
  : Job( new ItemDeleteJobPrivate( this ), parent )
{
  Q_D( ItemDeleteJob );

  d->mItem = item;
  d->mState = ItemDeleteJobPrivate::Begin;
}

ItemDeleteJob::~ItemDeleteJob()
{
}

void ItemDeleteJob::doStart()
{
  Q_D( ItemDeleteJob );

  TransactionSequence  *transaction = new TransactionSequence( this );
  connect( transaction, SIGNAL(result(KJob*)), SLOT(jobDone(KJob*)) );
  addSubjob( transaction );

  d->mItem.setFlag( "\\Deleted" );
  ItemModifyJob* store = new ItemModifyJob( d->mItem, transaction );
  store->disableRevisionCheck();

  new ExpungeJob( transaction );
  transaction->commit();
}

#include "itemdeletejob.moc"
