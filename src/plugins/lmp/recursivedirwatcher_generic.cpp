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

#include "recursivedirwatcher_generic.h"
#include <QFileSystemWatcher>
#include <QStringList>
#include <QDir>
#include <QFutureWatcher>
#include <QtConcurrentRun>

namespace LeechCraft
{
namespace LMP
{
	namespace
	{
		QStringList CollectSubdirs (const QString& path)
		{
			QDir dir (path);
			const auto& list = dir.entryList (QDir::Dirs | QDir::NoDotAndDotDot);

			QStringList result (path);
			std::for_each (list.begin (), list.end (),
					[&dir, &result] (decltype (list.front ()) item)
						{ result += CollectSubdirs (dir.filePath (item)); });
			return result;
		}
	}

	RecursiveDirWatcherImpl::RecursiveDirWatcherImpl (QObject *parent)
	: QObject { parent }
	, Watcher_ { new QFileSystemWatcher { this } }
	{
		connect (Watcher_,
				SIGNAL (directoryChanged (QString)),
				this,
				SIGNAL (directoryChanged (QString)));
	}

	void RecursiveDirWatcherImpl::AddRoot (const QString& root)
	{
		qDebug () << Q_FUNC_INFO << "scanning" << root;
		auto watcher = new QFutureWatcher<QStringList> ();
		watcher->setProperty ("Path", root);
		connect (watcher,
				SIGNAL (finished ()),
				this,
				SLOT (handleSubdirsCollected ()));

		watcher->setFuture (QtConcurrent::run (CollectSubdirs, root));
	}

	void RecursiveDirWatcherImpl::RemoveRoot (const QString& root)
	{
		Watcher_->removePaths (Dir2Subdirs_.take (root));
	}

	void RecursiveDirWatcherImpl::handleSubdirsCollected ()
	{
		auto watcher = dynamic_cast<QFutureWatcher<QStringList>*> (sender ());
		if (!watcher)
			return;

		watcher->deleteLater ();

		const auto& paths = watcher->result ();
		const auto& path = watcher->property ("Path").toString ();
		Dir2Subdirs_ [path] = paths;
		Watcher_->addPaths (paths);
	}
}
}
