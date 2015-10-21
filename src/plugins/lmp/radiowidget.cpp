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

#include "radiowidget.h"
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QInputDialog>
#include <QtDebug>
#include <util/gui/clearlineeditaddon.h>
#include <interfaces/media/iradiostationprovider.h>
#include <interfaces/core/iiconthememanager.h>
#include "core.h"
#include "player.h"
#include "previewhandler.h"
#include "radiomanager.h"
#include "engine/sourceobject.h"
#include "radiocustomdialog.h"

namespace LeechCraft
{
namespace LMP
{
	namespace
	{
		class StationsFilterModel : public QSortFilterProxyModel
		{
		public:
			StationsFilterModel (QObject *parent)
			: QSortFilterProxyModel (parent)
			{
			}
		protected:
			bool filterAcceptsRow (int row, const QModelIndex& parent) const
			{
				const auto& pat = this->filterRegExp ().pattern ();
				if (pat.isEmpty ())
					return true;

				const auto& idx = sourceModel ()->index (row, 0, parent);
				if (idx.data (Media::RadioItemRole::ItemType).toInt () == Media::RadioType::None)
					return true;

				return idx.data ().toString ().contains (pat, Qt::CaseInsensitive);
			}
		};
	}

	RadioWidget::RadioWidget (QWidget *parent)
	: QWidget (parent)
	, StationsProxy_ (new StationsFilterModel (this))
	{
		Ui_.setupUi (this);

		StationsProxy_->setDynamicSortFilter (true);
		StationsProxy_->setSourceModel (Core::Instance ().GetRadioManager ()->GetModel ());
		Ui_.StationsView_->setModel (StationsProxy_);

		connect (Ui_.StationsSearch_,
				SIGNAL (textChanged (QString)),
				StationsProxy_,
				SLOT (setFilterFixedString (QString)));

		new Util::ClearLineEditAddon (Core::Instance ().GetProxy (), Ui_.StationsSearch_);
	}

	void RadioWidget::SetPlayer (Player *player)
	{
		Player_ = player;
	}

	void RadioWidget::AddUrl (const QUrl& url)
	{
		RadioCustomDialog dia (this);
		dia.SetUrl (url);
		if (dia.exec () != QDialog::Accepted)
			return;

		const auto& unmapped = Ui_.StationsView_->currentIndex ();
		const auto& index = StationsProxy_->mapToSource (unmapped);
		Core::Instance ().GetRadioManager ()->
				AddUrl (index, dia.GetUrl (), dia.GetName ());
	}

	void RadioWidget::handleRefresh ()
	{
		const auto& unmapped = Ui_.StationsView_->currentIndex ();
		const auto& index = StationsProxy_->mapToSource (unmapped);
		Core::Instance ().GetRadioManager ()->Refresh (index);
	}

	void RadioWidget::handleAddUrl ()
	{
		AddUrl ({});
	}

	void RadioWidget::handleAddCurrentUrl ()
	{
		const auto& url = Player_->GetSourceObject ()->
				GetCurrentSource ().ToUrl ();
		if (url.isLocalFile ())
			return;

		AddUrl (url);
	}

	void RadioWidget::handleRemoveUrl ()
	{
		const auto& unmapped = Ui_.StationsView_->currentIndex ();
		const auto& index = StationsProxy_->mapToSource (unmapped);
		Core::Instance ().GetRadioManager ()->RemoveUrl (index);
	}

	void RadioWidget::on_StationsView__customContextMenuRequested (const QPoint& point)
	{
		const auto& idx = Ui_.StationsView_->indexAt (point);
		if (!idx.isValid ())
			return;

		const auto type = idx.data (Media::RadioItemRole::ItemType).toInt ();
		const auto parentType = idx.parent ().data (Media::RadioItemRole::ItemType).toInt ();

		const auto iconsMgr = Core::Instance ().GetProxy ()->GetIconThemeManager ();

		QMenu menu;
		menu.addAction (iconsMgr->GetIcon ("view-refresh"),
				tr ("Refresh"),
				this,
				SLOT (handleRefresh ()));

		switch (type)
		{
		case Media::RadioType::CustomAddableStreams:
		{
			menu.addAction (iconsMgr->GetIcon ("list-add"),
					tr ("Add an URL..."),
					this,
					SLOT (handleAddUrl ()));

			const auto& url = Player_->GetSourceObject ()->GetCurrentSource ().ToUrl ();
			if (url.isValid () && !url.isLocalFile ())
				menu.addAction (tr ("Add current stream..."),
						this,
						SLOT (handleAddCurrentUrl ()));
			break;
		}
		default:
			break;
		}

		if (parentType == Media::RadioType::CustomAddableStreams)
		{
			menu.addAction (iconsMgr->GetIcon ("list-remove"),
					tr ("Remove this URL"),
					this,
					SLOT (handleRemoveUrl ()));
		}

		menu.exec (Ui_.StationsView_->viewport ()->mapToGlobal (point));
	}

	void RadioWidget::on_StationsView__doubleClicked (const QModelIndex& unmapped)
	{
		const auto& index = StationsProxy_->mapToSource (unmapped);
		Core::Instance ().GetRadioManager ()->Handle (index, Player_);
	}
}
}
