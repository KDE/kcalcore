/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#include "krecursivefilterproxymodel.h"

#include <kdebug.h>

class KRecursiveFilterProxyModelPrivate
{
  Q_DECLARE_PUBLIC(KRecursiveFilterProxyModel)
  KRecursiveFilterProxyModel *q_ptr;
public:
  KRecursiveFilterProxyModelPrivate(KRecursiveFilterProxyModel *model)
    : q_ptr(model),
      ignoreRemove(false),
      completeInsert(false),
      completeRemove(false)
  {
    qRegisterMetaType<QModelIndex>("QModelIndex");
  }

  inline void invokeDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
  {
    Q_Q(KRecursiveFilterProxyModel);
    bool success = QMetaObject::invokeMethod(q, "_q_sourceDataChanged", Qt::DirectConnection,
        Q_ARG(QModelIndex, topLeft),
        Q_ARG(QModelIndex, bottomRight));
    Q_ASSERT(success);
  }

  inline void invokeRowsInserted(const QModelIndex &source_parent, int start, int end)
  {
    Q_Q(KRecursiveFilterProxyModel);
    bool success = QMetaObject::invokeMethod(q, "_q_sourceRowsInserted", Qt::DirectConnection,
        Q_ARG(QModelIndex, source_parent),
        Q_ARG(int, start),
        Q_ARG(int, end));
   Q_ASSERT(success);
  }

  inline void invokeRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end)
  {
    Q_Q(KRecursiveFilterProxyModel);
    bool success = QMetaObject::invokeMethod(q, "_q_sourceRowsAboutToBeInserted", Qt::DirectConnection,
        Q_ARG(QModelIndex, source_parent),
        Q_ARG(int, start),
        Q_ARG(int, end));
    Q_ASSERT(success);
  }

  inline void invokeRowsRemoved(const QModelIndex &source_parent, int start, int end)
  {
    Q_Q(KRecursiveFilterProxyModel);
    bool success = QMetaObject::invokeMethod(q, "_q_sourceRowsRemoved", Qt::DirectConnection,
        Q_ARG(QModelIndex, source_parent),
        Q_ARG(int, start),
        Q_ARG(int, end));
    Q_ASSERT(success);
  }

  inline void invokeRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end)
  {
    Q_Q(KRecursiveFilterProxyModel);
    bool success = QMetaObject::invokeMethod(q, "_q_sourceRowsAboutToBeRemoved", Qt::DirectConnection,
        Q_ARG(QModelIndex, source_parent),
        Q_ARG(int, start),
        Q_ARG(int, end));
    Q_ASSERT(success);
  }

  void sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right);
  void sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
  void sourceRowsInserted(const QModelIndex &source_parent, int start, int end);
  void sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
  void sourceRowsRemoved(const QModelIndex &source_parent, int start, int end);

  /**
    Given that @p index does not match the filter, clear mappings in the QSortFilterProxyModel up to and excluding the
    first ascendant that does match, and remake the mappings.

    If @p refreshAll is true, this method also refreshes intermediate mappings. This is significant when removing rows.
  */
  void refreshAscendantMapping(const QModelIndex &index, bool refreshAll = false);

  bool ignoreRemove;
  bool completeInsert;
  bool completeRemove;
};

void KRecursiveFilterProxyModelPrivate::sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right)
{
  Q_Q(KRecursiveFilterProxyModel);

  QModelIndex source_parent = source_top_left.parent();

  if (!source_parent.isValid() || q->filterAcceptsRow(source_parent.row(), source_parent.parent()))
  {
    invokeDataChanged(source_top_left, source_bottom_right);
    return;
  }

  int start = -1;
  int end = -1;
  bool requireRow = false;
  for (int row = source_top_left.row(); row <= source_bottom_right.row(); ++row)
    if (q->filterAcceptsRow(row, source_parent))
    {
      requireRow = true;
      break;
    }

  if (!requireRow) // None of the changed rows are now required in the model.
    return;

  refreshAscendantMapping(source_parent);
}

void KRecursiveFilterProxyModelPrivate::refreshAscendantMapping(const QModelIndex &index, bool refreshAll)
{
  Q_Q(KRecursiveFilterProxyModel);

  Q_ASSERT(index.isValid());
  QModelIndex lastAscendant = index;
  QModelIndex sourceAscendant = index.parent();
  int lastRow;
  // We got a matching descendant, so find the right place to insert the row.
  // We need to tell the QSortFilterProxyModel that the first child between an existing row in the model
  // has changed data so that it will get a mapping.
  while(sourceAscendant.isValid() && !q->acceptRow(sourceAscendant.row(), sourceAscendant.parent()))
  {
    if (refreshAll)
      invokeDataChanged(sourceAscendant, sourceAscendant);

    lastAscendant = sourceAscendant;
    sourceAscendant = sourceAscendant.parent();
  }

  // Inform the model that its data changed so that it creates new mappings and finds the rows which now match the filter.
  invokeDataChanged(lastAscendant, lastAscendant);
}

void KRecursiveFilterProxyModelPrivate::sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end)
{
  Q_Q(KRecursiveFilterProxyModel);

  if (!source_parent.isValid() || q->filterAcceptsRow(source_parent.row(), source_parent.parent()))
  {
    invokeRowsAboutToBeInserted(source_parent, start, end);
    completeInsert = true;
  }
}

void KRecursiveFilterProxyModelPrivate::sourceRowsInserted(const QModelIndex &source_parent, int start, int end)
{
  Q_Q(KRecursiveFilterProxyModel);

  if (completeInsert)
  {
    completeInsert = false;
    invokeRowsInserted(source_parent, start, end);
    // If the parent is already in the model, we can just pass on the signal.
    return;
  }

  bool requireRow = false;
  for (int row = start; row <= end; ++row)
  {
    if (q->filterAcceptsRow(row, source_parent))
    {
      requireRow = true;
      break;
    }
  }

  if (!requireRow)
  {
    // The row doesn't have descendants that match the filter. Filter it out.
    return;
  }

  refreshAscendantMapping(source_parent);
}

void KRecursiveFilterProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end)
{
  Q_Q(KRecursiveFilterProxyModel);

  if (q->filterAcceptsRow(source_parent.row(), source_parent.parent()))
  {
    invokeRowsAboutToBeRemoved(source_parent, start, end);
    completeRemove = true;
    return;
  }

  bool accepted = false;
  for (int row = start; row < end; ++row)
  {
    if (q->filterAcceptsRow(row, source_parent))
    {
      accepted = true;
      break;
    }
  }
  if (!accepted)
  {
    // All removed rows are already filtered out. We don't care about the signal.
    ignoreRemove = true;
  }
}

void KRecursiveFilterProxyModelPrivate::sourceRowsRemoved(const QModelIndex &source_parent, int start, int end)
{
  Q_Q(KRecursiveFilterProxyModel);

  if (completeRemove)
  {
    completeRemove = false;
    // Source parent is already in the model.
    invokeRowsRemoved(source_parent, start, end);
    // fall through. After removing rows, we need to refresh things so that intermediates will be removed too if necessary.
  }

  if (ignoreRemove)
  {
    ignoreRemove = false;
    return;
  }

  // Refresh intermediate rows too.
  // This is needed because QSFPM only invalidates the mapping for the
  // index range given to dataChanged, not its children.
  refreshAscendantMapping(source_parent, true);
}

KRecursiveFilterProxyModel::KRecursiveFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent), d_ptr(new KRecursiveFilterProxyModelPrivate(this))
{
  setDynamicSortFilter(true);
}

KRecursiveFilterProxyModel::~KRecursiveFilterProxyModel()
{

}

bool KRecursiveFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
  Q_ASSERT(sourceModel()->index(sourceRow, 0, sourceParent).isValid());
  QVariant da = sourceModel()->index(sourceRow, 0, sourceParent).data();

  if (acceptRow(sourceRow, sourceParent))
    return true;

  QModelIndex source_index = sourceModel()->index(sourceRow, 0, sourceParent);
  Q_ASSERT(source_index.isValid());
  bool accepted = false;

  for (int row = 0 ; row < sourceModel()->rowCount(source_index); ++row)
    if (filterAcceptsRow(row, source_index))
      accepted = true; // Need to do this in a loop so that all siblings in a parent get processed, not just the first.

  return accepted;
}

bool KRecursiveFilterProxyModel::acceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
  return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void KRecursiveFilterProxyModel::setSourceModel(QAbstractItemModel* model)
{
  Q_D(KRecursiveFilterProxyModel);

  // Standard disconnect.
  disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
      this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));

  disconnect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
      this, SLOT(sourceRowsAboutToBeInserted(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
      this, SLOT(sourceRowsInserted(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
      this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
      this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));

  QSortFilterProxyModel::setSourceModel(model);

  // Disconnect in the QSortFilterProxyModel. These methods will be invoked manually
  disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
      this, SLOT(_q_sourceDataChanged(QModelIndex,QModelIndex)));

  disconnect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
      this, SLOT(_q_sourceRowsAboutToBeInserted(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
      this, SLOT(_q_sourceRowsInserted(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
      this, SLOT(_q_sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
      this, SLOT(_q_sourceRowsRemoved(QModelIndex,int,int)));

  // Slots for manual invoking of QSortFilterProxyModel methods.
  connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
      this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));

  connect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
      this, SLOT(sourceRowsAboutToBeInserted(QModelIndex,int,int)));

  connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
      this, SLOT(sourceRowsInserted(QModelIndex,int,int)));

  connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
      this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex,int,int)));

  connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
      this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));

}

#include "krecursivefilterproxymodel.moc"
