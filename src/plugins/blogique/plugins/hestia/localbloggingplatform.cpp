/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
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

#include "localbloggingplatform.h"
#include <QIcon>
#include <QInputDialog>
#include <QSettings>
#include <QtDebug>
#include <QMainWindow>
#include <interfaces/core/irootwindowsmanager.h>
#include <util/xpc/passutils.h>
#include <util/xpc/util.h>
#include "accountconfigurationwidget.h"
#include "core.h"
#include "localblogaccount.h"

namespace LeechCraft
{
namespace Blogique
{
namespace Hestia
{
	LocalBloggingPlatform::LocalBloggingPlatform (QObject *parent)
	: ParentBlogginPlatfromPlugin_ { parent }
	{
	}

	QObject* LocalBloggingPlatform::GetQObject ()
	{
		return this;
	}

	IBloggingPlatform::BloggingPlatfromFeatures LocalBloggingPlatform::GetFeatures () const
	{
		return BPFSupportsRegistration | BPFLocalBlog;
	}

	QObjectList LocalBloggingPlatform::GetRegisteredAccounts ()
	{
		QObjectList result;
		for (auto acc : Accounts_)
			result << acc;
		return result;
	}

	QObject* LocalBloggingPlatform::GetParentBloggingPlatformPlugin () const
	{
		return ParentBlogginPlatfromPlugin_;
	}

	QString LocalBloggingPlatform::GetBloggingPlatformName () const
	{
		return "Local blog";
	}

	QIcon LocalBloggingPlatform::GetBloggingPlatformIcon () const
	{
		return QIcon ();
	}

	QByteArray LocalBloggingPlatform::GetBloggingPlatformID () const
	{
		return "Blogique.Hestia.LocalBlog";
	}

	QList<QWidget*> LocalBloggingPlatform::GetAccountRegistrationWidgets (AccountAddOptions opts,
			const QString& accName)
	{
		return { new AccountConfigurationWidget (0, opts, accName) };
	}

	void LocalBloggingPlatform::RegisterAccount (const QString& name,
			const QList<QWidget*>& widgets)
	{
		auto w = qobject_cast<AccountConfigurationWidget*> (widgets.value (0));
		if (!w)
		{
			qWarning () << Q_FUNC_INFO
					<< "got invalid widgets"
					<< widgets;
			return;
		}

		auto account = new LocalBlogAccount (name, this);
		account->FillSettings (w);

		const QString& path = w->GetAccountBasePath ();
		if (!path.isEmpty ())
		{
			Accounts_ << account;
			saveAccounts ();
			HandleAccountObject (account);
		}
	}

	void LocalBloggingPlatform::RemoveAccount (QObject *account)
	{
		auto acc = qobject_cast<LocalBlogAccount*> (account);
		if (Accounts_.removeAll (acc))
		{
			emit accountRemoved (account);
			account->deleteLater ();
			saveAccounts ();
		}
	}

	QList<QAction*> LocalBloggingPlatform::GetEditorActions () const
	{
		return {};
	}

	QList<InlineTagInserter> LocalBloggingPlatform::GetInlineTagInserters () const
	{
		return {};
	}

	QList<QWidget*> LocalBloggingPlatform::GetBlogiqueSideWidgets () const
	{
		return {};
	}

	void LocalBloggingPlatform::SetPluginProxy (QObject *proxy)
	{
		PluginProxy_ = proxy;
	}

	void LocalBloggingPlatform::Prepare ()
	{
		RestoreAccounts ();
	}

	void LocalBloggingPlatform::Release ()
	{
		saveAccounts ();
	}

	IAdvancedHTMLEditor::CustomTags_t LocalBloggingPlatform::GetCustomTags () const
	{
		return {};
	}

	void LocalBloggingPlatform::RestoreAccounts ()
	{
		QSettings settings (QSettings::IniFormat, QSettings::UserScope,
				QCoreApplication::organizationName (),
				QCoreApplication::applicationName () +
					"_Blogique_Hestia_Accounts");
		int size = settings.beginReadArray ("Accounts");
		for (int i = 0; i < size; ++i)
		{
			settings.setArrayIndex (i);
			QByteArray data = settings.value ("SerializedData").toByteArray ();
			LocalBlogAccount *acc = LocalBlogAccount::Deserialize (data, this);
			if (!acc)
			{
				qWarning () << Q_FUNC_INFO
						<< "unserializable acount"
						<< i;
				continue;
			}
			Accounts_ << acc;
			if (!acc->IsValid ())
				Core::Instance ().SendEntity (Util::MakeNotification ("Blogique",
						tr ("You have invalid account data."),
						Priority::Warning));

			HandleAccountObject (acc);
		}
		settings.endArray ();
	}

	void LocalBloggingPlatform::HandleAccountObject (LocalBlogAccount *account)
	{
		emit accountAdded (account);

		connect (account,
				SIGNAL (accountValidated (bool)),
				this,
				SLOT (handleAccountValidated (bool)));
		connect (account,
				SIGNAL (accountSettingsChanged ()),
				this,
				SLOT (saveAccounts ()));

		account->Init ();
	}

	void LocalBloggingPlatform::saveAccounts ()
	{
		QSettings settings (QSettings::IniFormat, QSettings::UserScope,
				QCoreApplication::organizationName (),
				QCoreApplication::applicationName () +
					"_Blogique_Hestia_Accounts");
		settings.beginWriteArray ("Accounts");
		for (int i = 0, size = Accounts_.size (); i < size; ++i)
		{
			settings.setArrayIndex (i);
			settings.setValue ("SerializedData",
					Accounts_.at (i)->Serialize ());
		}
		settings.endArray ();
	}

	void LocalBloggingPlatform::handleAccountValidated (bool valid)
	{
		IAccount *acc = qobject_cast<IAccount*> (sender ());
		if (!acc)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "is not an IAccount";;
			return;
		}

		emit accountValidated (acc->GetQObject (), valid);
	}

}
}
}
