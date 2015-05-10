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

#include "account.h"
#include <stdexcept>
#include <QUuid>
#include <QDataStream>
#include <QInputDialog>
#include <QMutex>
#include <QStandardItemModel>
#include <util/xpc/util.h>
#include <util/sll/slotclosure.h>
#include <util/xpc/passutils.h>
#include "core.h"
#include "accountconfigdialog.h"
#include "accountthread.h"
#include "accountthreadworker.h"
#include "storage.h"
#include "accountfoldermanager.h"
#include "mailmodel.h"
#include "taskqueuemanager.h"
#include "foldersmodel.h"
#include "mailmodelsmanager.h"
#include "accountlogger.h"

Q_DECLARE_METATYPE (QList<QStringList>)
Q_DECLARE_METATYPE (QList<QByteArray>)

namespace LeechCraft
{
namespace Snails
{
	Account::Account (QObject *parent)
	: QObject (parent)
	, Logger_ (new AccountLogger (this))
	, Thread_ (new AccountThread (true, "OpThread", this))
	, MessageFetchThread_ (new AccountThread (false, "MessageFetchThread", this))
	, AccMutex_ (new QMutex (QMutex::Recursive))
	, ID_ (QUuid::createUuid ().toByteArray ())
	, FolderManager_ (new AccountFolderManager (this))
	, FoldersModel_ (new FoldersModel (this))
	, MailModelsManager_ (new MailModelsManager (this))
	{
		Thread_->AddTask ({ "setNoopTimeout", { KeepAliveInterval_ } });
		MessageFetchThread_->AddTask ({ "setNoopTimeout", { KeepAliveInterval_ } });

		Thread_->start (QThread::IdlePriority);
		MessageFetchThread_->start (QThread::LowPriority);

		connect (FolderManager_,
				SIGNAL (foldersUpdated ()),
				this,
				SLOT (handleFoldersUpdated ()));
	}

	QByteArray Account::GetID () const
	{
		return ID_;
	}

	QString Account::GetName () const
	{
		return AccName_;
	}

	QString Account::GetServer () const
	{
		return InHost_ + ':' + QString::number (InPort_);
	}

	AccountLogger* Account::GetLogger () const
	{
		return Logger_;
	}

	AccountFolderManager* Account::GetFolderManager () const
	{
		return FolderManager_;
	}

	MailModelsManager* Account::GetMailModelsManager () const
	{
		return MailModelsManager_;
	}

	QAbstractItemModel* Account::GetFoldersModel () const
	{
		return FoldersModel_;
	}

	void Account::Synchronize ()
	{
		auto folders = FolderManager_->GetSyncFolders ();
		if (folders.isEmpty ())
			folders << QStringList ("INBOX");

		Thread_->AddTask ({
				"synchronize",
				{
					{ folders },
					QByteArray {}
				}
			});
	}

	void Account::Synchronize (const QStringList& path, const QByteArray& last)
	{
		Thread_->AddTask ({
				"synchronize",
				{
					QList<QStringList> { path },
					last
				},
				"syncFolder"
			});
	}

	void Account::FetchWholeMessage (const Message_ptr& msg)
	{
		MessageFetchThread_->AddTask ({
				"fetchWholeMessage",
				{ msg }
			});
	}

	QFuture<void> Account::SendMessage (const Message_ptr& msg)
	{
		auto pair = msg->GetAddress (Message::Address::From);
		if (pair.first.isEmpty ())
			pair.first = UserName_;
		if (pair.second.isEmpty ())
			pair.second = UserEmail_;
		msg->SetAddress (Message::Address::From, pair);

		return MessageFetchThread_->AddTask ({
				"sendMessage",
				{ msg }
			});
	}

	void Account::FetchAttachment (const Message_ptr& msg,
			const QString& attName, const QString& path)
	{
		MessageFetchThread_->AddTask ({
				"fetchAttachment",
				{
					msg,
					attName,
					path
				}
			});
	}

	void Account::SetReadStatus (bool read, const QList<QByteArray>& ids, const QStringList& folder)
	{
		MessageFetchThread_->AddTask ({
				"setReadStatus",
				{
					read,
					ids,
					folder
				}
			});
	}

	void Account::CopyMessages (const QList<QByteArray>& ids, const QStringList& from, const QList<QStringList>& to)
	{
		MessageFetchThread_->AddTask ({
				"copyMessages",
				{
					ids,
					from,
					to
				}
			});
	}

	void Account::MoveMessages (const QList<QByteArray>& ids, const QStringList& from, const QList<QStringList>& to)
	{
		CopyMessages (ids, from, to);
		DeleteMessages (ids, from);
	}

	void Account::DeleteMessages (const QList<QByteArray>& ids, const QStringList& folder)
	{
		MessageFetchThread_->AddTask ({
				"deleteMessages",
				{
					ids,
					folder
				}
			});
	}

	QByteArray Account::Serialize () const
	{
		QMutexLocker l (GetMutex ());

		QByteArray result;

		QDataStream out (&result, QIODevice::WriteOnly);
		out << static_cast<quint8> (2);
		out << ID_
			<< AccName_
			<< Login_
			<< UseSASL_
			<< SASLRequired_
			<< UseTLS_
			<< UseSSL_
			<< InSecurityRequired_
			<< static_cast<qint8> (OutSecurity_)
			<< OutSecurityRequired_
			<< SMTPNeedsAuth_
			<< InHost_
			<< InPort_
			<< OutHost_
			<< OutPort_
			<< OutLogin_
			<< static_cast<quint8> (OutType_)
			<< UserName_
			<< UserEmail_
			<< FolderManager_->Serialize ()
			<< KeepAliveInterval_;

		return result;
	}

	void Account::Deserialize (const QByteArray& arr)
	{
		QDataStream in (arr);
		quint8 version = 0;
		in >> version;

		if (version < 1 || version > 2)
			throw std::runtime_error { "Unknown version " + std::to_string (version) };

		quint8 outType = 0;
		qint8 type = 0;

		{
			QMutexLocker l (GetMutex ());
			in >> ID_
				>> AccName_
				>> Login_
				>> UseSASL_
				>> SASLRequired_
				>> UseTLS_
				>> UseSSL_
				>> InSecurityRequired_
				>> type
				>> OutSecurityRequired_;

			OutSecurity_ = static_cast<SecurityType> (type);

			in >> SMTPNeedsAuth_
				>> InHost_
				>> InPort_
				>> OutHost_
				>> OutPort_
				>> OutLogin_
				>> outType;

			OutType_ = static_cast<OutType> (outType);

			in >> UserName_
				>> UserEmail_;

			QByteArray fstate;
			in >> fstate;
			FolderManager_->Deserialize (fstate);

			handleFoldersUpdated ();

			if (version >= 2)
				in >> KeepAliveInterval_;
		}
	}

	void Account::OpenConfigDialog (const std::function<void ()>& onAccepted)
	{
		auto dia = new AccountConfigDialog;
		dia->setAttribute (Qt::WA_DeleteOnClose);

		{
			QMutexLocker l (GetMutex ());
			dia->SetName (AccName_);
			dia->SetUserName (UserName_);
			dia->SetUserEmail (UserEmail_);
			dia->SetLogin (Login_);
			dia->SetUseSASL (UseSASL_);
			dia->SetSASLRequired (SASLRequired_);

			if (UseSSL_)
				dia->SetInSecurity (SecurityType::SSL);
			else if (UseTLS_)
				dia->SetInSecurity (SecurityType::TLS);
			else
				dia->SetInSecurity (SecurityType::No);

			dia->SetInSecurityRequired (InSecurityRequired_);

			dia->SetOutSecurity (OutSecurity_);
			dia->SetOutSecurityRequired (OutSecurityRequired_);

			dia->SetSMTPAuth (SMTPNeedsAuth_);
			dia->SetInHost (InHost_);
			dia->SetInPort (InPort_);
			dia->SetOutHost (OutHost_);
			dia->SetOutPort (OutPort_);
			dia->SetOutLogin (OutLogin_);
			dia->SetOutType (OutType_);

			dia->SetKeepAliveInterval (KeepAliveInterval_);

			const auto& folders = FolderManager_->GetFoldersPaths ();
			dia->SetAllFolders (folders);
			const auto& toSync = FolderManager_->GetSyncFolders ();
			for (const auto& folder : folders)
			{
				const auto flags = FolderManager_->GetFolderFlags (folder);
				if (flags & AccountFolderManager::FolderOutgoing)
					dia->SetOutFolder (folder);
			}
			dia->SetFoldersToSync (toSync);
		}

		dia->open ();

		new Util::SlotClosure<Util::DeleteLaterPolicy>
		{
			[this, onAccepted, dia]
			{
				QMutexLocker l (GetMutex ());
				AccName_ = dia->GetName ();
				UserName_ = dia->GetUserName ();
				UserEmail_ = dia->GetUserEmail ();
				Login_ = dia->GetLogin ();
				UseSASL_ = dia->GetUseSASL ();
				SASLRequired_ = dia->GetSASLRequired ();

				UseSSL_ = false;
				UseTLS_ = false;
				switch (dia->GetInSecurity ())
				{
				case SecurityType::SSL:
					UseSSL_ = true;
					break;
				case SecurityType::TLS:
					UseTLS_ = true;
					break;
				case SecurityType::No:
					break;
				}

				InSecurityRequired_ = dia->GetInSecurityRequired ();

				OutSecurity_ = dia->GetOutSecurity ();
				OutSecurityRequired_ = dia->GetOutSecurityRequired ();

				SMTPNeedsAuth_ = dia->GetSMTPAuth ();
				InHost_ = dia->GetInHost ();
				InPort_ = dia->GetInPort ();
				OutHost_ = dia->GetOutHost ();
				OutPort_ = dia->GetOutPort ();
				OutLogin_ = dia->GetOutLogin ();
				OutType_ = dia->GetOutType ();

				if (KeepAliveInterval_ != dia->GetKeepAliveInterval ())
				{
					KeepAliveInterval_ = dia->GetKeepAliveInterval ();
					Thread_->AddTask ({ "setNoopTimeout", { KeepAliveInterval_ } });
					MessageFetchThread_->AddTask ({ "setNoopTimeout", { KeepAliveInterval_ } });
				}

				FolderManager_->ClearFolderFlags ();
				const auto& out = dia->GetOutFolder ();
				if (!out.isEmpty ())
					FolderManager_->AppendFolderFlags (out, AccountFolderManager::FolderOutgoing);

				for (const auto& sync : dia->GetFoldersToSync ())
					FolderManager_->AppendFolderFlags (sync, AccountFolderManager::FolderSyncable);

				emit accountChanged ();

				if (onAccepted)
					onAccepted ();
			},
			dia,
			SIGNAL (accepted ()),
			dia
		};
	}

	bool Account::IsNull () const
	{
		return AccName_.isEmpty () ||
			Login_.isEmpty ();
	}

	QString Account::GetInUsername ()
	{
		return Login_;
	}

	QString Account::GetOutUsername ()
	{
		return OutLogin_;
	}

	QMutex* Account::GetMutex () const
	{
		return AccMutex_;
	}

	QString Account::BuildInURL ()
	{
		QMutexLocker l (GetMutex ());

		QString result { UseSSL_ ? "imaps://" : "imap://" };
		result += Login_;
		result += ":";
		result.replace ('@', "%40");

		QString pass;
		getPassword (&pass);

		result += pass + '@';

		result += InHost_;

		qDebug () << Q_FUNC_INFO << result;

		return result;
	}

	QString Account::BuildOutURL ()
	{
		QMutexLocker l (GetMutex ());

		if (OutType_ == OutType::Sendmail)
			return "sendmail://localhost";

		QString result = OutSecurity_ == SecurityType::SSL ? "smtps://" : "smtp://";

		if (SMTPNeedsAuth_)
		{
			QString pass;
			if (OutLogin_.isEmpty ())
			{
				result += Login_;
				getPassword (&pass);
			}
			else
			{
				result += OutLogin_;
				getPassword (&pass, Direction::Out);
			}
			result += ":" + pass;

			result.replace ('@', "%40");
			result += '@';
		}

		result += OutHost_;

		qDebug () << Q_FUNC_INFO << result;

		return result;
	}

	QByteArray Account::GetStoreID (Account::Direction dir) const
	{
		QByteArray result = GetID ();
		if (dir == Direction::Out)
			result += "/out";
		return result;
	}

	void Account::UpdateFolderCount (const QStringList& folder)
	{
		const auto storage = Core::Instance ().GetStorage ();

		const auto totalCount = storage->GetNumMessages (this, folder);
		const auto unreadCount = storage->GetNumUnread (this, folder);
		FoldersModel_->SetFolderUnreadCount (folder, unreadCount);
		FoldersModel_->SetFolderMessageCount (folder, totalCount);
	}

	void Account::buildInURL (QString *res)
	{
		*res = BuildInURL ();
	}

	void Account::buildOutURL (QString *res)
	{
		*res = BuildOutURL ();
	}

	void Account::getPassword (QString *outPass, Direction dir)
	{
		*outPass = Util::GetPassword (GetStoreID (dir),
				tr ("Enter password for account %1:")
					.arg (GetName ()),
				Core::Instance ().GetProxy ());
	}

	void Account::handleMsgHeaders (const QList<Message_ptr>& messages, const QStringList& folder)
	{
		qDebug () << Q_FUNC_INFO << messages.size ();
		Core::Instance ().GetStorage ()->SaveMessages (this, folder, messages);
		emit mailChanged ();

		MailModelsManager_->Append (messages);

		UpdateFolderCount (folder);
	}

	void Account::handleGotUpdatedMessages (const QList<Message_ptr>& messages, const QStringList& folder)
	{
		qDebug () << Q_FUNC_INFO << messages.size ();
		Core::Instance ().GetStorage ()->SaveMessages (this, folder, messages);
		emit mailChanged ();

		MailModelsManager_->Update (messages);

		UpdateFolderCount (folder);
	}

	void Account::handleGotOtherMessages (const QList<QByteArray>& ids, const QStringList& folder)
	{
		qDebug () << Q_FUNC_INFO << ids.size () << folder;
		const auto& msgs = Core::Instance ().GetStorage ()->LoadMessages (this, folder, ids);

		MailModelsManager_->Append (msgs);

		UpdateFolderCount (folder);
	}

	void Account::handleMessagesRemoved (const QList<QByteArray>& ids, const QStringList& folder)
	{
		qDebug () << Q_FUNC_INFO << ids.size () << folder;
		for (const auto& id : ids)
			Core::Instance ().GetStorage ()->RemoveMessage (this, folder, id);

		MailModelsManager_->Remove (ids);

		UpdateFolderCount (folder);
	}

	void Account::handleFolderSyncFinished (const QStringList& folder, const QByteArray& lastRequestedId)
	{
		if (lastRequestedId.isEmpty ())
			return;

		Thread_->AddTask ({
				"getMessageCount",
				{
					folder,
					static_cast<QObject*> (this),
					QByteArray { "handleMessageCountFetched" }
				}
			});
	}

	void Account::handleMessageCountFetched (int count, int unread, const QStringList& folder)
	{
		const auto storedCount = Core::Instance ().GetStorage ()->GetNumMessages (this, folder);
		if (storedCount && count != storedCount)
			Synchronize (folder, {});

		FoldersModel_->SetFolderMessageCount (folder, count);
		FoldersModel_->SetFolderUnreadCount (folder, unread);
	}

	void Account::handleGotFolders (const QList<Folder>& folders)
	{
		FolderManager_->SetFolders (folders);
	}

	void Account::handleFoldersUpdated ()
	{
		const auto& folders = FolderManager_->GetFolders ();
		FoldersModel_->SetFolders (folders);

		for (const auto& folder : folders)
		{
			int count = -1, unread = -1;
			const auto storage = Core::Instance ().GetStorage ();
			try
			{
				count = storage->GetNumMessages (this, folder.Path_);
				unread = storage->GetNumUnread (this, folder.Path_);
			}
			catch (const std::exception&)
			{
			}

			FoldersModel_->SetFolderMessageCount (folder.Path_, count);
			FoldersModel_->SetFolderUnreadCount (folder.Path_, unread);

			Thread_->AddTask ({
					TaskQueueItem::Priority::Lowest,
					"getMessageCount",
					{
						folder.Path_,
						static_cast<QObject*> (this),
						QByteArray { "handleMessageCountFetched" }
					}
				});
		}
	}

	void Account::handleMessageBodyFetched (Message_ptr msg)
	{
		for (const auto& folder : msg->GetFolders ())
			Core::Instance ().GetStorage ()->SaveMessages (this, folder, { msg });
		emit messageBodyFetched (msg);
	}
}
}
