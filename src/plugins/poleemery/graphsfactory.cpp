/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "graphsfactory.h"
#include <QStringList>
#include <QMap>
#include <qwt_plot_curve.h>
#include <qwt_plot_histogram.h>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include "core.h"
#include "operationsmanager.h"
#include "prelude.h"
#include "accountsmanager.h"
#include "currenciesmanager.h"

namespace LeechCraft
{
namespace Poleemery
{
	namespace
	{
		QList<EntryBase_ptr> GetLastEntries (int days)
		{
			auto opsMgr = Core::Instance ().GetOpsManager ();
			auto entries = opsMgr->GetAllEntries ();

			const auto& now = QDateTime::currentDateTime ();
			const auto& startDt = now.addDays (-days);
			auto pos = std::upper_bound (entries.begin (), entries.end (), startDt,
					[] (const QDateTime& dt, const EntryBase_ptr& entry)
						{ return dt < entry->Date_; });
			entries.erase (entries.begin (), pos);
			return entries;
		}

		QMap<int, BalanceInfo> GetDays2Infos (int days)
		{
			auto opsMgr = Core::Instance ().GetOpsManager ();
			const auto& entries = opsMgr->GetEntriesWBalance ();

			const auto& now = QDateTime::currentDateTime ();
			const auto& startDt = now.addDays (-days);
			auto pos = std::upper_bound (entries.begin (), entries.end (), startDt,
					[] (const QDateTime& dt, const EntryWithBalance& entry)
						{ return dt < entry.Entry_->Date_; });

			QMap<int, BalanceInfo> days2infos;
			for (; pos != entries.end (); ++pos)
			{
				const auto daysBack = pos->Entry_->Date_.daysTo (now);
				days2infos [days - daysBack] = pos->Balance_;
			}

			return days2infos;
		}

		QMap<int, double> GetLastBalances (const QMap<int, BalanceInfo>& days2infos)
		{
			auto accsMgr = Core::Instance ().GetAccsManager ();
			const auto& accs = accsMgr->GetAccounts ();

			QMap<int, double> lastBalances;
			for (const auto& balance : days2infos)
			{
				for (const auto& id : balance.Accs_.keys ())
					if (!lastBalances.contains (id))
						lastBalances [id] = balance.Accs_ [id];

				if (accs.size () == lastBalances.size ())
					break;
			}

			return lastBalances;
		}

		void AddUp (QMap<int, QVector<double>>& accBalances)
		{
			if (accBalances.isEmpty ())
				return;

			QVector<double> curSum (accBalances.begin ()->size (), 0);
			for (auto& vec : accBalances)
				curSum = vec = ZipWith (vec, curSum, std::plus<double> ());
		}

		QList<QColor> GenerateColors ()
		{
			return { Qt::green, Qt::blue, Qt::red, Qt::magenta, Qt::darkRed };
		}

		QList<QwtPlotItem*> CreateBalanceItems (int days, bool cumulative)
		{
			const auto& days2infos = GetDays2Infos (days);
			const auto& xData = Map (days2infos.keys (), [] (int d) -> double { return d; }).toVector ();
			auto lastBalances = GetLastBalances (days2infos);

			const auto& periodAccounts = lastBalances.keys ();

			QMap<int, QVector<double>> accBalances;
			for (const auto& balance : days2infos)
				for (auto acc : periodAccounts)
				{
					auto value = balance.Accs_.value (acc, lastBalances [acc]);
					accBalances [acc] << value;
					lastBalances [acc] = value;
				}

			auto accsMgr = Core::Instance ().GetAccsManager ();
			auto curMgr = Core::Instance ().GetCurrenciesManager ();
			for (auto accId : accBalances.keys ())
			{
				const auto& acc = accsMgr->GetAccount (accId);
				const auto& rate = curMgr->GetUserCurrencyRate (acc.Currency_);
				if (rate != 1)
				{
					auto& vec = accBalances [accId];
					vec = Map (vec, [rate] (double x) { return x * rate; });
				}
			}

			if (cumulative)
				AddUp (accBalances);

			const auto& colors = GenerateColors ();
			int currentColor = 0;

			QList<QwtPlotItem*> result;
			int z = periodAccounts.size ();
			for (auto accId : periodAccounts)
			{
				const auto& acc = accsMgr->GetAccount (accId);

				auto curColor = colors.at (currentColor++ % colors.size ());

				auto item = new QwtPlotCurve (acc.Name_);
				item->setPen (curColor);

				if (!cumulative)
					curColor.setAlphaF (0.2);
				item->setBrush (curColor);

				item->setZ (z--);
				item->setSamples (xData, accBalances [accId]);

				result << item;
			}

			auto grid = new QwtPlotGrid;
			grid->enableYMin (true);
			grid->enableXMin (true);
			grid->setMinPen (QPen (Qt::gray, 1, Qt::DashLine));
			result << grid;

			return result;
		}

		QList<QwtPlotItem*> CreateSpendingBreakdownItems (int days, bool absolute)
		{
			double income = 0;
			double savings = 0;
			QMap<QString, double> cat2amount;
			for (auto entry : GetLastEntries (days))
			{
				switch (entry->GetType ())
				{
				case EntryType::Expense:
				{
					auto expense = std::dynamic_pointer_cast<ExpenseEntry> (entry);
					if (expense->Categories_.isEmpty ())
						cat2amount [QObject::tr ("uncategorized")] += expense->Amount_;
					else
						for (const auto& cat : expense->Categories_)
							cat2amount [cat] += expense->Amount_;

					savings -= entry->Amount_;
					break;
				}
				case EntryType::Receipt:
					income += entry->Amount_;
					savings += entry->Amount_;
					break;
				}
			}

			if (absolute)
				cat2amount [QObject::tr ("income")] = income;
			cat2amount [QObject::tr ("savings")] = savings;

			if (!absolute)
				for (auto& val : cat2amount)
					val = val * 100 / income;

			const auto& colors = GenerateColors ();
			int currentIndex = 0;

			QList<QwtPlotItem*> result;
			for (const auto& cat : cat2amount.keys ())
			{
				auto item = new QwtPlotHistogram (cat);

				auto curColor = colors.at (currentIndex++ % colors.size ());

				item->setPen (curColor);
				item->setBrush (curColor);
				item->setSamples ({ { cat2amount [cat],
							static_cast<double> (currentIndex * 5),
							static_cast<double> (currentIndex * 5 + 1) } });

				result << item;
			}

			auto grid = new QwtPlotGrid;
			grid->enableYMin (true);
			grid->enableX (false);
			grid->setMinPen (QPen (Qt::gray, 1, Qt::DashLine));
			result << grid;

			return result;
		}
	}

	GraphsFactory::GraphsFactory ()
	{
		Infos_.append ({
				QObject::tr ("Cumulative accounts balance (month)"),
				[this] { return CreateBalanceItems (30, true); },
				[] (QwtPlot *plot) -> void
				{
					auto curMgr = Core::Instance ().GetCurrenciesManager ();
					plot->enableAxis (QwtPlot::Axis::xBottom, true);
					plot->enableAxis (QwtPlot::Axis::yLeft, true);
					plot->setAxisTitle (QwtPlot::Axis::xBottom, QObject::tr ("Days"));
					plot->setAxisTitle (QwtPlot::Axis::yLeft, curMgr->GetUserCurrency ());
				}
			});
		Infos_.append ({
				QObject::tr ("Comparative accounts balance (month)"),
				[this] { return CreateBalanceItems (30, false); },
				[] (QwtPlot *plot) -> void
				{
					auto curMgr = Core::Instance ().GetCurrenciesManager ();
					plot->enableAxis (QwtPlot::Axis::xBottom, true);
					plot->enableAxis (QwtPlot::Axis::yLeft, true);
					plot->setAxisTitle (QwtPlot::Axis::xBottom, QObject::tr ("Days"));
					plot->setAxisTitle (QwtPlot::Axis::yLeft, curMgr->GetUserCurrency ());
				}
			});
		Infos_.append ({
				QObject::tr ("Per-category spendings breakdown (absolute, month)"),
				[this] { return CreateSpendingBreakdownItems (30, true); },
				[] (QwtPlot *plot) -> void
				{
					auto curMgr = Core::Instance ().GetCurrenciesManager ();
					plot->enableAxis (QwtPlot::Axis::xBottom, false);
					plot->enableAxis (QwtPlot::Axis::yLeft, true);
					plot->setAxisTitle (QwtPlot::Axis::yLeft, curMgr->GetUserCurrency ());
				}
			});
		Infos_.append ({
				QObject::tr ("Per-category spendings breakdown (relative, month)"),
				[this] { return CreateSpendingBreakdownItems (30, false); },
				[] (QwtPlot *plot) -> void
				{
					auto curMgr = Core::Instance ().GetCurrenciesManager ();
					plot->enableAxis (QwtPlot::Axis::xBottom, false);
					plot->enableAxis (QwtPlot::Axis::yLeft, true);
					plot->setAxisTitle (QwtPlot::Axis::yLeft, "%");
				}
			});
	}

	QStringList GraphsFactory::GetNames () const
	{
		QStringList result;
		for (const auto& info : Infos_)
			result << info.Name_;
		return result;
	}

	QList<QwtPlotItem*> GraphsFactory::CreateItems (int index)
	{
		if (index < 0 || index >= Infos_.size ())
			return {};

		return Infos_.at (index).Creator_ ();
	}

	void GraphsFactory::PreparePlot (int index, QwtPlot *plot)
	{
		if (index < 0 || index >= Infos_.size ())
			return;

		Infos_.at (index).Preparer_ (plot);
	}
}
}
