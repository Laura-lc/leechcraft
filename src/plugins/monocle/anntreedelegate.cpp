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

#include "anntreedelegate.h"
#include <cmath>
#include <QTreeView>
#include <QPainter>
#include <QTextDocument>
#include <QApplication>
#include <QResizeEvent>
#include "annmanager.h"

namespace LeechCraft
{
namespace Monocle
{
	const int DocMargin = 4;

	AnnTreeDelegate::AnnTreeDelegate (QTreeView *view, QObject *parent)
	: QStyledItemDelegate { parent }
	, View_ { view }
	{
		View_->viewport ()->installEventFilter (this);
	}

	void AnnTreeDelegate::paint (QPainter *painter,
			const QStyleOptionViewItem& opt, const QModelIndex& index) const
	{
		if (index.data (AnnManager::Role::ItemType) != AnnManager::ItemTypes::AnnItem)
		{
			QStyledItemDelegate::paint (painter, opt, index);
			return;
		}

		painter->save ();
		painter->translate (-View_->indentation (), 0);

		QStyleOptionViewItemV4 option = opt;
		option.rect.setWidth (option.rect.width () + option.decorationSize.width ());

		painter->fillRect (option.rect, QColor { 255, 234, 0 });
		const auto& oldPen = painter->pen ();
		painter->setPen ({ QColor { 255, 213, 0 }, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin });
		painter->drawRect (option.rect);
		painter->setPen (oldPen);

		const auto style = option.widget ? option.widget->style () : QApplication::style ();
		style->drawPrimitive (QStyle::PE_PanelItemViewItem, &option, painter, option.widget);

		painter->save ();
		painter->translate (option.rect.topLeft ());
		GetDoc (index, option.rect.width ())->drawContents (painter);
		painter->restore ();

		painter->restore ();
	}

	QSize AnnTreeDelegate::sizeHint (const QStyleOptionViewItem& opt, const QModelIndex& index) const
	{
		if (index.data (AnnManager::Role::ItemType) != AnnManager::ItemTypes::AnnItem)
			return QStyledItemDelegate::sizeHint (opt, index);

		QStyleOptionViewItemV4 option = opt;
		option.initFrom (View_->viewport ());
		initStyleOption (&option, index);

		auto width = option.rect.width ();

		auto parent = index.parent ();
		while (parent.isValid ())
		{
			width -= View_->indentation ();
			parent = parent.parent ();
		}

		const auto style = option.widget->style ();

		width -= style->pixelMetric (QStyle::PM_LayoutLeftMargin);
		const auto& doc = GetDoc (index, width);
		return
		{
			width,
			static_cast<int> (std::ceil (doc->size ().height ()))
		};
	}

	bool AnnTreeDelegate::eventFilter (QObject *obj, QEvent *event)
	{
		if (event->type () != QEvent::Resize)
			return QStyledItemDelegate::eventFilter (obj, event);

		auto resize = static_cast<QResizeEvent*> (event);
		const auto width = resize->size ().width ();
		if (width == PrevWidth_)
			return QStyledItemDelegate::eventFilter (obj, event);

		PrevWidth_ = width;

		auto model = View_->model ();

		QList<QModelIndex> queue { {} };
		for (int i = 0; i < queue.size (); ++i)
		{
			const auto& idx = queue.at (i);
			for (auto j = 0; j < model->rowCount (idx); ++j)
				queue << model->index (j, 0, idx);
		}

		for (const auto& index : queue)
			if (index.data (AnnManager::Role::ItemType) == AnnManager::ItemTypes::AnnItem)
				emit sizeHintChanged (index);

		return QStyledItemDelegate::eventFilter (obj, event);
	}

	std::shared_ptr<QTextDocument> AnnTreeDelegate::GetDoc (const QModelIndex& index, int width) const
	{
		auto text = new QTextDocument;
		text->setTextWidth (width);
		text->setDocumentMargin (DocMargin);
		text->setDefaultStyleSheet ("* { color: black; }");
		text->setHtml (GetText (index));
		return std::shared_ptr<QTextDocument> { text };
	}

	QString AnnTreeDelegate::GetText (const QModelIndex& index) const
	{
		const auto& ann = index.data (AnnManager::Role::Annotation).value<IAnnotation_ptr> ();

		return QString ("<html><body><strong>%1</strong>: %2<br/>"
				"<strong>%3</strong>: %4<hr/>")
					.arg (tr ("Author"))
					.arg (ann->GetAuthor ())
					.arg (tr ("Date"))
					.arg (ann->GetDate ().toString (Qt::DefaultLocaleShortDate)) +
				Qt::escape (ann->GetText ()) +
				"</body></html>";
	}
}
}
