/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "singleaccauth.h"
#include <QDateTime>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtDebug>
#include <QTimer>
#include <QSettings>
#include <QCoreApplication>
#include <util/passutils.h>
#include <util/util.h>
#include <interfaces/core/ientitymanager.h>
#include "util.h"

namespace LeechCraft
{
namespace Scroblibre
{
	SingleAccAuth::SingleAccAuth (const QUrl& url,
			const QString& login, ICoreProxy_ptr proxy, QObject *parent)
	: QObject { parent }
	, Proxy_ { proxy }
	, BaseURL_ { url }
	, Login_ { login }
	{
		reauth ();
	}

	void SingleAccAuth::Submit (const SubmitInfo& info)
	{
		if (SID_.isEmpty ())
		{
			Queue_ << info;
			return;
		}

		QByteArray data;
		auto append = [&data] (const QByteArray& param, const QString& value) -> void
		{
			if (!data.isEmpty ())
				data += '&';
			data += param + '=' + QUrl::toPercentEncoding (value);
		};

		append ("s", SID_);
		append ("a[0]", info.Info_.Artist_);
		append ("b[0]", info.Info_.Album_);
		append ("t[0]", info.Info_.Title_);
		append ("i[0]", QString::number (info.TS_.toTime_t ()));
		append ("o[0]", "P");
		append ("l[0]", QString::number (info.Info_.Length_));
		if (info.Info_.TrackNumber_)
			append ("n[0]", QString::number (info.Info_.TrackNumber_));

		const auto reply = Proxy_->GetNetworkAccessManager ()->
				post (QNetworkRequest (SubmissionsUrl_), data);
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleSubmission ()));
	}

	void SingleAccAuth::LoadQueue ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Scroblibre");
		settings.beginGroup ("Queues");
		settings.beginGroup (BaseURL_.toString ());
		settings.beginGroup (Login_);

		const auto size = settings.beginReadArray ("Items");
		for (auto i = 0; i < size; ++i)
		{
			settings.setArrayIndex (i);

			const auto& artist = settings.value ("Artist").toString ();
			const auto& album = settings.value ("Album").toString ();
			const auto& title = settings.value ("Title").toString ();
			const auto& ts = settings.value ("TS").toDateTime ();
			const auto& length = settings.value ("Length").toInt ();
			const auto& track = settings.value ("Track").toInt ();

			Queue_.append ({
				{
					artist,
					album,
					title,
					{},
					length,
					0,
					track,
					{}
				},
				ts
				});
		}
		settings.endArray ();

		settings.endGroup ();
		settings.endGroup ();
		settings.endGroup ();
	}

	void SingleAccAuth::SaveQueue () const
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Scroblibre");
		settings.beginGroup ("Queues");
		settings.beginGroup (BaseURL_.toString ());
		settings.beginGroup (Login_);

		settings.beginWriteArray ("Items");
		for (auto i = 0; i < Queue_.size (); ++i)
		{
			const auto& info = Queue_.at (i);

			settings.setArrayIndex (i);
			settings.setValue ("Artist", info.Info_.Artist_);
			settings.setValue ("Album", info.Info_.Album_);
			settings.setValue ("Title", info.Info_.Title_);
			settings.setValue ("TS", info.TS_);
			settings.setValue ("Length", info.Info_.Length_);
			settings.setValue ("Track", info.Info_.TrackNumber_);
		}
		settings.endArray ();

		settings.endGroup ();
		settings.endGroup ();
		settings.endGroup ();
	}

	void SingleAccAuth::reauth (bool failed)
	{
		ReauthScheduled_ = false;

		SID_.clear ();

		const auto nowTime_t = QDateTime::currentDateTime ().toTime_t ();
		const auto& nowStr = QString::number (nowTime_t);

		QUrl reqUrl { BaseURL_ };
		reqUrl.addQueryItem ("hs", "true");
		reqUrl.addQueryItem ("p", "1.2");
		reqUrl.addQueryItem ("c", "tst");
		reqUrl.addQueryItem ("v", "1.0");
		reqUrl.addQueryItem ("u", Login_);
		reqUrl.addQueryItem ("t", nowStr);

		const auto& service = UrlToService (BaseURL_);
		const auto& pass = Util::GetPassword ("org.LeechCraft.Scroblibre/" + service + '/' + Login_,
				tr ("Please enter password for account %1 on %2:")
					.arg ("<em>" + Login_ + "</em>")
					.arg (service),
				parent (),
				!failed);

		const auto& md5pass = QCryptographicHash::hash (pass.toUtf8 (),
				QCryptographicHash::Md5).toHex ();
		const auto& token = QCryptographicHash::hash (md5pass + nowStr.toUtf8 (),
				QCryptographicHash::Md5).toHex ();
		reqUrl.addQueryItem ("a", token);

		const auto reply = Proxy_->GetNetworkAccessManager ()->get (QNetworkRequest (reqUrl));
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleHSFinished ()));
	}

	void SingleAccAuth::handleHSFinished ()
	{
		const auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = QString::fromLatin1 (reply->readAll ());
		auto split = data.split ('\n', QString::SkipEmptyParts);
		for (auto& part : split)
			part = part.trimmed ();

		const auto& status = split.value (0);
		if (status == "OK")
		{
			SID_ = split.value (1);
			NowPlayingUrl_ = QUrl::fromEncoded (split.value (2).toLatin1 ());
			SubmissionsUrl_ = QUrl::fromEncoded (split.value (3).toLatin1 ());

			decltype (Queue_) queue;
			std::swap (queue, Queue_);
			for (const auto& item : queue)
				Submit (item);

			return;
		}

		if (status == "BADTIME")
		{
			const auto& e = Util::MakeNotification ("Scroblibre",
					tr ("Your system clock is too incorrect."),
					PCritical_);
			Proxy_->GetEntityManager ()->HandleEntity (e);
		}
		else if (status.startsWith ("FAILED"))
		{
			const auto& e = Util::MakeNotification ("Scroblibre",
					tr ("Temporary server failure for %1, please try again later.")
						.arg (UrlToService (BaseURL_)),
					PCritical_);
			Proxy_->GetEntityManager ()->HandleEntity (e);
		}
		else if (status == "BANNED")
		{
			const auto& e = Util::MakeNotification ("Scroblibre",
					tr ("Sorry, the client is banned on %1.")
						.arg (UrlToService (BaseURL_)),
					PCritical_);
			Proxy_->GetEntityManager ()->HandleEntity (e);
		}
		else if (status == "BADAUTH")
		{
			const auto& e = Util::MakeNotification ("Scroblibre",
					tr ("Invalid authentication on %1.")
						.arg (UrlToService (BaseURL_)),
					PCritical_);
			Proxy_->GetEntityManager ()->HandleEntity (e);
			reauth (true);
			return;
		}
		else
		{
			const auto& e = Util::MakeNotification ("Scroblibre",
					tr ("General server error for %1.")
						.arg (UrlToService (BaseURL_)),
					PCritical_);
			Proxy_->GetEntityManager ()->HandleEntity (e);
		}

		if (ReauthScheduled_)
			return;

		QTimer::singleShot (120 * 1000,
				this,
				SLOT (reauth ()));
		ReauthScheduled_ = true;
	}

	void SingleAccAuth::handleSubmission ()
	{
		const auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = reply->readAll ();
	}
}
}
