/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
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

#ifndef PLUGINS_POSHUKU_PLUGINS_ONLINEBOOKMARKS_PLUGINS_READITLATER_READITLATERACCOUNT_H
#define PLUGINS_POSHUKU_PLUGINS_ONLINEBOOKMARKS_PLUGINS_READITLATER_READITLATERACCOUNT_H

#include <QObject>
#include <QDateTime>
#include <interfaces/structures.h>
#include <interfaces/iaccount.h>

namespace LeechCraft
{
namespace Poshuku
{
namespace OnlineBookmarks
{
namespace ReadItLater
{
class ReadItLaterAccount : public QObject
							, public IAccount
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Poshuku::OnlineBookmarks::IAccount)

		QString Login_;
		QString Password_;

		QObject *ParentService_;

		bool IsSyncing_;

		QDateTime LastUpload_;
		QDateTime LastDownload_;

		QVariantList DownloadedBookmarks_;
	public:
		ReadItLaterAccount (const QString&, QObject* = 0);

		QObject* GetQObject () override;
		QObject* GetParentService () const override;
		QByteArray GetAccountID () const override;
		QString GetLogin () const override;

		QString GetPassword () const override;
		void SetPassword (const QString&) override;

		bool IsSyncing () const override;
		void SetSyncing (bool) override;

		QDateTime GetLastDownloadDateTime () const override;
		QDateTime GetLastUploadDateTime () const override;
		void SetLastUploadDateTime (const QDateTime&) override;
		void SetLastDownloadDateTime (const QDateTime&) override;

		void AppendDownloadedBookmarks (const QVariantList&);
		QVariantList GetBookmarksDiff (const QVariantList&);

		QByteArray Serialize () const ;
		static ReadItLaterAccount* Deserialize (const QByteArray&, QObject*);
	};
}
}
}
}

#endif // PLUGINS_POSHUKU_PLUGINS_ONLINEBOOKMARKS_PLUGINS_READITLATER_READITLATERACCOUNT_H
