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

#include "importbinary.h"
#include <algorithm>
#include <QFile>
#include <QDataStream>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QTimer>
#include <QtDebug>

namespace LeechCraft
{
namespace Aggregator
{
	ImportBinary::ImportBinary (QWidget *parent)
	: QDialog (parent)
	{
		Ui_.setupUi (this);
		on_Browse__released ();
	}

	ImportBinary::~ImportBinary ()
	{
	}

	QString ImportBinary::GetFilename () const
	{
		return Ui_.File_->text ();
	}

	QString ImportBinary::GetTags () const
	{
		return Ui_.AdditionalTags_->text ().trimmed ();
	}

	feeds_container_t ImportBinary::GetSelectedFeeds () const
	{
		feeds_container_t result;

		QMap<IDType_t, IDType_t> foreignIDs2Local;

		for (int i = 0, end = Ui_.FeedsToImport_->topLevelItemCount ();
				i < end; ++i)
		{
			if (Ui_.FeedsToImport_->topLevelItem (i)->checkState (0) !=
					Qt::Checked)
				continue;

			Channel_ptr chan = Channels_ [i];
			if (!foreignIDs2Local.contains (chan->FeedID_))
			{
				Feed_ptr feed (new Feed ());
				feed->LastUpdate_ = QDateTime::currentDateTime ();
				result.push_back (feed);
				foreignIDs2Local [chan->FeedID_] = feed->FeedID_;
			}

			IDType_t our = foreignIDs2Local [chan->FeedID_];
			auto pos = std::find_if (result.begin (), result.end (),
					[our] (Feed_ptr feed) { return feed->FeedID_ == our; });
			(*pos)->Channels_.push_back (chan);
		}

		return result;
	}

	void ImportBinary::on_File__textEdited (const QString& newFilename)
	{
		Reset ();

		if (QFile (newFilename).exists ())
			Ui_.ButtonBox_->button (QDialogButtonBox::Open)->
				setEnabled (HandleFile (newFilename));
		else
			Reset ();
	}

	void ImportBinary::on_Browse__released ()
	{
		QString startingPath = QFileInfo (Ui_.File_->text ()).path ();
		if (startingPath.isEmpty ())
			startingPath = QDir::homePath ();

		QString filename = QFileDialog::getOpenFileName (this,
				tr ("Select binary file"),
				startingPath,
				tr ("Aggregator exchange files (*.lcae);;"
					"All files (*.*)"));

		if (filename.isEmpty ())
		{
			QTimer::singleShot (0,
					this,
					SLOT (reject ()));
			return;
		}

		Reset ();

		Ui_.File_->setText (filename);

		Ui_.ButtonBox_->button (QDialogButtonBox::Open)->
			setEnabled (HandleFile (filename));
	}

	bool ImportBinary::HandleFile (const QString& filename)
	{
		QFile file (filename);
		if (!file.open (QIODevice::ReadOnly))
		{
			QMessageBox::critical (this,
					tr ("LeechCraft"),
					tr ("Could not open file %1 for reading.")
						.arg (filename));
			return false;
		}

		QByteArray buffer = qUncompress (file.readAll ());
		QDataStream stream (&buffer, QIODevice::ReadOnly);

		int magic = 0;
		stream >> magic;
		if (magic != static_cast<int> (0xd34df00d))
		{
			QMessageBox::warning (this,
					tr ("LeechCraft"),
					tr ("Selected file %1 is not a valid "
						"LeechCraft::Aggregator exchange file.")
					.arg (filename));
			return false;
		}

		int version = 0;
		stream >> version;

		if (version != 1)
		{
			QMessageBox::warning (this,
					tr ("LeechCraft"),
					tr ("Selected file %1 is a valid LeechCraft::Aggregator "
						"exchange file, but its version %2 is unknown")
					.arg (filename)
					.arg (version));
		}

		QString title, owner, ownerEmail;
		stream >> title >> owner >> ownerEmail;

		while (stream.status () == QDataStream::Ok)
		{
			Channel_ptr channel (new Channel (-1, -1));
			stream >> (*channel);
			Channels_.push_back (channel);

			QStringList strings (channel->Title_);
			strings << QString::number (channel->Items_.size ());

			QTreeWidgetItem *item =
				new QTreeWidgetItem (Ui_.FeedsToImport_, strings);

			item->setCheckState (0, Qt::Checked);
		}

		return true;
	}

	void ImportBinary::Reset ()
	{
		Channels_.clear ();
		Ui_.FeedsToImport_->clear ();

		Ui_.ButtonBox_->button (QDialogButtonBox::Open)->setEnabled (false);
	}
}
}
