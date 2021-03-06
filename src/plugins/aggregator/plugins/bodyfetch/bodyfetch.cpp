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

#include "bodyfetch.h"
#include <cstring>
#include <QIcon>
#include <interfaces/iscriptloader.h>
#include <interfaces/idownload.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/core/ipluginsmanager.h>
#include <util/xpc/util.h>
#include <util/sys/paths.h>
#include <util/sll/prelude.h>
#include <util/sll/either.h>
#include <util/sll/visitor.h>
#include <util/threads/futures.h>
#include "workerobject.h"

namespace LC
{
namespace Aggregator
{
namespace BodyFetch
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		StorageDir_ = Util::CreateIfNotExists ("aggregator/bodyfetcher/storage");

		static const auto suffixLength = std::strlen (".html");
		for (int i = 0; i < 10; ++i)
		{
			const QString& name = QString::number (i);
			if (!StorageDir_.exists (name))
				StorageDir_.mkdir (name);
			else
			{
				QDir dir = StorageDir_;
				dir.cd (name);
				FetchedItems_ = Util::MapAs<QSet> (dir.entryList (),
						[] (QString& name)
						{
							name.chop (suffixLength);
							return name.toULongLong ();
						});
			}
		}

		Proxy_ = proxy;
	}

	void Plugin::SecondInit ()
	{
		WO_ = new WorkerObject (AggregatorProxy_, this);

		IScriptLoader *loader = Proxy_->GetPluginsManager ()->
				GetAllCastableTo<IScriptLoader*> ().value (0, 0);
		if (!loader)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to find a suitable loader, aborting";
			return;
		}

		const auto& inst = loader->CreateScriptLoaderInstance ("aggregator/recipes/");
		if (!inst)
		{
			qWarning () << Q_FUNC_INFO
					<< "got a null script loader instance";
			return;
		}

		inst->AddGlobalPrefix ();
		inst->AddLocalPrefix ();

		WO_->SetLoaderInstance (inst);

		connect (WO_,
				SIGNAL (downloadRequested (QUrl)),
				this,
				SLOT (handleDownload (QUrl)));
		connect (WO_,
				SIGNAL (newBodyFetched (quint64)),
				this,
				SLOT (handleBodyFetched (quint64)),
				Qt::QueuedConnection);
		connect (this,
				SIGNAL (downloadFinished (QUrl, QString)),
				WO_,
				SLOT (handleDownloadFinished (QUrl, QString)));
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Aggregator.BodyFetch";
	}

	void Plugin::Release ()
	{
		delete WO_;
	}

	QString Plugin::GetName () const
	{
		return "Aggregator BodyFetch";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Fetches full bodies of news items following links in them.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/aggregator/bodyfetch/resources/images/bodyfetch.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> result;
		result << "org.LeechCraft.Aggregator.GeneralPlugin/1.0";
		return result;
	}

	void Plugin::InitPlugin (IProxyObject *proxy)
	{
		AggregatorProxy_ = proxy;
	}

	void Plugin::hookItemLoad (IHookProxy_ptr, Item *item)
	{
		if (!FetchedItems_.contains (item->ItemID_))
			return;

		const quint64 id = item->ItemID_;
		if (!ContentsCache_.contains (id))
		{
			QDir dir = StorageDir_;

			dir.cd (QString::number (id % 10));
			QFile file (dir.filePath (QString ("%1.html").arg (id)));
			if (!file.open (QIODevice::ReadOnly))
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to open file:"
						<< file.fileName ()
						<< file.errorString ();
				return;
			}

			ContentsCache_ [id] = QString::fromUtf8 (file.readAll ());
		}

		const QString& contents = ContentsCache_ [id];
		if (!contents.isEmpty ())
			item->Description_ = contents;
	}

	void Plugin::hookItemAdded (IHookProxy_ptr, const Item& item)
	{
		if (!WO_ || !WO_->IsOk ())
			return;

		WO_->AppendItem (item);
	}

	void Plugin::handleDownload (QUrl url)
	{
		const QString& temp = Util::GetTemporaryName ("agg_bodyfetcher");

		const Entity& e = Util::MakeEntity (url,
				temp,
				Internal |
					DoNotNotifyUser |
					DoNotSaveInHistory |
					OnlyDownload |
					NotPersistent);
		auto result = Proxy_->GetEntityManager ()->DelegateEntity (e);
		if (!result)
		{
			qWarning () << Q_FUNC_INFO
					<< "delegation failed";
			return;
		}

		Util::Sequence (this, result.DownloadResult_) >>
				Util::Visitor
				{
					[this, url, temp] (IDownload::Success) { emit downloadFinished (url, temp); },
					[] (const IDownload::Error&) {}
				};
	}

	void Plugin::handleBodyFetched (quint64 id)
	{
		FetchedItems_ << id;
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_aggregator_bodyfetch, LC::Aggregator::BodyFetch::Plugin);
