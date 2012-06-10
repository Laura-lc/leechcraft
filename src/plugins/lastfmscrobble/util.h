/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2012  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#pragma once

#include <QList>
#include <QPair>
#include <QString>
#include <QMap>

class QNetworkAccessManager;
class QNetworkReply;
class QDomElement;
class QUrl;

namespace LeechCraft
{
namespace Lastfmscrobble
{
	QByteArray MakeCall (QList<QPair<QString, QString>>);

	QNetworkReply* Request (const QString&, QNetworkAccessManager*, const QMap<QString, QString>&);
	QNetworkReply* Request (const QString&, QNetworkAccessManager*, QList<QPair<QString, QString>> = QList<QPair<QString, QString>> ());

	QUrl GetImage (const QDomElement&, const QString&);
}
}
