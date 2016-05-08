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

#include "dumper.h"
#include <QtDebug>
#include <util/sll/slotclosure.h>

namespace LeechCraft
{
namespace Azoth
{
namespace ChatHistory
{
	Dumper::Dumper (const QString& from, const QString& to, QObject *parent)
	: QObject { parent }
	, Dumper_ { new QProcess { this } }
	, Restorer_ { new QProcess { this } }
	{
		Iface_.reportStarted ();

		Dumper_->setStandardOutputProcess (Restorer_);

		new Util::SlotClosure<Util::NoDeletePolicy>
		{
			[this] { HandleProcessError (Dumper_); },
			Dumper_,
			SIGNAL (error (QProcess::ProcessError)),
			Dumper_
		};
		new Util::SlotClosure<Util::NoDeletePolicy>
		{
			[this] { HandleProcessError (Restorer_); },
			Restorer_,
			SIGNAL (error (QProcess::ProcessError)),
			Restorer_
		};

		new Util::SlotClosure<Util::NoDeletePolicy>
		{
			[this] { HandleProcessFinished (Dumper_); },
			Dumper_,
			SIGNAL (finished (int, QProcess::ExitStatus)),
			Dumper_
		};
		new Util::SlotClosure<Util::NoDeletePolicy>
		{
			[this] { HandleProcessFinished (Restorer_); },
			Restorer_,
			SIGNAL (finished (int, QProcess::ExitStatus)),
			Restorer_
		};

		Dumper_->start ("sqlite3", { from, ".dump" });
		Restorer_->start ("sqlite3", { to });
	}

	QFuture<Dumper::Result_t> Dumper::GetFuture ()
	{
		return Iface_.future ();
	}

	void Dumper::HandleProcessFinished (QProcess *process)
	{
		const auto& stderr = QString::fromUtf8 (process->readAllStandardError ());
		const auto exitCode = process->exitCode ();

		qDebug () << Q_FUNC_INFO
				<< process->exitStatus ()
				<< exitCode
				<< stderr;

		switch (process->exitStatus ())
		{
		case QProcess::CrashExit:
			if (HadError_)
				break;

			HadError_ = true;
			ReportResult (tr ("Dumping process crashed: %1.")
					.arg (stderr.isEmpty () ?
							process->errorString () :
							stderr));
			break;
		case QProcess::NormalExit:
			if (exitCode)
				ReportResult (tr ("Dumping process finished with error: %1 (%2).")
						.arg (stderr)
						.arg (exitCode));
			else if (++FinishedCount_ == 2)
			{
				ReportResult (Finished {});
				deleteLater ();
			}
			break;
		}
	}

	void Dumper::HandleProcessError (const QProcess *process)
	{
		qWarning () << Q_FUNC_INFO
				<< process->error ()
				<< process->errorString ();

		if (HadError_)
			return;

		HadError_ = true;

		if (process->error () == QProcess::FailedToStart)
			ReportResult (tr ("Unable to start dumping process: %1. Do you have sqlite3 installed?")
					.arg (process->errorString ()));
		else
			ReportResult (tr ("Unable to dump the database: %1.")
					.arg (process->errorString ()));
	}

	void Dumper::ReportResult (const Result_t& result)
	{
		Iface_.reportFinished (&result);
	}
}
}
}
