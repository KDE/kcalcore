/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#include "progressspinnerdelegate.h"

#include <QTimerEvent>
#include <QAbstractItemView>

#include <Akonadi/EntityTreeModel>

DelegateAnimator::DelegateAnimator(QAbstractItemView *view)
  : QObject(view), m_view(view)
{
  m_timerId = startTimer(40);

  m_pixmapSequence = KPixmapSequence(QLatin1String("process-working"), 22);
}

void DelegateAnimator::timerEvent(QTimerEvent* event)
{
  if (!(event->timerId() == m_timerId && m_view))
    return QObject::timerEvent(event);

  QRegion region;
  foreach (const Animation &animation, m_animations)
  {
    // This repaints the entire delegate.
    // TODO: See if there's a way to repaint only part of it.
    animation.animate();
    region += m_view->visualRect(animation.index);
  }

  m_view->viewport()->update(region);
}

QPixmap DelegateAnimator::sequenceFrame(const QModelIndex& index)
{
  foreach(const Animation &animation, m_animations)
  {
    if (animation.index == index)
    {
      return m_pixmapSequence.frameAt(animation.frame);
    }
  }
  return QPixmap();
}


ProgessSpinnerDelegate::ProgessSpinnerDelegate(DelegateAnimator *animator, QObject* parent)
  : QStyledItemDelegate(parent), m_animator(animator)
{

}

void ProgessSpinnerDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
{
  QStyledItemDelegate::initStyleOption(option, index);

  const Akonadi::Collection collection = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();

  if (!collection.isValid())
  {
    m_animator->pop(index);
    return;
  }

  if (index.data(Akonadi::EntityTreeModel::FetchStateRole).toInt() != Akonadi::EntityTreeModel::FetchingState)
  {
    m_animator->pop(index);
    return;
  }

  m_animator->push(index);

  if (QStyleOptionViewItemV4 *v4 = qstyleoption_cast<QStyleOptionViewItemV4 *>(option)) {
    v4->icon = m_animator->sequenceFrame(index);
  }
}

uint qHash(DelegateAnimator::Animation anim)
{
  return qHash(anim.index);
}
