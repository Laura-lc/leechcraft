/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "actionsmodel.h"
#include <QAction>
#include <QtDebug>

namespace LeechCraft
{
namespace AdvancedNotifications
{
	ActionsModel::ActionsModel (QObject *parent)
	: QStandardItemModel (parent)
	{
		QHash<int, QByteArray> roleNames;
		roleNames [Roles::IconName] = "iconName";
		roleNames [Roles::IsActionChecked] = "isActionChecked";
		setRoleNames (roleNames);
	}

	void ActionsModel::AddAction (QAction *action)
	{
		Actions_ << action;

		auto item = new QStandardItem;
		item->setData (action->property ("ActionIcon"), Roles::IconName);
		item->setData (action->isChecked (), Roles::IsActionChecked);
		appendRow (item);

		connect (action,
				SIGNAL (toggled (bool)),
				this,
				SLOT (handleActionToggled (bool)));
	}

	void ActionsModel::triggerAction (int index)
	{
		Actions_.at (index)->trigger ();
	}

	void ActionsModel::handleActionToggled (bool checked)
	{
		auto action = static_cast<QAction*> (sender ());
		const auto pos = Actions_.indexOf (action);
		if (pos == -1)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "not found in"
					<< Actions_;
			return;
		}

		item (pos)->setData (checked, Roles::IsActionChecked);

		const auto& off = action->property ("ActionIconOff").toString ();
		if (!off.isEmpty ())
		{
			const auto& on = action->property ("ActionIcon").toString ();
			item (pos)->setData (checked ? on : off, Roles::IconName);
		}
	}
}
}
