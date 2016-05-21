/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 * Copyright (C) 2010-2012  Oleg Linkin
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "ircserversocket.h"
#include <QTcpSocket>
#include <QTextCodec>
#include <QSettings>
#include "ircserverhandler.h"
#include "clientconnection.h"
#include "sslerrorsdialog.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Acetamide
{
	int elideWidth = 300;

	IrcServerSocket::IrcServerSocket (IrcServerHandler *ish)
	: QObject (ish)
	, ISH_ (ish)
	, SSL_ (ish->GetServerOptions ().SSL_)
	{
		Socket_ptr.reset (SSL_ ? new QSslSocket : new QTcpSocket);
		Init ();
	}

	void IrcServerSocket::ConnectToHost (const QString& host, int port)
	{
		if (!SSL_)
			Socket_ptr->connectToHost (host, port);
		else
		{
			std::shared_ptr<QSslSocket> s = std::dynamic_pointer_cast<QSslSocket> (Socket_ptr);
			s->connectToHostEncrypted (host, port);
		}
	}

	void IrcServerSocket::DisconnectFromHost ()
	{
		Socket_ptr->disconnectFromHost ();
	}

	void IrcServerSocket::Send (const QString& message)
	{
		if (!Socket_ptr->isWritable ())
		{
			qWarning () << Q_FUNC_INFO
					<< Socket_ptr->error ()
					<< Socket_ptr->errorString ();
			return;
		}

		RefreshCodec ();

		if (Socket_ptr->write (LastCodec_->fromUnicode (message)) == -1)
			qWarning () << Q_FUNC_INFO
					<< Socket_ptr->error ()
					<< Socket_ptr->errorString ();
	}

	void IrcServerSocket::Close ()
	{
		Socket_ptr->close ();
	}

	void IrcServerSocket::Init ()
	{
		connect (Socket_ptr.get (),
				SIGNAL (readyRead ()),
				this,
				SLOT (readReply ()));

		connect (Socket_ptr.get (),
				SIGNAL (connected ()),
				ISH_,
				SLOT (connectionEstablished ()));

		connect (Socket_ptr.get (),
				SIGNAL (disconnected ()),
				ISH_,
				SLOT (connectionClosed ()));

		connect (Socket_ptr.get (),
				SIGNAL (error (QAbstractSocket::SocketError)),
				ISH_,
				SLOT (handleSocketError (QAbstractSocket::SocketError)));

		if (SSL_)
			connect (Socket_ptr.get(),
					SIGNAL (sslErrors (const QList<QSslError> &)),
					this,
					SLOT (handleSslErrors (const QList<QSslError>&)));
	}

	void IrcServerSocket::RefreshCodec ()
	{
		const auto encoding = ISH_->GetServerOptions ().ServerEncoding_;
		if (LastCodec_ && LastCodec_->name () == encoding)
			return;

		if (const auto newCodec = QTextCodec::codecForName (encoding.toLatin1 ()))
		{
			LastCodec_ = newCodec;
			return;
		}

		qWarning () << Q_FUNC_INFO
				<< "unable to create codec for encoding `"
				<< encoding.toUtf8 ()
				<< "`; known codecs:"
				<< QTextCodec::availableCodecs ();

		if (LastCodec_)
			return;

		qWarning () << Q_FUNC_INFO
				<< "no codec is set, will fall back to locale-default codec";

		LastCodec_ = QTextCodec::codecForLocale ();
	}

	void IrcServerSocket::readReply ()
	{
		while (Socket_ptr->canReadLine ())
			ISH_->ReadReply (Socket_ptr->readLine ());
	}

	void IrcServerSocket::handleSslErrors (const QList<QSslError>& errors)
	{
		std::shared_ptr<QSslSocket> s = std::dynamic_pointer_cast<QSslSocket> (Socket_ptr);

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Azoth_Acetamide");
		settings.beginGroup ("SSL exceptions");
		QStringList keys = settings.allKeys ();
		const QString& key = s->peerName () + ":" + s->peerPort ();
		if (keys.contains (key))
		{
			if (settings.value (key).toBool ())
				s->ignoreSslErrors ();
		}
		else if (keys.contains (s->peerName ()))
		{
			if (settings.value (s->peerName ()).toBool ())
				s->ignoreSslErrors ();
		}
		else
		{
			QString msg = tr ("<code>%1</code><br />has SSL errors."
					" What do you want to do?")
						.arg (QApplication::fontMetrics ()
								.elidedText (key, Qt::ElideMiddle, elideWidth));

			std::unique_ptr<SslErrorsDialog> errDialog (new SslErrorsDialog ());
			errDialog->Update (msg, errors);

			bool ignore = (errDialog->exec () == QDialog::Accepted);

			SslErrorsDialog::RememberChoice choice = errDialog->GetRememberChoice ();

			if (choice != SslErrorsDialog::RCNot)
			{
				if (choice == SslErrorsDialog::RCFile)
					settings.setValue (key, ignore);
				else
					settings.setValue (s->peerName (), ignore);
			}

			if (ignore)
				s->ignoreSslErrors (errors);
		}
		settings.endGroup ();
	}

};
};
};
