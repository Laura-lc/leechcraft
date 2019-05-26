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

#ifndef PLUGINS_AZOTH_PLUGINS_XOOX_GLOOXMESSAGE_H
#define PLUGINS_AZOTH_PLUGINS_XOOX_GLOOXMESSAGE_H
#include <QObject>
#include <QXmppMessage.h>
#include <interfaces/azoth/imessage.h>
#include <interfaces/azoth/iadvancedmessage.h>
#include <interfaces/azoth/irichtextmessage.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	class GlooxCLEntry;
	class ClientConnection;

	class GlooxMessage : public QObject
					   , public IMessage
					   , public IAdvancedMessage
					   , public IRichTextMessage
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Azoth::IMessage
				LeechCraft::Azoth::IAdvancedMessage
				LeechCraft::Azoth::IRichTextMessage)

		Type Type_;
		SubType SubType_ = SubType::Other;
		Direction Direction_;
		QString BareJID_;
		QString Variant_;
		QDateTime DateTime_;
		QXmppMessage Message_;
		ClientConnection *Connection_;

		bool IsDelivered_ = false;
	public:
		GlooxMessage (IMessage::Type type,
				IMessage::Direction direction,
				const QString& jid,
				const QString& variant,
				ClientConnection *conn);
		GlooxMessage (const QXmppMessage& msg,
				ClientConnection *conn);

		// IMessage
		QObject* GetQObject () override;
		void Send () override;
		void Store () override;
		Direction GetDirection () const override;
		Type GetMessageType () const override;
		SubType GetMessageSubType () const override;
		void SetMessageSubType (SubType);
		QObject* OtherPart () const override;
		QString GetOtherVariant () const override;
		QString GetBody () const override;
		void SetBody (const QString&) override;
		QDateTime GetDateTime () const override;
		void SetDateTime (const QDateTime&) override;

		// IAdvancedMessage
		bool IsDelivered () const override;

		// IRichTextMessage
		QString GetRichBody () const override;
		void SetRichBody (const QString&) override;

		void SetReceiptRequested (bool);
		void SetDelivered (bool);

		void SetVariant (const QString&);

		const QXmppMessage& GetNativeMessage () const;
	signals:
		void messageDelivered () override;
	};
}
}
}

#endif
