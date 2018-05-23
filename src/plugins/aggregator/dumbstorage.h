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

#include "storagebackend.h"

namespace LeechCraft
{
namespace Aggregator
{
	class DumbStorage : public StorageBackend
	{
	public:
		void Prepare () override;
		ids_t GetFeedsIDs () const override;
		boost::optional<Feed> GetFeed (const IDType_t&) const override;
		IDType_t FindFeed (const QString&) const override;
		Feed::FeedSettings GetFeedSettings (const IDType_t&) const override;
		void SetFeedSettings (const Feed::FeedSettings&) override;
		channels_shorts_t GetChannels (const IDType_t&) const override;
		Channel GetChannel (const IDType_t&, const IDType_t&) const override;
		IDType_t FindChannel (const QString&, const QString&, const IDType_t&) const override;
		void TrimChannel (const IDType_t&, int, int) override;
		items_shorts_t GetItems (const IDType_t&) const override;
		int GetUnreadItems (const IDType_t&) const override;
		Item GetItem (const IDType_t&) const override;
		boost::optional<IDType_t> FindItem (const QString&, const QString&, const IDType_t&) const override;
		boost::optional<IDType_t> FindItemByTitle (const QString&, const IDType_t&) const override;
		boost::optional<IDType_t> FindItemByLink (const QString&, const IDType_t&) const override;
		items_container_t GetFullItems (const IDType_t&) const override;
		void AddFeed (const Feed&) override;
		void AddChannel (const Channel&) override;
		void AddItem (const Item&) override;
		void UpdateChannel (const Channel&) override;
		void UpdateChannel (const ChannelShort&) override;
		void UpdateItem (const Item&) override;
		void UpdateItem (const ItemShort&) override;
		void RemoveItems (const QSet<IDType_t>&) override;
		void RemoveChannel (const IDType_t&) override;
		void RemoveFeed (const IDType_t&) override;
		bool UpdateFeedsStorage (int, int) override;
		bool UpdateChannelsStorage (int, int) override;
		bool UpdateItemsStorage (int, int) override;
		void ToggleChannelUnread (const IDType_t&, bool) override;
		QList<ITagsManager::tag_id> GetItemTags (const IDType_t&) override;
		void SetItemTags (const IDType_t&, const QList<ITagsManager::tag_id>&) override;
		QList<IDType_t> GetItemsForTag (const ITagsManager::tag_id&) override;
		IDType_t GetHighestID (const PoolType&) const override;
	};
}
}
