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

#include "friendsmanager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QStandardItem>
#include <QIcon>
#include <QTimer>
#include <QtDebug>
#include <qjson/parser.h>
#include <interfaces/media/iradiostationprovider.h>
#include <interfaces/core/iiconthememanager.h>
#include <util/svcauth/vkauthmanager.h>
#include <util/svcauth/vkcaptchadialog.h>
#include <util/sll/queuemanager.h>
#include <util/util.h>
#include "albumsmanager.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace TouchStreams
{
	namespace
	{
		enum FriendRole
		{
			PhotoUrlRole = Media::RadioItemRole::MaxRadioRole + 1
		};
	}

	FriendsManager::FriendsManager (Util::SvcAuth::VkAuthManager *authMgr,
			Util::QueueManager *queueMgr, ICoreProxy_ptr proxy, QObject *parent)
	: QObject (parent)
	, Proxy_ (proxy)
	, AuthMgr_ (authMgr)
	, Queue_ (queueMgr)
	, Root_ (new QStandardItem (tr ("VKontakte: friends")))
	{
		Root_->setIcon (QIcon (":/touchstreams/resources/images/vk.svg"));
		Root_->setEditable (false);

		AuthMgr_->ManageQueue (&RequestQueue_);

		QTimer::singleShot (1000,
				this,
				SLOT (refetchFriends ()));

		XmlSettingsManager::Instance ().RegisterObject ("RequestFriendsData",
				this, "refetchFriends");
	}

	QStandardItem* FriendsManager::GetRootItem () const
	{
		return Root_;
	}

	void FriendsManager::RefreshItems (QList<QStandardItem*> items)
	{
		if (items.contains (Root_))
		{
			if (auto rc = Root_->rowCount ())
				Root_->removeRows (0, rc);

			Friend2Item_.clear ();
			Friend2AlbumsManager_.clear ();
			Queue_->Clear ();
			RequestQueue_.clear ();

			refetchFriends ();
			return;
		}

		const auto& mgrs = Friend2AlbumsManager_.values ();
		for (auto mgr : mgrs)
		{
			items.removeOne (mgr->RefreshItems (items));
			if (items.isEmpty ())
				break;
		}
	}

	void FriendsManager::refetchFriends ()
	{
		if (!XmlSettingsManager::Instance ()
				.property ("RequestFriendsData").toBool ())
			return;

		auto nam = Proxy_->GetNetworkAccessManager ();
		RequestQueue_.push_back ([this, nam] (const QString& key) -> void
			{
				QUrl friendsUrl ("https://api.vk.com/method/friends.get");
				friendsUrl.addQueryItem ("access_token", key);
				friendsUrl.addQueryItem ("order", "name");
				friendsUrl.addQueryItem ("fields", "uid,first_name,last_name,photo");
				auto reply = nam->get (QNetworkRequest (friendsUrl));
				connect (reply,
						SIGNAL (finished ()),
						this,
						SLOT (handleGotFriends ()));
			});
		AuthMgr_->GetAuthKey ();
	}

	FriendsManager::~FriendsManager ()
	{
		AuthMgr_->UnmanageQueue (&RequestQueue_);
	}

	void FriendsManager::handleGotFriends ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& data = QJson::Parser ().parse (reply).toMap ();
		auto usersList = data ["response"].toList ();

		QList<qlonglong> ids;
		QMap<qlonglong, QVariantMap> user2info;
		for (const auto& userVar : usersList)
		{
			const auto& map = userVar.toMap ();
			const auto id = map ["user_id"].toLongLong ();
			ids << id;
			user2info [id] = map;
		}

		const auto portion = 10;
		for (int i = 0; i < ids.size (); i += portion)
		{
			QStringList sub;
			QMap<qlonglong, QVariantMap> theseUsersMap;
			for (int j = i; j < std::min (ids.size (), i + portion); ++j)
			{
				sub << QString::number (ids.at (j));
				theseUsersMap [ids.at (j)] = user2info [ids.at (j)];
			}

			const auto& code = QString (R"d(
					var ids = [%1];
					var i = 0;
					var res = [];
					while (i < %2)
					{
						var id = ids [i];
						var albs = API.audio.getAlbums ({ "uid": id, "count": 100 });
						var trs = API.audio.get ({ "uid": id, "count": 1000 });
						res = res + [{ "id": id, "albums": albs, "tracks": trs }];
						i = i + 1;
					};
					return res;
				)d")
					.arg (sub.join (","))
					.arg (sub.size ());

			auto nam = Proxy_->GetNetworkAccessManager ();
			RequestQueue_.push_back ([this, nam, code, theseUsersMap] (const QString& key) -> void
				{
					auto f = [=] (const QMap<QString, QString>& map) -> QNetworkReply*
					{
						QUrl url ("https://api.vk.com/method/execute");

						auto query = "access_token=" + QUrl::toPercentEncoding (key.toUtf8 ());
						query += '&';
						query += "code=" + QUrl::toPercentEncoding (code.toUtf8 ());

						for (auto i = map.begin (); i != map.end (); ++i)
							query += '&' + i.key () + '=' + QUrl::toPercentEncoding (i->toUtf8 ());

						QNetworkRequest req (url);
						req.setHeader (QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
						auto reply = nam->post (req, query);
						connect (reply,
								SIGNAL (finished ()),
								this,
								SLOT (handleExecuted ()));

						Reply2Users_ [reply] = theseUsersMap;

						return reply;
					};
					Reply2Func_ [f ({})] = f;
				});
		}
		AuthMgr_->GetAuthKey ();
	}

	void FriendsManager::handleCaptchaEntered (const QString& cid, const QString& value)
	{
		if (Queue_->IsPaused ())
			Queue_->Resume ();

		if (!CaptchaReplyMaker_)
			return;

		if (value.isEmpty ())
			return;

		decltype (CaptchaReplyMaker_) maker;
		std::swap (maker, CaptchaReplyMaker_);
		Queue_->Schedule ([cid, value, maker, this] () -> void
				{
					const auto& map = Util::MakeMap<QString, QString> ({
							{ "captcha_sid", cid },
							{ "captcha_key", value }
						});
					Reply2Func_ [maker (map)] = maker;
				},
				nullptr,
				Util::QueuePriority::High);
	}

	void FriendsManager::handleExecuted ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		const auto& usersMap = Reply2Users_.take (reply);
		const auto& reqFunc = Reply2Func_.take (reply);

		const auto& data = QJson::Parser ().parse (reply).toMap ();

		if (data.contains ("error"))
		{
			const auto& errMap = data ["error"].toMap ();
			if (errMap ["error_code"].toULongLong () == 14)
			{
				qDebug () << Q_FUNC_INFO
						<< "captcha requested";
				if (Queue_->IsPaused ())
					return;

				Queue_->Pause ();

				auto captchaDialog = new Util::SvcAuth::VkCaptchaDialog (errMap,
						Proxy_->GetNetworkAccessManager ());
				captchaDialog->SetContextName ("TouchStreams");
				captchaDialog->show ();
				connect (captchaDialog,
						SIGNAL (gotCaptcha (QString, QString)),
						this,
						SLOT (handleCaptchaEntered (QString, QString)));
				CaptchaReplyMaker_ = std::move (reqFunc);
			}
			else
			{
				qWarning () << Q_FUNC_INFO
						<< "error"
						<< errMap;
				return;
			}
		}

		for (const auto& userDataVar : data ["response"].toList ())
		{
			const auto& userData = userDataVar.toMap ();

			const auto id = userData ["id"].toLongLong ();

			const auto& map = usersMap [id];

			auto mgr = new AlbumsManager (id, userData ["albums"], userData ["tracks"],
					AuthMgr_, Queue_, Proxy_, this);

			Friend2AlbumsManager_ [id] = mgr;

			const auto& name = map ["first_name"].toString () + " " + map ["last_name"].toString ();

			auto userItem = mgr->GetRootItem ();
			userItem->setText (name);
			userItem->setData (QUrl::fromEncoded (map ["photo"].toByteArray ()), PhotoUrlRole);
			userItem->setIcon (Proxy_->GetIconThemeManager ()->GetIcon ("user-identity"));
			Root_->appendRow (userItem);
			Friend2Item_ [id] = userItem;

			handleAlbumsFinished (mgr);
		}
	}

	void FriendsManager::handleAlbumsFinished (AlbumsManager *mgr)
	{
		mgr->deleteLater ();

		const auto uid = mgr->GetUserID ();
		if (!Friend2Item_.contains (uid))
			return;

		if (!mgr->GetTracksCount ())
		{
			Root_->removeRow (Friend2Item_.take (uid)->row ());
			return;
		}

		const auto& url = Friend2Item_ [uid]->data (PhotoUrlRole).toUrl ();
		const auto reply = Proxy_->GetNetworkAccessManager ()->get (QNetworkRequest (url));
		reply->setProperty ("TS/UID", uid);
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handlePhotoFetched ()));
	}

	void FriendsManager::handlePhotoFetched ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();

		QPixmap px;
		if (!px.loadFromData (reply->readAll ()))
			return;

		const auto uid = reply->property ("TS/UID").toLongLong ();
		Friend2Item_ [uid]->setIcon (px);
	}
}
}
