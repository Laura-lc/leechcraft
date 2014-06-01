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

#include "bookmark.h"
#include <QDataStream>
#include <QDomElement>
#include <QtDebug>

namespace LeechCraft
{
namespace Monocle
{
	Bookmark::Bookmark ()
	: Page_ (0)
	{
	}

	Bookmark::Bookmark (const QString& name, int page, const QPoint& position)
	: Name_ (name)
	, Page_ (page)
	, Position_ (position)
	{
	}

	QString Bookmark::GetName () const
	{
		return Name_;
	}

	void Bookmark::SetName (const QString& name)
	{
		Name_ = name;
	}

	int Bookmark::GetPage () const
	{
		return Page_;
	}

	void Bookmark::SetPage (int page)
	{
		Page_ = page;
	}

	QPoint Bookmark::GetPosition () const
	{
		return Position_;
	}

	void Bookmark::SetPosition (const QPoint& p)
	{
		Position_ = p;
	}

	void Bookmark::ToXML (QDomElement& elem, QDomDocument& doc) const
	{
		auto pageElem = doc.createElement ("page");
		pageElem.setAttribute ("num", Page_);
		elem.appendChild (pageElem);

		auto posElem = doc.createElement ("pos");
		posElem.setAttribute ("x", Position_.x ());
		posElem.setAttribute ("y", Position_.y ());
		elem.appendChild (posElem);

		elem.setAttribute ("name", Name_);
	}

	Bookmark Bookmark::FromXML (const QDomElement& elem)
	{
		const auto page = elem.firstChildElement ("page").attribute ("num").toInt ();
		const auto& posElem = elem.firstChildElement ("pos");
		const auto& name = elem.attribute ("name");
		return Bookmark (name,
				page,
				{ posElem.attribute ("x").toInt (), posElem.attribute ("y").toInt () });
	}

	bool operator== (const Bookmark& b1, const Bookmark& b2)
	{
		return b1.GetPosition () == b2.GetPosition () &&
			b1.GetName () == b2.GetName ();
	}

	QDataStream& operator<< (QDataStream& out, const Bookmark& bm)
	{
		return out << static_cast<quint8> (1)
				<< bm.GetName ()
				<< bm.GetPage ()
				<< bm.GetPosition ();
	}

	QDataStream& operator>> (QDataStream& in, Bookmark& bm)
	{
		quint8 version = 0;
		in >> version;
		if (version != 1)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown version"
					<< version;
			return in;
		}

		QString name;
		int page = 0;
		QPoint p;

		in >> name >> page >> p;

		bm = Bookmark (name, page, p);

		return in;
	}
}
}
