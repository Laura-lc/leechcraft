/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "transcodejob.h"
#include <stdexcept>
#include <functional>
#include <QMap>
#include <QDir>
#include <QtDebug>
#include "transcodingparams.h"

#ifdef Q_OS_UNIX
#include <sys/time.h>
#include <sys/resource.h>
#endif

namespace LeechCraft
{
namespace LMP
{
	namespace
	{
		QString BuildTranscodedPath (const QString& path, const TranscodingParams& params)
		{
			QDir dir = QDir::temp ();
			if (!dir.exists ("lmp_transcode"))
				dir.mkdir ("lmp_transcode");
			if (!dir.cd ("lmp_transcode"))
				throw std::runtime_error ("unable to cd into temp dir");

			const QFileInfo fi (path);

			const auto format = Formats ().GetFormat (params.FormatID_);

			auto result = dir.absoluteFilePath (fi.fileName ());
			const auto& ext = format->GetFileExtension ();
			const auto dotIdx = result.lastIndexOf ('.');
			if (dotIdx == -1)
				result += '.' + ext;
			else
				result.replace (dotIdx + 1, result.size () - dotIdx, ext);

			return result;
		}
	}
	TranscodeJob::TranscodeJob (const QString& path, const TranscodingParams& params, QObject* parent)
	: QObject (parent)
	, Process_ (new QProcess (this))
	, OriginalPath_ (path)
	, TranscodedPath_ (BuildTranscodedPath (path, params))
	, TargetPattern_ (params.FilePattern_)
	{
		QStringList args
		{
			"-i",
			path
		};
		args << Formats {}.GetFormat (params.FormatID_)->ToFFmpeg (params);
		args << "-map_metadata" << "0";
		args << TranscodedPath_;

		connect (Process_,
				SIGNAL (finished (int, QProcess::ExitStatus)),
				this,
				SLOT (handleFinished (int, QProcess::ExitStatus)));
		connect (Process_,
				SIGNAL (readyRead ()),
				this,
				SLOT (handleReadyRead ()));
		Process_->start ("ffmpeg", args);

#ifdef Q_OS_UNIX
		setpriority (PRIO_PROCESS, Process_->pid (), 19);
#endif
	}

	QString TranscodeJob::GetOrigPath () const
	{
		return OriginalPath_;
	}

	QString TranscodeJob::GetTranscodedPath () const
	{
		return TranscodedPath_;
	}

	QString TranscodeJob::GetTargetPattern () const
	{
		return TargetPattern_;
	}

	void TranscodeJob::handleFinished (int code, QProcess::ExitStatus status)
	{
		qDebug () << Q_FUNC_INFO << code << status;
		emit done (this, !code);

		if (code)
			qWarning () << Q_FUNC_INFO
					<< Process_->readAllStandardError ();
	}

	void TranscodeJob::handleReadyRead ()
	{
	}
}
}
