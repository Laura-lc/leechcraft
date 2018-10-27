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

#include <memory>
#include <functional>
#include <QObject>
#include <QHash>
#include "accountthread.h"
#include "accountthreadworkerfwd.h"
#include "progressfwd.h"

class QMutex;
class QAbstractItemModel;
class QStandardItemModel;
class QStandardItem;
class QModelIndex;

template<typename T>
class QFuture;

namespace LeechCraft
{
namespace Snails
{
	class AccountLogger;
	class AccountThread;
	class AccountThreadWorker;
	class AccountFolderManager;
	class AccountDatabase;
	class MailModel;
	class FoldersModel;
	class MailModelsManager;
	class ThreadPool;
	class Storage;
	struct Folder;
	struct OutgoingMessage;

	enum class TaskPriority;

	template<typename T>
	class AccountThreadNotifier;

	class Account : public QObject
	{
		Q_OBJECT

		friend class AccountThreadWorker;
		AccountLogger * const Logger_;

		ThreadPool * const WorkerPool_;

		QMutex * const AccMutex_;

		QByteArray ID_;

		QString AccName_;
		QString UserName_;
		QString UserEmail_;

		QString Login_;
		bool UseSASL_ = false;
		bool SASLRequired_ = false;
	public:
		enum class SecurityType
		{
			TLS,
			SSL,
			No
		};
	private:
		bool UseTLS_ = true;
		bool UseSSL_ = false;
		bool InSecurityRequired_ = false;

		SecurityType OutSecurity_ = SecurityType::SSL;
		bool OutSecurityRequired_ = false;

		bool SMTPNeedsAuth_ = true;

		QString InHost_;
		int InPort_;

		QString OutHost_;
		int OutPort_;

		QString OutLogin_;

		int KeepAliveInterval_ = 90 * 1000;

		bool LogToFile_ = true;
	public:
		enum class Direction
		{
			In,
			Out
		};

		enum class OutType
		{
			SMTP,
			Sendmail
		};

		enum class DeleteBehaviour
		{
			Default,
			Expunge,
			MoveToTrash
		};
	private:
		OutType OutType_;
		DeleteBehaviour DeleteBehaviour_ = DeleteBehaviour::Default;

		AccountFolderManager *FolderManager_;
		FoldersModel *FoldersModel_;

		MailModelsManager * const MailModelsManager_;

		std::shared_ptr<AccountThreadNotifier<int>> NoopNotifier_;

		ProgressManager * const ProgressMgr_;
		Storage * const Storage_;
	public:
		Account (Storage *st, ProgressManager*, QObject* = nullptr);

		QByteArray GetID () const;
		QString GetName () const;
		QString GetServer () const;

		QString GetUserName () const;
		QString GetUserEmail () const;

		bool ShouldLogToFile () const;
		AccountLogger* GetLogger () const;

		std::shared_ptr<AccountDatabase> GetDatabase () const;

		AccountFolderManager* GetFolderManager () const;
		MailModelsManager* GetMailModelsManager () const;
		FoldersModel* GetFoldersModel () const;

		struct SyncStats
		{
			size_t NewMsgsCount_ = 0;
		};
		using SynchronizeResult_t = Util::Either<InvokeError_t<>, SyncStats>;

		QFuture<SynchronizeResult_t> Synchronize ();
		QFuture<SynchronizeResult_t> Synchronize (const QStringList&);

		using FetchWholeMessageResult_t = QFuture<WrapReturnType_t<Snails::FetchWholeMessageResult_t>>;
		FetchWholeMessageResult_t FetchWholeMessage (const QStringList&, const QByteArray&);

		using SendMessageResult_t = Util::Either<InvokeError_t<>, Util::Void>;
		QFuture<SendMessageResult_t> SendMessage (const OutgoingMessage&);

		using FetchAttachmentResult_t = WrapReturnType_t<Snails::FetchAttachmentResult_t>;
		QFuture<FetchAttachmentResult_t> FetchAttachment (const QStringList& folder,
				const QByteArray& msgId, const QString& attName, const QString& path);

		void SetReadStatus (bool, const QList<QByteArray>&, const QStringList&);

		using CopyMessagesResult_t = Util::Either<InvokeError_t<>, Util::Void>;
		QFuture<CopyMessagesResult_t> CopyMessages (const QList<QByteArray>& ids, const QStringList& from, const QList<QStringList>& to);
		void MoveMessages (const QList<QByteArray>& ids, const QStringList& from, const QList<QStringList>& to);
		void DeleteMessages (const QList<QByteArray>& ids, const QStringList& folder);

		QByteArray Serialize () const;
		void Deserialize (const QByteArray&);

		void OpenConfigDialog (const std::function<void ()>& onAccepted = {});

		bool IsNull () const;

		QString GetInUsername () const;
		QString GetOutUsername () const;

		ProgressListener_ptr MakeProgressListener (const QString&) const;

		QFuture<QString> BuildInURL ();
		QFuture<QString> BuildOutURL ();
		QFuture<QString> GetPassword (Direction);
	private:
		QFuture<SynchronizeResult_t> SynchronizeImpl (const QList<QStringList>&, const QByteArray&, TaskPriority);
		void SyncStatuses (const QStringList&, TaskPriority);
		QMutex* GetMutex () const;

		void UpdateNoopInterval ();

		QByteArray GetStoreID (Direction) const;

		void DeleteFromFolder (const QList<QByteArray>& ids, const QStringList& folder);

		void UpdateFolderCount (const QStringList&);

		void RequestMessageCount (const QStringList&);
		void HandleMessageCountFetched (int, int, const QStringList&);

		void HandleReadStatusChanged (const QList<QByteArray>&, const QList<QByteArray>&, const QStringList&);
		void HandleMessagesRemoved (const QList<QByteArray>&, const QStringList&);
		void HandleMsgHeaders (const QList<FetchedMessageInfo>&);

		void HandleGotFolders (const QList<Folder>&);
	private slots:
		void handleFoldersUpdated ();
	signals:
		void accountChanged ();
		void willMoveMessages (const QList<QByteArray>& ids, const QStringList& folder);
	};

	typedef std::shared_ptr<Account> Account_ptr;
}
}

Q_DECLARE_METATYPE (LeechCraft::Snails::Account_ptr)
