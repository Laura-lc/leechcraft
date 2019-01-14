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

#include "updatesmanager.h"
#include <QDateTime>
#include <QDomDocument>
#include <QTimer>
#include <interfaces/idownload.h>
#include <interfaces/core/ientitymanager.h>
#include <util/gui/util.h>
#include <util/sll/either.h>
#include <util/sll/functor.h>
#include <util/sll/visitor.h>
#include <util/sys/paths.h>
#include <util/xpc/util.h>
#include "dbupdatethread.h"
#include "dbupdatethreadworker.h"
#include "parser.h"
#include "parserfactory.h"
#include "storagebackend.h"
#include "storagebackendmanager.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft::Aggregator
{
	namespace
	{
		using ParseResult = Util::Either<QString, channels_container_t>;

		ParseResult ParseChannels (const QString& path, const QString& url, IDType_t feedId)
		{
			QFile file { path };
			if (!file.open (QIODevice::ReadOnly))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to open the local file"
						<< path;
				return ParseResult::Left (UpdatesManager::tr ("Unable to open the temporary file."));
			}

			QDomDocument doc;
			QString errorMsg;
			int errorLine, errorColumn;
			if (!doc.setContent (&file, true, &errorMsg, &errorLine, &errorColumn))
			{
				const auto& copyPath = Util::GetTemporaryName ("lc_aggregator_failed.XXXXXX");
				file.copy (copyPath);
				qWarning () << Q_FUNC_INFO
						<< "error parsing XML for"
						<< url
						<< errorMsg
						<< errorLine
						<< errorColumn
						<< "; copy at"
						<< file.fileName ();
				return ParseResult::Left (UpdatesManager::tr ("XML parse error for the feed %1.")
						.arg (url));
			}

			auto parser = ParserFactory::Instance ().Return (doc);
			if (!parser)
			{
				const auto& copyPath = Util::GetTemporaryName ("lc_aggregator_failed.XXXXXX");
				file.copy (copyPath);
				qWarning () << Q_FUNC_INFO
						<< "no parser for"
						<< url
						<< "; copy at"
						<< copyPath;
				return ParseResult::Left (UpdatesManager::tr ("Could not find parser to parse %1.")
						.arg (url));
			}

			return ParseResult::Right (parser->ParseFeed (doc, feedId));
		}

		QString GetErrorString (const IDownload::Error::Type type)
		{
			switch (type)
			{
			case IDownload::Error::Type::Unknown:
				break;
			case IDownload::Error::Type::NoError:
				return UpdatesManager::tr ("no error");
			case IDownload::Error::Type::NotFound:
				return UpdatesManager::tr ("address not found");
			case IDownload::Error::Type::AccessDenied:
				return UpdatesManager::tr ("access denied");
			case IDownload::Error::Type::LocalError:
				return UpdatesManager::tr ("local error");
			case IDownload::Error::Type::UserCanceled:
				return UpdatesManager::tr ("user canceled the download");
			}

			return UpdatesManager::tr ("unknown error");
		}
	}

	UpdatesManager::UpdatesManager (const DBUpdateThread_ptr& dbUpThread, IEntityManager *iem, QObject *parent)
	: QObject { parent }
	, EntityManager_ { iem }
	, DBUpThread_ { dbUpThread }
	, StorageBackend_ { StorageBackendManager::Instance ().MakeStorageBackendForThread () }
	, UpdateTimer_ { new QTimer { this } }
	, CustomUpdateTimer_ { new QTimer { this } }
	{
		UpdateTimer_->setSingleShot (true);
		connect (UpdateTimer_,
				&QTimer::timeout,
				this,
				&UpdatesManager::UpdateFeeds);

		CustomUpdateTimer_->start (60 * 1000);
		connect (CustomUpdateTimer_,
				&QTimer::timeout,
				this,
				&UpdatesManager::HandleCustomUpdates);

		auto now = QDateTime::currentDateTime ();
		auto lastUpdated = XmlSettingsManager::Instance ()->Property ("LastUpdateDateTime", now).toDateTime ();
		if (auto interval = XmlSettingsManager::Instance ()->property ("UpdateInterval").toInt ())
		{
			auto updateDiff = lastUpdated.secsTo (now);
			if (XmlSettingsManager::Instance ()->property ("UpdateOnStartup").toBool () ||
					updateDiff > interval * 60)
				QTimer::singleShot (7000,
						this,
						&UpdatesManager::UpdateFeeds);
			else
				UpdateTimer_->start (updateDiff * 1000);
		}

		XmlSettingsManager::Instance ()->RegisterObject ("UpdateInterval", this, "updateIntervalChanged");
	}

	void UpdatesManager::UpdateFeeds ()
	{
		for (const auto id : StorageBackend_->GetFeedsIDs ())
		{
			// It's handled by custom timer.
			if (StorageBackend_->GetFeedSettings (id).value_or (Feed::FeedSettings {}).UpdateTimeout_)
				continue;

			UpdateFeed (id);
		}

		XmlSettingsManager::Instance ()->setProperty ("LastUpdateDateTime", QDateTime::currentDateTime ());
		if (int interval = XmlSettingsManager::Instance ()->property ("UpdateInterval").toInt ())
			UpdateTimer_->start (interval * 60 * 1000);
	}

	void UpdatesManager::UpdateFeed (IDType_t id)
	{
		if (UpdatesQueue_.isEmpty ())
			QTimer::singleShot (500,
					this,
					&UpdatesManager::RotateUpdatesQueue);

		UpdatesQueue_ << id;
	}

	void UpdatesManager::updateIntervalChanged ()
	{
		if (int min = XmlSettingsManager::Instance ()->property ("UpdateInterval").toInt ())
		{
			if (UpdateTimer_->isActive ())
				UpdateTimer_->setInterval (min * 60 * 1000);
			else
				UpdateTimer_->start (min * 60 * 1000);
		}
		else
			UpdateTimer_->stop ();
	}

	void UpdatesManager::HandleCustomUpdates ()
	{
		using Util::operator*;

		QDateTime current = QDateTime::currentDateTime ();
		for (const auto id : StorageBackend_->GetFeedsIDs ())
		{
			const auto ut = (StorageBackend_->GetFeedSettings (id) * &Feed::FeedSettings::UpdateTimeout_).value_or (0);

			// It's handled by normal timer.
			if (!ut)
				continue;

			if (!Updates_.contains (id) ||
					(Updates_ [id].isValid () &&
						Updates_ [id].secsTo (current) / 60 > ut))
			{
				UpdateFeed (id);
				Updates_ [id] = QDateTime::currentDateTime ();
			}
		}
	}

	void UpdatesManager::RotateUpdatesQueue ()
	{
		if (UpdatesQueue_.isEmpty ())
			return;

		const auto feedId = UpdatesQueue_.takeFirst ();

		if (!UpdatesQueue_.isEmpty ())
			QTimer::singleShot (2000,
					this,
					&UpdatesManager::RotateUpdatesQueue);

		const auto& url = StorageBackend_->GetFeed (feedId).URL_;

		auto filename = Util::GetTemporaryName ();

		auto e = Util::MakeEntity (QUrl (url),
				filename,
				Internal |
					DoNotNotifyUser |
					DoNotSaveInHistory |
					NotPersistent |
					DoNotAnnounceEntity);

		auto emitError = [this] (const QString& body)
		{
			EntityManager_->HandleEntity (Util::MakeNotification (tr ("Feed error"), body, Priority::Critical));
		};

		const auto& delegateResult = EntityManager_->DelegateEntity (e);
		if (!delegateResult)
		{
			emitError (tr ("Could not find plugin for feed with URL %1")
					.arg (url));
			return;
		}

		Util::Sequence (this, delegateResult.DownloadResult_) >>
				Util::Visitor
				{
					[=] (IDownload::Success)
					{
						Util::Visit (ParseChannels (filename, url, feedId),
								[&] (const channels_container_t& channels)
								{
									DBUpThread_->ScheduleImpl (&DBUpdateThreadWorker::updateFeed, channels, url);
								},
								emitError);
					},
					[=] (const IDownload::Error& error)
					{
						if (!XmlSettingsManager::Instance ()->property ("BeSilent").toBool ())
							emitError (tr ("Unable to download %1: %2.")
									.arg (Util::FormatName (url))
									.arg (GetErrorString (error.Type_)));
					}
				}.Finally ([filename] { QFile::remove (filename); });
	}
}