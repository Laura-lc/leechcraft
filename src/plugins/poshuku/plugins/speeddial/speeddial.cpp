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

#include "speeddial.h"
#include <QIcon>
#include <util/util.h>
#include <interfaces/poshuku/iproxyobject.h>
#include <interfaces/poshuku/ibrowserwidget.h>
#include "viewhandler.h"
#include "imagecache.h"
#include "customsitesmanager.h"
#include "xmlsettingsmanager.h"

namespace LC::Poshuku::SpeedDial
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("poshuku_speeddial");

		Cache_ = new ImageCache { proxy };

		qRegisterMetaType<AddrList_t> ("LC::Poshuku::SpeedDial::AddrList_t");
		qRegisterMetaTypeStreamOperators<AddrList_t> ();

		XSD_ = std::make_shared<Util::XmlSettingsDialog> ();
		XSD_->RegisterObject (&XmlSettingsManager::Instance (), "poshukuspeeddialsettings.xml");

		CustomSites_ = new CustomSitesManager;
		XSD_->SetDataSource ("SitesView", CustomSites_->GetModel ());
	}

	void Plugin::SecondInit ()
	{
	}

	void Plugin::Release ()
	{
		delete Cache_;
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Poshuku.SpeedDial";
	}

	QString Plugin::GetName () const
	{
		return "Poshuku SpeedDial";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Adds a special speed dial page.");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		return { "org.LeechCraft.Poshuku.Plugins/1.0" };
	}

	void Plugin::initPlugin (QObject *object)
	{
		PoshukuProxy_ = qobject_cast<IProxyObject*> (object);
	}

	void Plugin::hookBrowserWidgetInitialized (LC::IHookProxy_ptr,
			QObject *browserWidget)
	{
		new ViewHandler { qobject_cast<IBrowserWidget*> (browserWidget), Cache_, CustomSites_, PoshukuProxy_ };
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}
}

LC_EXPORT_PLUGIN (leechcraft_poshuku_speeddial, LC::Poshuku::SpeedDial::Plugin);
