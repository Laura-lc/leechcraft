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

#include "annotationsmanager.h"
#include "xeps/xmppannotationsmanager.h"
#include "clientconnection.h"
#include "clientconnectionextensionsmanager.h"

namespace LC
{
namespace Azoth
{
namespace Xoox
{
	AnnotationsManager::AnnotationsManager (ClientConnection& conn, QObject *parent)
	: QObject { parent }
	, XMPPAnnManager_ { conn.Exts ().Get<XMPPAnnotationsManager> () }
	{
		connect (&XMPPAnnManager_,
				&XMPPAnnotationsManager::notesReceived,
				this,
				[this] (const QList<XMPPAnnotationsIq::NoteItem>& notes)
				{
					for (const auto& item : notes)
						JID2Note_ [item.GetJid ()] = item;
				});

		connect (&conn,
				&ClientConnection::connected,
				this,
				[this]
				{
					JID2Note_.clear ();
					XMPPAnnManager_.RequestNotes ();
				});
	}

	XMPPAnnotationsIq::NoteItem AnnotationsManager::GetNote (const QString& jid) const
	{
		return JID2Note_ [jid];
	}

	void AnnotationsManager::SetNote (const QString& jid, const XMPPAnnotationsIq::NoteItem& note)
	{
		JID2Note_ [jid] = note;
		XMPPAnnManager_.SetNotes (JID2Note_.values ());
	}
}
}
}
