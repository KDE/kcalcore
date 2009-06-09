/*
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

#ifndef AKONADI_CHANGERECORDER_H
#define AKONADI_CHANGERECORDER_H

#include <akonadi/monitor.h>

class QSettings;

namespace Akonadi {

class ChangeRecorderPrivate;

/**
 * @short Records and replays change notification.
 *
 * This class is responsible for recording change notification during
 * an agent is not online and replay the notifications when the agent
 * is online again. Therefor the agent doesn't have to care about
 * online/offline mode in its synchronization algorithm.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT ChangeRecorder : public Monitor
{
  Q_OBJECT
  public:
    /**
     * Creates a new change recorder.
     */
    explicit ChangeRecorder( QObject *parent = 0 );

    /**
     * Destroys the change recorder.
     * All not yet processed changes are written back to the config file.
     */
    ~ChangeRecorder();

    /**
     * Sets the QSettings object used for persisting recorded changes.
     */
    void setConfig( QSettings *settings );

    /**
     * Returns whether there are recorded changes.
     */
    bool isEmpty() const;

    /**
     * Removes the previously emitted change from the records.
     */
    void changeProcessed();

    /**
     * Enables change recording. If change recording is disabled, this class
     * behaves exactly like Akonadi::Monitor.
     * Change recording is enabled by default.
     */
    void setChangeRecordingEnabled( bool enable );

  public Q_SLOTS:
    /**
     * Replay the next change notification and erase the previous one from the record.
     */
    void replayNext();

  Q_SIGNALS:
    /**
     * Emitted when new changes are recorded.
     */
    void changesAdded();

    /**
     * @internal
     * Emitted when replayNext() was called, but there was no valid change to replay.
     * This is used internally to prevent stuck notifications. You shouldn't need to
     * connect to this signal.
     */
    void nothingToReplay();

  private:
    Q_DECLARE_PRIVATE( ChangeRecorder )
};

}

#endif
