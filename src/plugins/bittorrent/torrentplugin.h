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

#ifndef PLUGINS_BITTORRENT_TORRENTPLUIGN_H
#define PLUGINS_BITTORRENT_TORRENTPLUIGN_H
#define PLUGINS_BITTORRENT_WIN32_LEAN_AND_MEAN
#include <memory>
#include <deque>
#include <QMainWindow>
#include <interfaces/iinfo.h>
#include <interfaces/idownload.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/ijobholder.h>
#include <interfaces/iimportexport.h>
#include <interfaces/itaggablejobs.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/ihaveshortcuts.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/istartupwizard.h>
#include <interfaces/iactionsexporter.h>
#include <interfaces/ihavediaginfo.h>
#include <util/tags/tagscompleter.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "tabwidget.h"
#include "torrentinfo.h"

class QTimer;
class QToolBar;
class QComboBox;
class QTabWidget;
class QTranslator;
class QSortFilterProxyModel;

namespace LeechCraft
{
	namespace Plugins
	{
		namespace BitTorrent
		{
			class AddTorrent;
			class RepresentationModel;
			class SpeedSelectorAction;
			class TorrentTab;

			class TorrentPlugin : public QObject
								, public IInfo
								, public IDownload
								, public IEntityHandler
								, public IJobHolder
								, public IImportExport
								, public ITaggableJobs
								, public IHaveSettings
								, public IHaveShortcuts
								, public IHaveTabs
								, public IStartupWizard
								, public IActionsExporter
								, public IHaveDiagInfo
			{
				Q_OBJECT

				Q_INTERFACES (IInfo
						IDownload
						IEntityHandler
						IJobHolder
						IImportExport
						ITaggableJobs
						IHaveSettings
						IHaveShortcuts
						IHaveTabs
						IStartupWizard
						IActionsExporter
						IHaveDiagInfo)

				std::shared_ptr<LeechCraft::Util::XmlSettingsDialog> XmlSettingsDialog_;
				std::auto_ptr<AddTorrent> AddTorrentDialog_;
				std::auto_ptr<QTimer> OverallStatsUpdateTimer_;
				std::auto_ptr<QTime> LastPeersUpdate_;
				bool TorrentSelectionChanged_;
				std::unique_ptr<LeechCraft::Util::TagsCompleter> TagsAddDiaCompleter_;
				std::unique_ptr<TabWidget> TabWidget_;
				std::unique_ptr<QToolBar> Toolbar_;
				std::unique_ptr<QAction> OpenTorrent_,
					RemoveTorrent_,
					Resume_,
					Stop_,
					CreateTorrent_,
					MoveUp_,
					MoveDown_,
					MoveToTop_,
					MoveToBottom_,
					ForceReannounce_,
					ForceRecheck_,
					OpenMultipleTorrents_,
					IPFilter_,
					MoveFiles_,
					ChangeTrackers_,
					MakeMagnetLink_;

				SpeedSelectorAction *DownSelectorAction_,
						*UpSelectorAction_;

				enum
				{
					EAOpenTorrent_,
					EAChangeTrackers_,
					EACreateTorrent_,
					EAOpenMultipleTorrents_,
					EARemoveTorrent_,
					EAResume_,
					EAStop_,
					EAMoveUp_,
					EAMoveDown_,
					EAMoveToTop_,
					EAMoveToBottom_,
					EAForceReannounce_,
					EAForceRecheck_,
					EAMoveFiles_,
					EAImport_,
					EAExport_,
					EAMakeMagnetLink_,
					EAIPFilter_
				};

				TabClassInfo TabTC_;
				TorrentTab *TorrentTab_;

				QSortFilterProxyModel *ReprProxy_;
			public:
				// IInfo
				void Init (ICoreProxy_ptr);
				void SecondInit ();
				QByteArray GetUniqueID () const;
				QString GetName () const;
				QString GetInfo () const;
				QStringList Provides () const;
				void Release ();
				QIcon GetIcon () const;

				// IDownload
				qint64 GetDownloadSpeed () const;
				qint64 GetUploadSpeed () const;
				void StartAll ();
				void StopAll ();
				EntityTestHandleResult CouldDownload (const LeechCraft::Entity&) const;
				int AddJob (LeechCraft::Entity);
				void KillTask (int);

				// IEntityHandler
				EntityTestHandleResult CouldHandle (const LeechCraft::Entity&) const;
				void Handle (LeechCraft::Entity);

				// IJobHolder
				QAbstractItemModel* GetRepresentation () const;

				// IImportExport
				void ImportSettings (const QByteArray&);
				void ImportData (const QByteArray&);
				QByteArray ExportSettings () const;
				QByteArray ExportData () const;

				// ITaggableJobs
				void SetTags (int, const QStringList&);

				// IHaveSettings
				Util::XmlSettingsDialog_ptr GetSettingsDialog () const;

				// IHaveShortcuts
				void SetShortcut (const QString&, const QKeySequences_t&);
				QMap<QString, ActionInfo> GetActionInfo () const;

				// IHaveTabs
				TabClasses_t GetTabClasses () const;
				void TabOpenRequested (const QByteArray&);

				// IStartupWizard
				QList<QWizardPage*> GetWizardPages () const;

				// IToolBarEmbedder
				QList<QAction*> GetActions (ActionsEmbedPlace) const;

				// IHaveDiagInfo
				QString GetDiagInfoString () const;
			private slots:
				void on_OpenTorrent__triggered ();
				void on_OpenMultipleTorrents__triggered ();
				void on_IPFilter__triggered ();
				void on_CreateTorrent__triggered ();
				void on_RemoveTorrent__triggered ();
				void on_Resume__triggered ();
				void on_Stop__triggered ();
				void on_MoveUp__triggered ();
				void on_MoveDown__triggered ();
				void on_MoveToTop__triggered ();
				void on_MoveToBottom__triggered ();
				void on_ForceReannounce__triggered ();
				void on_ForceRecheck__triggered ();
				void on_ChangeTrackers__triggered ();
				void on_MoveFiles__triggered ();
				void on_MakeMagnetLink__triggered ();
				void handleFastSpeedComboboxes ();
				void setActionsEnabled ();
				void showError (QString);
				void handleTasksTreeSelectionCurrentRowChanged (const QModelIndex&, const QModelIndex&);
			private:
				void SetupCore ();
				void SetupStuff ();
				void SetupActions ();
			signals:
				void gotEntity (const LeechCraft::Entity&);
				void jobFinished (int);
				void jobRemoved (int);

				void addNewTab (const QString&, QWidget*);
				void changeTabIcon (QWidget*, const QIcon&);
				void changeTabName (QWidget*, const QString&);
				void raiseTab (QWidget*);
				void removeTab (QWidget*);
				void statusBarChanged (QWidget*, const QString&);

				void gotActions (QList<QAction*>, LeechCraft::ActionsEmbedPlace);
			};
		};
	};
};

#endif

