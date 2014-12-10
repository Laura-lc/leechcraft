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

#ifndef CORE_H
#define CORE_H
#include <memory>
#include <QObject>
#include <QString>
#include <QPair>
#include <QTimer>
#include <interfaces/iinfo.h>
#include <interfaces/structures.h>
#include "pluginmanager.h"

class QAbstractProxyModel;
class QAction;
class IDownload;
class IShortcutProxy;
class QToolBar;
class QNetworkAccessManager;

namespace LeechCraft
{
	class RootWindowsManager;
	class MainWindow;
	class NewTabMenuManager;
	class StorageBackend;
	class LocalSocketHandler;
	class CoreInstanceObject;
	class DockManager;

	/** Contains all the plugins' models, maps from end-user's tree view
	 * to plugins' models and much more.
	 */
	class Core : public QObject
	{
		Q_OBJECT

		PluginManager *PluginManager_;
		std::shared_ptr<QNetworkAccessManager> NetworkAccessManager_;
		std::shared_ptr<StorageBackend> StorageBackend_;
		std::shared_ptr<LocalSocketHandler> LocalSocketHandler_;
		std::shared_ptr<NewTabMenuManager> NewTabMenuManager_;
		std::shared_ptr<CoreInstanceObject> CoreInstanceObject_;
		std::shared_ptr<RootWindowsManager> RootWindowsManager_;
		DockManager *DM_;
		QList<Entity> QueuedEntities_;
		bool IsShuttingDown_;

		Core ();
	public:
		enum FilterType
		{
			FTFixedString
			, FTWildcard
			, FTRegexp
			, FTTags
		};

		virtual ~Core ();
		static Core& Instance ();
		void Release ();

		bool IsShuttingDown () const;

		/** Returns the dock manager over the main window.
		 */
		DockManager* GetDockManager () const;

		/** Returns the pointer to the app-wide shortcut proxy.
		 */
		IShortcutProxy* GetShortcutProxy () const;

		/** Returns all plugins that implement IHaveSettings as
		 * QObjectList.
		 *
		 * @return List of objects.
		 */
		QObjectList GetSettables () const;

		/** Returns all the actions from plugins that implement
		 * IToolBarEmbedder.
		 *
		 * @return List of actions.
		 */
		QList<QList<QAction*>> GetActions2Embed () const;

		/** Returns the model which manages the plugins, displays
		 * various info about them like name, description, icon and
		 * allows one to switch them off.
		 *
		 * For example, this model is used in the Plugin Manager page
		 * in the settings.
		 */
		QAbstractItemModel* GetPluginsModel () const;

		/** Returns pointer to the app-wide Plugin Manager.
		 *
		 * Note that plugin manager is only initialized after the call
		 * to DelayedInit().
		 */
		PluginManager* GetPluginManager () const;

		/** Returns pointer to the storage backend of the Core.
		 */
		StorageBackend* GetStorageBackend () const;

		/** @brief Returns the pointer to the core instance.
		 *
		 * The core instance object is inserted into the plugin manager
		 * to simulate the plugin-like behavior of the Core: some of its
		 * functions are better and shorter expressed as if we consider
		 * the Core to be a plugin for itself.
		 */
		CoreInstanceObject* GetCoreInstanceObject () const;

		/** Performs the initialization of systems that are dependant
		 * on others, like the main window or the Tab Contents Manager.
		 */
		void DelayedInit ();

		/** Tries to add a task from the Add Task Dialog.
		 */
		void TryToAddJob (QString);

		/** Returns true if both indexes belong to the same model. If
		 * both indexes are invalid, true is returned.
		 *
		 * The passed indexes shouldn't be mapped to source from filter
		 * model or merge model, Core will do it itself.
		 *
		 * @param[in] i1 The first index.
		 * @param[in] i2 The second index.
		 * @return Whether the indexes belong to the same model.
		 */
		bool SameModel (const QModelIndex& i1, const QModelIndex& i2) const;

		/** Calculates and returns current upload/download speeds.
		 */
		QPair<qint64, qint64> GetSpeeds () const;

		/** Returns the app-wide network access manager.
		 */
		QNetworkAccessManager* GetNetworkAccessManager () const;

		RootWindowsManager* GetRootWindowsManager () const;

		/** Maps given index from a model obtained from GetTasksModel()
		 * to the index provided by a corresponding plugin's model.
		 */
		QModelIndex MapToSource (const QModelIndex& index) const;

		/** Returns the app-wide new tab menu manager.
		 */
		NewTabMenuManager* GetNewTabMenuManager () const;

		/** Sets up connections for the given object which is expected
		 * to be a plugin instance.
		 */
		void Setup (QObject *object);

		void PostSecondInit (QObject *object);
	public slots:
		/* Dispatcher of button clicks in the Settings Dialog (of the
		 * settings of type 'pushbutton').
		 */
		void handleSettingClicked (const QString&);

		/** Handles the entity which could be anything - path to a file,
		 * link, contents of a .torrent file etc. If the entity is a
		 * string, this parameter is considered to be an UTF-8
		 * representation of it.
		 *
		 * If id is not null and the job is handled by a downloader,
		 * the return value of IDownloader::AddJob() is assigned to *id.
		 * The same is with the provider.
		 *
		 * @param[in] entity Entity.
		 * @param[out] id The ID of the job if applicable.
		 * @param[out] provider The provider that downloads this job.
		 * @return True if the entity was actually handled.
		 */
		bool handleGotEntity (LeechCraft::Entity entity,
				int *id = 0, QObject **provider = 0);
	private slots:
		/** Returns whether the given entity could be handlerd.
		 *
		 * @param[in] entity The download entity to be checked.
		 * @param[out] could Whether the given entity could be checked.
		 */
		void handleCouldHandle (const LeechCraft::Entity& entity,
				bool *could);

		void queueEntity (const LeechCraft::Entity&);

		void pullEntityQueue ();

		void handlePluginLoadErrors ();
	private:
		/** Initializes IInfo's signals of the object.
		 */
		void InitDynamicSignals (const QObject *object);

		/** Initializes the object as a IMultiTabs. The object is assumed
		 * to be a valid IMultiTabs*.
		 */
		void InitMultiTab (const QObject *object);
	signals:
		/** Notifies the user about an error by a pop-up message box.
		 */
		void error (QString error) const;
	};
};



#endif

