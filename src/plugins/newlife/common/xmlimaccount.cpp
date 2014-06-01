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

#include "xmlimaccount.h"
#include <QDir>
#include <QStandardItemModel>
#include <QDomDocument>
#include "imimportpage.h"

namespace LeechCraft
{
namespace NewLife
{
namespace Common
{
	XMLIMAccount::XMLIMAccount (const XMLIMAccount::ConfigAdapter& adapter)
	: C_ (adapter)
	{
	}

	void XMLIMAccount::FindAccounts ()
	{
		QDir dir = QDir::home ();
		Q_FOREACH (QString path, C_.ProfilesPath_)
		{
			if (!dir.cd (path))
			{
				const auto& list = dir.entryList (QDir::AllDirs | QDir::Hidden | QDir::NoDotAndDotDot);
				for (const auto& candidate : list)
					if (!QString::compare (candidate, path, Qt::CaseInsensitive))
					{
						path = candidate;
						break;
					}
			}
			else
				continue;

			if (!dir.cd (path))
			{
				qWarning () << Q_FUNC_INFO
						<< "cannot cd into"
						<< path
						<< C_.ProfilesPath_.join ("/");
				return;
			}
		}

		Q_FOREACH (const QString& entry,
				dir.entryList (QDir::NoDotAndDotDot | QDir::Dirs))
			ScanProfile (dir.filePath (entry), entry);
	}

	void XMLIMAccount::ScanProfile (const QString& path, const QString& profileName)
	{
		QDir dir (path);
		if (!dir.exists (C_.AccountsFileName_))
		{
			qWarning () << Q_FUNC_INFO
					<< "no accounts.xml in"
					<< path;
			return;
		}

		QFile file (dir.filePath (C_.AccountsFileName_));
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
		item->setEditable (false);
		C_.Model_->appendRow (item);

		const auto& accs = doc.documentElement ().firstChildElement ("accounts");
		auto acc = accs.firstChildElement ();
		while (!acc.isNull ())
		{
			ScanAccount (item, acc);
			acc = acc.nextSiblingElement ();
		}
	}

	void XMLIMAccount::ScanAccount (QStandardItem *item, const QDomElement& account)
	{
		QVariantMap accountData;
		accountData ["ParentProfile"] = item->text ();
		accountData ["Protocol"] = C_.Protocol_ (account);
		accountData ["Name"] = C_.Name_ (account);
		accountData ["Enabled"] = C_.IsEnabled_ (account);
		accountData ["Jid"] = C_.JID_ (account);
		C_.Additional_ (account, accountData);

		QList<QStandardItem*> row;
		row << new QStandardItem (accountData ["Name"].toString ());
		row << new QStandardItem (accountData ["Jid"].toString ());
		row << new QStandardItem ();
		row << new QStandardItem ();
		row.first ()->setData (accountData, IMImportPage::Roles::AccountData);
		row [IMImportPage::Column::ImportAcc]->setCheckState (Qt::Checked);
		row [IMImportPage::Column::ImportAcc]->setCheckable (true);
		row [IMImportPage::Column::ImportHist]->setCheckState (Qt::Checked);
		row [IMImportPage::Column::ImportHist]->setCheckable (true);
		Q_FOREACH (auto item, row)
			item->setEditable (false);

		item->appendRow (row);
	}
}
}
}
