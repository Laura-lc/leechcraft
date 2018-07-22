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

#include "basesettingsmanager.h"
#include <QtDebug>
#include <QTimer>
#include <util/sll/visitor.h>
#include "settingsthreadmanager.h"

namespace LeechCraft
{
namespace Util
{
	BaseSettingsManager::BaseSettingsManager (bool readAllKeys, QObject *parent)
	: QObject (parent)
	, ReadAllKeys_ (readAllKeys)
	{
	}

	void BaseSettingsManager::Init ()
	{
		IsInitializing_ = true;

		auto settings = GetSettings ();
		const auto& properties = ReadAllKeys_ ?
				settings->allKeys () :
				settings->childKeys ();
		for (const auto& prop : properties)
			setProperty (PROP2CHAR (prop), settings->value (prop));

		IsInitializing_ = false;
	}

	void BaseSettingsManager::Release ()
	{
		auto settings = GetSettings ();

		for (const auto& dProp : dynamicPropertyNames ())
			settings->setValue (QString::fromUtf8 (dProp), property (dProp.constData ()));
	}

	void BaseSettingsManager::RegisterObject (const QByteArray& propName,
			QObject *object, const QByteArray& funcName, EventFlags flags)
	{
		if (flags & EventFlag::Apply)
			ApplyProps_ [propName].append ({ object, funcName });
		if (flags & EventFlag::Select)
			SelectProps_ [propName].append ({ object, funcName });

		connect (object,
				SIGNAL (destroyed (QObject*)),
				this,
				SLOT (scheduleCleanup ()),
				Qt::UniqueConnection);
	}

	void BaseSettingsManager::RegisterObject (const QByteArray& propName,
			QObject *object, const VariantHandler_f& func, EventFlags flags)
	{
		if (flags & EventFlag::Apply)
			ApplyProps_ [propName].append ({ object, func });
		if (flags & EventFlag::Select)
			SelectProps_ [propName].append ({ object, func });

		connect (object,
				SIGNAL (destroyed (QObject*)),
				this,
				SLOT (scheduleCleanup ()),
				Qt::UniqueConnection);
	}

	void BaseSettingsManager::RegisterObject (const QList<QByteArray>& propNames,
			QObject* object, const QByteArray& funcName, EventFlags flags)
	{
		for (const auto& prop : propNames)
			RegisterObject (prop, object, funcName, flags);
	}

	QVariant BaseSettingsManager::Property (const QString& propName, const QVariant& def)
	{
		auto result = property (PROP2CHAR (propName));
		if (!result.isValid ())
		{
			result = def;
			setProperty (PROP2CHAR (propName), def);
		}

		return result;
	}

	void BaseSettingsManager::SetRawValue (const QString& path, const QVariant& val)
	{
		GetSettings ()->setValue (path, val);
	}

	QVariant BaseSettingsManager::GetRawValue (const QString& path, const QVariant& def) const
	{
		return GetSettings ()->value (path, def);
	}

	void BaseSettingsManager::ShowSettingsPage (const QString& optionName)
	{
		emit showPageRequested (this, optionName);
	}

	void BaseSettingsManager::OptionSelected (const QByteArray& prop, const QVariant& val)
	{
		for (const auto& object : SelectProps_.value (prop))
		{
			if (!object.first)
				continue;

			Visit (object.second,
					[&val] (const VariantHandler_f& func) { func (val); },
					[&] (const QByteArray& methodName)
					{
						if (!QMetaObject::invokeMethod (object.first,
								methodName,
								Q_ARG (QVariant, val)))
							qWarning () << Q_FUNC_INFO
									<< "could not find method in the metaobject"
									<< prop
									<< object.first
									<< methodName;
					});
		}
	}

	std::shared_ptr<void> BaseSettingsManager::EnterInitMode ()
	{
		if (IsInitializing_)
			return {};

		IsInitializing_ = true;
		return std::shared_ptr<void> (nullptr, [this] (void*) { IsInitializing_ = false; });
	}

	bool BaseSettingsManager::event (QEvent *e)
	{
		if (e->type () != QEvent::DynamicPropertyChange)
			return false;

		auto event = static_cast<QDynamicPropertyChangeEvent*> (e);

		const QByteArray& name = event->propertyName ();
		const auto& propName = QString::fromUtf8 (name);
		const auto& propValue = property (name);

		if (!IsInitializing_)
			SettingsThreadManager::Instance ().Add (this,
					propName, propValue);

		PropertyChanged (propName, propValue);

		for (const auto& object : ApplyProps_.value (name))
		{
			if (!object.first)
				continue;

			Visit (object.second,
					[&propValue] (const VariantHandler_f& func) { func (propValue); },
					[&] (const QByteArray& methodName)
					{
						if (!QMetaObject::invokeMethod (object.first, methodName))
							qWarning () << Q_FUNC_INFO
									<< "could not find method in the metaobject"
									<< name
									<< object.first
									<< methodName;
					});
		}

		event->accept ();
		return true;
	}

	void BaseSettingsManager::PropertyChanged (const QString&, const QVariant&)
	{
	}

	Settings_ptr BaseSettingsManager::GetSettings () const
	{
		return Settings_ptr (BeginSettings (),
				[this] (QSettings *settings)
				{
					EndSettings (settings);
					delete settings;
				});
	}

	void BaseSettingsManager::scheduleCleanup ()
	{
		if (CleanupScheduled_)
			return;

		CleanupScheduled_ = true;
		QTimer::singleShot (100,
				this,
				SLOT (cleanupObjects ()));
	}

	void BaseSettingsManager::cleanupObjects ()
	{
		CleanupScheduled_= false;

		auto cleanupMap = [] (Properties2Object_t& map)
		{
			for (auto it = map.begin (); it != map.end (); )
			{
				auto& subscribers = it.value ();
				for (auto lit = subscribers.begin (); lit != subscribers.end (); )
				{
					if (!lit->first)
						lit = subscribers.erase (lit);
					else
						++lit;
				}

				if (subscribers.isEmpty ())
					it = map.erase (it);
				else
					++it;
			}
		};

		cleanupMap (ApplyProps_);
		cleanupMap (SelectProps_);
	}
}
}
