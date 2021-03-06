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

#ifndef PLUGINS_AZOTH_PLUGINS_ACETAMIDE_CHANNELSMANAGER_H
#define PLUGINS_AZOTH_PLUGINS_ACETAMIDE_CHANNELSMANAGER_H

#include <memory>
#include <QObject>
#include <QHash>
#include <QSet>
#include <QQueue>
#include <QDateTime>
#include "localtypes.h"

namespace LC
{
namespace Azoth
{
namespace Acetamide
{
	class IrcServerHandler;
	class ChannelHandler;
	class IrcAccount;

	using ChannelHandler_ptr = std::shared_ptr<ChannelHandler>;

	class ChannelsManager : public QObject
	{
		Q_OBJECT

		IrcServerHandler *ISH_;

		QHash<QString, ChannelHandler_ptr> ChannelHandlers_;
		QSet<ChannelOptions> ChannelsQueue_;

		QString LastActiveChannel_;
	public:
		ChannelsManager (IrcServerHandler* = 0);
		IrcAccount* GetAccount () const;

		QString GetOurNick () const;

		QString GetServerID () const;

		ServerOptions GetServerOptions () const;

		QObjectList GetCLEntries () const;

		ChannelHandler* GetChannelHandler (const QString& channel);
		QList<ChannelHandler*> GetChannels () const;

		bool IsChannelExists (const QString& channel) const;

		int Count () const;

		QSet<ChannelOptions> GetChannelsQueue () const;
		void CleanQueue ();

		void AddChannel2Queue (const ChannelOptions& options);
		bool AddChannel (const ChannelOptions& options);

		void LeaveChannel (const QString& channel, const QString& msg);
		void CloseChannel (const QString& channel);
		void CloseAllChannels ();
		void UnregisterChannel (ChannelHandler *ich);

		QHash<QString, QObject*> GetParticipantsByNick (const QString& nick) const;

		void AddParticipant (const QString& channel, const QString& nick,
				const QString& user = QString (), const QString& host = QString ());

		void LeaveParticipant (const QString& channel,
				const QString& nick, const QString& msg);
		void QuitParticipant (const QString& nick, const QString& msg);

		void KickParticipant (const QString& channel, const QString& target,
				const QString& reason, const QString& who);
		void KickCommand (const QString& channel,
				const QString& nick, const QString& reason);

		void ChangeNickname (const QString& oldNick, const QString& newNick);

		void GotNames (const QString& channel, const QStringList& participants);
		void GotEndOfNamesCmd (const QString& channel);

		void SendPublicMessage (const QString& channel, const QString& msg);

		void ReceivePublicMessage (const QString& channel,
				const QString& nick, const QString& msg);
		bool ReceiveCmdAnswerMessage (const QString& cmd,
				const QString& answer, bool endOdfCmd = false);

		void SetMUCSubject (const QString& channel, const QString& topic);
		void SetTopic (const QString& channel, const QString& topic);

		void CTCPReply (const QString& msg);
		void CTCPRequestResult (const QString& msg);

		void SetBanListItem (const QString& channel,
				const QString& mask, const QString& nick, const QDateTime& time);
		void RequestBanList (const QString& channel);
		void AddBanListItem (const QString& channel, const QString& mask);
		void RemoveBanListItem (const QString& channel, const QString& mask);

		void SetExceptListItem (const QString& channel,
				const QString& mask, const QString& nick, const QDateTime& time);
		void RequestExceptList (const QString& channel);
		void AddExceptListItem (const QString& channel, const QString& mask);
		void RemoveExceptListItem (const QString& channel, const QString& mask);

		void SetInviteListItem (const QString& channel,
				const QString& mask, const QString& nick, const QDateTime& time);
		void RequestInviteList (const QString& channel);
		void AddInviteListItem (const QString& channel, const QString& mask);
		void RemoveInviteListItem (const QString& channel, const QString& mask);

		void ParseChanMode (const QString& channel,
				const QString& mode, const QString& value);
		void SetNewChannelMode (const QString& channel,
				const QString& mode, const QString& name);
		void SetNewChannelModes (const QString& channel, const ChannelModes& modes);

		void RequestWhoIs (const QString& channel, const QString& nick);
		void RequestWhoWas (const QString& channel, const QString& nick);
		void RequestWho (const QString& channel, const QString& nick);

		void CTCPRequest (const QStringList& cmd, const QString& channel);

		QMap<QString, QString> GetISupport () const;

		void SetPrivateChat (const QString& nick);

		void CreateServerParticipantEntry (QString nick);

		void UpdateEntry (const WhoMessage& message);

		int GetChannelUsersCount (const QString& channel);

		void ClosePrivateChat (const QString& nick);

		void SetChannelUrl (const QString& channel, const QString& url);
		void SetTopicWhoTime (const QString& channel,
				const QString& who, quint64 time);
	};
}
}
}

#endif // PLUGINS_AZOTH_PLUGINS_ACETAMIDE_CHANNELSMANAGER_H
