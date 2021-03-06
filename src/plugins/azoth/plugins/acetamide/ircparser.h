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

#pragma once

#include <QObject>
#include "core.h"
#include "localtypes.h"

namespace LC
{
namespace Azoth
{
namespace Acetamide
{
	class IrcServerHandler;

	class IrcParser : public QObject
	{
		Q_OBJECT

		IrcServerHandler *ISH_;
		ServerOptions ServerOptions_;
		IrcMessageOptions IrcMessageOptions_;

		QStringList LongAnswerCommands_;
	public:
		IrcParser (IrcServerHandler*);

		bool CmdHasLongAnswer (const QString& cmd);

		void AuthCommand ();
		void UserCommand ();
		void NickCommand (const QStringList&);
		void JoinCommand (QStringList);
		void PrivMsgCommand (const QStringList&);
		void PartCommand (const QStringList&);
		void PongCommand (const QStringList&);
		void RawCommand (const QStringList&);
		void CTCPRequest (const QStringList&);
		void CTCPReply (const QStringList&);
		void TopicCommand (const QStringList&);
		void NamesCommand (const QStringList&);
		void InviteCommand (const QStringList&);
		void KickCommand (const QStringList&);
		void OperCommand (const QStringList&);
		void SQuitCommand (const QStringList&);
		void MOTDCommand (const QStringList&);
		void LusersCommand (const QStringList&);
		void VersionCommand (const QStringList&);
		void StatsCommand (const QStringList&);
		void LinksCommand (const QStringList&);
		void TimeCommand (const QStringList&);
		void ConnectCommand (const QStringList&);
		void TraceCommand (const QStringList&);
		void AdminCommand (const QStringList&);
		void InfoCommand (const QStringList&);
		void WhoCommand (const QStringList&);
		void WhoisCommand (const QStringList&);
		void WhowasCommand (const QStringList&);
		void KillCommand (const QStringList&);
		void PingCommand (const QStringList&);
		void AwayCommand (const QStringList&);
		void RehashCommand (const QStringList&);
		void DieCommand (const QStringList&);
		void RestartCommand (const QStringList&);
		void SummonCommand (const QStringList&);
		void UsersCommand (const QStringList&);
		void UserhostCommand (const QStringList&);
		void WallopsCommand (const QStringList&);
		void IsonCommand (const QStringList&);
		void QuitCommand (const QStringList&);
		void ChanModeCommand (const QStringList&);
		void ChannelsListCommand (const QStringList&);

		/** Automatically converts the \em ba to UTF-8.
		 */
		bool ParseMessage (const QByteArray& ba);
		IrcMessageOptions GetIrcMessageOptions () const;
	private:
		QTextCodec* GetCodec ();
		QStringList EncodingList (const QStringList&);
	};
}
}
}
