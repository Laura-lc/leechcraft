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

#include "accountconfigdialog.h"
#include <numeric>
#include <QMenu>

namespace LC
{
namespace Snails
{
	AccountConfigDialog::AccountConfigDialog (QWidget *parent)
	: QDialog (parent)
	{
		Ui_.setupUi (this);
		Ui_.BrowseToSync_->setMenu (new QMenu (tr ("Folders to sync")));
		Ui_.OutgoingFolder_->addItem (QString ());

		connect (Ui_.InSecurityType_,
				SIGNAL (currentIndexChanged (int)),
				this,
				SLOT (resetInPort ()));
	}

	AccountConfig AccountConfigDialog::GetConfig () const
	{
		return
		{
			Ui_.AccName_->text (),
			Ui_.UserName_->text (),
			Ui_.UserEmail_->text (),
			Ui_.InLogin_->text (),

			Ui_.UseSASL_->checkState () == Qt::Checked,
			Ui_.SASLRequired_->checkState () == Qt::Checked,
			static_cast<AccountConfig::SecurityType> (Ui_.InSecurityType_->currentIndex ()),
			Ui_.InSecurityRequired_->checkState () == Qt::Checked,
			static_cast<AccountConfig::SecurityType> (Ui_.OutSecurityType_->currentIndex ()),
			Ui_.OutSecurityRequired_->checkState () == Qt::Checked,

			Ui_.SMTPAuthRequired_->checkState () == Qt::Checked,
			Ui_.InHost_->text (),
			Ui_.InPort_->value (),
			Ui_.OutAddress_->text (),
			Ui_.OutPort_->value (),
			Ui_.CustomOut_->isChecked () ? Ui_.OutLogin_->text () : QString {},
			static_cast<AccountConfig::OutType> (Ui_.OutType_->currentIndex ()),

			Ui_.KeepAliveInterval_->value () * 1000,
			Ui_.LogConnectionsToFile_->checkState () == Qt::Checked,

			static_cast<AccountConfig::DeleteBehaviour> (Ui_.DeletionBehaviour_->currentIndex ())
		};
	}

	void AccountConfigDialog::SetConfig (const AccountConfig& cfg)
	{
		Ui_.AccName_->setText (cfg.AccName_);
		Ui_.UserName_->setText (cfg.UserName_);
		Ui_.UserEmail_->setText (cfg.UserEmail_);
		Ui_.InLogin_->setText (cfg.Login_);

		Ui_.UseSASL_->setCheckState (cfg.UseSASL_ ? Qt::Checked : Qt::Unchecked);
		Ui_.SASLRequired_->setCheckState (cfg.SASLRequired_ ? Qt::Checked : Qt::Unchecked);
		Ui_.InSecurityType_->setCurrentIndex (static_cast<int> (cfg.InSecurity_));
		Ui_.InSecurityRequired_->setCheckState (cfg.InSecurityRequired_ ? Qt::Checked : Qt::Unchecked);
		Ui_.OutSecurityType_->setCurrentIndex (static_cast<int> (cfg.OutSecurity_));
		Ui_.OutSecurityRequired_->setCheckState (cfg.OutSecurityRequired_ ? Qt::Checked : Qt::Unchecked);

		Ui_.SMTPAuthRequired_->setCheckState (cfg.SMTPNeedsAuth_ ? Qt::Checked : Qt::Unchecked);
		Ui_.InHost_->setText (cfg.InHost_);
		Ui_.InPort_->setValue (cfg.InPort_);
		Ui_.OutAddress_->setText (cfg.OutHost_);
		Ui_.OutPort_->setValue (cfg.OutPort_);

		Ui_.CustomOut_->setChecked (!cfg.OutLogin_.isEmpty ());
		Ui_.OutLogin_->setText (cfg.OutLogin_);

		Ui_.OutType_->setCurrentIndex (static_cast<int> (cfg.OutType_));

		Ui_.KeepAliveInterval_->setValue (cfg.KeepAliveInterval_ / 1000);
		Ui_.LogConnectionsToFile_->setCheckState (cfg.LogToFile_ ? Qt::Checked : Qt::Unchecked);

		Ui_.DeletionBehaviour_->setCurrentIndex (static_cast<int> (cfg.DeleteBehaviour_));
	}

	void AccountConfigDialog::SetAllFolders (const QList<QStringList>& folders)
	{
		for (const auto& f : folders)
		{
			const auto& name = f.join ("/");
			Ui_.OutgoingFolder_->addItem (name, f);

			auto act = Ui_.BrowseToSync_->menu ()->addAction (name);
			act->setCheckable (true);
			act->setData (f);

			connect (act,
					SIGNAL (toggled (bool)),
					this,
					SLOT (rebuildFoldersToSyncLine ()));
		}
	}

	QList<QStringList> AccountConfigDialog::GetFoldersToSync () const
	{
		QList<QStringList> result;
		for (const auto& action : Ui_.BrowseToSync_->menu ()->actions ())
			if (action->isChecked ())
				result << action->data ().toStringList ();

		return result;
	}

	void AccountConfigDialog::SetFoldersToSync (const QList<QStringList>& folders)
	{
		for (const auto& action : Ui_.BrowseToSync_->menu ()->actions ())
		{
			const auto& folder = action->data ().toStringList ();
			action->setChecked (folders.contains (folder));
		}

		rebuildFoldersToSyncLine ();
	}

	QStringList AccountConfigDialog::GetOutFolder () const
	{
		return Ui_.OutgoingFolder_->itemData (Ui_.OutgoingFolder_->currentIndex ()).toStringList ();
	}

	void AccountConfigDialog::SetOutFolder (const QStringList& folder)
	{
		const int idx = Ui_.OutgoingFolder_->findData (folder);
		if (idx == -1)
			return;

		Ui_.OutgoingFolder_->setCurrentIndex (-1);
	}

	void AccountConfigDialog::resetInPort ()
	{
		const QList<int> values { 465, 993, 143 };

		const int pos = Ui_.InSecurityType_->currentIndex ();
		Ui_.InPort_->setValue (values.value (pos));
	}

	void AccountConfigDialog::rebuildFoldersToSyncLine ()
	{
		const auto& sync = GetFoldersToSync ();
		const auto& folders = std::accumulate (sync.begin (), sync.end (), QStringList (),
				[] (QStringList fs, const QStringList& f) { return fs << f.join ("/"); });
		Ui_.FoldersToSync_->setText (folders.join ("; "));
	}
}
}
