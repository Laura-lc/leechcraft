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

#include "viewsettingsmanager.h"
#include <QCoreApplication>
#include <xmlsettingsdialog/basesettingsmanager.h>
#include "viewmanager.h"
#include "sb2util.h"

namespace LeechCraft
{
namespace SB2
{
	namespace
	{
		class XmlViewSettingsManager : public Util::BaseSettingsManager
		{
			ViewManager * const ViewMgr_;

			mutable std::shared_ptr<QSettings> SettingsInstance_;
		public:
			XmlViewSettingsManager (ViewManager*);
		protected:
			Settings_ptr GetSettings () const;

			QSettings* BeginSettings () const;
			void EndSettings (QSettings*) const;
		};

		XmlViewSettingsManager::XmlViewSettingsManager (ViewManager *view)
		: BaseSettingsManager ()
		, ViewMgr_ (view)
		{
			Util::BaseSettingsManager::Init ();
		}

		Settings_ptr XmlViewSettingsManager::GetSettings () const
		{
			return ViewMgr_->GetSettings ();
		}

		QSettings* XmlViewSettingsManager::BeginSettings () const
		{
			return nullptr;
		}

		void XmlViewSettingsManager::EndSettings (QSettings*) const
		{
		}
	}

	ViewSettingsManager::ViewSettingsManager (ViewManager *mgr)
	: QObject (mgr)
	, ViewMgr_ (mgr)
	, XSM_ (new XmlViewSettingsManager (mgr))
	, XSD_ (new Util::XmlSettingsDialog)
	{
		XSD_->RegisterObject (XSM_.get (), "sb2panelsettings.xml");
	}

	void ViewSettingsManager::ShowSettings ()
	{
		OpenSettingsDialog (XSD_.get (), tr ("SB2 panel settings"));
	}

	const std::shared_ptr<Util::BaseSettingsManager>& ViewSettingsManager::GetXSM () const
	{
		return XSM_;
	}
}
}
