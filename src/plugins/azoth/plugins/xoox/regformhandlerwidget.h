/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
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

#pragma once

#include <QWidget>
#include <QXmppClient.h>
#include "legacyformbuilder.h"
#include "formbuilder.h"

class QXmppClient;

namespace LC
{
namespace Azoth
{
namespace Xoox
{
	class XMPPBobManager;

	class RegFormHandlerWidget : public QWidget
	{
		Q_OBJECT

		QXmppClient *Client_;
		XMPPBobManager *BobManager_;
		LegacyFormBuilder LFB_;
		FormBuilder FB_;
		QWidget *Widget_;

		QString LastStanzaID_;

		QString ReqJID_;

		enum FormType
		{
			FTLegacy,
			FTNew
		} FormType_;
	public:
		enum class State
		{
			Error,
			Idle,
			Connecting,
			FetchingForm,
			AwaitingUserInput,
			AwaitingRegistrationResult
		};
	private:
		State State_;
	public:
		RegFormHandlerWidget (QXmppClient*, QWidget* = 0);

		QString GetUser () const;
		QString GetPassword () const;

		bool IsComplete () const;

		void HandleConnecting (const QString&);

		void SendRequest (const QString& jid = QString ());
		void Register ();
	private:
		void Clear ();
		void ShowMessage (const QString&);
		void SetState (State);
		void HandleRegForm (const QXmppIq&);
		void HandleRegResult (const QXmppIq&);
	private slots:
		void handleError (QXmppClient::Error);
		void handleIqReceived (const QXmppIq&);
	signals:
		void completeChanged ();
		void successfulReg ();
		void regError (const QString&);
	};
}
}
}
