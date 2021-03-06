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

#include "util.h"
#include <QSize>
#include <QApplication>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QLabel>
#include <QStyleOptionViewItem>
#include <QtDebug>
#include <util/compat/screengeometry.h>

namespace LC
{
namespace Util
{
	QPoint FitRectScreen (QPoint pos, const QSize& size, FitFlags flags, const QPoint& shiftAdd)
	{
		return FitRect (pos, size, Compat::ScreenGeometry (pos), flags, shiftAdd);
	}

	QPoint FitRect (QPoint pos, const QSize& size, const QRect& geometry,
			FitFlags flags, const QPoint& shiftAdd)
	{
		int xDiff = std::max (0, pos.x () + size.width () - (geometry.width () + geometry.x ()));
		if (!xDiff)
			xDiff = std::min (0, pos.x () - geometry.x ());
		int yDiff = std::max (0, pos.y () + size.height () - (geometry.height () + geometry.y ()));
		if (!yDiff)
			yDiff = std::min (0, pos.y () - geometry.y ());

		if (flags & FitFlag::NoOverlap)
		{
			auto overlapFixer = [] (int& diff, int dim)
			{
				if (diff > 0)
					diff = dim > diff ? dim : diff;
			};

			if (QRect (pos - QPoint (xDiff, yDiff), size).contains (pos) && yDiff < size.height ())
				overlapFixer (yDiff, size.height ());
			if (QRect (pos - QPoint (xDiff, yDiff), size).contains (pos) && xDiff < size.width ())
				overlapFixer (xDiff, size.width ());
		}

		if (xDiff)
			pos.rx () -= xDiff + shiftAdd.x ();
		if (yDiff)
			pos.ry () -= yDiff + shiftAdd.y ();

		return pos;
	}

	namespace
	{
		class AADisplayEventFilter : public QObject
		{
			QWidget *Display_;
		public:
			AADisplayEventFilter (QWidget *display)
			: QObject (display)
			, Display_ (display)
			{
			}
		protected:
			bool eventFilter (QObject*, QEvent *event)
			{
				bool shouldClose = false;
				switch (event->type ())
				{
				case QEvent::KeyRelease:
					shouldClose = static_cast<QKeyEvent*> (event)->key () == Qt::Key_Escape;
					break;
				case QEvent::MouseButtonRelease:
					shouldClose = true;
					break;
				default:
					break;
				}

				if (!shouldClose)
					return false;

				QTimer::singleShot (0,
						Display_,
						SLOT (close ()));
				return true;
			}
		};
	}

	QLabel* ShowPixmapLabel (const QPixmap& srcPx, const QPoint& pos)
	{
		const auto& availGeom = Compat::AvailableGeometry (pos).size () * 0.9;

		auto px = srcPx;
		if (px.size ().width () > availGeom.width () ||
			px.size ().height () > availGeom.height ())
			px = px.scaled (availGeom, Qt::KeepAspectRatio, Qt::SmoothTransformation);

		auto label = new QLabel;
		label->setWindowFlags (Qt::Tool);
		label->setAttribute (Qt::WA_DeleteOnClose);
		label->setFixedSize (px.size ());
		label->setPixmap (px);
		label->show ();
		label->activateWindow ();
		label->installEventFilter (new AADisplayEventFilter (label));
		label->move (pos);
		return label;
	}

	QColor TintColors (const QColor& c1, const QColor& c2, double alpha)
	{
		QColor color;
		color.setRedF (alpha * c1.redF () + (1 - alpha) * c2.redF ());
		color.setGreenF (alpha * c1.greenF () + (1 - alpha) * c2.greenF ());
		color.setBlueF (alpha * c1.blueF () + (1 - alpha) * c2.blueF ());
		return color;
	}

	QString ElideProgressBarText (const QString& text, const QStyleOptionViewItem& option)
	{
		return option.fontMetrics.elidedText (text, Qt::ElideRight, option.rect.width ());
	}

	void TintPalette (QWidget *widget, const QColor& color, double alpha, const QList<QPalette::ColorRole>& roles)
	{
		auto palette = widget->palette ();
		for (auto role : roles)
			palette.setColor (role, TintColors (palette.color (role), color, alpha));
		widget->setPalette (palette);
	}

	QString FormatName (const QString& name)
	{
		return "<em>" + name + "</em>";
	}
}
}
