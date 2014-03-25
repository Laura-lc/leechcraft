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

#include "q2wproxymodel.h"
#include <QAbstractItemModel>
#include <QVector>
#include <QtDebug>
#include <QDateTime>
#include <QIcon>
#include <Wt/WDateTime>
#include <Wt/WApplication>
#include <util/util.h>
#include "util.h"

namespace LeechCraft
{
namespace Aggregator
{
namespace WebAccess
{
	typedef std::weak_ptr<ModelItem> ModelItem_wtr;
	typedef QVector<ModelItem_ptr> ModelItemsList_t;

	class ModelItem : public std::enable_shared_from_this<ModelItem>
	{
		ModelItem_wtr Parent_;
		ModelItemsList_t Children_;

		QAbstractItemModel *Model_;
		QModelIndex SrcIdx_;
	public:
		ModelItem (QAbstractItemModel *model)
		: Model_ { model }
		{
		}

		ModelItem (QAbstractItemModel *model, const QModelIndex& idx, const ModelItem_wtr& parent)
		: Parent_ { parent }
		, Model_ { model }
		, SrcIdx_ { idx }
		{
		}

		ModelItem_ptr GetChild (int row) const
		{
			return Children_.value (row);
		}

		ModelItem* EnsureChild (int row)
		{
			if (Children_.value (row))
				return Children_.at (row).get ();

			if (Children_.size () <= row)
				Children_.resize (row + 1);

			const auto& childIdx = Model_->index (row, 0, SrcIdx_);
			Children_ [row].reset (new ModelItem { Model_, childIdx, shared_from_this () });
			return Children_.at (row).get ();
		}

		const QModelIndex& GetIndex () const
		{
			return SrcIdx_;
		}

		ModelItem_ptr GetParent () const
		{
			return Parent_.lock ();
		}

		int GetRow (const ModelItem_ptr& item) const
		{
			return Children_.indexOf (item);
		}
	};

	Q2WProxyModel::Q2WProxyModel (QAbstractItemModel *src, Wt::WApplication *app)
	: Wt::WAbstractItemModel { }
	, Src_ { src }
	, Root_ { new ModelItem { src } }
	, App_ { app }
	, Update_ { app }
	{
		const auto type = Qt::DirectConnection;
		connect (src,
				SIGNAL (dataChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (handleDataChanged (QModelIndex, QModelIndex)),
				type);

		connect (src,
				SIGNAL (rowsAboutToBeInserted (QModelIndex, int, int)),
				this,
				SLOT (handleRowsAboutToBeInserted (QModelIndex, int, int)),
				type);
		connect (src,
				SIGNAL (rowsInserted (QModelIndex, int, int)),
				this,
				SLOT (handleRowsInserted (QModelIndex, int, int)),
				type);
		connect (src,
				SIGNAL (rowsAboutToBeRemoved (QModelIndex, int, int)),
				this,
				SLOT (handleRowsAboutToBeRemoved (QModelIndex, int, int)),
				type);
		connect (src,
				SIGNAL (rowsRemoved (QModelIndex, int, int)),
				this,
				SLOT (handleRowsRemoved (QModelIndex, int, int)),
				type);

		connect (src,
				SIGNAL (modelAboutToBeReset ()),
				this,
				SLOT (handleModelAboutToBeReset ()),
				type);
		connect (src,
				SIGNAL (modelReset ()),
				this,
				SLOT (handleModelReset ()),
				type);
	}

	void Q2WProxyModel::SetRoleMappings (const QMap<int, int>& mapping)
	{
		Mapping_ = mapping;
	}

	void Q2WProxyModel::AddDataMorphism (const Morphism_t& morphism)
	{
		Morphisms_ << morphism;
	}

	QModelIndex Q2WProxyModel::MapToSource (const Wt::WModelIndex& index) const
	{
		return W2QIdx (index);
	}

	int Q2WProxyModel::columnCount (const Wt::WModelIndex& parent) const
	{
		return Src_->columnCount (W2QIdx (parent));
	}

	int Q2WProxyModel::rowCount (const Wt::WModelIndex& parent) const
	{
		return Src_->rowCount (W2QIdx (parent));
	}

	Wt::WModelIndex Q2WProxyModel::parent (const Wt::WModelIndex& index) const
	{
		if (!index.isValid ())
			return {};

		if (!index.internalPointer () ||
				index.internalPointer () == Root_.get ())
			return {};

		const auto child = static_cast<ModelItem*> (index.internalPointer ());
		const auto parentItem = child->GetParent ();
		if (parentItem == Root_)
			return {};

		const auto parentParent = parentItem->GetParent ();
		return createIndex (parentParent->GetRow (parentItem), 0, parentItem.get ());
	}

	namespace
	{
		const int IconSize = 16;

		boost::any Variant2Any (const QVariant& var)
		{
			switch (var.type ())
			{
			case QVariant::Bool:
				return var.toBool ();
			case QVariant::DateTime:
				return Wt::WDateTime::fromTime_t (var.toDateTime ().toTime_t ());
			case QVariant::String:
				return ToW (var.toString ());
			case QVariant::Double:
				return var.toDouble ();
			case QVariant::Int:
				return var.toInt ();
			case QVariant::ULongLong:
				return var.toULongLong ();
			case QVariant::Icon:
			{
				const auto& icon = var.value<QIcon> ();
				if (icon.isNull ())
					return {};

				return ToW (Util::GetAsBase64Src (icon.pixmap (IconSize, IconSize).toImage ()));
			}
			default:
				if (var.canConvert<double> ())
					return var.toDouble ();
				if (var.canConvert<int> ())
					return var.toInt ();
				if (var.canConvert<QString> ())
					return ToW (var.toString ());
				return {};
			}
		}
	}

	boost::any Q2WProxyModel::data (const Wt::WModelIndex& index, int role) const
	{
		const auto& src = W2QIdx (index);

		for (const auto& m : Morphisms_)
		{
			const auto& result = m (src, role);
			if (!result.empty ())
				return result;
		}

		return Variant2Any (src.data (WtRole2Qt (role)));
	}

	Wt::WModelIndex Q2WProxyModel::index (int row, int column, const Wt::WModelIndex& parent) const
	{
		if (!hasIndex (row, column, parent))
			return {};

		const auto parentPtr = parent.internalPointer () ?
				static_cast<ModelItem*> (parent.internalPointer ()) :
				Root_.get ();
		return createIndex (row, column, parentPtr->EnsureChild (row));
	}

	boost::any Q2WProxyModel::headerData (int section, Wt::Orientation orientation, int role) const
	{
		if (orientation != Wt::Horizontal || role != Wt::DisplayRole)
			return Wt::WAbstractItemModel::headerData (section, orientation, role);

		return Variant2Any (Src_->headerData (section, Qt::Horizontal, Qt::DisplayRole));
	}

	QModelIndex Q2WProxyModel::W2QIdx (const Wt::WModelIndex& index) const
	{
		if (!index.isValid ())
			return {};

		const auto ptr = index.internalPointer () ?
				static_cast<ModelItem*> (index.internalPointer ()) :
				Root_.get ();
		const auto& srcIdx = ptr->GetIndex ();
		return srcIdx.sibling (index.row (), index.column ());
	}

	Wt::WModelIndex Q2WProxyModel::Q2WIdx (const QModelIndex& index) const
	{
		if (!index.isValid ())
			return {};

		struct Info
		{
			QModelIndex Idx_;
		};
		QList<Info> parentsRows;

		auto parent = index;
		while (parent.isValid ())
		{
			parentsRows.prepend ({ parent });
			parent = parent.parent ();
		}
		parentsRows.removeFirst ();

		auto current = Root_;
		for (const auto& info : parentsRows)
			if (!(current = current->GetChild (info.Idx_.row ())))
				return {};

		return createIndex (index.row (), index.column (), current.get ());
	}

	int Q2WProxyModel::WtRole2Qt (int role) const
	{
		switch (role)
		{
		case Wt::DisplayRole:
			return Qt::DisplayRole;
		case Wt::DecorationRole:
			return Qt::DecorationRole;
		}

		return Mapping_.value (role, -1);
	}

	void Q2WProxyModel::handleDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
	{
		const auto& wtl = Q2WIdx (topLeft);
		const auto& wbr = Q2WIdx (bottomRight);
		if (!wtl.isValid () || !wbr.isValid ())
			return;

		Wt::WApplication::UpdateLock lock { App_ };
		dataChanged () (wtl, wbr);

		Update_ ();
	}

	void Q2WProxyModel::handleRowsAboutToBeInserted (const QModelIndex& srcParent, int first, int last)
	{
		const auto& parent = Q2WIdx (srcParent);
		if (!parent.isValid ())
			return;

		rowsAboutToBeInserted () (parent, first, last);
	}

	void Q2WProxyModel::handleRowsInserted (const QModelIndex& srcParent, int first, int last)
	{
		const auto& parent = Q2WIdx (srcParent);
		if (!parent.isValid ())
			return;

		rowsInserted () (parent, first, last);
	}

	void Q2WProxyModel::handleRowsAboutToBeRemoved (const QModelIndex& srcParent, int first, int last)
	{
		const auto& parent = Q2WIdx (srcParent);
		if (!parent.isValid ())
			return;

		rowsAboutToBeRemoved () (parent, first, last);
	}

	void Q2WProxyModel::handleRowsRemoved (const QModelIndex& srcParent, int first, int last)
	{
		const auto& parent = Q2WIdx (srcParent);
		if (!parent.isValid ())
			return;

		rowsRemoved () (parent, first, last);
	}

	void Q2WProxyModel::handleModelAboutToBeReset ()
	{
		Wt::WApplication::UpdateLock lock { App_ };

		if ((LastModelResetRC_ = rowCount ({})))
			rowsAboutToBeRemoved () ({}, 0, LastModelResetRC_ - 1);
	}

	void Q2WProxyModel::handleModelReset ()
	{
		Wt::WApplication::UpdateLock lock { App_ };

		if (LastModelResetRC_)
			rowsRemoved () ({}, 0, LastModelResetRC_);

		LastModelResetRC_ = 0;

		if (const auto rc = rowCount ({}))
		{
			rowsAboutToBeInserted () ({}, 0, rc - 1);
			rowsInserted () ({}, 0, rc - 1);
		}

		Update_ ();
	}
}
}
}
