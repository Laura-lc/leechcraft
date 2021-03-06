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

#include "structuresops.h"

QDataStream& operator<< (QDataStream& out, const LC::Entity& e)
{
	quint16 version = 2;
	out << version
		<< e.Entity_
		<< e.Location_
		<< e.Mime_
		<< static_cast<quint32> (e.Parameters_)
		<< e.Additional_;
	return out;
}

QDataStream& operator>> (QDataStream& in, LC::Entity& e)
{
	quint16 version = 0;
	in >> version;
	if (version == 2)
	{
		quint32 parameters;
		in >> e.Entity_
			>> e.Location_
			>> e.Mime_
			>> parameters
			>> e.Additional_;

		if (parameters & LC::NoAutostart)
			e.Parameters_ |= LC::NoAutostart;
		if (parameters & LC::DoNotSaveInHistory)
			e.Parameters_ |= LC::DoNotSaveInHistory;
		if (parameters & LC::IsDownloaded)
			e.Parameters_ |= LC::IsDownloaded;
		if (parameters & LC::FromUserInitiated)
			e.Parameters_ |= LC::FromUserInitiated;
		if (parameters & LC::DoNotNotifyUser)
			e.Parameters_ |= LC::DoNotNotifyUser;
		if (parameters & LC::Internal)
			e.Parameters_ |= LC::Internal;
		if (parameters & LC::NotPersistent)
			e.Parameters_ |= LC::NotPersistent;
		if (parameters & LC::DoNotAnnounceEntity)
			e.Parameters_ |= LC::DoNotAnnounceEntity;
		if (parameters & LC::OnlyHandle)
			e.Parameters_ |= LC::OnlyHandle;
		if (parameters & LC::OnlyDownload)
			e.Parameters_ |= LC::OnlyDownload;
		if (parameters & LC::AutoAccept)
			e.Parameters_ |= LC::AutoAccept;
		if (parameters & LC::FromCommandLine)
			e.Parameters_ |= LC::FromCommandLine;
	}
	else if (version == 1)
	{
		QByteArray buf;
		quint32 parameters;
		in >> buf
			>> e.Location_
			>> e.Mime_
			>> parameters
			>> e.Additional_;

		e.Entity_ = buf;

		if (parameters & LC::NoAutostart)
			e.Parameters_ |= LC::NoAutostart;
		if (parameters & LC::DoNotSaveInHistory)
			e.Parameters_ |= LC::DoNotSaveInHistory;
		if (parameters & LC::IsDownloaded)
			e.Parameters_ |= LC::IsDownloaded;
		if (parameters & LC::FromUserInitiated)
			e.Parameters_ |= LC::FromUserInitiated;
		if (parameters & LC::DoNotNotifyUser)
			e.Parameters_ |= LC::DoNotNotifyUser;
		if (parameters & LC::Internal)
			e.Parameters_ |= LC::Internal;
		if (parameters & LC::NotPersistent)
			e.Parameters_ |= LC::NotPersistent;
		if (parameters & LC::DoNotAnnounceEntity)
			e.Parameters_ |= LC::DoNotAnnounceEntity;
		if (parameters & LC::OnlyHandle)
			e.Parameters_ |= LC::OnlyHandle;
		if (parameters & LC::OnlyDownload)
			e.Parameters_ |= LC::OnlyDownload;
		if (parameters & LC::AutoAccept)
			e.Parameters_ |= LC::AutoAccept;
		if (parameters & LC::FromCommandLine)
			e.Parameters_ |= LC::FromCommandLine;
	}
	else
	{
		qWarning () << Q_FUNC_INFO
			<< "unknown version"
			<< "version";
	}
	return in;
}

namespace LC
{
	bool operator< (const LC::Entity& e1, const LC::Entity& e2)
	{
		return e1.Mime_ < e2.Mime_ &&
			e1.Location_ < e2.Location_ &&
			e1.Parameters_ < e2.Parameters_;
	}

	bool operator== (const LC::Entity& e1, const LC::Entity& e2)
	{
		return e1.Mime_ == e2.Mime_ &&
			e1.Entity_ == e2.Entity_ &&
			e1.Location_ == e2.Location_ &&
			e1.Parameters_ == e2.Parameters_ &&
			e1.Additional_ == e2.Additional_;
	}
}
