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

#include "core.h"
#include <QStandardItemModel>
#include <QSettings>
#include <QCoreApplication>
#include <QtDebug>

#ifdef Q_OS_WIN32
#include <vmime/platforms/windows/windowsHandler.hpp>
#else
#include <vmime/platforms/posix/posixHandler.hpp>
#endif

#include <util/sys/resourceloader.h>
#include <util/sll/delayedexecutor.h>
#include <interfaces/core/ientitymanager.h>
#include "message.h"
#include "storage.h"
#include "progressmanager.h"
#include "accountfoldermanager.h"
#include "composemessagetab.h"

namespace LeechCraft
{
namespace Snails
{
	Core::Core ()
	: AccountsModel_ { new QStandardItemModel }
	, Storage_ { new Storage { this } }
	, ProgressManager_ { new ProgressManager { this } }
	, MsgView_ { new Util::ResourceLoader { "snails/msgview" } }
	{
#ifdef Q_OS_WIN32
		vmime::platform::setHandler<vmime::platforms::windows::windowsHandler> ();
#else
		vmime::platform::setHandler<vmime::platforms::posix::posixHandler> ();
#endif

		MsgView_->AddGlobalPrefix ();
		MsgView_->AddLocalPrefix ();

		qRegisterMetaType<size_t> ("size_t");
		qRegisterMetaType<Message_ptr> ("LeechCraft::Snails::Message_ptr");
		qRegisterMetaType<Message_ptr> ("Message_ptr");
		qRegisterMetaType<QList<Message_ptr>> ("QList<LeechCraft::Snails::Message_ptr>");
		qRegisterMetaType<QList<Message_ptr>> ("QList<Message_ptr>");
		qRegisterMetaType<AttDescr> ("LeechCraft::Snails::AttDescr");
		qRegisterMetaType<AttDescr> ("AttDescr");
		qRegisterMetaType<ProgressListener_g_ptr> ("ProgressListener_g_ptr");
		qRegisterMetaType<QList<QStringList>> ("QList<QStringList>");
		qRegisterMetaType<QList<QByteArray>> ("QList<QByteArray>");
		qRegisterMetaType<Folder> ("LeechCraft::Snails::Folder");
		qRegisterMetaType<QList<Folder>> ("QList<LeechCraft::Snails::Folder>");

		qRegisterMetaTypeStreamOperators<AttDescr> ();
		qRegisterMetaTypeStreamOperators<Folder> ();
		qRegisterMetaTypeStreamOperators<QList<Folder>> ();

		AccountsModel_->setHorizontalHeaderLabels ({ tr ("Name"), tr ("Server") });

		new Util::DelayedExecutor
		{
			[this] { LoadAccounts (); },
			0
		};
	}

	Core& Core::Instance ()
	{
		static Core c;
		return c;
	}

	void Core::Release ()
	{
		MsgView_.reset ();
	}

	void Core::SetProxy (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
	}

	ICoreProxy_ptr Core::GetProxy () const
	{
		return Proxy_;
	}

	void Core::SendEntity (const Entity& e)
	{
		Proxy_->GetEntityManager ()->HandleEntity (e);
	}

	QAbstractItemModel* Core::GetAccountsModel () const
	{
		return AccountsModel_;
	}

	QList<Account_ptr> Core::GetAccounts () const
	{
		return Accounts_;
	}

	Account_ptr Core::GetAccount (const QModelIndex& index) const
	{
		if (!index.isValid ())
			return Account_ptr ();

		return Accounts_ [index.row ()];
	}

	Storage* Core::GetStorage () const
	{
		return Storage_;
	}

	ProgressManager* Core::GetProgressManager () const
	{
		return ProgressManager_;
	}

	QString Core::GetMsgViewTemplate () const
	{
		auto dev = MsgView_->Load ("default/msgview.css");
		if (!dev || !dev->open (QIODevice::ReadOnly))
			return {};

		return QString::fromUtf8 (dev->readAll ());
	}

	void Core::PrepareReplyTab (const Message_ptr& message, const Account_ptr& account)
	{
		auto cmt = new ComposeMessageTab ();
		cmt->SelectAccount (account);
		cmt->PrepareReply (message);
		emit gotTab (cmt->GetTabClassInfo ().VisibleName_, cmt);
	}

	void Core::AddAccount (Account_ptr account)
	{
		AddAccountImpl (account);

		saveAccounts ();
	}

	void Core::AddAccountImpl (Account_ptr account)
	{
		Accounts_ << account;

		QList<QStandardItem*> row;
		row << new QStandardItem (account->GetName ());
		row << new QStandardItem (account->GetServer ());
		AccountsModel_->appendRow (row);

		ProgressManager_->AddAccount (account.get ());

		connect (account.get (),
				SIGNAL (accountChanged ()),
				this,
				SLOT (saveAccounts ()));
		connect (account->GetFolderManager (),
				SIGNAL (foldersUpdated ()),
				this,
				SLOT (saveAccounts ()));
	}

	void Core::LoadAccounts ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Snails_Accounts");
		Q_FOREACH (const QVariant& var, settings.value ("Accounts").toList ())
		{
			Account_ptr acc (new Account);
			try
			{
				acc->Deserialize (var.toByteArray ());
			}
			catch (const std::exception& e)
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to deserialize account, sorry :("
						<< e.what ();
				continue;
			}
			AddAccountImpl (acc);
		}
	}

	void Core::saveAccounts () const
	{
		QList<QVariant> serialized;
		Q_FOREACH (Account_ptr acc, Accounts_)
			serialized << acc->Serialize ();

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Snails_Accounts");
		settings.setValue ("Accounts", serialized);
	}
}
}
