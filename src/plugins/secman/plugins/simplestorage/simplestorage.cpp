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

#include "simplestorage.h"
#include <QSettings>
#include <QIcon>
#include <QCoreApplication>
#include <util/sll/prelude.h>

namespace LeechCraft
{
namespace SecMan
{
namespace SimpleStorage
{
	void Plugin::Init (ICoreProxy_ptr)
	{
		Storage_ = std::make_shared<QSettings> (QSettings::IniFormat,
				QSettings::UserScope,
				QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_SecMan_SimpleStorage");
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.SecMan.StoragePlugins.SimpleStorage";
	}

	void Plugin::Release ()
	{
	}

	QString Plugin::GetName () const
	{
		return "SimpleStorage";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Simple unencrypted storage plugin for SecMan");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/secman/simplestorage/resources/images/simplestorage.svg");
		return icon;
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		return { "org.LeechCraft.SecMan.StoragePlugins/1.0" };
	}

	IStoragePlugin::StorageTypes Plugin::GetStorageTypes () const
	{
		return STInsecure;
	}

	QList<QByteArray> Plugin::ListKeys (IStoragePlugin::StorageType)
	{
		auto keys = Storage_->allKeys ();
		qDebug () << Q_FUNC_INFO << keys;
		return Util::Map (keys, [] (auto&& str) { return str.toUtf8 (); });
	}

	void Plugin::Save (const QByteArray& key, const QVariant& value,
			IStoragePlugin::StorageType)
	{
		Storage_->setValue (key, value);
	}

	QVariant Plugin::Load (const QByteArray& key, IStoragePlugin::StorageType)
	{
		return Storage_->value (key);
	}
}
}
}

LC_EXPORT_PLUGIN (leechcraft_secman_simplestorage, LeechCraft::SecMan::SimpleStorage::Plugin);
