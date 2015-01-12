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

#include "util.h"
#include <QStandardItem>
#include <util/util.h>
#include <interfaces/idatafilter.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/an/entityfields.h>
#include <interfaces/ipersistentstorageplugin.h>
#include <interfaces/ijobholder.h>

Q_DECLARE_METATYPE (QVariantList*);

namespace LeechCraft
{
namespace Util
{
	Entity MakeAN (const QString& header, const QString& text, Priority priority,
			const QString& senderID, const QString& cat, const QString& type,
			const QString& id, const QStringList& visualPath,
			int delta, int count,
			const QString& fullText, const QString& extendedText)
	{
		auto e = MakeNotification (header, text, priority);
		e.Additional_ [AN::EF::SenderID] = senderID;
		e.Additional_ [AN::EF::EventCategory] = cat;
		e.Additional_ [AN::EF::EventID] = id;
		e.Additional_ [AN::EF::VisualPath] = visualPath;
		e.Additional_ [AN::EF::EventType] = type;
		e.Additional_ [AN::EF::FullText] = fullText.isNull () ? text : fullText;
		e.Additional_ [AN::EF::ExtendedText] = extendedText.isNull () ? text : extendedText;
		if (delta)
			e.Additional_ [AN::EF::DeltaCount] = delta;
		else
			e.Additional_ [AN::EF::Count] = count;
		return e;
	}

	Entity MakeANRule (const QString& title, const QString& senderID,
			const QString& cat, const QStringList& types, AN::NotifyFlags flags,
			bool openConfiguration, const QList<QPair<QString, ANFieldValue>>& fields)
	{
		auto e = MakeNotification (title, {}, PLog_);
		e.Additional_ [AN::EF::SenderID] = senderID;
		e.Additional_ [AN::EF::EventID] = "org.LC.AdvNotifications.RuleRegister";
		e.Additional_ [AN::EF::EventCategory] = cat;
		e.Additional_ [AN::EF::EventType] = types;
		e.Additional_ [AN::EF::OpenConfiguration] = openConfiguration;
		e.Mime_ += "-rule-create";

		for (const auto& field : fields)
			e.Additional_ [field.first] = QVariant::fromValue (field.second);

		if (flags & AN::NotifySingleShot)
			e.Additional_ [AN::EF::IsSingleShot] = true;
		if (flags & AN::NotifyTransient)
			e.Additional_ [AN::EF::NotifyTransient] = true;
		if (flags & AN::NotifyPersistent)
			e.Additional_ [AN::EF::NotifyPersistent] = true;
		if (flags & AN::NotifyAudio)
			e.Additional_ [AN::EF::NotifyAudio] = true;

		return e;
	}

	QList<QObject*> GetDataFilters (const QVariant& data, IEntityManager* manager)
	{
		const auto& e = MakeEntity (data, QString (), {}, "x-leechcraft/data-filter-request");
		const auto& handlers = manager->GetPossibleHandlers (e);

		QList<QObject*> result;
		std::copy_if (handlers.begin (), handlers.end (), std::back_inserter (result),
				[] (QObject *obj) { return qobject_cast<IDataFilter*> (obj); });
		return result;
	}

	Entity MakeEntity (const QVariant& entity,
			const QString& location,
			TaskParameters tp,
			const QString& mime)
	{
		Entity result;
		result.Entity_ = entity;
		result.Location_ = location;
		result.Parameters_ = tp;
		result.Mime_ = mime;
		return result;
	}

	Entity MakeNotification (const QString& header,
			const QString& text, Priority priority)
	{
		Entity result = MakeEntity (header,
				QString (),
				AutoAccept | OnlyHandle,
				"x-leechcraft/notification");
		result.Additional_ ["Text"] = text;
		result.Additional_ ["Priority"] = priority;
		return result;
	}

	Entity MakeANCancel (const Entity& event)
	{
		Entity e = MakeNotification (event.Entity_.toString (), QString (), PInfo_);
		e.Additional_ [AN::EF::SenderID] = event.Additional_ [AN::EF::SenderID];
		e.Additional_ [AN::EF::EventID] = event.Additional_ [AN::EF::EventID];
		e.Additional_ [AN::EF::EventCategory] = AN::CatEventCancel;
		return e;
	}

	Entity MakeANCancel (const QString& senderId, const QString& eventId)
	{
		Entity e = MakeNotification (QString (), QString (), PInfo_);
		e.Additional_ [AN::EF::SenderID] = senderId;
		e.Additional_ [AN::EF::EventID] = eventId;
		e.Additional_ [AN::EF::EventCategory] = AN::CatEventCancel;
		return e;
	}

	QVariant GetPersistentData (const QByteArray& key,
			const ICoreProxy_ptr& proxy)
	{
		const auto& plugins = proxy->GetPluginsManager ()->
				GetAllCastableTo<IPersistentStoragePlugin*> ();
		for (const auto plug : plugins)
		{
			const auto& storage = plug->RequestStorage ();
			if (!storage)
				continue;

			const auto& value = storage->Get (key);
			if (!value.isNull ())
				return value;
		}
		return {};
	}

	void SetJobHolderProgress (const QList<QStandardItem*>& row,
			qint64 done, qint64 total, const QString& text)
	{
		const auto item = row.value (JobHolderColumn::JobProgress);
		item->setText (text);
		SetJobHolderProgress (item, done, total);
	}

	void SetJobHolderProgress (QStandardItem *item, qint64 done, qint64 total)
	{
		item->setData (done, ProcessState::Done);
		item->setData (total, ProcessState::Total);
	}
}
}
