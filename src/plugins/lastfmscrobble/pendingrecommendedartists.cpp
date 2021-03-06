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

#include "pendingrecommendedartists.h"
#include <QDomDocument>
#include <QNetworkReply>
#include <QtDebug>
#include <util/sll/util.h>
#include <util/sll/domchildrenrange.h>
#include <util/sll/prelude.h>
#include <util/network/handlenetworkreply.h>
#include "authenticator.h"
#include "util.h"

namespace LC
{
namespace Lastfmscrobble
{
	PendingRecommendedArtists::PendingRecommendedArtists (Authenticator *auth,
			QNetworkAccessManager *nam, int num, QObject *obj)
	: BaseSimilarArtists (num, obj)
	, NAM_ (nam)
	{
		if (auth->IsAuthenticated ())
			request ();
		else
			connect (auth,
					SIGNAL (authenticated ()),
					this,
					SLOT (request ()));
	}

	void PendingRecommendedArtists::request ()
	{
		const ParamsList_t params { { "limit", QString::number (NumGet_) } };
		Util::HandleReplySeq (Request ("user.getRecommendedArtists", NAM_, params), this) >>
				Util::Visitor
				{
					[this] (Util::Void) { ReportError ("Unable to query last.fm."); },
					[this] (const QByteArray& data) { HandleData (data); }
				};
	}

	void PendingRecommendedArtists::HandleData (const QByteArray& data)
	{
		QDomDocument doc;
		if (!doc.setContent (data))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to parse reply";
			ReportError ("Unable to parse service reply.");
			return;
		}

		const auto& recsElem = doc.documentElement ().firstChildElement ("recommendations");
		for (const auto& artistElem : Util::DomChildren (recsElem, "artist"))
		{
			const auto& name = artistElem.firstChildElement ("name").text ();
			if (name.isEmpty ())
				continue;

			const auto& similarTo = Util::Map (Util::DomChildren (artistElem.firstChildElement ("context"), "artist"),
					[] (const auto& elem) { return elem.firstChildElement ("name").text (); });

			++InfosWaiting_;

			QMap<QString, QString> params { { "artist", name } };
			AddLanguageParam (params);
			HandleReply (Request ("artist.getInfo", NAM_, params), {}, similarTo);
		}

		if (!InfosWaiting_)
			ReportError ("No results from last.fm.");
	}
}
}
