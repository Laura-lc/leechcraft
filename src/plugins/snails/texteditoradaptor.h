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

#include <interfaces/itexteditor.h>
#include <interfaces/iadvancedplaintexteditor.h>
#include <interfaces/iwkfontssettable.h>

class QTextEdit;

namespace LC
{
namespace Snails
{
	class TextEditorAdaptor : public QObject
							, public IEditorWidget
							, public IAdvancedPlainTextEditor
							, public IWkFontsSettable
	{
		Q_OBJECT
		Q_INTERFACES (IEditorWidget IAdvancedPlainTextEditor IWkFontsSettable)

		QTextEdit * const Edit_;
	public:
		explicit TextEditorAdaptor (QTextEdit*);

		QString GetContents (ContentType type) const override;
		void SetContents (QString contents, ContentType type) override;

		QAction* GetEditorAction (EditorAction) override;
		void AppendAction (QAction*) override;
		void AppendSeparator () override;
		void RemoveAction (QAction*) override;
		void SetBackgroundColor (const QColor&, ContentType) override;
		QWidget* GetWidget () override;
		QObject* GetQObject () override;

		bool FindText (const QString&) override;
		void DeleteSelection () override;

		void SetFontFamily (FontFamily family, const QFont& font) override;
		void SetFontSize (FontSize type, int size) override;
	signals:
		void textChanged () override;
	};
}
}
