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
/**
  @file

  This file is part of the API for accessing XML-RPC Servers
  and defines the #Client class.

  @author Narayan Newton <narayannewton@gmail.com>
  @author Frerich Raabe <raabe@kde.org>
  @author Tobias Koenig <tokoe@kde.org>

  \par Maintainer: Narayan Newton <narayannewton@gmail.com>
*/

#ifndef KXML_RPC_CLIENT_H
#define KXML_RPC_CLIENT_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QVariant>

#include <kurl.h>
#include "kxmlrpcclient.h"

namespace KXmlRpc {

/**
  @brief
  A class that represents a connection to a XML-RPC server.
  This is the main interface to the XML-RPC client library.

  @code
    KXmlRpc::Client *c = new Client(KUrl( "http://localhost" ), this);
    c->setUserAgent( "Test/1.0" );
    c->call( "xmlrpc.command1", "Hi!",
       this, SLOT( gotData( const QList<QVariant>&, const QVariant ) ),
       this, SLOT( gotError( const QString&, const QVariant& ) ) );
  @endcode

  @author Narayan Newton <narayannewton@gmail.com>
 */
class KXMLRPCCLIENT_EXPORT Client : public QObject
{
  Q_OBJECT

  public:
    /**
			Constructs a XML-RPC #Client.

      @param parent the parent of this object, defaults to NULL.
     */
    Client( QObject *parent = 0 );

    /**
      Constructs a XML-RPC #Client, which will connect to @p url.

      @param url the url of the xml-rpc server.
      @param parent the parent of this object, defaults to NULL.
     */
    explicit Client( const KUrl &url, QObject *parent = 0 );

    /**
      Destroys the XML-RPC #Client.
     */
    virtual ~Client();

    /**
      Returns the current url the XML-RPC #Client will connect to.

      @see setUrl()
     */
    KUrl url() const;

    /**
      Sets the url the #Client will connect to.

      @param url the url for the xml-rpc server we will be connecting to.

      @see url()
     */
    void setUrl( const KUrl &url );

    /**
      Returns the user agent string currently used by the #Client.

      @see setUserAgent()
     */
    QString userAgent() const;

    /**
      Sets the userAgent string the #Client will use to identify itself.

      @param userAgent the user agent string to use.

      @see userAgent()
     */
    void setUserAgent( const QString &userAgent );

    /**
      Returns true if HTTP-Digest authentication is enabled, false
      if not.

      @see enableDigestAuth(), disableDigestAuth()
     */
    bool digestAuth() const;

    /**
      Sets HTTP-Digest authentication on.

      @see digestAuth(), disableDigestAuth()
     */
    void enableDigestAuth();

    /**
      Sets HTTP-Digest authentication off.

      @see enableDigestAuth(), digestAuth()
     */
    void disableDigestAuth();

  public slots:
    /**
      Calls the given method on a XML-RPC server, with the given
      argument list.

      @param method the method on the server we are going to be calling
      @param arg the argument list to pass to the server
      @param obj the object containing the error slot
      @param faultSlot the error slot itself
      @param obj the object containing the data slot
      @param messageSlot the data slot itself
      @param id the id for our #Client object, defaults to empty
     */
    void call( const QString &method, const QList<QVariant> &args,
        QObject *msgObj, const char *messageSlot,
        QObject *faultObj, const char *faultSlot,
        const QVariant &id = QVariant() );

    /**
      Calls the given method on a XML-RPC server, with the given
      argument.

      @param method the method on the server we are going to be calling
      @param arg the argument to pass to the server
      @param obj the object containing the error slot
      @param faultSlot the error slot itself
      @param obj the object containing the data slot
      @param messageSlot the data slot itself
      @param id the id for our #Client object, defaults to empty
     */
    void call( const QString &method, const QVariant &arg,
        QObject *msgObj, const char *messageSlot,
        QObject *faultObj, const char *faultSlot,
        const QVariant &id = QVariant() );

    /**
      Calls the given method on a XML-RPC server, with the given
      int as the argument.

      @param method the method on the server we are going to be calling
      @param arg the int to pass to the server
      @param obj the object containing the error slot
      @param faultSlot the error slot itself
      @param obj the object containing the data slot
      @param messageSlot the data slot itself
      @param id the id for our #Client object, defaults to empty
     */
    void call( const QString &method, int arg ,
        QObject *msgObj, const char *messageSlot,
        QObject *faultObj, const char *faultSlot,
        const QVariant &id = QVariant() );

    /**
      Calls the given method on a XML-RPC server, with the given
      bool as the argument.

      @param method the method on the server we are going to be calling
      @param arg the bool to pass to the server
      @param obj the object containing the error slot
      @param faultSlot the error slot itself
      @param obj the object containing the data slot
      @param messageSlot the data slot itself
      @param id the id for our #Client object, defaults to empty
     */
    void call( const QString &method, bool arg,
        QObject *msgObj, const char *messageSlot,
        QObject *faultObj, const char *faultSlot,
        const QVariant &id = QVariant() );

    /**
      Calls the given method on a XML-RPC server, with the given
      double as the argument.

      @param method the method on the server we are going to be calling
      @param arg the double to pass to the server
      @param obj the object containing the error slot
      @param faultSlot the error slot itself
      @param obj the object containing the data slot
      @param messageSlot the data slot itself
      @param id the id for our #Client object, defaults to empty
     */
    void call( const QString &method, double arg,
        QObject *msgObj, const char *messageSlot,
        QObject *faultObj, const char *faultSlot,
        const QVariant &id = QVariant() );

    /**
      Calls the given method on a XML-RPC server, with the given
      string as the argument.

      @param method the method on the server we are going to be calling
      @param arg the string to pass to the server
      @param obj the object containing the error slot
      @param faultSlot the error slot itself
      @param obj the object containing the data slot
      @param messageSlot the data slot itself
      @param id the id for our #Client object, defaults to empty
     */
    void call( const QString &method, const QString &arg,
        QObject *msgObj, const char *messageSlot,
        QObject *faultObj, const char *faultSlot,
        const QVariant &id = QVariant() );

    /**
      Calls the given method on a XML-RPC server, with the given
      byte array as the argument.

      @param method the method on the server we are going to be calling
      @param arg the array to pass to the server
      @param obj the object containing the error slot
      @param faultSlot the error slot itself
      @param obj the object containing the data slot
      @param messageSlot the data slot itself
      @param id the id for our #Client object, defaults to empty
     */
    void call( const QString &method, const QByteArray &arg,
        QObject *msgObj, const char *messageSlot,
        QObject *faultObj, const char *faultSlot,
        const QVariant &id = QVariant() );

    /**
      Calls the given method on a XML-RPC server, with the given
      date as the argument

      @param method the method on the server we are going to be calling
      @param arg the date and/or time to pass to the server
      @param obj the object containing the error slot
      @param faultSlot the error slot itself
      @param obj the object containing the data slot
      @param messageSlot the data slot itself
      @param id the id for our #Client object, defaults to empty
     */
    void call( const QString &method, const QDateTime &arg,
        QObject *msgObj, const char *messageSlot,
        QObject *faultObj, const char *faultSlot,
        const QVariant &id = QVariant() );

    /**
      Calls the given method on a XML-RPC server, with the given
      string list as the argument

      @param method the method on the server we are going to be calling
      @param arg the list of strings to pass to the server
      @param obj the object containing the error slot
      @param faultSlot the error slot itself
      @param obj the object containing the data slot
      @param messageSlot the data slot itself
      @param id the id for our #Client object, defaults to empty
     */
    void call( const QString &method, const QStringList &arg,
        QObject *msgObj, const char *messageSlot,
        QObject *faultObj, const char *faultSlot,
        const QVariant &id = QVariant() );

  private:
    class Private;
    Private *const d;

    template <typename T>
    void call( const QString &method, const QList<T> &arg,
        QObject *obj1, const char *messageSlot,
        QObject *obj2, const char *faultSlot,
        const QVariant &id = QVariant() );

    Q_PRIVATE_SLOT( d, void queryFinished( Query * ) )
};

template <typename T>
void Client::call( const QString &method, const QList<T> &arg,
                   QObject *msgObj, const char *messageSlot,
                   QObject *faultObj, const char *faultSlot,
                   const QVariant &id )
{
  QList<QVariant> args;

  for ( int i = 0; i < arg.count(); ++i ) {
    args << QVariant( arg[ i ] );
  }

  return call( method, args, faultObj, faultSlot, msgObj, messageSlot, id );
}

}

#endif

