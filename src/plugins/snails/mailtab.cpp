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

#include "mailtab.h"
#include <QToolBar>
#include <QStandardItemModel>
#include <QTextDocument>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QFileDialog>
#include <QToolButton>
#include <util/util.h>
#include <util/tags/categoryselector.h>
#include <util/sys/extensionsdata.h>
#include <interfaces/core/iiconthememanager.h>
#include "core.h"
#include "storage.h"
#include "mailtreedelegate.h"
#include "mailmodel.h"
#include "viewcolumnsmanager.h"
#include "accountfoldermanager.h"
#include "vmimeconversions.h"
#include "mailsortmodel.h"
#include "headersviewwidget.h"
#include "mailwebpage.h"
#include "foldersmodel.h"
#include "mailmodelsmanager.h"

namespace LeechCraft
{
namespace Snails
{
	MailTab::MailTab (const ICoreProxy_ptr& proxy, const TabClassInfo& tc, QObject *pmt, QWidget *parent)
	: QWidget (parent)
	, Proxy_ (proxy)
	, TabToolbar_ (new QToolBar)
	, MsgToolbar_ (new QToolBar)
	, TabClass_ (tc)
	, PMT_ (pmt)
	, MailSortFilterModel_ (new MailSortModel { this })
	{
		Ui_.setupUi (this);
		//Ui_.MailTreeLay_->insertWidget (0, MsgToolbar_);

		const auto mailWebPage = new MailWebPage { Proxy_, Ui_.MailView_ };
		connect (mailWebPage,
				SIGNAL (attachmentSelected (QByteArray, QStringList, QString)),
				this,
				SLOT (handleAttachment (QByteArray, QStringList, QString)));
		Ui_.MailView_->setPage (mailWebPage);
		Ui_.MailView_->settings ()->setAttribute (QWebSettings::DeveloperExtrasEnabled, true);

		auto colMgr = new ViewColumnsManager (Ui_.MailTree_->header ());
		colMgr->SetStretchColumn (static_cast<int> (MailModel::Column::Subject));
		colMgr->SetDefaultWidths ({
				"Typical sender name and surname",
				"999",
				{},
				{},
				{},
				QDateTime::currentDateTime ().toString () + "  ",
				Util::MakePrettySize (999 * 1024) + "  "
			});

		const auto iconSize = style ()->pixelMetric (QStyle::PM_ListViewIconSize) + 6;
		colMgr->SetDefaultWidth (static_cast<int> (MailModel::Column::StatusIcon), iconSize + 6);
		colMgr->SetDefaultWidth (static_cast<int> (MailModel::Column::AttachIcon), iconSize + 6);
		colMgr->SetSwaps ({
					{
						static_cast<int> (MailModel::Column::From),
						static_cast<int> (MailModel::Column::StatusIcon)
					}
				});

		Ui_.AccountsTree_->setModel (Core::Instance ().GetAccountsModel ());

		MailSortFilterModel_->setDynamicSortFilter (true);
		MailSortFilterModel_->setSortRole (MailModel::MailRole::Sort);
		MailSortFilterModel_->sort (static_cast<int> (MailModel::Column::Date),
				Qt::DescendingOrder);
		Ui_.MailTree_->setItemDelegate (new MailTreeDelegate (this));
		Ui_.MailTree_->setModel (MailSortFilterModel_);

		connect (Ui_.AccountsTree_->selectionModel (),
				SIGNAL (currentChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (handleCurrentAccountChanged (QModelIndex)));
		connect (Ui_.MailTree_->selectionModel (),
				SIGNAL (selectionChanged (QItemSelection, QItemSelection)),
				this,
				SLOT (handleMailSelected ()));

		FillTabToolbarActions ();
	}

	TabClassInfo MailTab::GetTabClassInfo () const
	{
		return TabClass_;
	}

	QObject* MailTab::ParentMultiTabs ()
	{
		return PMT_;
	}

	void MailTab::Remove ()
	{
		emit removeTab (this);
		deleteLater ();
	}

	QToolBar* MailTab::GetToolBar () const
	{
		return TabToolbar_;
	}

	void MailTab::FillCommonActions ()
	{
		const auto fetch = new QAction (tr ("Fetch new mail"), this);
		fetch->setProperty ("ActionIcon", "mail-receive");
		TabToolbar_->addAction (fetch);
		connect (fetch,
				SIGNAL (triggered ()),
				this,
				SLOT (handleFetchNewMail ()));
		TabToolbar_->addAction (fetch);

		const auto refresh = new QAction (tr ("Refresh the folder"), this);
		refresh->setProperty ("ActionIcon", "view-refresh");
		TabToolbar_->addAction (refresh);
		connect (refresh,
				SIGNAL (triggered ()),
				this,
				SLOT (handleRefreshFolder ()));
		TabToolbar_->addAction (refresh);
	}

	void MailTab::FillMailActions ()
	{
		auto registerMailAction = [this] (QObject *obj)
		{
			connect (this,
					SIGNAL (mailActionsEnabledChanged (bool)),
					obj,
					SLOT (setEnabled (bool)));
		};

		const auto msgReply = new QAction (tr ("Reply..."), this);
		msgReply->setProperty ("ActionIcon", "mail-reply-sender");
		connect (msgReply,
				SIGNAL (triggered ()),
				this,
				SLOT (handleReply ()));
		TabToolbar_->addAction (msgReply);
		registerMailAction (msgReply);

		MsgAttachments_ = new QMenu (tr ("Attachments"));
		MsgAttachmentsButton_ = new QToolButton;
		MsgAttachmentsButton_->setProperty ("ActionIcon", "mail-attachment");
		MsgAttachmentsButton_->setMenu (MsgAttachments_);
		MsgAttachmentsButton_->setPopupMode (QToolButton::InstantPopup);
		TabToolbar_->addWidget (MsgAttachmentsButton_);

		TabToolbar_->addSeparator ();

		MsgCopy_ = new QMenu (tr ("Copy messages"));

		const auto msgCopyButton = new QToolButton;
		msgCopyButton->setMenu (MsgCopy_);
		msgCopyButton->setProperty ("ActionIcon", "edit-copy");
		msgCopyButton->setPopupMode (QToolButton::InstantPopup);
		TabToolbar_->addWidget (msgCopyButton);

		registerMailAction (msgCopyButton);

		MsgMove_ = new QMenu (tr ("Move messages"));
		connect (MsgMove_,
				SIGNAL (triggered (QAction*)),
				this,
				SLOT (handleMoveMessages (QAction*)));

		const auto msgMoveButton = new QToolButton;
		msgMoveButton->setMenu (MsgMove_);
		msgMoveButton->setProperty ("ActionIcon", "transform-move");
		msgMoveButton->setPopupMode (QToolButton::InstantPopup);
		TabToolbar_->addWidget (msgMoveButton);

		registerMailAction (msgMoveButton);

		const auto msgMarkUnread = new QAction (tr ("Mark as unread"), this);
		msgMarkUnread->setProperty ("ActionIcon", "mail-mark-unread");
		connect (msgMarkUnread,
				SIGNAL (triggered ()),
				this,
				SLOT (handleMarkMsgUnread ()));
		TabToolbar_->addAction (msgMarkUnread);
		registerMailAction (msgMarkUnread);

		const auto msgRemove = new QAction (tr ("Delete messages"), this);
		msgRemove->setProperty ("ActionIcon", "list-remove");
		connect (msgRemove,
				SIGNAL (triggered ()),
				this,
				SLOT (handleRemoveMsgs ()));
		TabToolbar_->addAction (msgRemove);
		registerMailAction (msgRemove);

		TabToolbar_->addSeparator ();

		const auto msgViewHeaders = new QAction (tr ("View headers"), this);
		msgViewHeaders->setProperty ("ActionIcon", "text-plain");
		connect (msgViewHeaders,
				SIGNAL (triggered ()),
				this,
				SLOT (handleViewHeaders ()));
		TabToolbar_->addAction (msgViewHeaders);
		registerMailAction (msgViewHeaders);

		SetMsgActionsEnabled (false);
	}

	void MailTab::FillTabToolbarActions ()
	{
		FillCommonActions ();
		TabToolbar_->addSeparator ();
		FillMailActions ();
	}

	QList<QByteArray> MailTab::GetSelectedIds () const
	{
		const auto& rows = Ui_.MailTree_->selectionModel ()->selectedRows ();

		QList<QByteArray> ids;
		for (const auto& index : rows)
			ids << index.data (MailModel::MailRole::ID).toByteArray ();

		const auto& currentId = Ui_.MailTree_->currentIndex ()
				.data (MailModel::MailRole::ID).toByteArray ();
		if (!currentId.isEmpty () && !ids.contains (currentId))
			ids << currentId;

		return ids;
	}

	void MailTab::SetMsgActionsEnabled (bool enable)
	{
		emit mailActionsEnabledChanged (enable);
	}

	QList<Folder> MailTab::GetActualFolders () const
	{
		if (!CurrAcc_)
			return {};

		auto folders = CurrAcc_->GetFolderManager ()->GetFolders ();
		const auto& curFolder = MailModel_->GetCurrentFolder ();

		const auto curPos = std::find_if (folders.begin (), folders.end (),
				[&curFolder] (const Folder& other) { return other.Path_ == curFolder; });
		if (curPos != folders.end ())
			folders.erase (curPos);

		return folders;
	}

	void MailTab::handleCurrentAccountChanged (const QModelIndex& idx)
	{
		if (CurrAcc_)
		{
			disconnect (CurrAcc_.get (),
					0,
					this,
					0);
			disconnect (CurrAcc_->GetFolderManager (),
					0,
					this,
					0);

			MailSortFilterModel_->setSourceModel (nullptr);
			MailModel_.reset ();
			CurrAcc_.reset ();

			rebuildOpsToFolders ();
		}

		CurrAcc_ = Core::Instance ().GetAccount (idx);
		if (!CurrAcc_)
			return;

		connect (CurrAcc_.get (),
				SIGNAL (messageBodyFetched (Message_ptr)),
				this,
				SLOT (handleMessageBodyFetched (Message_ptr)));

		MailModel_.reset (CurrAcc_->GetMailModelsManager ()->CreateModel ());
		MailSortFilterModel_->setSourceModel (MailModel_.get ());
		MailSortFilterModel_->setDynamicSortFilter (true);

		if (Ui_.TagsTree_->selectionModel ())
			Ui_.TagsTree_->selectionModel ()->deleteLater ();
		Ui_.TagsTree_->setModel (CurrAcc_->GetFoldersModel ());

		connect (Ui_.TagsTree_->selectionModel (),
				SIGNAL (currentRowChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (handleCurrentTagChanged (QModelIndex)));

		const auto fm = CurrAcc_->GetFolderManager ();
		connect (fm,
				SIGNAL (foldersUpdated ()),
				this,
				SLOT (rebuildOpsToFolders ()));
		rebuildOpsToFolders ();
	}

	namespace
	{
		QString HTMLize (const QList<QPair<QString, QString>>& adds)
		{
			QStringList result;

			for (const auto& pair : adds)
			{
				const bool hasName = !pair.first.isEmpty ();

				QString thisStr;

				if (hasName)
					thisStr += "<span style='address_name'>" + pair.first + "</span> &lt;";

				thisStr += QString ("<span style='address_email'><a href='mailto:%1'>%1</a></span>")
						.arg (pair.second);

				if (hasName)
					thisStr += '>';

				result << thisStr;
			}

			return result.join (", ");
		}
	}

	void MailTab::handleCurrentTagChanged (const QModelIndex& sidx)
	{
		const auto& folder = sidx.data (FoldersModel::Role::FolderPath).toStringList ();
		CurrAcc_->GetMailModelsManager ()->ShowFolder (folder, MailModel_.get ());
		Ui_.MailTree_->setCurrentIndex ({});

		handleMailSelected ();
		rebuildOpsToFolders ();
	}

	namespace
	{
		QString GetStyle ()
		{
			const auto& palette = qApp->palette ();

			auto result = Core::Instance ().GetMsgViewTemplate ();
			result.replace ("$WindowText", palette.color (QPalette::ColorRole::WindowText).name ());
			result.replace ("$Window", palette.color (QPalette::ColorRole::Window).name ());
			result.replace ("$Base", palette.color (QPalette::ColorRole::Base).name ());
			result.replace ("$Text", palette.color (QPalette::ColorRole::Text).name ());
			result.replace ("$LinkVisited", palette.color (QPalette::ColorRole::LinkVisited).name ());
			result.replace ("$Link", palette.color (QPalette::ColorRole::Link).name ());

			result += R"delim(
						html, body {
							width: 100%;
							height: 100%;
							overflow: hidden;
							margin: 0 !important;
						}

						.header {
							top: 0;
							left: 0;
							right: 0;
							position: fixed;
						}

						.body {
							width: 100%;
							bottom: 0;
							position: absolute;
							overflow: auto;
						}
					)delim";

			return result;
		}

		QString AttachmentsToHtml (const Message_ptr& msg, const QList<AttDescr>& attachments)
		{
			if (attachments.isEmpty ())
				return {};

			QString result;
			result += "<div class='attachments'>";

			const auto& extData = Util::ExtensionsData::Instance ();
			for (const auto& attach : attachments)
			{
				result += "<span class='attachment'>";

				QUrl linkUrl { "snails://attachment/" };
				linkUrl.addQueryItem ("msgId", msg->GetFolderID ());
				linkUrl.addQueryItem ("folderId", msg->GetFolders ().value (0).join ("/"));
				linkUrl.addQueryItem ("attName", attach.GetName ());
				const auto& link = linkUrl.toEncoded ();

				result += "<a href='" + link + "'>";

				const auto& mimeType = attach.GetType () + '/' + attach.GetSubType ();
				const auto& icon = extData.GetMimeIcon (mimeType);
				if (!icon.isNull ())
				{
					const auto& iconData = Util::GetAsBase64Src (icon.pixmap (16, 16).toImage ());

					result += "<img class='attachMime' style='float:left' src='" + iconData + "' alt='" + mimeType + "' />";
				}

				result += "</a><span><a href='" + link + "'>";

				result += attach.GetName ();
				if (!attach.GetDescr ().isEmpty ())
					result += " (" + attach.GetDescr () + ")";
				result += " &mdash; " + Util::MakePrettySize (attach.GetSize ());

				result += "</a></span>";
				result += "</span>";
			}

			result += "</div>";
			return result;
		}

		QString ToHtml (const Message_ptr& msg)
		{
			QString html = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">";
			html += "<html xmlns='http://www.w3.org/1999/xhtml'><head><title>Message</title><style>";
			html += GetStyle ();
			html += "</style>";
			html += R"d(
						<script language='javascript'>
							function resizeBody() {
								var headHeight = document.getElementById('msgHeader').offsetHeight;

								var bodyElem = document.getElementById('msgBody');
								bodyElem.style.top = headHeight + "px";
							}
							function setupResize() {
								window.onresize = resizeBody;
								resizeBody();
							}
						</script>
					)d";
			html += "</head><body onload='setupResize()'><header class='header' id='msgHeader'>";
			auto addField = [&html] (const QString& cssClass, const QString& name, const QString& text)
			{
				if (!text.trimmed ().isEmpty ())
					html += "<span class='field " + cssClass + "'><span class='fieldName'>" +
							name + ": </span>" + text + "</span><br />";
			};

			addField ("subject", MailTab::tr ("Subject"), msg->GetSubject ());
			addField ("from", MailTab::tr ("From"), HTMLize ({ msg->GetAddress (Message::Address::From) }));
			addField ("replyTo", MailTab::tr ("Reply to"), HTMLize ({ msg->GetAddress (Message::Address::ReplyTo) }));
			addField ("to", MailTab::tr ("To"), HTMLize (msg->GetAddresses (Message::Address::To)));
			addField ("cc", MailTab::tr ("Copy"), HTMLize (msg->GetAddresses (Message::Address::Cc)));
			addField ("bcc", MailTab::tr ("Blind copy"), HTMLize (msg->GetAddresses (Message::Address::Bcc)));
			addField ("date", MailTab::tr ("Date"), msg->GetDate ().toString ());
			html += AttachmentsToHtml (msg, msg->GetAttachments ());

			const auto& htmlBody = msg->IsFullyFetched () ?
					msg->GetHTMLBody () :
					"<em>" + MailTab::tr ("Fetching the message...") + "</em>";

			html += "</header><div class='body' id='msgBody'>";

			if (!htmlBody.isEmpty ())
				html += htmlBody;
			else
			{
				auto body = msg->GetBody ();
				body.replace ("\r\n", "\n");

				auto lines = body.split ('\n');
				for (auto& line : lines)
				{
					auto escaped = Qt::escape (line);
					if (line.startsWith ('>'))
						line = "<span class='replyPart'>" + escaped + "</span>";
					else
						line = escaped;
				}

				html += "<pre>" + lines.join ("\n") + "</pre>";
			}

			html += "</div>";
			html += "</body></html>";

			return html;
		}
	}

	void MailTab::handleMailSelected ()
	{
		if (!CurrAcc_)
		{
			SetMsgActionsEnabled (false);
			Ui_.MailView_->setHtml ({});
			return;
		}

		const auto& sidx = Ui_.MailTree_->currentIndex ();

		CurrMsg_.reset ();

		if (!sidx.isValid () ||
				!Ui_.MailTree_->selectionModel ()->selectedIndexes ().contains (sidx))
		{
			SetMsgActionsEnabled (false);
			Ui_.MailView_->setHtml ({});
			return;
		}

		const auto& idx = MailSortFilterModel_->mapToSource (sidx);
		const auto& id = idx.sibling (idx.row (), 0).data (MailModel::MailRole::ID).toByteArray ();
		const auto& folder = MailModel_->GetCurrentFolder ();

		Message_ptr msg;
		try
		{
			msg = Core::Instance ().GetStorage ()->LoadMessage (CurrAcc_.get (), folder, id);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to load message"
					<< CurrAcc_->GetID ().toHex ()
					<< id.toHex ()
					<< e.what ();

			SetMsgActionsEnabled (false);
			const QString& html = tr ("<h2>Unable to load mail</h2><em>%1</em>").arg (e.what ());
			Ui_.MailView_->setHtml (html);
			return;
		}

		SetMsgActionsEnabled (true);

		if (!msg->IsFullyFetched ())
			CurrAcc_->FetchWholeMessage (msg);
		else if (!msg->IsRead ())
			CurrAcc_->SetReadStatus (true, { id }, folder);

		const auto& html = ToHtml (msg);

		Ui_.MailView_->setHtml (html);

		MsgAttachments_->clear ();
		MsgAttachmentsButton_->setEnabled (!msg->GetAttachments ().isEmpty ());
		for (const auto& att : msg->GetAttachments ())
		{
			const auto& name = att.GetName () + " (" + Util::MakePrettySize (att.GetSize ()) + ")";
			const auto act = MsgAttachments_->addAction (name,
					this,
					SLOT (handleAttachment ()));
			act->setProperty ("Snails/MsgId", id);
			act->setProperty ("Snails/AttName", att.GetName ());
			act->setProperty ("Snails/Folder", folder);
		}

		CurrMsg_ = msg;
	}

	void MailTab::rebuildOpsToFolders ()
	{
		MsgCopy_->clear ();
		MsgMove_->clear ();

		if (!CurrAcc_)
			return;

		const auto& folders = GetActualFolders ();

		auto setter = [this, &folders] (QMenu *menu, const char *multislot)
		{
			menu->addAction ("Multiple folders...", this, multislot);
			menu->addSeparator ();

			for (const auto& folder : GetActualFolders ())
			{
				const auto& icon = GetFolderIcon (folder.Type_);
				const auto act = menu->addAction (icon, folder.Path_.join ("/"));
				act->setProperty ("Snails/FolderPath", folder.Path_);
			}
		};

		setter (MsgCopy_, SLOT (handleCopyMultipleFolders ()));
		setter (MsgMove_, SLOT (handleMoveMultipleFolders ()));
	}

	void MailTab::handleReply ()
	{
		if (!CurrAcc_ || !CurrMsg_)
			return;

		Core::Instance ().PrepareReplyTab (CurrMsg_, CurrAcc_);
	}

	namespace
	{
		QList<QStringList> RunSelectFolders (const QList<Folder>& folders, const QString& title)
		{
			Util::CategorySelector sel;
			sel.setWindowTitle (title);
			sel.SetCaption (MailTab::tr ("Folders"));

			QStringList folderNames;
			QList<QStringList> folderPaths;
			for (const auto& folder : folders)
			{
				folderNames << folder.Path_.join ("/");
				folderPaths << folder.Path_;
			}

			sel.setPossibleSelections (folderNames, false);
			sel.SetButtonsMode (Util::CategorySelector::ButtonsMode::AcceptReject);

			if (sel.exec () != QDialog::Accepted)
				return {};

			QList<QStringList> selectedPaths;
			for (const auto index : sel.GetSelectedIndexes ())
				selectedPaths << folderPaths [index];
			return selectedPaths;
		}
	}

	void MailTab::handleCopyMultipleFolders ()
	{
		if (!CurrAcc_)
			return;

		const auto& ids = GetSelectedIds ();
		if (ids.isEmpty ())
			return;

		const auto& selectedPaths = RunSelectFolders (GetActualFolders (),
				ids.size () == 1 ?
					tr ("Copy message") :
					tr ("Copy %n message(s)", 0, ids.size ()));

		if (selectedPaths.isEmpty ())
			return;

		CurrAcc_->CopyMessages (ids, MailModel_->GetCurrentFolder (), selectedPaths);
	}

	void MailTab::handleCopyMessages (QAction *action)
	{
		if (!CurrAcc_)
			return;

		const auto& folderPath = action->property ("Snails/FolderPath").toStringList ();
		if (folderPath.isEmpty ())
			return;

		const auto& ids = GetSelectedIds ();
		CurrAcc_->CopyMessages (ids, MailModel_->GetCurrentFolder (), { folderPath });
	}

	void MailTab::handleMoveMultipleFolders ()
	{
		if (!CurrAcc_)
			return;

		const auto& ids = GetSelectedIds ();
		if (ids.isEmpty ())
			return;

		const auto& selectedPaths = RunSelectFolders (GetActualFolders (),
				ids.size () == 1 ?
					tr ("Move message") :
					tr ("Move %n message(s)", 0, ids.size ()));

		if (selectedPaths.isEmpty ())
			return;

		CurrAcc_->MoveMessages (ids, MailModel_->GetCurrentFolder (), selectedPaths);
	}

	void MailTab::handleMoveMessages (QAction *action)
	{
		if (!CurrAcc_)
			return;

		const auto& folderPath = action->property ("Snails/FolderPath").toStringList ();
		if (folderPath.isEmpty ())
			return;

		const auto& ids = GetSelectedIds ();
		CurrAcc_->MoveMessages (ids, MailModel_->GetCurrentFolder (), { folderPath });
	}

	void MailTab::handleMarkMsgUnread ()
	{
		if (!CurrAcc_)
			return;

		const auto& ids = GetSelectedIds ();
		CurrAcc_->SetReadStatus (false, ids, MailModel_->GetCurrentFolder ());
	}

	void MailTab::handleRemoveMsgs ()
	{
		if (!CurrAcc_)
			return;

		const auto& ids = GetSelectedIds ();
		CurrAcc_->DeleteMessages (ids, MailModel_->GetCurrentFolder ());
	}

	void MailTab::handleViewHeaders ()
	{
		if (!CurrAcc_)
			return;

		for (const auto& id : GetSelectedIds ())
		{
			const auto& msg = MailModel_->GetMessage (id);
			if (!msg)
			{
				qWarning () << Q_FUNC_INFO
						<< "no message for id"
						<< id;
				continue;
			}

			const auto& header = msg->GetVmimeHeader ();
			if (!header)
				continue;

			auto widget = new HeadersViewWidget { header, this };
			widget->setAttribute (Qt::WA_DeleteOnClose);
			widget->setWindowFlags (Qt::Dialog);
			widget->setWindowTitle (tr ("Headers for \"%1\"").arg (msg->GetSubject ()));
			widget->show ();
		}
	}

	void MailTab::handleAttachment ()
	{
		if (!CurrAcc_)
			return;

		const auto& name = sender ()->property ("Snails/AttName").toString ();
		const auto& id = sender ()->property ("Snails/MsgId").toByteArray ();
		const auto& folder = sender ()->property ("Snails/Folder").toStringList ();
		handleAttachment (id, folder, name);
	}

	void MailTab::handleAttachment (const QByteArray& id,
			const QStringList& folder, const QString& name)
	{
		const auto& path = QFileDialog::getSaveFileName (0,
				tr ("Save attachment"),
				QDir::homePath () + '/' + name);
		if (path.isEmpty ())
			return;

		const auto& msg = Core::Instance ().GetStorage ()->LoadMessage (CurrAcc_.get (), folder, id);
		CurrAcc_->FetchAttachment (msg, name, path);
	}

	void MailTab::handleFetchNewMail ()
	{
		for (const auto acc : Core::Instance ().GetAccounts ())
			acc->Synchronize ();
	}

	void MailTab::handleRefreshFolder ()
	{
		if (!CurrAcc_)
			return;

		CurrAcc_->Synchronize (MailModel_->GetCurrentFolder (), {});
	}

	void MailTab::handleMessageBodyFetched (Message_ptr msg)
	{
		const QModelIndex& cur = Ui_.MailTree_->currentIndex ();
		if (cur.data (MailModel::MailRole::ID).toByteArray () != msg->GetFolderID ())
			return;

		handleMailSelected ();
	}
}
}
