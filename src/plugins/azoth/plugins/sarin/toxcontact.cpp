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

#include "toxcontact.h"
#include <QImage>
#include <interfaces/azoth/azothutil.h>
#include "toxaccount.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	ToxContact::ToxContact (const QByteArray& pubkey, ToxAccount *account)
	: QObject { account }
	, Pubkey_ { pubkey }
	, Acc_ { account }
	{
	}

	QObject* ToxContact::GetQObject ()
	{
		return this;
	}

	QObject* ToxContact::GetParentAccount () const
	{
		return Acc_;
	}

	ICLEntry::Features ToxContact::GetEntryFeatures () const
	{
		return FPermanentEntry;
	}

	ICLEntry::EntryType ToxContact::GetEntryType () const
	{
		return EntryType::Chat;
	}

	QString ToxContact::GetEntryName () const
	{
		return Pubkey_;
	}

	void ToxContact::SetEntryName (const QString&)
	{
	}

	QString ToxContact::GetEntryID () const
	{
		return Acc_->GetAccountID () + '/' + Pubkey_;
	}

	QString ToxContact::GetHumanReadableID () const
	{
		return Pubkey_;
	}

	QStringList ToxContact::Groups () const
	{
		return {};
	}

	void ToxContact::SetGroups (const QStringList&)
	{
	}

	QStringList ToxContact::Variants () const
	{
		return { "" };
	}

	QObject* ToxContact::CreateMessage (IMessage::Type type, const QString&, const QString& body)
	{
		return nullptr;
	}

	QList<QObject*> ToxContact::GetAllMessages () const
	{
		return {};
	}

	void ToxContact::PurgeMessages (const QDateTime& before)
	{
	}

	void ToxContact::SetChatPartState (ChatPartState state, const QString&)
	{
	}

	EntryStatus ToxContact::GetStatus (const QString&) const
	{
		return {};
	}

	QImage ToxContact::GetAvatar () const
	{
		return {};
	}

	void ToxContact::ShowInfo ()
	{
	}

	QList<QAction*> ToxContact::GetActions () const
	{
		return {};
	}

	QMap<QString, QVariant> ToxContact::GetClientInfo (const QString&) const
	{
		return {};
	}

	void ToxContact::MarkMsgsRead ()
	{
	}

	void ToxContact::ChatTabClosed ()
	{
	}
}
}
}
