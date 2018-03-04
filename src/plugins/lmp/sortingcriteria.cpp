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

#include "sortingcriteria.h"
#include <QVariant>
#include <QtDebug>
#include <util/sll/unreachable.h>

namespace LeechCraft
{
namespace LMP
{
	QList<SortingCriteria> GetAllCriteria ()
	{
		return
		{
			SortingCriteria::Artist,
			SortingCriteria::Year,
			SortingCriteria::Album,
			SortingCriteria::TrackNumber,
			SortingCriteria::TrackTitle,
			SortingCriteria::DirectoryPath,
			SortingCriteria::FileName
		};
	}

	QVariant SaveCriteria (const QList<SortingCriteria>& criteria)
	{
		if (criteria.isEmpty ())
			return false;

		QVariantList result;
		for (const auto crit : criteria)
			result << static_cast<quint8> (crit);
		return result;
	}

	QList<SortingCriteria> LoadCriteria (const QVariant& var)
	{
		QList<SortingCriteria> result;
		for (const auto& critVar : var.toList ())
		{
			const auto val = critVar.value<quint8> ();
			for (auto crit : GetAllCriteria ())
				if (static_cast<decltype (val)> (crit) == val)
				{
					result << crit;
					break;
				}
		}
		return result;
	}

	QString GetCriteriaName (SortingCriteria crit)
	{
		switch (crit)
		{
		case SortingCriteria::Artist:
			return QObject::tr ("Artist");
		case SortingCriteria::Year:
			return QObject::tr ("Year");
		case SortingCriteria::Album:
			return QObject::tr ("Album");
		case SortingCriteria::TrackNumber:
			return QObject::tr ("Track number");
		case SortingCriteria::TrackTitle:
			return QObject::tr ("Title");
		case SortingCriteria::DirectoryPath:
			return QObject::tr ("Directory");
		case SortingCriteria::FileName:
			return QObject::tr ("File name");
		}

		Util::Unreachable ();
	}
}
}
