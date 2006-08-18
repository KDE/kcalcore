/**************************************************************************
*   Copyright (C) 2006 by Narayan Newton <narayannewton@gmail.com>        *
*   Copyright (C) 2003 - 2004 by Frerich Raabe <raabe@kde.org>            *
*                                Tobias Koenig <tokoe@kde.org>            *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
***************************************************************************/

#ifndef KXML_RPC_QUERY_H
#define KXML_RPC_QUERY_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QVariant>

#include <kio/job.h>

class QString;
class QDomDocument;
class QDomElement;

namespace KXmlRpc {

/**
  @file

  This file is part of KXmlRpc and defines our internal classes.

  \par Maintainer: Narayan Newton <narayannewton@gmail.com>

  @author Narayan Newton <narayannewton@gmail.com>
  @author Frerich Raabe <raabe@kde.org>
  @author Tobias Koenig <tokoe@kde.org>
 */

/**
  @brief
  Query is a class that represents an individual XML-RPC call.

  This is an internal class and is only used by the KXmlRpc::Server class.
  @internal
 */
class Query : public QObject
{
  friend class Result;
  Q_OBJECT

  public:
    /**
      Constructs a query

      @param id an optional id for the query
      @param parent an optional parent for the query
     */
    static Query *create( const QVariant &id = QVariant(), QObject *parent = 0 );

  public slots:
    /**
      Calls the specified method on the specified server with 
      the given argument list.

      @param server the server to contact
      @param method the method to call
      @param args an argument list to pass to said method
      @param userAgent the string to identify as to the server
     */
    void call( const QString &server, const QString &method,
               const QList<QVariant> &args = QList<QVariant>(),
               const QString &userAgent = "KDE-XMLRPC" );

  Q_SIGNALS:
    /**
      A signal sent when we receive a result from the server
     */
    void message( const QList<QVariant> &result, const QVariant &id );

    /**
      A signal sent when we receive an error from the server
     */
    void fault( int, const QString&, const QVariant &id );

    /**
      A signal sent when a query finishes
     */
    void finished( Query* );

  private:
    Query( const QVariant &id, QObject *parent = 0 );
    virtual ~Query();

    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void slotData( KIO::Job*, const QByteArray& ) )
    Q_PRIVATE_SLOT( d, void slotResult( KIO::Job* ) )
};


/**
  @brief
  Result is an internal class that represents a response 
  from a XML-RPC server.

  This is an internal class and is only used by Query
  @internal
 */
class Result
{
  friend class Query;
  friend class Query::Private;

  public:
    /**
      Constructs a result
     */
    Result();

    /**
      Constructs a result based on another result
     */
    Result( const Result &other );

    /**
      Destroys a result
     */
    virtual ~Result();

    /**
      Assigns the values of one result to this one
     */
    Result& operator=( const Result &other );

    /**
      @return whether the method call succeeded, 
      basically whether or not there was an XML-RPC fault
     */
    bool success() const;

    /**
      @return the error code of the fault 
     */
    int errorCode() const;

    /**
      @return the error string that describes the fault
     */
    QString errorString() const;

    /**
      @return the data returned by the method call
     */
    QList<QVariant> data() const;

  private:
    class Private;
    Private* const d;
};

} // namespace XmlRpc

#endif

