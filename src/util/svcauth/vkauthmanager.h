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

#include <functional>
#include <QObject>
#include <QDateTime>
#include <QUrl>
#include <interfaces/core/icoreproxy.h>
#include <util/sll/util.h>
#include "svcauthconfig.h"

class QTimer;

template<typename>
class QFuture;

namespace LeechCraft
{
namespace Util
{
class QueueManager;
enum class QueuePriority;

class CustomCookieJar;

namespace SvcAuth
{
	class UTIL_SVCAUTH_API VkAuthManager : public QObject
	{
		Q_OBJECT

		ICoreProxy_ptr Proxy_;

		const QString AccountHR_;

		QNetworkAccessManager * const AuthNAM_;
		Util::CustomCookieJar * const Cookies_;

		QueueManager * const Queue_;

		QString Token_;
		QDateTime ReceivedAt_;
		qint32 ValidFor_;

		bool IsRequesting_;

		const QString ID_;
		QUrl URL_;

		bool IsRequestScheduled_;
		QTimer * const ScheduleTimer_;

		bool SilentMode_ = false;

		bool HasTracked_ = false;
	public:
		using RequestQueue_t = QList<std::function<void (QString)>> ;
		using RequestQueue_ptr = RequestQueue_t*;

		using PrioRequestQueue_t = QList<QPair<std::function<void (QString)>, QueuePriority>>;
		using PrioRequestQueue_ptr = PrioRequestQueue_t*;

		using ScheduleGuard_t = Util::DefaultScopeGuard;
	private:
		QList<RequestQueue_ptr> ManagedQueues_;
		QList<PrioRequestQueue_ptr> PrioManagedQueues_;
	public:
		VkAuthManager (const QString& accountName, const QString& clientId,
				const QStringList& scope, const QByteArray& cookies,
				ICoreProxy_ptr, QueueManager* = nullptr, QObject* = nullptr);

		bool IsAuthenticated () const;
		bool HadAuthentication () const;

		void UpdateScope (const QStringList&);

		void GetAuthKey ();

		[[nodiscard]] QFuture<QString> GetAuthKeyFuture ();

		[[nodiscard]] ScheduleGuard_t ManageQueue (RequestQueue_ptr);
		[[nodiscard]] ScheduleGuard_t ManageQueue (PrioRequestQueue_ptr);

		void SetSilentMode (bool);
	private:
		void InvokeQueues (const QString&);

		void RequestURL (const QUrl&);
		void RequestAuthKey ();
		bool CheckReply (QUrl);
		bool CheckError (const QUrl&);

		void ScheduleTrack (const QString&);
	public slots:
		void clearAuthData ();
		void reauth ();
	private slots:
		void execScheduledRequest ();
		void handleGotForm ();
		void handleViewUrlChanged (const QUrl&);
	signals:
		void gotAuthKey (const QString&);
		void cookiesChanged (const QByteArray&);
		void authCanceled ();
		void justAuthenticated ();
	};
}
}
}
