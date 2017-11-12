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

#include "customwebview.h"
#include <QIcon>
#include <QFile>
#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QWebEngineContextMenuData>
#include <QWebChannel>
#include <QPrinter>
#include <QPrintDialog>
#include <QContextMenuEvent>
#include <util/sll/unreachable.h>
#include <interfaces/poshuku/iwebviewhistory.h>
#include <interfaces/poshuku/iproxyobject.h>
#include "customwebpage.h"
#include "findnotification.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace WebEngineView
{
	CustomWebView::CustomWebView (const ICoreProxy_ptr& proxy, IProxyObject *poshukuProxy)
	: Proxy_ { proxy }
	, PoshukuProxy_ { poshukuProxy }
	{
		const auto page = new CustomWebPage { proxy, poshukuProxy, this };
		setPage (page);

		connect (page,
				SIGNAL (loadFinished (bool)),
				this,
				SIGNAL (earliestViewLayout ()));
		connect (page,
				SIGNAL (iconChanged (const QIcon&)),
				this,
				SIGNAL (iconChanged ()));
		connect (page,
				SIGNAL (windowCloseRequested ()),
				this,
				SIGNAL (closeRequested ()));
		connect (page,
				&QWebEnginePage::linkHovered,
				this,
				[this] (const QString& url) { emit linkHovered (url, {}, {}); });
		connect (page,
				&CustomWebPage::webViewCreated,
				this,
				&CustomWebView::webViewCreated);
	}

	void CustomWebView::SurroundingsInitialized ()
	{
		FindDialog_ = new FindNotification { Proxy_, this };
		FindDialog_->hide ();
	}

	QWidget* CustomWebView::GetQWidget ()
	{
		return this;
	}

	QList<QAction*> CustomWebView::GetActions (ActionArea) const
	{
		return {};
	}

	QAction* CustomWebView::GetPageAction (PageAction action) const
	{
#define ACT(x) \
		case PageAction::x: \
			return pageAction (QWebEnginePage::x);

		switch (action)
		{
		ACT (Reload)
		ACT (ReloadAndBypassCache)
		ACT (Stop)
		ACT (Back)
		ACT (Forward)
		ACT (Cut)
		ACT (Copy)
		ACT (Paste)
		ACT (CopyLinkToClipboard)
		ACT (DownloadLinkToDisk)
		ACT (DownloadImageToDisk)
		ACT (CopyImageToClipboard)
		ACT (CopyImageUrlToClipboard)
		ACT (InspectElement)
		case PageAction::OpenImageInNewWindow:
			return nullptr;
		}

#undef ACT

		Util::Unreachable ();
	}

	QString CustomWebView::GetTitle () const
	{
		return title ();
	}

	QUrl CustomWebView::GetUrl () const
	{
		return url ();
	}

	QString CustomWebView::GetHumanReadableUrl () const
	{
		return url ().toDisplayString ();
	}

	QIcon CustomWebView::GetIcon () const
	{
		return icon ();
	}

	void CustomWebView::Load (const QUrl& url, const QString& title)
	{
		if (title.isEmpty ())
			emit titleChanged (tr ("Loading..."));
		else
			emit titleChanged (title);

		load (url);
		emit loadStarted ();
		emit urlChanged (url);
	}

	void CustomWebView::SetContent (const QByteArray& data, const QByteArray& mime, const QUrl& base)
	{
		setContent (data, mime, base);
	}

	void CustomWebView::ToHtml (const std::function<void (QString)>& handler) const
	{
		page ()->toHtml (handler);
	}

	void CustomWebView::EvaluateJS (const QString& js,
			const std::function<void (QVariant)>& handler,
			Util::BitFlags<EvaluateJSFlag> flags)
	{
		QString jsToRun;
		std::function<void (QVariant)> modifiedHandler;
		if (flags & EvaluateJSFlag::RecurseSubframes)
		{
			jsToRun = QString { R"(
					(function(){
						var result = [];
						var f = function(document) {
							var r = __FUNCTION__
							result.push(r);
						};
						f(document);
						var recurse = function(document) {
							var frames = document.querySelectorAll('iframe');
							for (var i = 0; i < frames.length; ++i) {
								try {
									var child = frames[i].contentDocument.children[0];
									f(child);
									recurse(child)
								} catch(e) { console.log("frame read failure: " + e); };
							}
						};
						recurse(document);
						return result;
					})();
				)" };
			jsToRun.replace ("__FUNCTION__", js);

			modifiedHandler = [handler] (const QVariant& result)
			{
				for (const auto& item : result.toList ())
					handler (item);
			};
		}
		else
		{
			jsToRun = js;
			modifiedHandler = handler;
		}

		if (handler)
			page ()->runJavaScript (jsToRun, modifiedHandler);
		else
			page ()->runJavaScript (jsToRun);
	}

	void CustomWebView::AddJavaScriptObject (const QString& id, QObject *object)
	{
		auto channel = page ()->webChannel ();
		if (!channel)
		{
			channel = new QWebChannel;
			page ()->setWebChannel (channel);
		}

		EvaluateJS ("typeof QWebChannel === 'undefined'",
				[=] (const QVariant& res)
				{
					if (res.toBool ())
					{
						QFile file { ":/qtwebchannel/qwebchannel.js" };
						if (!file.open (QIODevice::ReadOnly))
						{
							qWarning () << Q_FUNC_INFO
									<< "unable to open WebChannel setup file"
									<< file.errorString ();
							return;
						}

						page ()->runJavaScript (file.readAll ());
					}

					channel->registerObject (id, object);

					auto js = QString { R"(
								new QWebChannel(qt.webChannelTransport,
									function(channel) {
										window.%1 = channel.objects.%1;
										if (window.%1.init)
											window.%1.init();
									});
							)" }.arg (id);
					page ()->runJavaScript (js);
				},
				{});
	}

	void CustomWebView::Print (bool preview)
	{
		if (preview)
			qWarning () << Q_FUNC_INFO
					<< "printing with preview is not supported yet";

		auto printer = std::make_shared<QPrinter> ();
		QPrintDialog dialog (printer.get (), this);
		dialog.setWindowTitle (tr ("Print web page"));

		if (dialog.exec () != QDialog::Accepted)
			return;

		page ()->print (printer.get (), [printer] (bool) {});
	}

	QPixmap CustomWebView::MakeFullPageSnapshot ()
	{
		// TODO
		return {};
	}

	QPoint CustomWebView::GetScrollPosition () const
	{
		return page ()->scrollPosition ().toPoint ();
	}

	void CustomWebView::SetScrollPosition (const QPoint& point)
	{
		// TODO
	}

	double CustomWebView::GetZoomFactor () const
	{
		return page ()->zoomFactor ();
	}

	void CustomWebView::SetZoomFactor (double factor)
	{
		page ()->setZoomFactor (factor);
	}

	double CustomWebView::GetTextSizeMultiplier () const
	{
		return 1;
	}

	void CustomWebView::SetTextSizeMultiplier (double)
	{
	}

	QString CustomWebView::GetDefaultTextEncoding () const
	{
		return settings ()->defaultTextEncoding ();
	}

	void CustomWebView::SetDefaultTextEncoding (const QString& encoding)
	{
		settings ()->setDefaultTextEncoding (encoding);
	}

	void CustomWebView::InitiateFind (const QString& text)
	{
		if (!text.isEmpty ())
			FindDialog_->SetText (text);
		FindDialog_->show ();
		FindDialog_->setFocus ();
	}

	QMenu* CustomWebView::CreateStandardContextMenu ()
	{
		return page ()->createStandardContextMenu ();
	}

	namespace
	{
		class HistoryWrapper : public IWebViewHistory
		{
			QWebEngineHistory * const History_;

			class Item : public IItem
			{
				const QWebEngineHistoryItem Item_;
				QWebEngineHistory * const History_;
			public:
				Item (const QWebEngineHistoryItem& item, QWebEngineHistory * const history)
				: Item_ { item }
				, History_ { history }
				{
				}

				bool IsValid () const override
				{
					return Item_.isValid ();
				}

				QString GetTitle () const override
				{
					return Item_.title ();
				}

				QUrl GetUrl () const override
				{
					return Item_.url ();
				}

				QIcon GetIcon () const override
				{
					return {};
				}

				void Navigate () override
				{
					History_->goToItem (Item_);
				}
			};
		public:
			HistoryWrapper (QWebEngineHistory *history)
			: History_ { history }
			{
			}

			void Save (QDataStream& out) const override
			{
				out << *History_;
			}

			void Load (QDataStream& in) override
			{
				in >> *History_;
			}

			QList<IItem_ptr> GetItems (Direction dir, int maxItems) const override
			{
				const auto& srcItems = [&]
				{
					switch (dir)
					{
					case Direction::Forward:
						return History_->forwardItems (maxItems);
					case Direction::Backward:
						return History_->backItems (maxItems);
					}

					Util::Unreachable ();
				} ();

				QList<IItem_ptr> result;
				result.reserve (srcItems.size ());

				for (const auto& item : srcItems)
					result << std::make_shared<Item> (item, History_);

				return result;
			}
		};
	}

	IWebViewHistory_ptr CustomWebView::GetHistory ()
	{
		return std::make_shared<HistoryWrapper> (history ());
	}

	void CustomWebView::SetAttribute (Attribute attribute, bool enable)
	{
#define ATTR(x) \
		case Attribute::x: \
			settings ()->setAttribute (QWebEngineSettings::x, enable); \
			break;

		switch (attribute)
		{
		ATTR (AutoLoadImages)
		ATTR (PluginsEnabled)
		ATTR (JavascriptEnabled)
		ATTR (JavascriptCanOpenWindows)
		ATTR (JavascriptCanAccessClipboard)
		ATTR (LocalStorageEnabled)
		ATTR (XSSAuditingEnabled)
		ATTR (HyperlinkAuditingEnabled)
		ATTR (WebGLEnabled)
		ATTR (ScrollAnimatorEnabled)
		}

#undef ATTR
	}

	void CustomWebView::SetFontFamily (FontFamily family, const QFont& font)
	{
		const auto webFamily = static_cast<QWebEngineSettings::FontFamily> (family);
		settings ()->setFontFamily (webFamily, font.family ());
	}

	void CustomWebView::SetFontSize (FontSize type, int size)
	{
		const auto webSize = static_cast<QWebEngineSettings::FontSize> (type);
		settings ()->setFontSize (webSize, size);
	}

	void CustomWebView::SetFontSizeMultiplier (qreal factor)
	{
	}

	QObject* CustomWebView::GetQObject ()
	{
		return this;
	}

	void CustomWebView::contextMenuEvent (QContextMenuEvent *event)
	{
		const auto& data = page ()->contextMenuData ();
		emit contextMenuRequested (event->globalPos (),
				{
					data.isContentEditable (),
					data.selectedText (),
					data.linkUrl (),
					data.linkText (),
					data.mediaUrl (),
					{}
				});
	}
}
}
}
