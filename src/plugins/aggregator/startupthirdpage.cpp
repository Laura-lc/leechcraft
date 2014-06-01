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

#include "startupthirdpage.h"
#include <QLineEdit>
#include <QTextCodec>
#include <QDomDocument>
#include <util/util.h>
#include "xmlsettingsmanager.h"
#include "core.h"

namespace LeechCraft
{
namespace Aggregator
{
	StartupThirdPage::StartupThirdPage (QWidget *parent)
	: QWizardPage (parent)
	{
		ParseFeedsSets ();

		Ui_.setupUi (this);
		Ui_.Tree_->header ()->setResizeMode (0, QHeaderView::ResizeToContents);
		Ui_.Tree_->header ()->setResizeMode (1, QHeaderView::ResizeToContents);

		connect (Ui_.LocalizationBox_,
				SIGNAL (currentIndexChanged (const QString&)),
				this,
				SLOT (handleCurrentIndexChanged (const QString&)));

		QMap<QString, int> languages;
		languages ["ru"] = 1;

		QString language = Util::GetLanguage ();
		Ui_.LocalizationBox_->setCurrentIndex (languages.contains (language) ?
				languages [language] :
				0);
		handleCurrentIndexChanged (QString ("(") + language + ")");

		setTitle ("Aggregator");
		setSubTitle (tr ("Select feeds"));
	}

	void StartupThirdPage::initializePage ()
	{
		connect (wizard (),
				SIGNAL (accepted ()),
				this,
				SLOT (handleAccepted ()),
				Qt::UniqueConnection);
		wizard ()->setMinimumWidth (std::max (wizard ()->minimumWidth (), 800));
		wizard ()->setMinimumHeight (std::max (wizard ()->minimumHeight (), 500));

		XmlSettingsManager::Instance ()->
				setProperty ("StartupVersion", 3);
	}

	namespace
	{
		QStringList ParseTags (const QDomElement& feedElem)
		{
			QStringList result;

			auto tagElem = feedElem.firstChildElement ("tags").firstChildElement ("tag");
			while (!tagElem.isNull ())
			{
				result << tagElem.text ();
				tagElem = tagElem.nextSiblingElement ("tag");
			}

			return result;
		}
	}

	void StartupThirdPage::ParseFeedsSets ()
	{
		QFile file (":/resources/data/default_feeds.xml");
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot open feeds resource file:"
					<< file.errorString ();
			return;
		}

		QDomDocument doc;
		QString msg;
		int line = 0;
		int col = 0;
		if (!doc.setContent (&file, &msg, &line, &col))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot parse feed resource file:"
					<< msg
					<< "on"
					<< line
					<< col;
			return;
		}

		auto setElem = doc.documentElement ().firstChildElement ("set");
		while (!setElem.isNull ())
		{
			auto& set = Sets_ [setElem.attribute ("lang", "general")];

			auto feedElem = setElem.firstChildElement ("feed");
			while (!feedElem.isNull ())
			{
				FeedInfo info
				{
					feedElem.firstChildElement ("name").text (),
					ParseTags (feedElem),
					feedElem.firstChildElement ("url").text ()
				};
				set << info;

				feedElem = feedElem.nextSiblingElement ("feed");
			}

			setElem = setElem.nextSiblingElement ();
		}
	}

	void StartupThirdPage::Populate (const QString& title)
	{
		FeedInfos_t engines = Sets_ [title];
		Q_FOREACH (FeedInfo info, engines)
		{
			QStringList strings;
			strings << info.Name_
				<< info.DefaultTags_
				<< info.URL_;

			QTreeWidgetItem *item = new QTreeWidgetItem (Ui_.Tree_, strings);
			item->setCheckState (0, Qt::Checked);

			QLineEdit *edit = new QLineEdit (Ui_.Tree_);
			edit->setFrame (false);
			edit->setText (info.DefaultTags_.join ("; "));
			Ui_.Tree_->setItemWidget (item, 1, edit);
		}
	}

	void StartupThirdPage::handleAccepted ()
	{
		if (wizard ()->field ("Aggregator/StorageDirty").toBool ())
			Core::Instance ().ReinitStorage ();

		for (int i = 0; i < Ui_.Tree_->topLevelItemCount (); ++i)
		{
			QTreeWidgetItem *item = Ui_.Tree_->topLevelItem (i);
			if (item->checkState (0) != Qt::Checked)
				continue;

			QString url = item->text (2);
			QString tags = static_cast<QLineEdit*> (Ui_.Tree_->itemWidget (item, 1))->text ();
			Core::Instance ().AddFeed (url, tags);
		}
	}

	void StartupThirdPage::handleCurrentIndexChanged (const QString& text)
	{
		Ui_.Tree_->clear ();
		if (text.endsWith (')'))
		{
			QString selected = text.mid (text.size () - 3, 2);
			Populate (selected);
		}
		Populate ("general");
	}

	void StartupThirdPage::on_SelectAll__released ()
	{
		for (int i = 0; i < Ui_.Tree_->topLevelItemCount (); ++i)
			Ui_.Tree_->topLevelItem (i)->setCheckState (0, Qt::Checked);
	}

	void StartupThirdPage::on_DeselectAll__released ()
	{
		for (int i = 0; i < Ui_.Tree_->topLevelItemCount (); ++i)
			Ui_.Tree_->topLevelItem (i)->setCheckState (0, Qt::Unchecked);
	}
}
}
