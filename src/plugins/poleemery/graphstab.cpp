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

#include "graphstab.h"
#include <numeric>
#include <qwt_legend.h>
#include "graphsfactory.h"
#include "core.h"
#include "operationsmanager.h"

namespace LeechCraft
{
namespace Poleemery
{
	GraphsTab::GraphsTab (const TabClassInfo& tc, QObject *plugin)
	: TC_ (tc)
	, ParentPlugin_ (plugin)
	{
		Ui_.setupUi (this);

		Ui_.GraphType_->addItems (GraphsFactory ().GetNames ());
		Ui_.GraphType_->setCurrentIndex (-1);

		auto legend = new QwtLegend;
		legend->setDefaultItemMode (QwtLegendData::Clickable);
		Ui_.Plot_->insertLegend (legend, QwtPlot::BottomLegend);

		connect (Ui_.GraphType_,
				SIGNAL (activated (int)),
				this,
				SLOT (updateGraph ()));
		connect (Ui_.From_,
				SIGNAL (dateChanged (QDate)),
				this,
				SLOT (updateGraph ()));
		connect (Ui_.To_,
				SIGNAL (dateChanged (QDate)),
				this,
				SLOT (updateGraph ()));

		connect (Ui_.PredefinedDate_,
				SIGNAL (currentIndexChanged (int)),
				this,
				SLOT (setPredefinedDate (int)));
		setPredefinedDate (0);
		Ui_.To_->setDateTime (QDateTime::currentDateTime ());
	}

	TabClassInfo GraphsTab::GetTabClassInfo () const
	{
		return TC_;
	}

	QObject* GraphsTab::ParentMultiTabs ()
	{
		return ParentPlugin_;
	}

	void GraphsTab::Remove ()
	{
		emit removeTab (this);
		deleteLater ();
	}

	QToolBar* GraphsTab::GetToolBar () const
	{
		return 0;
	}

	void GraphsTab::updateGraph ()
	{
		Ui_.Plot_->detachItems ();

		const auto index = Ui_.GraphType_->currentIndex ();
		if (index < 0)
			return;

		const auto& from = Ui_.From_->dateTime ();
		const auto& to = Ui_.To_->dateTime ();

		GraphsFactory f;
		for (const auto& item : f.CreateItems (index, { from, to }))
		{
			item->setRenderHint (QwtPlotItem::RenderAntialiased);
			item->attach (Ui_.Plot_);
		}
		f.PreparePlot (index, Ui_.Plot_);

		Ui_.Plot_->replot ();
	}

	void GraphsTab::setPredefinedDate (int index)
	{
		const auto& now = QDateTime::currentDateTime ();

		QDateTime first;
		switch (index)
		{
		case 0:
			first = now.addDays (-7);
			break;
		case 1:
			first = now.addMonths (-1);
			break;
		default:
		{
			const auto& entries = Core::Instance ().GetOpsManager ()->GetAllEntries ();
			first = std::accumulate (entries.begin (), entries.end (), now,
					[] (const QDateTime& d, EntryBase_ptr e)
						{ return std::min (d, e->Date_); });
			break;
		}
		}

		Ui_.From_->setDateTime (first);
		Ui_.To_->setDateTime (now);

		updateGraph ();
	}
}
}
