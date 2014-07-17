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

#include "msnmessage.h"
#include <msn/message.h>
#include <msn/notificationserver.h>
#include "msnbuddyentry.h"
#include "msnaccount.h"
#include "zheetutil.h"
#include "sbmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Zheet
{
	MSNMessage::MSNMessage (Direction dir, Type type, MSNBuddyEntry *entry)
	: QObject (entry)
	, Entry_ (entry)
	, Dir_ (dir)
	, MT_ (type)
	, MST_ (SubType::Other)
	, DateTime_ (QDateTime::currentDateTime ())
	, IsDelivered_ (dir == Direction::In)
	, MsgID_ (-1)
	{
	}

	MSNMessage::MSNMessage (MSN::Message *msg, MSNBuddyEntry *entry)
	: QObject (entry)
	, Entry_ (entry)
	, Dir_ (Direction::In)
	, MT_ (Type::ChatMessage)
	, Body_ (ZheetUtil::FromStd (msg->getBody ()))
	, DateTime_ (QDateTime::currentDateTime ())
	, IsDelivered_ (true)
	, MsgID_ (-1)
	{
	}

	int MSNMessage::GetID () const
	{
		return MsgID_;
	}

	void MSNMessage::SetID (int id)
	{
		MsgID_ = id;
	}

	void MSNMessage::SetDelivered ()
	{
		if (IsDelivered_)
			return;

		IsDelivered_ = true;
		emit messageDelivered ();
	}

	QObject* MSNMessage::GetQObject ()
	{
		return this;
	}

	void MSNMessage::Send ()
	{
		Entry_->HandleMessage (this);

		auto acc = qobject_cast<MSNAccount*> (Entry_->GetParentAccount ());
		acc->GetSBManager ()->SendMessage (this, Entry_);
	}

	void MSNMessage::Store ()
	{
	}

	IMessage::Direction MSNMessage::GetDirection () const
	{
		return Dir_;
	}

	IMessage::Type MSNMessage::GetMessageType () const
	{
		return MT_;
	}

	IMessage::SubType MSNMessage::GetMessageSubType () const
	{
		return MST_;
	}

	QObject* MSNMessage::OtherPart () const
	{
		return Entry_;
	}

	QString MSNMessage::GetOtherVariant () const
	{
		return QString ();
	}

	QString MSNMessage::GetBody () const
	{
		return Body_;
	}

	void MSNMessage::SetBody (const QString& body)
	{
		Body_ = body;
	}

	QDateTime MSNMessage::GetDateTime () const
	{
		return DateTime_;
	}

	void MSNMessage::SetDateTime (const QDateTime& timestamp)
	{
		DateTime_ = timestamp;
	}

	bool MSNMessage::IsDelivered () const
	{
		return IsDelivered_;
	}
}
}
}
