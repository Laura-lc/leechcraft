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

#ifndef PLUGINS_AGGREGATOR_INTERFACES_AGGREGATOR_ITEM_H
#define PLUGINS_AGGREGATOR_INTERFACES_AGGREGATOR_ITEM_H
#include <vector>
#include <memory>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMetaType>
#include "common.h"

namespace LeechCraft
{
namespace Aggregator
{
	struct ItemShort
	{
		IDType_t ItemID_;
		IDType_t ChannelID_;
		QString Title_;
		QString URL_;
		QStringList Categories_;
		QDateTime PubDate_;
		bool Unread_;
	};

	/** Describes an enclosure associated with an item.
		*/
	struct Enclosure
	{
		/** @brief Enclosure ID.
			*/
		IDType_t EnclosureID_ = IDNotFound;

		/** @brief Parent item's ID.
			*/
		IDType_t ItemID_ = IDNotFound;

		/** @brief The URL this enclosure refers to.
			*/
		QString URL_;

		/** @brief MIME type of the enclosure.
			*/
		QString Type_;

		/** @brief Length of the attached enclosure or -1 if unknown.
			*/
		qint64 Length_ = 0;

		/** @brief  For the Atom's hreflang attribute.
			*/
		QString Lang_;

		static Enclosure CreateForItem (IDType_t itemId);
	private:
		friend QDataStream& operator>> (QDataStream&, QList<Enclosure>&);
	};

	bool operator== (const Enclosure&, const Enclosure&);

	struct MRSSThumbnail
	{
		IDType_t MRSSThumbnailID_;
		IDType_t MRSSEntryID_;
		QString URL_;
		int Width_ = 0;
		int Height_ = 0;
		QString Time_;

		static MRSSThumbnail CreateForEntry (IDType_t entryId);
		MRSSThumbnail (const IDType_t& entryId);
		MRSSThumbnail (const IDType_t& entryId, const IDType_t& thisId);
	private:
		MRSSThumbnail ();
		friend QDataStream& operator>> (QDataStream&, QList<MRSSThumbnail>&);
	};

	bool operator== (const MRSSThumbnail&, const MRSSThumbnail&);

	struct MRSSCredit
	{
		IDType_t MRSSCreditID_;
		IDType_t MRSSEntryID_;
		QString Role_;
		QString Who_;

		MRSSCredit (const IDType_t& entryId);
		MRSSCredit (const IDType_t& entryId, const IDType_t& thisId);
	private:
		MRSSCredit ();
		friend QDataStream& operator>> (QDataStream&, QList<MRSSCredit>&);
	};

	bool operator== (const MRSSCredit&, const MRSSCredit&);

	struct MRSSComment
	{
		IDType_t MRSSCommentID_;
		IDType_t MRSSEntryID_;
		QString Type_;
		QString Comment_;

		MRSSComment (const IDType_t& entryId);
		MRSSComment (const IDType_t& entryId, const IDType_t& thisId);
	private:
		MRSSComment ();
		friend QDataStream& operator>> (QDataStream&, QList<MRSSComment>&);
	};

	bool operator== (const MRSSComment&, const MRSSComment&);

	struct MRSSPeerLink
	{
		IDType_t MRSSPeerLinkID_;
		IDType_t MRSSEntryID_;
		QString Type_;
		QString Link_;

		MRSSPeerLink (const IDType_t& entryId);
		MRSSPeerLink (const IDType_t& entryId, const IDType_t& thisId);
	private:
		MRSSPeerLink ();
		friend QDataStream& operator>> (QDataStream&, QList<MRSSPeerLink>&);
	};

	bool operator== (const MRSSPeerLink&, const MRSSPeerLink&);

	struct MRSSScene
	{
		IDType_t MRSSSceneID_;
		IDType_t MRSSEntryID_;
		QString Title_;
		QString Description_;
		QString StartTime_;
		QString EndTime_;

		MRSSScene (const IDType_t& entryId);
		MRSSScene (const IDType_t& entryId, const IDType_t& thisId);
	private:
		MRSSScene ();
		friend QDataStream& operator>> (QDataStream&, QList<MRSSScene>&);
	};

	bool operator== (const MRSSScene&, const MRSSScene&);

	struct MRSSEntry
	{
		IDType_t MRSSEntryID_ = IDNotFound;
		IDType_t ItemID_ = IDNotFound;
		QString URL_;
		qint64 Size_ = 0;
		QString Type_;
		QString Medium_;
		bool IsDefault_ = false;
		QString Expression_;
		int Bitrate_ = 0;
		double Framerate_ = 0;
		double SamplingRate_ = 0;
		int Channels_ = 0;
		int Duration_ = 0;
		int Width_ = 0;
		int Height_ = 0;
		QString Lang_;
		int Group_ = 0;
		QString Rating_;
		QString RatingScheme_;
		QString Title_;
		QString Description_;
		QString Keywords_;
		QString CopyrightURL_;
		QString CopyrightText_;
		int RatingAverage_ = 0;
		int RatingCount_ = 0;
		int RatingMin_ = 0;
		int RatingMax_ = 0;
		int Views_ = 0;
		int Favs_ = 0;
		QString Tags_;
		QList<MRSSThumbnail> Thumbnails_;
		QList<MRSSCredit> Credits_;
		QList<MRSSComment> Comments_;
		QList<MRSSPeerLink> PeerLinks_;
		QList<MRSSScene> Scenes_;

		static MRSSEntry CreateForItem (IDType_t itemId);
	private:
		friend QDataStream& operator>> (QDataStream&, QList<MRSSEntry>&);
	};

	bool operator== (const MRSSEntry&, const MRSSEntry&);

	struct Item
	{
		/** The unique ID of the channel this item belongs to.
			*/
		IDType_t ChannelID_ = IDNotFound;

		/** The unique ID of the item.
			*/
		IDType_t ItemID_ = IDNotFound;

		/** The title of the item as showed in the item list.
			*/
		QString Title_;

		/** Link which should be opened when user activates the item, for
			* example, by double-clicking on the header or by clicking the
			* appropriate button.
			*/
		QString Link_;

		/** Main body of the item, showed in the main Aggregator area. Item
			* contents go here.
			*/
		QString Description_;

		/** Author of the item.
			*/
		QString Author_;

		/** Categories of this item.
			*/
		QStringList Categories_;

		/** Unique ID of the item, but it may be empty because at least
			* RSS 2.0 standard makes this field optional.
			*/
		QString Guid_;

		/** Publication datetime of the item. Should be set to invalid
			* datetime if it could not be determined from the item
			* representation in the feed.
			*/
		QDateTime PubDate_;

		/** Indicates whether this item is unread or not.
			*/
		bool Unread_ = false;

		/** Number of comments for this item. Should be set to -1 if it could
			* not be determined from the item representation in the feed.
			*/
		int NumComments_ = 0;

		/** Link to the comments RSS. Should be left blank if it could not
			* be determined from the item representation in the feed.
			*/
		QString CommentsLink_;

		/** Link to the page with comments. Should be left blank if it could
			* not be determined from the item representation in the feed.
			*/
		QString CommentsPageLink_;

		/** List of enclosures of the item.
			*/
		QList<Enclosure> Enclosures_;

		/** Latitude in the GeoRSS context.
			*/
		double Latitude_ = -1;

		/** Longitude in the GeoRSS context.
			*/
		double Longitude_ = -1;

		/* List of MediaRSS entries.
			*/
		QList<MRSSEntry> MRSSEntries_;

		static Item CreateForChannel (IDType_t channelId);

		/** Returns the simplified (short) representation of this item.
			*
			* @return The simplified (short) representation.
			*/
		ItemShort ToShort () const;

		/** @brief Fixes the date of the item.
		 *
		 * Sets the pubdate to current date if the pubdate is invalid.
		 */
		void FixDate ();
	};

	using Item_ptr = std::shared_ptr<Item>;
	using Item_cptr = std::shared_ptr<const Item>;

	typedef std::vector<Item_ptr> items_container_t;
	typedef std::vector<ItemShort> items_shorts_t;

	bool operator== (const Item&, const Item&);
	QDataStream& operator<< (QDataStream&, const Enclosure&);
	QDataStream& operator>> (QDataStream&, Enclosure&);
	QDataStream& operator<< (QDataStream&, const MRSSEntry&);
	QDataStream& operator>> (QDataStream&, MRSSEntry&);
	QDataStream& operator<< (QDataStream&, const MRSSThumbnail&);
	QDataStream& operator>> (QDataStream&, MRSSThumbnail&);
	QDataStream& operator<< (QDataStream&, const MRSSCredit&);
	QDataStream& operator>> (QDataStream&, MRSSCredit&);
	QDataStream& operator<< (QDataStream&, const MRSSComment&);
	QDataStream& operator>> (QDataStream&, MRSSComment&);
	QDataStream& operator<< (QDataStream&, const MRSSPeerLink&);
	QDataStream& operator>> (QDataStream&, MRSSPeerLink&);
	QDataStream& operator<< (QDataStream&, const MRSSScene&);
	QDataStream& operator>> (QDataStream&, MRSSScene&);
	QDataStream& operator<< (QDataStream&, const MRSSEntry&);
	QDataStream& operator>> (QDataStream&, MRSSEntry&);
	QDataStream& operator<< (QDataStream&, const Item&);
	QDataStream& operator>> (QDataStream&, Item&);
	void Print (const Item&);
	void Diff (const Item&, const Item&);

	bool IsModified (const Item&, const Item&);
}
}

Q_DECLARE_METATYPE (LeechCraft::Aggregator::Item_ptr)
Q_DECLARE_METATYPE (LeechCraft::Aggregator::Item)
Q_DECLARE_METATYPE (LeechCraft::Aggregator::ItemShort)

#endif
