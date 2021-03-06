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

#include "artistbrowsertab.h"
#include <QMessageBox>
#include <QQuickWidget>
#include <interfaces/media/iartistbiofetcher.h>
#include <interfaces/core/ipluginsmanager.h>
#include <util/gui/clearlineeditaddon.h>
#include <util/qml/standardnamfactory.h>
#include <util/sys/paths.h>
#include "similarviewmanager.h"
#include "bioviewmanager.h"
#include "previewhandler.h"

namespace LC
{
namespace LMP
{
	ArtistBrowserTab::ArtistBrowserTab (const ICoreProxy_ptr& proxy,
			const TabClassInfo& tc, QObject *plugin)
	: TC_ (tc)
	, Plugin_ (plugin)
	, View_ (new QQuickWidget)
	, BioMgr_ (new BioViewManager (proxy, View_, this))
	, SimilarMgr_ (new SimilarViewManager (proxy, View_, this))
	, Proxy_ (proxy)
	{
		Ui_.setupUi (this);

		View_->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
		View_->setResizeMode (QQuickWidget::SizeRootObjectToView);
		layout ()->addWidget (View_);

		new Util::StandardNAMFactory ("lmp/qml",
				[] { return 50 * 1024 * 1024; },
				View_->engine ());
		View_->setSource (Util::GetSysPathUrl (Util::SysPath::QML, "lmp", "ArtistBrowserView.qml"));

		BioMgr_->InitWithSource ();
		SimilarMgr_->InitWithSource ();

		new Util::ClearLineEditAddon (proxy, Ui_.ArtistNameEdit_);
	}

	TabClassInfo ArtistBrowserTab::GetTabClassInfo () const
	{
		return TC_;
	}

	QObject* ArtistBrowserTab::ParentMultiTabs ()
	{
		return Plugin_;
	}

	void ArtistBrowserTab::Remove ()
	{
		emit removeTab (this);
	}

	QToolBar* ArtistBrowserTab::GetToolBar () const
	{
		return 0;
	}

	QByteArray ArtistBrowserTab::GetTabRecoverData () const
	{
		const auto& artist = Ui_.ArtistNameEdit_->text ();
		if (artist.isEmpty ())
			return QByteArray ();

		QByteArray result;
		QDataStream stream (&result, QIODevice::WriteOnly);
		stream << QByteArray ("artistbrowser") << artist;
		return result;
	}

	QIcon ArtistBrowserTab::GetTabRecoverIcon () const
	{
		return TC_.Icon_;
	}

	QString ArtistBrowserTab::GetTabRecoverName () const
	{
		const auto& artist = Ui_.ArtistNameEdit_->text ();
		return artist.isEmpty () ? QString () : tr ("Artist browser: %1");
	}

	void ArtistBrowserTab::Browse (const QString& artist)
	{
		Ui_.ArtistNameEdit_->setText (artist);
		on_ArtistNameEdit__returnPressed ();
	}

	void ArtistBrowserTab::on_ArtistNameEdit__returnPressed ()
	{
		auto provs = Proxy_->GetPluginsManager ()->
				GetAllCastableTo<Media::IArtistBioFetcher*> ();
		if (provs.isEmpty ())
		{
			QMessageBox::critical (this,
					"LeechCraft",
					tr ("There aren't any plugins that can fetch biography. Check if "
						"you have installed for example the LastFMScrobble plugin."));
			return;
		}

		auto artist = Ui_.ArtistNameEdit_->text ().trimmed ();

		BioMgr_->Request (provs.first (), artist, {});
		SimilarMgr_->DefaultRequest (artist);

		emit tabRecoverDataChanged ();
	}
}
}
