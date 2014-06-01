/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2013  Oleg Linkin <MaledictusDeMagog@gmail.com>
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

#include "authmanager.h"
#include <QInputDialog>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMainWindow>
#include <qjson/parser.h>
#include <util/xpc/util.h>
#include "account.h"
#include "core.h"
#include <interfaces/core/irootwindowsmanager.h>

namespace LeechCraft
{
namespace NetStoreManager
{
namespace DBox
{
	AuthManager::AuthManager (QObject *parent)
	: QObject (parent)
	, ClientId_ ("9yg24d86mu0tijt")
	, ClientSecret_ ("izq73ex1ei0uf5i")
	, ResponseType_ ("code")
	{
	}

	void AuthManager::Auth (Account *acc)
	{
		QUrl url (QString ("https://www.dropbox.com/1/oauth2/authorize?client_id=%1&response_type=%2")
				.arg (ClientId_)
				.arg (ResponseType_));

		Entity e = Util::MakeEntity (url,
				QString (),
				FromUserInitiated | OnlyHandle);
		emit gotEntity (e);

		auto rootWM = Core::Instance ().GetProxy ()->GetRootWindowsManager ();
		InputDialog_ = new QInputDialog (rootWM->GetPreferredWindow (), Qt::Widget);
		Dialog2Account_ [InputDialog_] = acc;
		connect (InputDialog_,
				SIGNAL (finished (int)),
				this,
				SLOT (handleDialogFinished (int)));

		InputDialog_->setLabelText (tr ("A browser window will pop up with a request for "
				"permissions to access your DropBox account. Once you accept it, a "
				"verification code will appear. Enter that verification code in the box below:"));
		InputDialog_->setWindowTitle (tr ("Account configuration"));
		InputDialog_->setTextEchoMode (QLineEdit::Normal);

		InputDialog_->show ();
		InputDialog_->activateWindow ();
	}

	void AuthManager::RequestAuthToken (const QString& code, Account *acc)
	{
		QNetworkRequest request (QUrl ("https://api.dropbox.com/1/oauth2/token"));
		QString str = QString ("code=%1&client_id=%2&client_secret=%3&grant_type=%4")
				.arg (code)
				.arg (ClientId_)
				.arg (ClientSecret_)
				.arg ("authorization_code");

		request.setHeader (QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

		QNetworkReply *reply = Core::Instance ().GetProxy ()->
				GetNetworkAccessManager ()->post (request, str.toUtf8 ());
		Reply2Account_ [reply] = acc;

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleRequestAuthTokenFinished ()));
	}

	void AuthManager::handleDialogFinished (int code)
	{
		InputDialog_->deleteLater ();
		Account *acc = Dialog2Account_.take (InputDialog_);
		std::shared_ptr<void> guard (static_cast<void*> (0),
				[this] (void*) { InputDialog_ = 0; });

		if (code == QDialog::Rejected)
			return;

		if (InputDialog_ &&
				InputDialog_->textValue ().isEmpty ())
			return;

		RequestAuthToken (InputDialog_->textValue (), acc);
	}

	void AuthManager::handleRequestAuthTokenFinished ()
	{
		QNetworkReply *reply = qobject_cast<QNetworkReply*> (sender ());
		if (!reply)
			return;

		Account *acc = Reply2Account_.take (reply);
		reply->deleteLater ();

		QByteArray data = reply->readAll ();

		bool ok = false;
		QVariant res = QJson::Parser ().parse (data, &ok);
		if (!ok)
			return;

		QVariantMap map = res.toMap ();

		if (map.contains ("error"))
			return;

		if (map.contains ("access_token"))
			acc->SetAccessToken (map ["access_token"].toString ());
		if (map.contains ("uid"))
			acc->SetUserID (map ["uid"].toString ());
		acc->SetTrusted (true);

		emit authSuccess (acc);
	}

}
}
}
