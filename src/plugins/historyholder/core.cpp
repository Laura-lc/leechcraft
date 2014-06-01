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

#include "core.h"
#include <typeinfo>
#include <stdexcept>
#include <QUrl>
#include <QSettings>
#include <QTextCodec>
#include <QToolBar>
#include <QAction>
#include <QTreeView>
#include <QMainWindow>
#include <QCoreApplication>
#include <QTimer>
#include <QtConcurrentRun>
#include <QFutureWatcher>
#include <util/structuresops.h>
#include <util/util.h>
#include <util/sll/onetimerunner.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/itagsmanager.h>
#include "findproxy.h"

using namespace LeechCraft::Plugins::HistoryHolder;

QDataStream& operator<< (QDataStream& out, const Core::HistoryEntry& e)
{
	quint16 version = 1;
	out << version;

	out << e.Entity_
		<< e.DateTime_;

	return out;
}

QDataStream& operator>> (QDataStream& in, Core::HistoryEntry& e)
{
	quint16 version;
	in >> version;
	if (version == 1)
	{
		in >> e.Entity_
			>> e.DateTime_;
	}
	else
	{
		qWarning () << Q_FUNC_INFO
			<< "unknown version"
			<< version;
	}
	return in;
}

LeechCraft::Plugins::HistoryHolder::Core::Core ()
: ToolBar_ (new QToolBar)
, WriteScheduled_ (false)
{
	Headers_ << tr ("Entity/location")
			<< tr ("Date")
			<< tr ("Tags");

	qRegisterMetaType<HistoryEntry> ("LeechCraft::Plugins::HistoryHolder::Core::HistoryEntry");
	qRegisterMetaTypeStreamOperators<HistoryEntry> ("LeechCraft::Plugins::HistoryHolder::Core::HistoryEntry");

	QSettings settings (QCoreApplication::organizationName (),
			QCoreApplication::applicationName () + "_HistoryHolder");
	int size = settings.beginReadArray ("History");
	for (int i = 0; i < size; ++i)
	{
		settings.setArrayIndex (i);
		History_.append (settings.value ("Item").value<HistoryEntry> ());
	}
	settings.endArray ();

	Remove_ = ToolBar_->addAction (tr ("Remove"),
			this,
			SLOT (remove ()));
	Remove_->setProperty ("ActionIcon", "list-remove");
}

Core& Core::Instance ()
{
	static Core core;
	return core;
}

void Core::Release ()
{
	ToolBar_.reset ();
}

void Core::SetCoreProxy (ICoreProxy_ptr proxy)
{
	CoreProxy_ = proxy;
}

ICoreProxy_ptr Core::GetCoreProxy () const
{
	return CoreProxy_;
}

void Core::Handle (const LeechCraft::Entity& entity)
{
	if (entity.Parameters_ & LeechCraft::DoNotSaveInHistory ||
			entity.Parameters_ & LeechCraft::Internal ||
			!(entity.Parameters_ & LeechCraft::IsDownloaded))
		return;

	HistoryEntry entry =
	{
		entity,
		QDateTime::currentDateTime ()
	};

	beginInsertRows (QModelIndex (), 0, 0);
	History_.prepend (entry);
	endInsertRows ();

	ScheduleWrite ();
}

void Core::SetShortcut (const QString& id, const QKeySequences_t& seq)
{
	if (id == "HistHolderRemove")
		Remove_->setShortcuts (seq);
}

QMap<QString, LeechCraft::ActionInfo> Core::GetActionInfo () const
{
	QMap<QString, ActionInfo> result;
	result ["HistHolderRemove"] = ActionInfo (Remove_->text (),
			Remove_->shortcut (), Remove_->icon ());
	return result;
}

int Core::columnCount (const QModelIndex&) const
{
	return Headers_.size ();
}

QVariant Core::data (const QModelIndex& index, int role) const
{
	int row = index.row ();
	HistoryEntry e = History_.at (row);
	if (role == Qt::DisplayRole)
	{
		switch (index.column ())
		{
			case 0:
				{
					QString stren;
					if (e.Entity_.Entity_.canConvert<QUrl> ())
						stren = e.Entity_.Entity_.toUrl ().toString ();
					else if (e.Entity_.Entity_.canConvert<QByteArray> ())
					{
						QByteArray entity = e.Entity_.Entity_.toByteArray ();
						if (entity.size () < 250)
							stren = QTextCodec::codecForName ("UTF-8")->
								toUnicode (entity);
					}
					else
						stren = tr ("Binary data");

					if (!e.Entity_.Location_.isEmpty ())
					{
						stren += " (";
						stren += e.Entity_.Location_;
						stren += ")";
					}
					return stren;
				}
			case 1:
				return e.DateTime_;
			case 2:
				return CoreProxy_->GetTagsManager ()->
					Join (data (index, RoleTags).toStringList ());
			default:
				return QVariant ();
		}
	}
	else if (role == RoleTags)
	{
		QStringList tagids = History_.at (row).Entity_.Additional_ [" Tags"].toStringList ();
		QStringList result;
		Q_FOREACH (QString id, tagids)
			result << CoreProxy_->GetTagsManager ()->GetTag (id);
		return result;
	}
	else if (role == RoleControls)
		return QVariant::fromValue<QToolBar*> (ToolBar_.get ());
	else if (role == RoleHash)
		return e.Entity_.Entity_;
	else if (role == RoleMime)
		return e.Entity_.Mime_;
	else
		return QVariant ();
}

QVariant Core::headerData (int section, Qt::Orientation orient, int role) const
{
	if (orient != Qt::Horizontal ||
			role != Qt::DisplayRole)
		return QVariant ();

	return Headers_ [section];
}

QModelIndex Core::index (int row, int column, const QModelIndex& parent) const
{
	if (parent.isValid () ||
			!hasIndex (row, column, parent))
		return QModelIndex ();

	return createIndex (row, column);
}

QModelIndex Core::parent (const QModelIndex&) const
{
	return QModelIndex ();
}

int Core::rowCount (const QModelIndex& index) const
{
	return index.isValid () ? 0 : History_.size ();
}

void Core::ScheduleWrite ()
{
	if (WriteScheduled_)
		return;

	if (!WritePending_)
		QTimer::singleShot (2000,
				this,
				SLOT (writeSettings ()));
	WriteScheduled_ = true;
}

void Core::writeSettings ()
{
	auto history = History_;
	WriteScheduled_ = false;

	WritePending_ = true;

	auto watcher = new QFutureWatcher<void> ();
	watcher->setFuture (QtConcurrent::run ([history] () -> void
			{
				QSettings settings (QCoreApplication::organizationName (),
						QCoreApplication::applicationName () + "_HistoryHolder");
				settings.beginWriteArray ("History");
				settings.remove ("");
				int i = 0;
				for (const auto& e : history)
				{
					settings.setArrayIndex (i++);
					settings.setValue ("Item", QVariant::fromValue (e));
				}
				settings.endArray ();
			}));

	new Util::OneTimeRunner
	{
		[this, watcher]
		{
			watcher->deleteLater ();

			WritePending_ = false;
			if (WriteScheduled_)
				writeSettings ();
		},
		watcher,
		SIGNAL (finished ()),
		watcher
	};
}

void Core::remove ()
{
	QModelIndexList sis;
	try
	{
		sis = Util::GetSummarySelectedRows (sender ());
	}
	catch (const std::runtime_error& e)
	{
		qWarning () << Q_FUNC_INFO
				<< e.what ();
		return;
	}

	QList<int> rows;
	Q_FOREACH (QModelIndex index, sis)
	{
		if (!index.isValid ())
		{
			qWarning () << Q_FUNC_INFO
				<< "invalid index"
				<< index;
			continue;
		}

		const FindProxy *sm = qobject_cast<const FindProxy*> (index.model ());
		if (!sm)
			continue;

		index = sm->mapToSource (index);

		rows << index.row ();
	}

	qSort (rows.begin (), rows.end (), qGreater<int> ());
	Q_FOREACH (int row, rows)
	{
		beginRemoveRows (QModelIndex (), row, row);
		History_.removeAt (row);
		endRemoveRows ();
	}

	ScheduleWrite ();
}

void Core::handleTasksTreeActivated (const QModelIndex& si)
{
	QModelIndex index = CoreProxy_->MapToSource (si);
	if (!index.isValid ())
	{
		qWarning () << Q_FUNC_INFO
			<< "invalid index"
			<< index;
		return;
	}

	const FindProxy *sm = qobject_cast<const FindProxy*> (index.model ());
	if (!sm)
		return;

	index = sm->mapToSource (index);

	LeechCraft::Entity e = History_.at (index.row ()).Entity_;
	e.Parameters_ |= LeechCraft::FromUserInitiated;
	e.Parameters_ &= ~LeechCraft::IsDownloaded;
	emit gotEntity (e);
}

