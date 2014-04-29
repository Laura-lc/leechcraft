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

#include <QObject>
#include <interfaces/core/icoreproxy.h>
#include <util/utilconfig.h>

class QLineEdit;
class QToolButton;

namespace LeechCraft
{
namespace Util
{
	class LineEditButtonManager;

	/** @brief Provides a "clear text" action for line edits.
	 *
	 * Using this class is as simple as this:
	 * \code
		QLineEdit *edit = ...; // or some QLineEdit-derived class
		new ClearLineEditAddon (proxy, edit); // proxy is the one passed to IInfo::Init()
	   \endcode
	 *
	 * The constructor takes a pointer to the proxy object that is passed
	 * to IInfo::Init() method of the plugin instance object and the
	 * pointer to the line edit where the addon should be installed.
	 *
	 * The line edit takes the ownership of the addon, so there is no
	 * need to keep track of it or explicitly delete it.
	 *
	 * @sa IInfo::Init()
	 */
	class UTIL_API ClearLineEditAddon : public QObject
	{
		Q_OBJECT

		QToolButton *Button_;
		QLineEdit *Edit_;
	public:
		/** @brief Creates the addon and installs it on the given edit.
		 *
		 * Please note that if you want to use this addon with other
		 * buttons on the line edit you may consider using another
		 * constructor overload, taking an additional
		 * LineEditButtonManager parameter.
		 *
		 * @param[in] proxy The proxy passed to IInfo::Init() of the
		 * plugin.
		 * @param[in] edit The line edit to install this addon into. The
		 * edit takes ownership of the addon.
		 */
		ClearLineEditAddon (ICoreProxy_ptr proxy, QLineEdit *edit);

		/** @brief Creates the addon and installs it on the given edit.
		 *
		 * This constructors allows the line edit addon to be used easily
		 * with other buttons installed on the line \em edit using the
		 * passed button \em manager.
		 *
		 * @param[in] proxy The proxy passed to IInfo::Init() of the
		 * plugin.
		 * @param[in] edit The line edit to install this addon into. The
		 * edit takes ownership of the addon.
		 * @param[in] manager The line edit button manager to use instead
		 * of the default internal one.
		 */
		ClearLineEditAddon (ICoreProxy_ptr proxy, QLineEdit *edit, LineEditButtonManager *manager);
	private slots:
		void updateButton (const QString&);
	};
}
}
