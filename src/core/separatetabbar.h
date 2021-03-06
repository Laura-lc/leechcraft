/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
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

#include <QTabBar>
#include <QHash>

namespace LC
{
	class MainWindow;
	class SeparateTabWidget;

	struct TabInfo;

	class SeparateTabBar : public QTabBar
	{
		Q_OBJECT

		MainWindow *Window_ = nullptr;

		bool InMove_ = false;
		SeparateTabWidget *TabWidget_ = nullptr;

		QPoint DragStartPos_;

		QWidget *AddTabButton_ = nullptr;
		mutable QVector<int> ComputedWidths_;
	public:
		explicit SeparateTabBar (QWidget* = 0);

		void SetWindow (MainWindow*);

		void SetTabClosable (int index, bool closable, QWidget *closeButton = 0);
		void SetTabWidget (SeparateTabWidget*);

		void SetAddTabButton (QWidget*);

		QTabBar::ButtonPosition GetCloseButtonPosition () const;

		void SetInMove (bool inMove);
	private:
		QTabBar::ButtonPosition GetAntiCloseButtonPosition () const;

		QVector<TabInfo> GetTabInfos () const;
		void UpdateComputedWidths () const;
	private slots:
		void toggleCloseButtons () const;
	protected:
		void tabLayoutChange ();
		QSize tabSizeHint (int) const;

		void mouseReleaseEvent (QMouseEvent*);

		void mousePressEvent (QMouseEvent*);
		void mouseMoveEvent (QMouseEvent*);
		void dragEnterEvent (QDragEnterEvent*);
		void dragMoveEvent (QDragMoveEvent*);
		void dropEvent (QDropEvent*);

		void mouseDoubleClickEvent (QMouseEvent*);

		void tabInserted (int);
		void tabRemoved (int);
	signals:
		void addDefaultTab ();
		void tabWasInserted (int);
		void tabWasRemoved (int);
		void releasedMouseAfterMove (int index);
	};
}
