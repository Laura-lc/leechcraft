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

#include "gwoptionsdialog.h"
#include <QPushButton>
#include <QMessageBox>
#include "regformhandlerwidget.h"

namespace LC
{
namespace Azoth
{
namespace Xoox
{
	GWOptionsDialog::GWOptionsDialog (QXmppClient *client, const QString& to, QWidget *parent)
	: QDialog (parent)
	, RegForm_ (new RegFormHandlerWidget (client))
	{
		Ui_.setupUi (this);
		qobject_cast<QVBoxLayout*> (layout ())->insertWidget (0, RegForm_);

		connect (RegForm_,
				SIGNAL (completeChanged ()),
				this,
				SLOT (handleCompleteChanged ()));

		disconnect (Ui_.ButtonBox_,
				SIGNAL (accepted ()),
				this,
				SLOT (accept ()));
		connect (Ui_.ButtonBox_,
				SIGNAL (accepted ()),
				this,
				SLOT (sendRegistration ()));

		RegForm_->SendRequest (to);
	}

	void GWOptionsDialog::sendRegistration ()
	{
		connect (RegForm_,
				SIGNAL (successfulReg ()),
				this,
				SLOT (accept ()));
		connect (RegForm_,
				SIGNAL (regError (QString)),
				this,
				SLOT (handleError (QString)));

		RegForm_->Register ();
		RegForm_->setEnabled (false);
	}

	void GWOptionsDialog::handleError (const QString& error)
	{
		QMessageBox::critical (this,
				"LeechCraft",
				tr ("Error updating gateway information: %1.")
					.arg (error));
		reject ();
	}

	void GWOptionsDialog::handleCompleteChanged ()
	{
		Ui_.ButtonBox_->button (QDialogButtonBox::Ok)->setEnabled (RegForm_->IsComplete ());
	}
}
}
}
