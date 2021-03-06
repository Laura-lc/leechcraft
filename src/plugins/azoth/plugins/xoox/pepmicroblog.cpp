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

#include "pepmicroblog.h"
#include <QUuid>
#include <QDomElement>
#include <QXmppElement.h>
#include <util/sll/qtutil.h>

namespace LC
{
namespace Azoth
{
namespace Xoox
{
	const QString NsMicroblog = "urn:xmpp:microblog:0";
	const QString NsAtom = "http://www.w3.org/2005/Atom";
	const QString NsThread = "http://purl.org/syndication/thread/1.0";

	QString PEPMicroblog::GetNodeString ()
	{
		return NsMicroblog;
	}

	PEPMicroblog::PEPMicroblog ()
	: EventID_ (QUuid::createUuid ().toString ().remove ('{').remove ('}'))
	{
	}

	PEPMicroblog::PEPMicroblog (const Post& post)
	: EventID_ (QUuid::createUuid ().toString ().remove ('{').remove ('}'))
	, AuthorName_ (post.Author_.Name_)
	, AuthorURI_ (post.Author_.URI_)
	, Contents_ (post.Contents_)
	, Published_ (post.Published_)
	, Updated_ (post.Updated_)
	{
	}

	namespace
	{
		QXmppElement GetAuthorElem (const QString& name, const QString& uri)
		{
			QXmppElement nameElem;
			nameElem.setTagName ("name");
			nameElem.setValue (name);

			QXmppElement uriElem;
			uriElem.setTagName ("uri");
			uriElem.setValue (uri);

			QXmppElement author;
			author.setTagName ("author");
			author.appendChild (nameElem);
			author.appendChild (uriElem);

			return author;
		}
	}

	QXmppElement PEPMicroblog::ToXML () const
	{
		QXmppElement entry;
		entry.setTagName ("entry");
		entry.setAttribute ("xmlns", NsAtom);

		QXmppElement id;
		id.setTagName ("id");
		id.setValue (EventID_);
		entry.appendChild (id);

		QXmppElement source;
		source.setTagName ("source");
		source.appendChild (GetAuthorElem (AuthorName_, AuthorURI_));
		entry.appendChild (source);

		for (const auto& [key, value] : Util::Stlize (Contents_))
		{
			QXmppElement cElem;
			cElem.setTagName ("content");
			cElem.setAttribute ("type", key);
			cElem.setValue (value);
			entry.appendChild (cElem);
		}

		for (const auto& linkInfo : Alternates_)
		{
			QXmppElement link;
			link.setTagName ("link");
			link.setAttribute ("rel", "alternate");
			if (!linkInfo.Type_.isEmpty ())
				link.setAttribute ("type", linkInfo.Type_);
			link.setAttribute ("href", linkInfo.Href_);
			entry.appendChild (link);
		}

		for (const auto& repInfo : InReplyTo_)
		{
			QXmppElement elem;
			elem.setTagName ("in-reply-to");
			elem.setAttribute ("xmlns", NsThread);

			if (!repInfo.Type_.isNull ())
				elem.setAttribute ("type", repInfo.Type_);
			elem.setAttribute ("ref", repInfo.Ref_);
			elem.setAttribute ("href", repInfo.Href_);

			entry.appendChild (elem);
		}

		QXmppElement published;
		published.setTagName ("published");
		published.setValue (Published_.toString (Qt::ISODate));
		entry.appendChild (published);

		if (!Updated_.isNull ())
		{
			QXmppElement updated;
			updated.setTagName ("updated");
			updated.setValue (Updated_.toString (Qt::ISODate));
			entry.appendChild (updated);
		}

		QXmppElement result;
		result.setTagName ("item");
		result.setAttribute ("id", EventID_);
		result.appendChild (entry);
		return result;
	}

	void PEPMicroblog::Parse (const QDomElement& elem)
	{
		EventID_ = elem.attribute ("id");

		const QDomElement& entry = elem.firstChildElement ("entry");
		if (entry.namespaceURI () != NsAtom)
			return;

		const QDomElement& author = entry
				.firstChildElement ("source")
				.firstChildElement ("author");
		AuthorName_ = author.firstChildElement ("author").text ();
		AuthorURI_ = author.firstChildElement ("uri").text ();

		QDomElement content = entry.firstChildElement ("content");
		while (!content.isNull ())
		{
			Contents_ [content.attribute ("type")] = content.text ();
			content = content.nextSiblingElement ("content");
		}

		QDomElement link = entry.firstChildElement ("link");
		while (!link.isNull ())
		{
			if (link.attribute ("rel") == "alternate")
			{
				AlternateLink l =
				{
					link.attribute ("type"),
					link.attribute ("href")
				};

				Alternates_ << l;
			}

			link = link.nextSiblingElement ("link");
		}

		QDomElement inReplyTo = entry.firstChildElement ("in-reply-to");
		while (!inReplyTo.isNull ())
		{
			ReplyInfo info =
			{
				inReplyTo.attribute ("type"),
				inReplyTo.attribute ("ref"),
				inReplyTo.attribute ("href")
			};
			InReplyTo_ << info;

			inReplyTo = inReplyTo.nextSiblingElement ("in-reply-to");
		}

		Published_ = QDateTime::fromString (entry.firstChildElement ("published").text (), Qt::ISODate);
		Updated_ = QDateTime::fromString (entry.firstChildElement ("updated").text (), Qt::ISODate);
	}

	QString PEPMicroblog::Node () const
	{
		return GetNodeString ();
	}

	PEPEventBase* PEPMicroblog::Clone () const
	{
		return new PEPMicroblog (*this);
	}

	PEPMicroblog::operator Post () const
	{
		const Post p =
		{
			GetEventID (),
			Contents_,
			Published_,
			Updated_,
			{ AuthorName_, AuthorURI_ }
		};
		return p;
	}

	QString PEPMicroblog::GetEventID () const
	{
		return EventID_;
	}

	QString PEPMicroblog::GetAuthorName () const
	{
		return AuthorName_;
	}

	void PEPMicroblog::SetAuthorName (const QString& name)
	{
		AuthorName_ = name;
	}

	QString PEPMicroblog::GetAuthorURI () const
	{
		return AuthorURI_;
	}

	void PEPMicroblog::SetAuthorURI (const QString& uri)
	{
		AuthorURI_ = uri;
	}

	QString PEPMicroblog::GetContent (const QString& type) const
	{
		return Contents_ [type];
	}

	void PEPMicroblog::SetContent (const QString& type, const QString& content)
	{
		Contents_ [type] = content;
	}

	QDateTime PEPMicroblog::GetPublishedDate () const
	{
		return Published_;
	}

	void PEPMicroblog::SetPublishedDate (const QDateTime& date)
	{
		Published_ = date;
	}

	QDateTime PEPMicroblog::GetUpdatedDate () const
	{
		return Updated_;
	}

	void PEPMicroblog::SetUpdatedDate (const QDateTime& date)
	{
		Updated_ = date;
	}

	PEPMicroblog::AlternateLinks_t PEPMicroblog::GetAlternateLinks () const
	{
		return Alternates_;
	}

	void PEPMicroblog::SetAlternateLinks (const AlternateLinks_t& links)
	{
		Alternates_ = links;
	}

	void PEPMicroblog::AddAlternateLink (const AlternateLink& link)
	{
		Alternates_ << link;
	}

	PEPMicroblog::ReplyInfos_t PEPMicroblog::GetInReplyTos () const
	{
		return InReplyTo_;
	}

	void PEPMicroblog::SetInReplyTos (const ReplyInfos_t& infos)
	{
		InReplyTo_ = infos;
	}

	void PEPMicroblog::AddInReplyTos (const ReplyInfo& info)
	{
		InReplyTo_ << info;
	}
}
}
}
