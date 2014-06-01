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

#include "accountsmanager.h"
#include <QStandardItemModel>
#include <QSettings>
#include <QCoreApplication>
#include <QUrl>
#include <xmlsettingsdialog/datasourceroles.h>
#include <util/util.h>
#include "util.h"

namespace LeechCraft
{
namespace Scroblibre
{
	namespace
	{
		enum Column
		{
			CService,
			CLogin
		};
	}

	AccountsManager::AccountsManager (QObject *parent)
	: QObject (parent)
	, Model_ (new QStandardItemModel (this))
	{
		Model_->setHorizontalHeaderLabels ({ tr ("Service"), tr ("Login") });

		Model_->setHeaderData (Column::CService, Qt::Horizontal,
				DataSources::DataFieldType::Enum,
				DataSources::DataSourceRole::FieldType);

		Model_->setHeaderData (Column::CService, Qt::Horizontal,
				QVariantList
				{
					Util::MakeMap<QString, QVariant> ({
							{ "Icon", {} },
							{ "Name", "libre.fm" },
							{ "ID", "libre.fm" }
						})
				},
				DataSources::DataSourceRole::FieldValues);

		Model_->setHeaderData (Column::CLogin, Qt::Horizontal,
				DataSources::DataFieldType::String,
				DataSources::DataSourceRole::FieldType);
	}

	void AccountsManager::LoadAccounts ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Scroblibre");
		const auto accCount = settings.beginReadArray ("Accounts");
		for (auto i = 0; i < accCount; ++i)
		{
			settings.setArrayIndex (i);
			AddRow ({ settings.value ("Service"), settings.value ("Login") });
		}
		settings.endArray ();
	}

	QAbstractItemModel* AccountsManager::GetModel () const
	{
		return Model_;
	}

	void AccountsManager::SaveSettings ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Scroblibre");
		settings.beginWriteArray ("Accounts");
		for (auto i = 0; i < Model_->rowCount (); ++i)
		{
			settings.setArrayIndex (i);

			auto getVal = [this, i] (Column c)
			{
					return Model_->item (i, c)->data (Qt::DisplayRole).toString ();
			};
			settings.setValue ("Service", getVal (Column::CService));
			settings.setValue ("Login", getVal (Column::CLogin));
		}
		settings.endArray ();
	}

	void AccountsManager::AddRow (const QVariantList& row)
	{
		const auto& service = row.value (0).toString ();
		const auto& login = row.value (1).toString ();

		QList<QStandardItem*> itemsRow
		{
			new QStandardItem (service),
			new QStandardItem (login)
		};

		for (auto item : itemsRow)
			item->setEditable (false);

		Model_->appendRow (itemsRow);

		emit accountAdded (ServiceToUrl (service), login);
	}

	void AccountsManager::addRequested (const QString&, const QVariantList& row)
	{
		AddRow (row);

		SaveSettings ();
	}

	void AccountsManager::removeRequested (const QString&, const QModelIndexList& list)
	{
		for (const auto& item : list)
		{
			const auto& row = Model_->takeRow (item.row ());
			emit accountRemoved (ServiceToUrl (row.at (Column::CService)->data ().toString ()),
					row.at (Column::CLogin)->data ().toString ());
		}

		SaveSettings ();
	}
}
}
