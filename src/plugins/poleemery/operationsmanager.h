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

#include <memory>
#include <QObject>
#include <QSet>
#include "structures.h"

class QStandardItemModel;
class QAbstractItemModel;
class QModelIndex;

namespace LC
{
namespace Poleemery
{
	class Storage;
	typedef std::shared_ptr<Storage> Storage_ptr;

	class EntriesModel;

	class OperationsManager : public QObject
	{
		Q_OBJECT

		const Storage_ptr Storage_;
		EntriesModel *Model_;

		QSet<QString> KnownCategories_;
	public:
		OperationsManager (Storage_ptr, QObject* = 0);

		void Load ();

		QAbstractItemModel* GetModel () const;

		QList<EntryBase_ptr> GetAllEntries () const;
		QList<EntryWithBalance> GetEntriesWBalance () const;

		QSet<QString> GetKnownCategories () const;

		void AddEntry (EntryBase_ptr);
		void UpdateEntry (EntryBase_ptr);
		void RemoveEntry (const QModelIndex&);
	};
}
}
