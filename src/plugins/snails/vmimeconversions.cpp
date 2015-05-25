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

#include "vmimeconversions.h"
#include <QStringList>
#include <QIcon>
#include <vmime/net/folder.hpp>
#include <interfaces/core/iiconthememanager.h>
#include "folder.h"
#include "core.h"

namespace LeechCraft
{
namespace Snails
{
	QStringList GetFolderPath (const vmime::shared_ptr<vmime::net::folder>& folder)
	{
		QStringList pathList;
		const auto& path = folder->getFullPath ();
		for (size_t i = 0; i < path.getSize (); ++i)
			pathList << StringizeCT (path.getComponentAt (i));
		return pathList;
	}

	vmime::net::messageSet ToMessageSet (const QList<QByteArray>& ids)
	{
		auto set = vmime::net::messageSet::empty ();
		for (const auto& id : ids)
			set.addRange (vmime::net::UIDMessageRange (id.constData ()));
		return set;
	}

	QString GetFolderIconName (FolderType type)
	{
		switch (type)
		{
		case FolderType::Inbox:
			return "mail-folder-inbox";
		case FolderType::Drafts:
			return "mail-folder-outbox";
		case FolderType::Sent:
			return "mail-folder-sent";
		case FolderType::Important:
			return "mail-mark-important";
		case FolderType::Junk:
			return "mail-mark-junk";
		case FolderType::Trash:
			return "user-trash";
		default:
			return "folder-documents";
		}
	}

	QIcon GetFolderIcon (FolderType type)
	{
		return Core::Instance ().GetProxy ()->
				GetIconThemeManager ()->GetIcon (GetFolderIconName (type));
	}
}
}
