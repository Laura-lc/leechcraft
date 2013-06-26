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

#include "util.h"
#include <functional>
#include <stdexcept>
#include <QString>
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QFile>
#include <QDir>
#include <QTemporaryFile>
#include <QTime>
#include <QSettings>
#include <QTextCodec>
#include <QUrl>
#include <QAction>
#include <QBuffer>
#include <QPainter>
#include <QtDebug>

Q_DECLARE_METATYPE (QList<QModelIndex>);
Q_DECLARE_METATYPE (QVariantList*);

UTIL_API LeechCraft::Util::IDPool<qint64> LeechCraft::Entity::IDPool_;

QString LeechCraft::Util::GetAsBase64Src (const QImage& pix)
{
	QBuffer buf;
	buf.open (QIODevice::ReadWrite);
	pix.save (&buf, "PNG", 100);
	return QString ("data:image/png;base64,%1")
			.arg (QString (buf.buffer ().toBase64 ()));
}

QString LeechCraft::Util::GetUserText (const Entity& p)
{
	QString string = QObject::tr ("Too long to show");
	if (p.Additional_.contains ("UserVisibleName") &&
			p.Additional_ ["UserVisibleName"].canConvert<QString> ())
		string = p.Additional_ ["UserVisibleName"].toString ();
	else if (p.Entity_.canConvert<QByteArray> ())
	{
		QByteArray entity = p.Entity_.toByteArray ();
		if (entity.size () < 100)
			string = QTextCodec::codecForName ("UTF-8")->toUnicode (entity);
	}
	else if (p.Entity_.canConvert<QUrl> ())
	{
		string = p.Entity_.toUrl ().toString ();
		if (string.size () > 100)
			string = string.left (97) + "...";
	}
	else
		string = QObject::tr ("Binary entity");

	if (!p.Mime_.isEmpty ())
		string += QObject::tr ("<br /><br />of type <code>%1</code>").arg (p.Mime_);

	if (!p.Additional_ ["SourceURL"].toUrl ().isEmpty ())
	{
		QString urlStr = p.Additional_ ["SourceURL"].toUrl ().toString ();
		if (urlStr.size () > 63)
			urlStr = urlStr.left (60) + "...";
		string += QObject::tr ("<br />from %1")
			.arg (urlStr);
	}

	return string;
}

QString LeechCraft::Util::MakePrettySize (qint64 sourcesize)
{
	int strNum = 0;
	long double size = sourcesize;
	if (size >= 1024)
	{
		strNum = 1;
		size /= 1024;
	}
	if (size >= 1024)
	{
		strNum = 2;
		size /= 1024;
	}
	if (size >= 1024)
	{
		strNum = 3;
		size /= 1024;
	}

	switch (strNum)
	{
		case 0:
			return QString::number (size, 'f', 1) + QObject::tr (" b");
		case 1:
			return QString::number (size, 'f', 1) + QObject::tr (" KiB");
		case 2:
			return QString::number (size, 'f', 1) + QObject::tr (" MiB");
		case 3:
			return QString::number (size, 'f', 1) + QObject::tr (" GiB");
		default:
			return "unknown";
	}
}

QString LeechCraft::Util::MakeTimeFromLong (ulong time)
{
	int d = time / 86400;
	time -= d * 86400;
	QString result;
	if (d)
		result += QObject::tr ("%n day(s), ", "", d);
	result += QTime (0, 0, 0).addSecs (time).toString ();
	return result;
}

QTranslator* LeechCraft::Util::InstallTranslator (const QString& baseName,
		const QString& prefix,
		const QString& appName)
{
	QString localeName = GetLocaleName ();
	QString filename = prefix;
	filename.append ("_");
	if (!baseName.isEmpty ())
		filename.append (baseName).append ("_");
	filename.append (localeName);

	QTranslator *transl = new QTranslator;
#ifdef Q_OS_WIN32
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QCoreApplication::applicationDirPath () + "/translations"))
#elif defined (Q_OS_MAC)
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QCoreApplication::applicationDirPath () + "/../Resources/translations"))
#elif defined (INSTALL_PREFIX)
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QString (INSTALL_PREFIX "/share/%1/translations").arg (appName)))
#else
	if (transl->load (filename, ":/") ||
			transl->load (filename,
					QString ("/usr/local/share/%1/translations").arg (appName)) ||
			transl->load (filename,
					QString ("/usr/share/%1/translations").arg (appName)))
#endif
	{
		qApp->installTranslator (transl);
		return transl;
	}
	delete transl;

	qWarning () << Q_FUNC_INFO
		<< "could not load translation file for locale"
		<< localeName
		<< filename;
	return 0;
}

QString LeechCraft::Util::GetLocaleName ()
{
	QSettings settings (QCoreApplication::organizationName (),
			QCoreApplication::applicationName ());
	QString localeName = settings.value ("Language", "system").toString ();

	if (localeName == "system")
	{
		localeName = QString (::getenv ("LANG")).left (5);
		if (localeName.isEmpty () || localeName.size () != 5)
			localeName = QLocale::system ().name ();
		localeName = localeName.left (5);
	}

	if (localeName.size () == 2)
	{
		QLocale::Language lang = QLocale (localeName).language ();
		QList<QLocale::Country> cs = QLocale::countriesForLanguage (lang);
		if (cs.isEmpty ())
			localeName += "_00";
		else
			localeName = QLocale (lang, cs.at (0)).name ();
	}

	return localeName;
}

QString LeechCraft::Util::GetInternetLocaleName (const QLocale& locale)
{
#if QT_VERSION >= 0x040800
	if (locale.language () == QLocale::AnyLanguage)
		return "*";
#endif

	QString locStr = locale.name ();
	locStr.replace ('_', '-');
	return locStr;
}

QString LeechCraft::Util::GetLanguage ()
{
	return GetLocaleName ().left (2);
}

QDir LeechCraft::Util::CreateIfNotExists (const QString& opath)
{
	QString path = opath;

	QDir home = QDir::home ();
	path.prepend (".leechcraft/");

	if (!home.exists (path) &&
			!home.mkpath (path))
		throw std::runtime_error (qPrintable (QObject::tr ("Could not create %1")
					.arg (QDir::toNativeSeparators (home.filePath (path)))));

	if (home.cd (path))
		return home;
	else
		throw std::runtime_error (qPrintable (QObject::tr ("Could not cd into %1")
					.arg (QDir::toNativeSeparators (home.filePath (path)))));
}

QDir LeechCraft::Util::GetUserDir (const QString& opath)
{
	QString path = opath;
	QDir home = QDir::home ();
	path.prepend (".leechcraft/");

	if (!home.exists (path))
		throw std::runtime_error (qPrintable (QString ("The specified path doesn't exist: %1")
					.arg (QDir::toNativeSeparators (home.filePath (path)))));

	if (home.cd (path))
		return home;
	else
		throw std::runtime_error (qPrintable (QObject::tr ("Could not cd into %1")
		.arg (QDir::toNativeSeparators (home.filePath (path)))));
}

QString LeechCraft::Util::GetTemporaryName (const QString& pattern)
{
	QTemporaryFile file (QDir::tempPath () + "/" + pattern);
	file.open ();
	QString name = file.fileName ();
	file.close ();
	file.remove ();
	return name;
}

LeechCraft::Entity LeechCraft::Util::MakeEntity (const QVariant& entity,
		const QString& location,
		LeechCraft::TaskParameters tp,
		const QString& mime)
{
	Entity result;
	result.Entity_ = entity;
	result.Location_ = location;
	result.Parameters_ = tp;
	result.Mime_ = mime;
	return result;
}

LeechCraft::Entity LeechCraft::Util::MakeNotification (const QString& header,
		const QString& text, Priority priority)
{
	Entity result = MakeEntity (header,
			QString (),
			AutoAccept | OnlyHandle,
			"x-leechcraft/notification");
	result.Additional_ ["Text"] = text;
	result.Additional_ ["Priority"] = priority;
	return result;
}

LeechCraft::Entity LeechCraft::Util::MakeANCancel (const LeechCraft::Entity& event)
{
	Entity e = MakeNotification (event.Entity_.toString (), QString (), PInfo_);
	e.Additional_ ["org.LC.AdvNotifications.SenderID"] = event.Additional_ ["org.LC.AdvNotifications.SenderID"];
	e.Additional_ ["org.LC.AdvNotifications.EventID"] = event.Additional_ ["org.LC.AdvNotifications.EventID"];
	e.Additional_ ["org.LC.AdvNotifications.EventCategory"] = "org.LC.AdvNotifications.Cancel";
	return e;
}

LeechCraft::Entity LeechCraft::Util::MakeANCancel (const QString& senderId, const QString& eventId)
{
	Entity e = MakeNotification (QString (), QString (), PInfo_);
	e.Additional_ ["org.LC.AdvNotifications.SenderID"] = senderId;
	e.Additional_ ["org.LC.AdvNotifications.EventID"] = eventId;
	e.Additional_ ["org.LC.AdvNotifications.EventCategory"] = "org.LC.AdvNotifications.Cancel";
	return e;
}

QModelIndexList LeechCraft::Util::GetSummarySelectedRows (QObject *sender)
{
	QAction *senderAct = qobject_cast<QAction*> (sender);
	if (!senderAct)
	{
		QString debugString;
		{
			QDebug d (&debugString);
			d << "sender is not a QAction*"
					<< sender;
		}
		throw std::runtime_error (qPrintable (debugString));
	}

	return senderAct->
			property ("SelectedRows").value<QList<QModelIndex>> ();
}

QAction* LeechCraft::Util::CreateSeparator (QObject *parent)
{
	QAction *result = new QAction (parent);
	result->setSeparator (true);
	return result;
}

QVariantList LeechCraft::Util::GetPersistentData (const QList<QVariant>& keys,
		QObject* object)
{
	Entity e = MakeEntity (keys,
			QString (),
			Internal,
			"x-leechcraft/data-persistent-load");
	QVariantList values;
	e.Additional_ ["Values"] = QVariant::fromValue<QVariantList*> (&values);

	QMetaObject::invokeMethod (object,
			"delegateEntity",
			Q_ARG (LeechCraft::Entity, e),
			Q_ARG (int*, 0),
			Q_ARG (QObject**, 0));

	return values;
}

QPixmap LeechCraft::Util::DrawOverlayText (QPixmap px, const QString& text, QFont font, const QPen& pen)
{
	const auto& iconSize = px.size ();

	const auto fontHeight = px.height () * 0.45;
	font.setPixelSize (std::max (6., fontHeight));

	const QFontMetrics fm (font);
	const auto width = fm.width (text) + 2. * px.width () / 10.;
	const auto height = fm.height () + 2. * px.height () / 10.;
	const bool tooSmall = width > iconSize.width ();

	const QRect textRect (iconSize.width () - width, iconSize.height () - height, width, height);

	QPainter p (&px);
	p.setBrush (Qt::white);
	p.setFont (font);
	p.setPen (pen);
	p.setRenderHint (QPainter::Antialiasing);
	p.setRenderHint (QPainter::TextAntialiasing);
	p.setRenderHint (QPainter::HighQualityAntialiasing);
	p.drawRoundedRect (textRect, 4, 4);
	p.drawText (textRect,
			Qt::AlignCenter,
			tooSmall ? "#" : text);
	p.end ();

	return px;
}
