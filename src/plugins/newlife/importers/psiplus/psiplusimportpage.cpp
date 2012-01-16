/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2012  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "psiplusimportpage.h"
#include <QDir>
#include <QStandardItemModel>
#include <QDomDocument>
#include <QtDebug>
#include <util/util.h>

namespace LeechCraft
{
namespace NewLife
{
namespace Importers
{
	PsiPlusImportPage::PsiPlusImportPage (QWidget *parent)
	: QWizardPage (parent)
	, AccountsModel_ (new QStandardItemModel (this))
	{
		Ui_.setupUi (this);
		Ui_.AccountsTree_->setModel (AccountsModel_);
	}

	bool PsiPlusImportPage::isComplete () const
	{
		return true;
	}

	int PsiPlusImportPage::nextId () const
	{
		return -1;
	}

	void PsiPlusImportPage::initializePage ()
	{
		connect (wizard (),
				SIGNAL (accepted ()),
				this,
				SLOT (handleAccepted ()),
				Qt::UniqueConnection);
		connect (this,
				SIGNAL (gotEntity (LeechCraft::Entity)),
				wizard (),
				SIGNAL (gotEntity (LeechCraft::Entity)));

		AccountsModel_->clear ();

		QStringList labels;
		labels << tr ("Account name")
				<< tr ("JID")
				<< tr ("Import account settings")
				<< tr ("Import history");
		AccountsModel_->setHorizontalHeaderLabels (labels);

		FindAccounts ();
	}

	void PsiPlusImportPage::FindAccounts ()
	{
		QDir dir = QDir::home ();
		if (!dir.cd (".config") ||
				!dir.cd ("Psi+") ||
				!dir.cd ("profiles"))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot cd into ~/.config/Psi+/profiles";
			return;
		}

		Q_FOREACH (const QString& entry,
				dir.entryList (QDir::NoDotAndDotDot | QDir::Dirs))
			ScanProfile (dir.filePath (entry), entry);
	}

	void PsiPlusImportPage::ScanProfile (const QString& path, const QString& profileName)
	{
		QDir dir (path);
		if (!dir.exists ("accounts.xml"))
		{
			qWarning () << Q_FUNC_INFO
					<< "no accounts.xml in"
					<< path;
			return;
		}

		QFile file (dir.filePath ("accounts.xml"));
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open file"
					<< file.fileName ()
					<< file.errorString ();
			return;
		}

		QDomDocument doc;

		QString error;
		int line = 0, col = 0;
		if (!doc.setContent (&file, &error, &line, &col))
		{
			qWarning () << Q_FUNC_INFO
					<< "failed to parse"
					<< file.fileName ()
					<< error
					<< line
					<< col;
			return;
		}

		QStandardItem *item = new QStandardItem (profileName);
		AccountsModel_->appendRow (item);

		const auto& accs = doc.documentElement ().firstChildElement ("accounts");
		auto acc = accs.firstChildElement ();
		while (!acc.isNull ())
		{
			ScanAccount (item, acc);
			acc = acc.nextSiblingElement ();
		}

		Ui_.AccountsTree_->expand (item->index ());
	}

	void PsiPlusImportPage::ScanAccount (QStandardItem *item,
			const QDomElement& account)
	{
		auto tfd = [&account] (const QString& field)
			{ return account.firstChildElement (field).text (); };
		auto ifd = [&account, &tfd] (const QString& field)
			{ return tfd (field).toInt (); };
		auto bfd = [&account, &tfd] (const QString& field)
			{ return tfd (field) == "true"; };

		QMap<QString, QVariant> accountData;
		accountData ["ParentProfile"] = item->text ();
		accountData ["Protocol"] = "xmpp";
		accountData ["Name"] = tfd ("name");
		accountData ["Enabled"] = bfd ("enabled");
		accountData ["Jid"] = tfd ("jid");
		accountData ["Port"] = ifd ("port");
		accountData ["Host"] = bfd ("use-host") ? tfd ("host") : QString ();

		qDebug () << accountData;

		QList<QStandardItem*> row;
		row << new QStandardItem (accountData ["Name"].toString ());
		row << new QStandardItem (accountData ["Jid"].toString ());
		row << new QStandardItem ();
		row << new QStandardItem ();
		row.first ()->setData (accountData, Roles::AccountData);
		row [Column::ImportAcc]->setCheckState (Qt::Checked);
		row [Column::ImportHist]->setCheckState (Qt::Checked);

		item->appendRow (row);
	}

	void PsiPlusImportPage::SendImportAcc (QStandardItem *accItem)
	{
		Entity e = Util::MakeEntity (QVariant (),
				QString (),
				FromUserInitiated | OnlyHandle,
				"x-leechcraft/im-account-import");

		QVariantMap data = accItem->data (Roles::AccountData).toMap ();
		data.remove ("Contacts");
		e.Additional_ ["AccountData"] = data;

		emit gotEntity (e);
	}

	void PsiPlusImportPage::SendImportHist (QStandardItem *accItem)
	{
	}

	void PsiPlusImportPage::handleAccepted ()
	{
		for (int i = 0; i < AccountsModel_->rowCount (); ++i)
		{
			QStandardItem *profItem = AccountsModel_->item (i);
			for (int j = 0; j < profItem->rowCount (); ++j)
			{
				QStandardItem *accItem = profItem->child (j);
				if (profItem->child (j, Column::ImportAcc)->checkState () == Qt::Checked)
					SendImportAcc (accItem);
				if (profItem->child (j, Column::ImportHist)->checkState () == Qt::Checked)
					SendImportHist (accItem);
			}
		}
	}
}
}
}
