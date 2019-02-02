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

#include "proxyobject.h"
#include <interfaces/core/icoreproxy.h>
#include <util/xpc/coreproxyholder.h>
#include "poolsmanager.h"
#include "storagebackendmanager.h"
#include "channelsmodel.h"
#include "itemslistmodel.h"
#include "channelutils.h"

namespace LeechCraft
{
namespace Aggregator
{
	namespace
	{
		void FixItemID (Item& item)
		{
			if (item.ItemID_)
				return;

			item.ItemID_ = PoolsManager::Instance ().GetPool (PTItem).GetID ();

			for (auto& enc : item.Enclosures_)
				enc.ItemID_ = item.ItemID_;
		}

		void FixChannelID (Channel& channel)
		{
			if (channel.ChannelID_)
				return;

			channel.ChannelID_ = PoolsManager::Instance ().GetPool (PTChannel).GetID ();
			for (const auto& item : channel.Items_)
			{
				item->ChannelID_ = channel.ChannelID_;
				FixItemID (*item);
			}
		}

		void FixFeedID (Feed& feed)
		{
			if (feed.FeedID_)
				return;

			feed.FeedID_ = PoolsManager::Instance ().GetPool (PTFeed).GetID ();

			for (const auto& channel : feed.Channels_)
			{
				channel->FeedID_ = feed.FeedID_;
				FixChannelID (*channel);
			}
		}
	}

	ProxyObject::ProxyObject (ChannelsModel *cm, QObject *parent)
	: QObject { parent }
	, ChannelsModel_ { cm }
	{
	}

	void ProxyObject::AddFeed (Feed feed)
	{
		FixFeedID (feed);
		StorageBackendManager::Instance ().MakeStorageBackendForThread ()->AddFeed (feed);
	}

	void ProxyObject::AddChannel (Channel channel)
	{
		FixChannelID (channel);
		StorageBackendManager::Instance ().MakeStorageBackendForThread ()->AddChannel (channel);
	}

	void ProxyObject::AddItem (Item item)
	{
		FixItemID (item);
		StorageBackendManager::Instance ().MakeStorageBackendForThread ()->AddItem (item);
	}

	QAbstractItemModel* ProxyObject::GetChannelsModel () const
	{
		return ChannelsModel_;
	}

	QList<Channel> ProxyObject::GetAllChannels () const
	{
		QList<Channel> result;
		const auto& sb = StorageBackendManager::Instance ().MakeStorageBackendForThread ();
		for (const auto& cs : ChannelUtils::GetAllChannels ())
			result << sb->GetChannel (cs.ChannelID_);
		return result;
	}

	Channel ProxyObject::GetChannel (IDType_t id) const
	{
		const auto& sb = StorageBackendManager::Instance ().MakeStorageBackendForThread ();
		return sb->GetChannel (id);
	}

	int ProxyObject::CountUnreadItems (IDType_t channel) const
	{
		return StorageBackendManager::Instance ().MakeStorageBackendForThread ()->GetUnreadItemsCount (channel);
	}

	QList<Item_ptr> ProxyObject::GetChannelItems (IDType_t channelId) const
	{
		return StorageBackendManager::Instance ().MakeStorageBackendForThread ()->GetFullItems (channelId);
	}

	std::optional<Item> ProxyObject::GetItem (IDType_t id) const
	{
		return StorageBackendManager::Instance ().MakeStorageBackendForThread ()->GetItem (id);
	}

	void ProxyObject::SetItemRead (IDType_t id, bool read) const
	{
		const auto& sb = StorageBackendManager::Instance ().MakeStorageBackendForThread ();

		auto item = sb->GetItem (id);
		if (!item)
			return;

		item->Unread_ = !read;
		sb->UpdateItem (*item);
	}

	QAbstractItemModel* ProxyObject::CreateItemsModel () const
	{
		return new ItemsListModel { Util::CoreProxyHolder::Get ()->GetIconThemeManager () };
	}
}
}
