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

#pragma once

#include <memory>
#include <QWidget>
#include <interfaces/ihavetabs.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/lmp/ilmpplugin.h>
#include "ui_graffititab.h"

class QFileSystemModel;

namespace Media
{
	struct AudioInfo;
}

namespace LC
{
namespace LMP
{
struct MediaInfo;

namespace Graffiti
{
	class FilesModel;
	class FilesWatcher;
	class CueSplitter;

	class GraffitiTab : public QWidget
					  , public ITabWidget
	{
		Q_OBJECT
		Q_INTERFACES (ITabWidget)

		ICoreProxy_ptr CoreProxy_;
		ILMPProxy_ptr LMPProxy_;

		const TabClassInfo TC_;
		QObject * const Plugin_;

		Ui::GraffitiTab Ui_;

		QFileSystemModel *FSModel_;
		FilesModel *FilesModel_;
		FilesWatcher *FilesWatcher_;

		std::shared_ptr<QToolBar> Toolbar_;
		QAction *Save_;
		QAction *Revert_;
		QAction *RenameFiles_;
		QAction *GetTags_;
		QAction *SplitCue_;

		bool IsChangingCurrent_;
	public:
		GraffitiTab (ICoreProxy_ptr, ILMPProxy_ptr, const TabClassInfo&, QObject*);

		TabClassInfo GetTabClassInfo () const;
		QObject* ParentMultiTabs ();
		void Remove ();
		QToolBar* GetToolBar () const;

		void SetPath (const QString& dir, const QString& filename = {});
	private:
		template<typename T, typename F>
		void UpdateData (const T& newData, F getter);

		void SetupEdits ();
		void SetupViews ();
		void SetupToolbar ();

		void RestorePathHistory ();
		void AddToPathHistory (const QString&);
	private slots:
		void on_Artist__textChanged ();
		void on_Album__textChanged ();
		void on_Title__textChanged ();
		void on_Genre__textChanged ();
		void on_Year__valueChanged ();
		void on_TrackNumber__valueChanged ();

		void on_TrackNumberAutoFill__released ();

		void save ();
		void revert ();
		void renameFiles ();
		void fetchTags ();
		void splitCue ();

		void handleTagsFetched (const QString&);

		void on_DirectoryTree__activated (const QModelIndex&);
		void handlePathLine ();

		void currentFileChanged (const QModelIndex&);
		void handleRereadFiles ();

		void handleIterateFinished ();
		void handleIterateCanceled ();
		void handleScanFinished ();

		void handleCueSplitError (const QString&);
		void handleCueSplitFinished ();
	signals:
		void removeTab (QWidget*);

		void tagsFetchProgress (int, int, QObject*);
		void cueSplitStarted (CueSplitter*);
	};
}
}
}
