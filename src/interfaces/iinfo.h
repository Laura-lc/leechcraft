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
#include <QString>
#include <QStringList>
#include <QtPlugin>
#include "structures.h"

class ICoreProxy;
typedef std::shared_ptr<ICoreProxy> ICoreProxy_ptr;

/** @brief Required interface for every plugin.
 *
 * This interface is a base for all plugins loadable by LeechCraft. If a
 * plugin doesn't provide this interface (qobject_cast<IInfo*> to it
 * fails), it would be considered as a malformed one and would be
 * unloaded.
 *
 * The initialization of plugins is split into two stages: Init() and
 * SecondInit().
 *
 * In the first one plugins ought to initialize their data structures,
 * allocate memory and perform other initializations that don't depend
 * depend on other plugins. After the first stage (after Init()) the
 * plugin should be in a usable state: no call shall fail because
 * required data hasn't been initialized.
 *
 * In the second stage, SecondInit(), plugins can fill the data that
 * depends on other plugins. For example, in SecondInit() the Tab++
 * plugin, which shows a fancy tab tree, queries the MainWindow about
 * existing tabs and about plugins that can open tabs and connects to
 * their respective signals.
 *
 * So, as the rule of thumb, if the action required to initialize your
 * plugin depends on others - move it to SecondInit(), leaving in Init()
 * only basic initialization/allocation stuff like allocation memory for
 * the objects.
 */
class Q_DECL_EXPORT IInfo
{
public:
	/** @brief Initializes the plugin.
	 *
	 * Init is called by the LeechCraft when it has finished
	 * initializing its core and is ready to initialize plugins.
	 * Generally, all initialization code should be placed into this
	 * method instead of plugin's instance object's constructor.
	 *
	 * After this call plugin should be in a usable state. That means
	 * that all data members should be initialized, callable from other
	 * plugins etc. That also means that in this function you can't rely
	 * on other plugins being initialized.
	 *
	 * @param[in] proxy The pointer to proxy to LeechCraft.
	 *
	 * @sa Release
	 * @sa SecondInit
	 */
	virtual void Init (ICoreProxy_ptr proxy) = 0;

	/** @brief Performs second stage of initialization.
	 *
	 * This function is called when all the plugins are initialized by
	 * IInfo::Init() and may now communicate with others with no fear of
	 * getting init-order bugs.
	 *
	 * @sa Init
	 */
	virtual void SecondInit () = 0;

	/** @brief Returns the unique ID of the plugin.
	 *
	 * The ID should never change, event between different versions of
	 * the plugin and between renames of the plugin. It should be unique
	 * among all other plugins, thus the Vendor.AppName form is
	 * suggested. For example, Poshuku Browser plugin would return an ID
	 * like "org.LeechCraft.Poshuku", and Poshuku CleanWeb, which is
	 * Poshuku plugin, would return "org.LeechCraft.Poshuku.CleanWeb".
	 *
	 * The ID is allowed to consist of upper- and lowercase latin
	 * letters, numbers, dots¸ plus and minus sign.
	 *
	 * @return Unique and persistent ID of the plugin.
	 */
	virtual QByteArray GetUniqueID () const = 0;

	/** @brief Returns the name of the plugin.
	 *
	 * This name is used only for the UI, internal communication is done
	 * through pointers to QObjects representing plugin instance
	 * objects.
	 *
	 * @note This function should be able to work before Init() is
	 * called.
	 *
	 * @sa GetInfo
	 * @sa GetIcon
	 */
	virtual QString GetName () const = 0;

	/** @brief Returns the information string about the plugin.
	 *
	 * Information string is only used to describe the plugin to the
	 * user.
	 *
	 * @note This function should be able to work before Init() is
	 * called.
	 *
	 * @sa GetName
	 * @sa GetInfo
	 */
	virtual QString GetInfo () const = 0;

	/** @brief Returns the list of provided features.
	 *
	 * The return value is used by LeechCraft to calculate the
	 * dependencies between plugins and link them together by passing
	 * object pointers to SetProvider().
	 *
	 * The default implementation returns an empty list.
	 *
	 * @note This function should be able to work before Init() is
	 * called.
	 *
	 * @return List of provided features.
	 *
	 * @sa Needs
	 * @sa Uses
	 * @sa SetProvider
	 */
	virtual QStringList Provides () const
	{
		return QStringList ();
	}

	/** @brief Returns the list of needed features.
	 *
	 * The return value is used by LeechCraft to calculate the
	 * dependencies between plugins and link them together by passing
	 * object pointers to SetProvider().
	 *
	 * If not all requirements are satisfied, LeechCraft would mark the
	 * plugin as unusable and would not make it active or use its
	 * features returned by Provides() in dependency calculations. So,
	 * the returned list should mention those features that plugin can't
	 * live without and would not work at all.
	 *
	 * The default implementation returns an empty list.
	 *
	 * @note This function should be able to work before Init() is
	 * called.
	 *
	 * @return List of needed features.
	 *
	 * @sa Provides
	 * @sa Uses
	 * @sa SetProvider
	 */
	virtual QStringList Needs () const
	{
		return QStringList ();
	}

	/** @brief Returns the list of used features.
	 *
	 * The return value is used by LeechCraft to calculate the
	 * dependencies between plugins and link them together by passing
	 * object pointers to SetProvider().
	 *
	 * If not all requirements are satisfied, LeechCraft would still
	 * consider the plugin as usable and functional, make it available
	 * to user and use the features returned by Provides() in dependency
	 * calculations. So, the return list should mention those features
	 * that plugin can use but which are not required to start it and do
	 * some basic work.
	 *
	 * @note This function should be able to work before Init() is
	 * called.
	 *
	 * @return List of used features.
	 *
	 * @sa Provides
	 * @sa Uses
	 * @sa SetProvider
	 */
	virtual QStringList Uses () const
	{
		return QStringList ();
	}

	/** @brief Sets the provider plugin for a given feature.
	 *
	 * This function is called by LeechCraft after dependency
	 * calculations to inform plugin about other plugins which provide
	 * the required features.
	 *
	 * @note This function should be able to work before Init() is
	 * called.
	 *
	 * @param[in] object Pointer to plugin instance of feature provider.
	 * @param[in] feature The feature which this object provides.
	 *
	 * @sa Provides
	 * @sa Needs
	 * @sa Uses
	 */
	virtual void SetProvider (QObject* object,
			const QString& feature)
	{
		Q_UNUSED (object);
		Q_UNUSED (feature);
	}

	/** @brief Destroys the plugin.
	 *
	 * This function is called to notify that the plugin would be
	 * unloaded soon - either the application is closing down or the
	 * plugin is unloaded for some reason. Plugin should free its
	 * resources and especially all the GUI stuff in this function
	 * instead of plugin instance's destructor.
	 *
	 * @sa Init
	 */
	virtual void Release () = 0;

	/** @brief Returns the plugin icon.
	 *
	 * The icon is used only in GUI stuff.
	 *
	 * @note This function should be able to work before Init() is
	 * called.
	 *
	 * @return Icon object.
	 *
	 * @sa GetName
	 * @sa GetInfo
	 */
	virtual QIcon GetIcon () const = 0;

	/** @brief Virtual destructor.
	 */
	virtual ~IInfo () {}

	virtual void gotEntity (const LC::Entity& entity)
	{
		Q_UNUSED (entity);
	}
};

Q_DECLARE_INTERFACE (IInfo, "org.Deviant.LeechCraft.IInfo/1.0")

#define CURRENT_API_LEVEL 20

#define LC_EXPORT_PLUGIN(name,file) \
	extern "C"\
	{\
		Q_DECL_EXPORT quint64 GetAPILevels ()\
		{\
			return CURRENT_API_LEVEL;\
		}\
	}

#define LC_PLUGIN_METADATA(id) Q_PLUGIN_METADATA (IID id)
