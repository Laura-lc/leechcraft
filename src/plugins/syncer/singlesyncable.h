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
#include <unordered_set>
#include <QObject>

class QTcpSocket;
class QSettings;

class ISyncProxy;

namespace Laretz
{
	struct ParseResult;

	class Item;
	class Operation;
}

namespace LeechCraft
{
namespace Syncer
{
	class SingleSyncable : public QObject
	{
		Q_OBJECT

		const QByteArray ID_;
		ISyncProxy * const Proxy_;

		QTcpSocket * const Socket_;

		enum class State
		{
			Idle,
			ListRequested,
			FetchRequested,
			Sent,
			RootCreateRequested
		} State_ = State::Idle;

		bool IsFirstSync_ = true;

		std::unordered_set<std::string> RemoteIDs_;
	public:
		SingleSyncable (const QByteArray& id, ISyncProxy *proxy, QObject* = 0);
	private:
		std::shared_ptr<QSettings> GetSettings ();

		void HandleList (const Laretz::ParseResult&);
		void HandleFetch (const Laretz::ParseResult&);
		void HandleSendResult (const Laretz::ParseResult&);
		void HandleRootCreated (const Laretz::ParseResult&);
		void CreateRoot ();
	private slots:
		void handleSocketRead ();

		void startSync ();
		void handleSocketConnected ();
	};
}
}
