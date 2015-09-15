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

#include "vcarddialog.h"
#include <QFuture>
#include <util/xpc/util.h>
#include <util/threads/futures.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/azoth/iproxyobject.h>
#include "structures.h"
#include "georesolver.h"
#include "vkentry.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	VCardDialog::VCardDialog (VkEntry *entry, IAvatarsManager *avatarsMgr,
			GeoResolver *geo, ICoreProxy_ptr proxy, QWidget *parent)
	: QDialog (parent)
	, Proxy_ (proxy)
	, Info_ (entry->GetInfo ())
	{
		Ui_.setupUi (this);
		setAttribute (Qt::WA_DeleteOnClose);

		Ui_.FirstName_->setText (Info_.FirstName_);
		Ui_.LastName_->setText (Info_.LastName_);
		Ui_.Nickname_->setText (Info_.Nick_);

		Ui_.Birthday_->setDate (Info_.Birthday_);
		Ui_.Birthday_->setDisplayFormat (Info_.Birthday_.year () != 1800 ? "dd MMMM yyyy" : "dd MMMM");

		if (Info_.Gender_)
			Ui_.Gender_->setText (Info_.Gender_ == 1 ? tr ("female") : tr ("male"));

		Ui_.HomePhone_->setText (Info_.HomePhone_);
		Ui_.MobilePhone_->setText (Info_.MobilePhone_);

		auto timezoneText = QString::number (Info_.Timezone_) + " GMT";
		if (Info_.Timezone_ > 0)
			timezoneText.prepend ('+');
		Ui_.Timezone_->setText (timezoneText);

		QPointer<VCardDialog> safeThis (this);

		if (Info_.Country_ > 0)
			geo->GetCountry (Info_.Country_,
					[safeThis, this] (const QString& country)
					{
						if (safeThis)
							Ui_.Country_->setText (country);
					});
		if (Info_.City_ > 0)
			geo->GetCity (Info_.City_,
					[safeThis, this] (const QString& country)
					{
						if (safeThis)
							Ui_.City_->setText (country);
					});

		if (!Info_.BigPhoto_.isValid ())
			return;

		Util::Sequence (this, avatarsMgr->GetAvatar (entry, IHaveAvatars::Size::Full)) >>
				[this] (const QImage& image)
				{
					return QtConcurrent::run ([this, image]
							{
								return image.scaled (Ui_.PhotoLabel_->size (),
										Qt::KeepAspectRatio,
										Qt::SmoothTransformation);
							});
				} >>
				[this] (const QImage& image)
				{
					Ui_.PhotoLabel_->setPixmap (QPixmap::fromImage (image));
				};
	}

	void VCardDialog::on_OpenVKPage__released ()
	{
		const auto& pageUrlStr = "http://vk.com/id" + QString::number (Info_.ID_);

		const auto& e = Util::MakeEntity (QUrl (pageUrlStr),
				QString (), FromUserInitiated | OnlyHandle);
		Proxy_->GetEntityManager ()->HandleEntity (e);
	}
}
}
}
