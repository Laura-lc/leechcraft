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

#include "feedsettings.h"
#include <QDesktopServices>
#include <util/tags/tagscompleter.h>
#include <util/tags/tagscompletionmodel.h>
#include <util/sll/functor.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/itagsmanager.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/iwebbrowser.h>
#include "storagebackendmanager.h"
#include "storagebackend.h"
#include "core.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Aggregator
{
	namespace
	{
		void SetLabelLink (QLabel *label, QString link)
		{
			QString shortLink;
			label->setToolTip (link);
			if (link.size () >= 160)
				shortLink = link.left (78) + "..." + link.right (78);
			else
				shortLink = link;

			if (QUrl { link }.isValid ())
			{
				link.insert (0, "<a href=\"");
				link.append ("\">" + shortLink + "</a>");
				label->setText (link);
			}
			else
				label->setText (shortLink);
		}
	}

	FeedSettings::FeedSettings (const QModelIndex& mapped, const ICoreProxy_ptr& proxy, QWidget *parent)
	: QDialog (parent)
	, Index_ (mapped)
	{
		Ui_.setupUi (this);

		ChannelTagsCompleter_ = std::make_shared<Util::TagsCompleter> (Ui_.ChannelTags_);
		Ui_.ChannelTags_->AddSelector ();

		connect (Ui_.ChannelLink_,
				&QLabel::linkActivated,
				[proxy] (const QString& url)
				{
					const auto browser = proxy->GetPluginsManager ()->GetAllCastableTo<IWebBrowser*> ().value (0);
					if (!browser || XmlSettingsManager::Instance ()->property ("AlwaysUseExternalBrowser").toBool ())
						QDesktopServices::openUrl ({ url });
					else
						browser->Open (url);
				});

		const auto& tags = Index_.data (ChannelRoles::HumanReadableTags).toStringList ();
		Ui_.ChannelTags_->setText (proxy->GetTagsManager ()->Join (tags));

		using Util::operator*;
		const auto feedId = Index_.data (ChannelRoles::FeedID).value<IDType_t> ();
		[&] (auto&& settings)
		{
			Ui_.UpdateInterval_->setValue (settings.UpdateTimeout_);
			Ui_.NumItems_->setValue (settings.NumItems_);
			Ui_.ItemAge_->setValue (settings.ItemAge_);
			Ui_.AutoDownloadEnclosures_->setChecked (settings.AutoDownloadEnclosures_);
		} * StorageBackendManager::Instance ().MakeStorageBackendForThread ()->GetFeedSettings (feedId);

		Core::ChannelInfo ci = Core::Instance ().GetChannelInfo (Index_);

		SetLabelLink (Ui_.ChannelLink_, ci.Link_);
		SetLabelLink (Ui_.FeedURL_, ci.URL_);

		Ui_.ChannelDescription_->setHtml (ci.Description_);
		Ui_.ChannelAuthor_->setText (ci.Author_);

		Ui_.FeedNumItems_->setText (QString::number (ci.NumItems_));

		QPixmap pixmap = Core::Instance ().GetChannelPixmap (Index_);
		if (pixmap.width () > 400 || pixmap.height () > 300)
			pixmap = pixmap.scaled (400, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		Ui_.ChannelImage_->setPixmap (pixmap);
	}

	void FeedSettings::accept ()
	{
		QString tags = Ui_.ChannelTags_->text ();
		Core::Instance ().SetTagsForIndex (tags, Index_);

		StorageBackendManager::Instance ().MakeStorageBackendForThread ()->SetFeedSettings ({
				Index_.data (ChannelRoles::FeedID).value<IDType_t> (),
				Ui_.UpdateInterval_->value (),
				Ui_.NumItems_->value (),
				Ui_.ItemAge_->value (),
				Ui_.AutoDownloadEnclosures_->checkState () == Qt::Checked
			});

		QDialog::accept ();
	}

	void FeedSettings::on_UpdateFavicon__released ()
	{
		Core::Instance ().UpdateFavicon (Index_);
	}
}
}
