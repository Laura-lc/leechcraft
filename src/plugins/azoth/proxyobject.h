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

#pragma once

#include <QObject>
#include <QHash>
#include <QIcon>
#include <QColor>
#include <QDateTime>
#include <QRegExp>
#include "interfaces/azoth/iproxyobject.h"

namespace LC
{
namespace Azoth
{
	class FormatterProxyObject : public IFormatterProxyObject
	{
		QRegExp LinkRegexp_;
	public:
		FormatterProxyObject ();

		QList<QColor> GenerateColors (const QString&, QColor) const override;
		QString GetNickColor (const QString&, const QList<QColor>&) const override;
		QString FormatDate (QDateTime, QObject*) const override;
		QString FormatNickname (QString, QObject*, const QString&) const override;
		QString FormatBody (QString, QObject*, const QList<QColor>&) const override;
		void PreprocessMessage (QObject*) override;
		void FormatLinks (QString&) override;
		QStringList FindLinks (const QString&) override;
	};

	class ProxyObject : public QObject
					  , public IProxyObject
	{
		Q_OBJECT
		Q_INTERFACES (LC::Azoth::IProxyObject)

		QHash<QString, AuthStatus> SerializedStr2AuthStatus_;

		FormatterProxyObject Formatter_;
		IAvatarsManager * const AvatarsManager_;
	public:
		ProxyObject (IAvatarsManager*, QObject* = nullptr);
	public slots:
		QObject* GetSettingsManager () override;
		void SetPassword (const QString&, QObject*) override;
		QString GetAccountPassword (QObject*, bool) override;
		bool IsAutojoinAllowed () override;
		QString StateToString (State) const override;
		QString AuthStatusToString (AuthStatus) const override;
		AuthStatus AuthStatusFromString (const QString&) const override;
		QObject* GetAccount (const QString&) const override;
		QList<QObject*> GetAllAccounts () const override;
		QObject* GetEntry (const QString&, const QString&) const override;
		void OpenChat (const QString&, const QString&, const QString&, const QString&) const override;
		QWidget* FindOpenedChat (const QString&, const QByteArray&) const override;
		Util::ResourceLoader* GetResourceLoader (PublicResourceLoader) const override;
		QIcon GetIconForState (State) const override;

		QObject* CreateCoreMessage (const QString&, const QDateTime&,
				IMessage::Type, IMessage::Direction, QObject*, QObject*) override;

		QString ToPlainBody (QString) override;

		bool IsMessageRead (QObject*) override;
		void MarkMessagesAsRead (QObject*) override;

		QString PrettyPrintDateTime (const QDateTime&) override;

		std::optional<CustomStatus> FindCustomStatus (const QString&) const override;
		QStringList GetCustomStatusNames () const override;

		QImage GetDefaultAvatar (int) const override;

		void RedrawItem (QObject*) const override;

		QObject* GetFirstUnreadMessage (QObject *entryObj) const override;

		IFormatterProxyObject& GetFormatterProxy () override;
		IAvatarsManager* GetAvatarsManager() override;
	};
}
}
