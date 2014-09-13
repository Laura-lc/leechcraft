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

#ifndef PLUGINS_AZOTH_PLUGINS_CHATHISTORY_STORAGE_H
#define PLUGINS_AZOTH_PLUGINS_CHATHISTORY_STORAGE_H
#include <memory>
#include <QSqlQuery>
#include <QHash>
#include <QVariant>
#include <QDateTime>

class QSqlDatabase;

namespace LeechCraft
{
namespace Azoth
{
class IMessage;

namespace ChatHistory
{
	class Storage : public QObject
	{
		Q_OBJECT

		std::shared_ptr<QSqlDatabase> DB_;
		QSqlQuery UserSelector_;
		QSqlQuery AccountSelector_;
		QSqlQuery UserIDSelector_;
		QSqlQuery AccountIDSelector_;
		QSqlQuery UserInserter_;
		QSqlQuery AccountInserter_;
		QSqlQuery MessageDumper_;
		QSqlQuery UsersForAccountGetter_;
		QSqlQuery Date2Pos_;
		QSqlQuery GetMonthDates_;
		QSqlQuery LogsSearcher_;
		QSqlQuery LogsSearcherWOContact_;
		QSqlQuery LogsSearcherWOContactAccount_;
		QSqlQuery HistoryGetter_;
		QSqlQuery HistoryClearer_;
		QSqlQuery UserClearer_;
		QSqlQuery EntryCacheSetter_;
		QSqlQuery EntryCacheGetter_;
		QSqlQuery EntryCacheClearer_;

		QHash<QString, qint32> Users_;
		QHash<QString, qint32> Accounts_;

		QHash<qint32, QString> EntryCache_;

		struct RawSearchResult
		{
			qint32 EntryID_;
			qint32 AccountID_;
			QDateTime Date_;

			RawSearchResult ();
			RawSearchResult (qint32 entryId, qint32 accountId, const QDateTime& date);

			bool IsEmpty () const;
		};
	public:
		Storage (QObject* = 0);
	private:
		void InitializeTables ();
		void UpdateTables (const QList<QPair<QString, QString>>&);

		QHash<QString, qint32> GetUsers ();
		qint32 GetUserID (const QString&);
		void AddUser (const QString& id, const QString& accountId);

		void PrepareEntryCache ();

		QHash<QString, qint32> GetAccounts ();
		qint32 GetAccountID (const QString&);
		void AddAccount (const QString& id);
		RawSearchResult Search (const QString& accountId, const QString& entryId,
				const QString& text, int shift, bool cs);
		RawSearchResult Search (const QString& accountId, const QString& text, int shift, bool cs);
		RawSearchResult Search (const QString& text, int shift, bool cs);
		void SearchDate (qint32, qint32, const QDateTime&);
	public slots:
		void regenUsersCache ();

		void addMessage (const QVariantMap&);
		void getOurAccounts ();
		void getUsersForAccount (const QString&);
		void getChatLogs (const QString& accountId,
				const QString& entryId, int backpages, int amount);
		void search (const QString& accountId, const QString& entryId,
				const QString& text, int shift, bool cs);
		void searchDate (const QString& accountId, const QString& entryId, const QDateTime& dt);
		void getDaysForSheet (const QString& accountId, const QString& entryId, int year, int month);
		void clearHistory (const QString& accountId, const QString& entryId);
	signals:
		void gotOurAccounts (const QStringList&);
		void gotUsersForAccount (const QStringList&, const QString&, const QStringList&);
		void gotChatLogs (const QString&, const QString&,
				int, int, const QVariant&);
		void gotSearchPosition (const QString&, const QString&, int);
		void gotDaysForSheet (const QString& accountId, const QString& entryId,
				int year, int month, const QList<int>& days);
	};
}
}
}

#endif
