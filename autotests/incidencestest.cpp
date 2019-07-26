/*
  This file is part of the kcalcore library.

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

#include "icalformat.h"
#include "event.h"
#include "todo.h"

#include <QDebug>
#include <QCoreApplication>
#include <QCommandLineParser>

using namespace KCalendarCore;

int main(int argc, char **argv)
{
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("verbose"), QStringLiteral("Verbose output")));

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("testincidence"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1"));
    parser.process(app);

    const bool verbose = parser.isSet(QStringLiteral("verbose"));

    ICalFormat f;

    Event::Ptr event1 = Event::Ptr(new Event);
    event1->setSummary(QStringLiteral("Test Event"));
    event1->recurrence()->setDaily(2);
    event1->recurrence()->setDuration(3);

    QString eventString1 = f.toString(event1.staticCast<Incidence>());
    if (verbose) {
        qDebug() << "EVENT1 START:" << eventString1 << "EVENT1 END";
    }

    event1->setSchedulingID(QStringLiteral("foo"));
    Incidence::Ptr event2 = Incidence::Ptr(event1->clone());

    Q_ASSERT(event1->uid() == event2->uid());
    Q_ASSERT(event1->schedulingID() == event2->schedulingID());

    QString eventString2 = f.toString(event2.staticCast<Incidence>());
    if (verbose) {
        qDebug() << "EVENT2 START:" << eventString2 << "EVENT2 END";
    }

    if (eventString1 != eventString2) {
        qDebug() << "Clone Event FAILED.";
    } else {
        qDebug() << "Clone Event SUCCEEDED.";
    }

    Todo::Ptr todo1 = Todo::Ptr(new Todo);
    todo1->setSummary(QStringLiteral("Test todo"));
    QString todoString1 = f.toString(todo1.staticCast<Incidence>());
    if (verbose) {
        qDebug() << "todo1 START:" << todoString1 << "todo1 END";
    }

    Incidence::Ptr todo2 = Incidence::Ptr(todo1->clone());
    QString todoString2 = f.toString(todo2);
    if (verbose) {
        qDebug() << "todo2 START:" << todoString2 << "todo2 END";
    }

    if (todoString1 != todoString2) {
        qDebug() << "Clone Todo FAILED.";
    } else {
        qDebug() << "Clone Todo SUCCEEDED.";
    }
}
