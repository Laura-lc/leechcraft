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

#include "msnbuddyentry.h"
#include <algorithm>
#include <QImage>
#include <msn/notificationserver.h>
#include <util/sll/prelude.h>
#include <interfaces/azoth/iproxyobject.h>
#include <interfaces/azoth/azothutil.h>
#include "msnaccount.h"
#include "msnmessage.h"
#include "zheetutil.h"
#include "core.h"
#include "groupmanager.h"
#include "sbmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Zheet
{
	MSNBuddyEntry::MSNBuddyEntry (const MSN::Buddy& buddy, MSNAccount *acc)
	: QObject (acc)
	, Account_ (acc)
	, Buddy_ (buddy)
	, Groups_ (Util::Map (buddy.groups, ZheetUtil::FromStd))
	{
		qDebug () << Q_FUNC_INFO << Groups_;
		std::for_each (buddy.properties.cbegin (), buddy.properties.cend (),
				[] (decltype (*buddy.properties.cbegin ()) item) { qDebug () << item.first.c_str () << ": " << item.second.c_str (); });

		try
		{
			ContactID_ = ZheetUtil::FromStd (buddy.properties.at ("contactId"));
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to get contact ID for"
					<< GetHumanReadableID ()
					<< e.what ();
			throw;
		}
	}

	void MSNBuddyEntry::HandleMessage (MSNMessage *msg)
	{
		AllMessages_ << msg;
		emit gotMessage (msg);
	}

	void MSNBuddyEntry::HandleNudge ()
	{
		emit attentionDrawn (QString (), QString ());
	}

	void MSNBuddyEntry::UpdateState (State st)
	{
		const auto& oldVars = Variants ();
		Status_.State_ = st;

		if (oldVars != Variants ())
			emit availableVariantsChanged (Variants ());
		emit statusChanged (Status_, QString ());
	}

	void MSNBuddyEntry::AddGroup (const QString& group)
	{
		if (Groups_.contains (group))
			return;

		Groups_ << group;
		emit groupsChanged (Groups_);
	}

	void MSNBuddyEntry::RemoveGroup (const QString& group)
	{
		if (Groups_.removeOne (group))
			emit groupsChanged (Groups_);
	}

	QString MSNBuddyEntry::GetContactID () const
	{
		return ContactID_;
	}

	QObject* MSNBuddyEntry::GetQObject ()
	{
		return this;
	}

	MSNAccount* MSNBuddyEntry::GetParentAccount () const
	{
		return Account_;
	}

	ICLEntry::Features MSNBuddyEntry::GetEntryFeatures () const
	{
		return FPermanentEntry |
				//FSupportsAuth |
				FSupportsGrouping;
	}

	ICLEntry::EntryType MSNBuddyEntry::GetEntryType () const
	{
		return EntryType::Chat;
	}

	QString MSNBuddyEntry::GetEntryName () const
	{
		QString res = ZheetUtil::FromStd (Buddy_.friendlyName);
		if (res.isEmpty ())
			res = GetHumanReadableID ();
		return res;
	}

	void MSNBuddyEntry::SetEntryName (const QString&)
	{
	}

	QString MSNBuddyEntry::GetEntryID () const
	{
		return Account_->GetAccountID () + "_" + GetHumanReadableID ();
	}

	QString MSNBuddyEntry::GetHumanReadableID () const
	{
		return ZheetUtil::FromStd (Buddy_.userName);
	}

	QStringList MSNBuddyEntry::Groups () const
	{
		return Groups_;
	}

	void MSNBuddyEntry::SetGroups (const QStringList& groups)
	{
		Account_->GetGroupManager ()->SetGroups (this, groups, Groups ());
	}

	QStringList MSNBuddyEntry::Variants () const
	{
		return Status_.State_ == SOffline ?
				QStringList () :
				QStringList (QString ());
	}

	IMessage* MSNBuddyEntry::CreateMessage (IMessage::Type type, const QString&, const QString& body)
	{
		MSNMessage *msg = new MSNMessage (IMessage::Direction::Out, type, this);
		msg->SetBody (body);
		return msg;
	}

	QList<IMessage*> MSNBuddyEntry::GetAllMessages () const
	{
		QList<IMessage*> result;
		std::copy (AllMessages_.begin (), AllMessages_.end (), std::back_inserter (result));
		return result;
	}

	void MSNBuddyEntry::PurgeMessages (const QDateTime& before)
	{
		AzothUtil::StandardPurgeMessages (AllMessages_, before);
	}

	void MSNBuddyEntry::SetChatPartState (ChatPartState, const QString&)
	{
	}

	EntryStatus MSNBuddyEntry::GetStatus (const QString&) const
	{
		return Status_;
	}

	void MSNBuddyEntry::ShowInfo ()
	{
	}

	QList<QAction*> MSNBuddyEntry::GetActions () const
	{
		return QList<QAction*> ();
	}

	QMap<QString, QVariant> MSNBuddyEntry::GetClientInfo (const QString&) const
	{
		return QMap<QString, QVariant> ();
	}

	void MSNBuddyEntry::MarkMsgsRead ()
	{
	}

	void MSNBuddyEntry::ChatTabClosed ()
	{
	}

	IAdvancedCLEntry::AdvancedFeatures MSNBuddyEntry::GetAdvancedFeatures () const
	{
		return AFSupportsAttention;
	}

	void MSNBuddyEntry::DrawAttention (const QString& text, const QString&)
	{
		Account_->GetSBManager ()->SendNudge (text, this);
	}
}
}
}
