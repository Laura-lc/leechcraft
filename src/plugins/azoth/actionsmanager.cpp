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

#include "actionsmanager.h"
#include <functional>
#include <algorithm>
#include <map>
#include <QAction>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QClipboard>
#include <QFileDialog>
#include <util/util.h>
#include <util/defaulthookproxy.h>
#include <util/shortcuts/shortcutmanager.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include "interfaces/azoth/iclentry.h"
#include "interfaces/azoth/imucperms.h"
#include "interfaces/azoth/iadvancedclentry.h"
#include "interfaces/azoth/imucentry.h"
#include "interfaces/azoth/iauthable.h"
#include "interfaces/azoth/iaccount.h"
#include "interfaces/azoth/itransfermanager.h"
#include "interfaces/azoth/iconfigurablemuc.h"
#include "interfaces/azoth/ihavedirectedstatus.h"

#ifdef ENABLE_CRYPT
#include "interfaces/azoth/isupportpgp.h"
#include "pgpkeyselectiondialog.h"
#endif

#include "core.h"
#include "util.h"
#include "xmlsettingsmanager.h"
#include "chattabsmanager.h"
#include "drawattentiondialog.h"
#include "groupeditordialog.h"
#include "shareriexdialog.h"
#include "mucinvitedialog.h"
#include "addcontactdialog.h"
#include "transferjobmanager.h"
#include "bookmarksmanagerdialog.h"
#include "simpledialog.h"
#include "setstatusdialog.h"
#include "filesenddialog.h"
#include "advancedpermchangedialog.h"

typedef std::function<void (LeechCraft::Azoth::ICLEntry*)> EntryActor_f;
Q_DECLARE_METATYPE (EntryActor_f);

typedef QList<LeechCraft::Azoth::ICLEntry*> EntriesList_t;
Q_DECLARE_METATYPE (EntriesList_t);

namespace LeechCraft
{
namespace Azoth
{
	ActionsManager::ActionsManager (QObject *parent)
	: QObject (parent)
	{
	}

	namespace
	{
		void DrawAttention (ICLEntry *entry)
		{
			IAdvancedCLEntry *advEntry = qobject_cast<IAdvancedCLEntry*> (entry->GetQObject ());
			if (!advEntry)
			{
				qWarning () << Q_FUNC_INFO
						<< entry->GetQObject ()
						<< "doesn't implement IAdvancedCLEntry";
				return;
			}

			const auto& vars = entry->Variants ();

			DrawAttentionDialog dia (vars);
			if (dia.exec () != QDialog::Accepted)
				return;

			const auto& variant = dia.GetResource ();
			const auto& text = dia.GetText ();

			QStringList varsToDraw;
			if (!variant.isEmpty ())
				varsToDraw << variant;
			else if (vars.isEmpty ())
				varsToDraw << QString ();
			else
				varsToDraw = vars;

			for (const auto& var : varsToDraw)
				advEntry->DrawAttention (text, var);
		}

		void Rename (ICLEntry *entry)
		{
			const QString& oldName = entry->GetEntryName ();
			const QString& newName = QInputDialog::getText (0,
					ActionsManager::tr ("Rename contact"),
					ActionsManager::tr ("Please enter new name for the contact %1:")
						.arg (oldName),
					QLineEdit::Normal,
					oldName);

			if (newName.isEmpty () ||
					oldName == newName)
				return;

			entry->SetEntryName (newName);
		}

		// We shall probably handle this differently for the list.
		void ChangeGroups (ICLEntry *entry)
		{
			const auto& groups = entry->Groups ();
			const auto& allGroups = Core::Instance ().GetChatGroups ();

			GroupEditorDialog dia (groups, allGroups);
			if (dia.exec () != QDialog::Accepted)
				return;

			entry->SetGroups (dia.GetGroups ());
		}

		void Remove (ICLEntry *entry)
		{
			auto account = qobject_cast<IAccount*> (entry->GetParentAccount ());
			if (!account)
			{
				qWarning () << Q_FUNC_INFO
						<< entry->GetQObject ()
						<< "doesn't return proper IAccount:"
						<< entry->GetParentAccount ();
				return;
			}

			account->RemoveEntry (entry->GetQObject ());
		}

		QString GetMUCRealID (ICLEntry *entry)
		{
			const auto mucEntry = qobject_cast<IMUCEntry*> (entry->GetParentCLEntry ());
			return mucEntry ?
					mucEntry->GetRealID (entry->GetQObject ()) :
					QString ();
		}

		void SendDirectedStatus (ICLEntry *entry)
		{
			auto ihds = qobject_cast<IHaveDirectedStatus*> (entry->GetQObject ());

			QStringList variants (ActionsManager::tr ("All variants"));
			for (const QString& var : entry->Variants ())
				if (!var.isEmpty () &&
						ihds->CanSendDirectedStatusNow (var))
					variants << var;

			QString variant;
			if (variants.size () > 2)
			{
				variant = QInputDialog::getItem (0,
						ActionsManager::tr ("Select variant"),
						ActionsManager::tr ("Select variant to send directed status to:"),
						variants,
						0,
						false);
				if (variant.isEmpty ())
					return;

				if (variant == variants.front ())
					variant.clear ();
			}

			SetStatusDialog dia ((QString ()));
			if (dia.exec () != QDialog::Accepted)
				return;

			const EntryStatus st (dia.GetState (), dia.GetStatusText ());
			ihds->SendDirectedStatus (st, variant);
		}

		void AddContactFromMUC (ICLEntry *entry)
		{
			const auto& nick = entry->GetEntryName ();

			IAccount *account = qobject_cast<IAccount*> (entry->GetParentAccount ());

			AddContactDialog dia (account);
			dia.SetContactID (GetMUCRealID (entry));
			dia.SetNick (nick);
			if (dia.exec () != QDialog::Accepted)
				return;

			dia.GetSelectedAccount ()->RequestAuth (dia.GetContactID (),
						dia.GetReason (),
						dia.GetNick (),
						dia.GetGroups ());
		}

		void CopyMUCParticipantID (ICLEntry *entry)
		{
			const auto& id = GetMUCRealID (entry);
			QApplication::clipboard ()->setText (id, QClipboard::Clipboard);
			QApplication::clipboard ()->setText (id, QClipboard::Selection);
		}

#ifdef ENABLE_CRYPT
		void ManagePGP (ICLEntry *entry)
		{
			QObject *accObj = entry->GetParentAccount ();
			IAccount *acc = qobject_cast<IAccount*> (accObj);
			ISupportPGP *pgp = qobject_cast<ISupportPGP*> (accObj);

			if (!pgp)
			{
				qWarning () << Q_FUNC_INFO
						<< accObj
						<< "doesn't implement ISupportPGP";
				QMessageBox::warning (0,
						"LeechCraft",
						ActionsManager::tr ("The parent account %1 for entry %2 doesn't "
							"support encryption.")
								.arg (acc->GetAccountName ())
								.arg (entry->GetEntryName ()));
				return;
			}

			const QString& str = ActionsManager::tr ("Please select the key for %1 (%2).")
					.arg (entry->GetEntryName ())
					.arg (entry->GetHumanReadableID ());
			PGPKeySelectionDialog dia (str, PGPKeySelectionDialog::TPublic,
					pgp->GetEntryKey (entry->GetQObject ()));
			if (dia.exec () != QDialog::Accepted)
				return;

			const QCA::PGPKey& key = dia.GetSelectedKey ();

			pgp->SetEntryKey (entry->GetQObject (), key);

			QSettings settings (QCoreApplication::organizationName (),
					QCoreApplication::applicationName () + "_Azoth");
			settings.beginGroup ("PublicEntryKeys");
			if (key.isNull ())
				settings.remove (entry->GetEntryID ());
			else
				settings.setValue (entry->GetEntryID (), key.keyId ());
			settings.endGroup ();
		}
#endif

		void ShareRIEX (ICLEntry *entry)
		{
			auto riex = qobject_cast<ISupportRIEX*> (entry->GetParentAccount ());
			if (!riex)
			{
				qWarning () << Q_FUNC_INFO
						<< entry->GetParentAccount ()
						<< "doesn't implement ISupportRIEX";
				return;
			}

			ShareRIEXDialog dia (entry);
			if (dia.exec () != QDialog::Accepted)
				return;

			const bool shareGroups = dia.ShouldSuggestGroups ();

			QList<RIEXItem> items;
			for (ICLEntry *toShare : dia.GetSelectedEntries ())
			{
				RIEXItem item =
				{
					RIEXItem::AAdd,
					toShare->GetHumanReadableID (),
					toShare->GetEntryName (),
					shareGroups ? toShare->Groups () : QStringList ()
				};
				items << item;
			}

			riex->SuggestItems (items, entry->GetQObject (), dia.GetShareMessage ());
		}

		void Invite (ICLEntry *entry)
		{
			auto mucEntry = qobject_cast<IMUCEntry*> (entry->GetQObject ());

			MUCInviteDialog dia (qobject_cast<IAccount*> (entry->GetParentAccount ()));
			if (dia.exec () != QDialog::Accepted)
				return;

			const QString& id = dia.GetID ();
			const QString& msg = dia.GetInviteMessage ();
			if (id.isEmpty ())
				return;

			mucEntry->InviteToMUC (id, msg);
		}

		void Leave (ICLEntry *entry)
		{
			IMUCEntry *mucEntry =
					qobject_cast<IMUCEntry*> (entry->GetQObject ());
			if (!mucEntry)
			{
				qWarning () << Q_FUNC_INFO
						<< "hm, requested leave on an entry"
						<< entry->GetQObject ()
						<< "that doesn't implement IMUCEntry";
				return;
			}

			if (XmlSettingsManager::Instance ().property ("CloseConfOnLeave").toBool ())
			{
				Core::Instance ().GetChatTabsManager ()->CloseChat (entry);
				Q_FOREACH (QObject *partObj, mucEntry->GetParticipants ())
				{
					ICLEntry *partEntry = qobject_cast<ICLEntry*> (partObj);
					if (!partEntry)
					{
						qWarning () << Q_FUNC_INFO
								<< "unable to cast"
								<< partObj
								<< "to ICLEntry";
						continue;
					}

					Core::Instance ().GetChatTabsManager ()->CloseChat (partEntry);
				}
			}

			mucEntry->Leave ();
		}

		void ConfigureMUC (ICLEntry *entry)
		{
			QObject *entryObj = entry->GetQObject ();
			IConfigurableMUC *confMUC = qobject_cast<IConfigurableMUC*> (entryObj);
			if (!confMUC)
				return;

			QWidget *w = confMUC->GetConfigurationWidget ();
			if (!w)
			{
				qWarning () << Q_FUNC_INFO
						<< "empty conf widget"
						<< entryObj;
				return;
			}

			SimpleDialog *dia = new SimpleDialog ();
			dia->setWindowTitle (ActionsManager::tr ("Room configuration"));
			dia->SetWidget (w);
			QObject::connect (dia,
					SIGNAL (accepted ()),
					dia,
					SLOT (deleteLater ()),
					Qt::QueuedConnection);
			dia->show ();
		}

		const std::map<QByteArray, EntryActor_f> BeforeRolesNames
		{
			{
				"openchat",
				[] (ICLEntry *e) { Core::Instance ().GetChatTabsManager ()->OpenChat (e); }
			},
			{ "drawattention", DrawAttention },
			{ "sendfile", [] (ICLEntry *entry) { new FileSendDialog (entry); } },
			{ "sep_afterinitiate", {} },
			{ "rename", Rename },
			{ "changegroups", ChangeGroups },
			{ "remove", Remove },
			{ "sep_afterrostermodify", {} },
			{ "directedpresence", SendDirectedStatus },
			{ "authorization", {} }
		};

		const std::map<QByteArray, EntryActor_f> AfterRolesNames
		{
			{ "sep_afterroles", {} },
			{ "add_contact", AddContactFromMUC },
			{ "copy_muc_id", CopyMUCParticipantID },
			{ "sep_afterjid", {} },
#ifdef ENABLE_CRYPT
			{ "managepgp", ManagePGP },
#endif
			{ "shareRIEX", ShareRIEX },
			{
				"copy_id",
				[] (ICLEntry *e) -> void
				{
					const auto& id = e->GetHumanReadableID ();
					QApplication::clipboard ()->setText (id, QClipboard::Clipboard);
				}
			},
			{ "vcard", [] (ICLEntry *e) { e->ShowInfo (); } },
			{ "invite", Invite },
			{ "leave", Leave },
			{
				"addtobm",
				[] (ICLEntry *e) -> void
				{
					auto dia = new BookmarksManagerDialog ();
					dia->SuggestSaving (e->GetQObject ());
					dia->show ();
				}
			},
			{ "configuremuc", ConfigureMUC },
			{
				"userslist",
				[] (ICLEntry *e) -> void
				{
					auto chatWidget = Core::Instance ().GetChatTabsManager ()->OpenChat (e);
					auto tab = qobject_cast<ChatTab*> (chatWidget);
					tab->ShowUsersList ();
				}
			},
			{ "authorize", AuthorizeEntry },
			{ "denyauth", DenyAuthForEntry }
		};
	}

	QList<QAction*> ActionsManager::GetEntryActions (ICLEntry *entry)
	{
		if (!entry)
			return {};

		if (!Entry2Actions_.contains (entry))
			CreateActionsForEntry (entry);
		UpdateActionsForEntry (entry);

		const QHash<QByteArray, QAction*>& id2action = Entry2Actions_ [entry];
		QList<QAction*> result;

		auto setter = [&result, &id2action, this] (decltype (BeforeRolesNames) pairs) -> void
		{
			for (auto pair : pairs)
			{
				const auto& name = pair.first;
				const auto action = id2action.value (name);
				if (!action)
					continue;

				action->setProperty ("Azoth/EntryActor", QVariant::fromValue (pair.second));
				connect (action,
						SIGNAL (triggered ()),
						this,
						SLOT (handleActoredActionTriggered ()),
						Qt::UniqueConnection);
				result << action;
			}
		};

		setter (BeforeRolesNames);

		if (auto perms =  qobject_cast<IMUCPerms*> (entry->GetParentCLEntry ()))
			for (const auto& permClass : perms->GetPossiblePerms ().keys ())
				result << id2action.value (permClass);

		setter (AfterRolesNames);

		result << entry->GetActions ();

		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		proxy->SetReturnValue (QVariantList ());
		emit hookEntryActionsRequested (proxy, entry->GetQObject ());
		Q_FOREACH (const QVariant& var, proxy->GetReturnValue ().toList ())
		{
			QObject *obj = var.value<QObject*> ();
			QAction *act = qobject_cast<QAction*> (obj);
			if (!act)
				continue;

			result << act;

			proxy.reset (new Util::DefaultHookProxy);
			emit hookEntryActionAreasRequested (proxy, act, entry->GetQObject ());
			Q_FOREACH (const QString& place, proxy->GetReturnValue ().toStringList ())
			{
				if (place == "contactListContextMenu")
					Action2Areas_ [act] << CLEAAContactListCtxtMenu;
				else if (place == "tabContextMenu")
					Action2Areas_ [act] << CLEAATabCtxtMenu;
				else if (place == "applicationMenu")
					Action2Areas_ [act] << CLEAAApplicationMenu;
				else if (place == "toolbar")
					Action2Areas_ [act] << CLEAAToolbar;
				else
					qWarning () << Q_FUNC_INFO
							<< "unknown embed place ID"
							<< place;
			}
		}

		result.removeAll (0);

		Core::Instance ().GetProxy ()->UpdateIconset (result);

		return result;
	}

	QList<QAction*> ActionsManager::CreateEntriesActions (QList<ICLEntry*> entries, QObject *parent)
	{
		entries.removeAll (nullptr);
		if (entries.isEmpty ())
			return {};

		for (auto entry : entries)
		{
			if (!Entry2Actions_.contains (entry))
				CreateActionsForEntry (entry);
			UpdateActionsForEntry (entry);
		}

		QList<QAction*> result;
		auto setter = [&result, &entries, parent, this] (decltype (BeforeRolesNames) pairs) -> void
		{
			for (auto pair : pairs)
			{
				const auto& name = pair.first;

				if (!std::all_of (entries.begin (), entries.end (),
						[this, &name] (ICLEntry *entry) { return Entry2Actions_ [entry].value (name); }))
					continue;

				const auto refAction = Entry2Actions_ [entries.first ()] [name];
				if (!pair.second && !refAction->isSeparator ())
					continue;

				auto action = new QAction (refAction->text (), parent);
				action->setSeparator (refAction->isSeparator ());
				action->setProperty ("Azoth/Entries", QVariant::fromValue (entries));
				action->setProperty ("Azoth/EntryActor", QVariant::fromValue (pair.second));
				action->setProperty ("ActionIcon", refAction->property ("ActionIcon"));
				action->setProperty ("ReferenceAction", QVariant::fromValue<QObject*> (refAction));
				connect (action,
						SIGNAL (triggered ()),
						this,
						SLOT (handleActoredActionTriggered ()));

				result << action;
			}
		};

		setter (BeforeRolesNames);
		setter (AfterRolesNames);

		Core::Instance ().GetProxy ()->UpdateIconset (result);

		return result;
	}

	QList<ActionsManager::CLEntryActionArea> ActionsManager::GetAreasForAction (const QAction *action) const
	{
		return Action2Areas_.value (action, { CLEAAContactListCtxtMenu });
	}

	void ActionsManager::HandleEntryRemoved (ICLEntry *entry)
	{
		auto actions = Entry2Actions_.take (entry);
		Q_FOREACH (QAction *action, actions.values ())
		{
			Action2Areas_.remove (action);
			delete action;
		}

		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		emit hookEntryActionsRemoved (proxy, entry->GetQObject ());
	}

	QString ActionsManager::GetReason (const QString&, const QString& text)
	{
		return QInputDialog::getText (0,
					tr ("Enter reason"),
					text);
	}

	void ActionsManager::ManipulateAuth (const QString& id, const QString& text,
			std::function<void (IAuthable*, const QString&)> func)
	{
		QAction *action = qobject_cast<QAction*> (sender ());
		if (!action)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "is not a QAction";
			return;
		}

		ICLEntry *entry = action->
				property ("Azoth/Entry").value<ICLEntry*> ();
		IAuthable *authable =
				qobject_cast<IAuthable*> (entry->GetQObject ());
		if (!authable)
		{
			qWarning () << Q_FUNC_INFO
					<< entry->GetQObject ()
					<< "doesn't implement IAuthable";
			return;
		}

		QString reason;
		if (action->property ("Azoth/WithReason").toBool ())
		{
			reason = GetReason (id, text.arg (entry->GetEntryName ()));
			if (reason.isEmpty ())
				return;
		}
		func (authable, reason);
	}

	void ActionsManager::CreateActionsForEntry (ICLEntry *entry)
	{
		if (!entry)
			return;

		auto sm = Core::Instance ().GetShortcutManager ();

		QObject *accObj = entry->GetParentAccount ();
		IAccount *acc = qobject_cast<IAccount*> (accObj);

		IAdvancedCLEntry *advEntry = qobject_cast<IAdvancedCLEntry*> (entry->GetQObject ());

		if (Entry2Actions_.contains (entry))
			Q_FOREACH (const QAction *action,
						Entry2Actions_.take (entry).values ())
			{
				Action2Areas_.remove (action);
				delete action;
			}

		QAction *openChat = new QAction (tr ("Open chat"), entry->GetQObject ());
		openChat->setProperty ("ActionIcon", "view-conversation-balloon");
		Entry2Actions_ [entry] ["openchat"] = openChat;
		Action2Areas_ [openChat] << CLEAAContactListCtxtMenu;
		if (entry->GetEntryType () == ICLEntry::ETPrivateChat)
			Action2Areas_ [openChat] << CLEAAChatCtxtMenu;

		auto copyEntryId = new QAction (tr ("Copy full entry ID"), entry->GetQObject ());
		copyEntryId->setProperty ("ActionIcon", "edit-copy");
		Action2Areas_ [copyEntryId] << CLEAAContactListCtxtMenu;
		Entry2Actions_ [entry] ["copy_id"] = copyEntryId;

		if (advEntry)
		{
			QAction *drawAtt = new QAction (tr ("Draw attention..."), entry->GetQObject ());
			drawAtt->setProperty ("ActionIcon", "task-attention");
			Entry2Actions_ [entry] ["drawattention"] = drawAtt;
			Action2Areas_ [drawAtt] << CLEAAContactListCtxtMenu
					<< CLEAAToolbar;
		}

		if (qobject_cast<ITransferManager*> (acc->GetTransferManager ()))
		{
			QAction *sendFile = new QAction (tr ("Send file..."), entry->GetQObject ());
			sendFile->setProperty ("ActionIcon", "mail-attachment");
			Entry2Actions_ [entry] ["sendfile"] = sendFile;
			Action2Areas_ [sendFile] << CLEAAContactListCtxtMenu
					<< CLEAAToolbar;
		}

		QAction *rename = new QAction (tr ("Rename"), entry->GetQObject ());
		rename->setProperty ("ActionIcon", "edit-rename");
		Entry2Actions_ [entry] ["rename"] = rename;
		Action2Areas_ [rename] << CLEAAContactListCtxtMenu
				<< CLEAATabCtxtMenu;

		if (entry->GetEntryFeatures () & ICLEntry::FSupportsGrouping)
		{
			QAction *changeGroups = new QAction (tr ("Change groups..."), entry->GetQObject ());
			changeGroups->setProperty ("ActionIcon", "user-group-properties");
			Entry2Actions_ [entry] ["changegroups"] = changeGroups;
			Action2Areas_ [changeGroups] << CLEAAContactListCtxtMenu;
		}

		if (qobject_cast<IHaveDirectedStatus*> (entry->GetQObject ()))
		{
			QAction *sendDirected = new QAction (tr ("Send directed status..."), entry->GetQObject ());
			sendDirected->setProperty ("ActionIcon", "im-status-message-edit");
			Entry2Actions_ [entry] ["directedpresence"] = sendDirected;
			Action2Areas_ [sendDirected] << CLEAAContactListCtxtMenu;
		}

		if (entry->GetEntryFeatures () & ICLEntry::FSupportsAuth)
		{
			QMenu *authMenu = new QMenu (tr ("Authorization"));
			Entry2Actions_ [entry] ["authorization"] = authMenu->menuAction ();
			Action2Areas_ [authMenu->menuAction ()] << CLEAAContactListCtxtMenu
					<< CLEAATabCtxtMenu;

			QAction *grantAuth = authMenu->addAction (tr ("Grant"),
					this, SLOT (handleActionGrantAuthTriggered ()));
			grantAuth->setProperty ("Azoth/WithReason", false);

			QAction *grantAuthReason = authMenu->addAction (tr ("Grant with reason..."),
					this, SLOT (handleActionGrantAuthTriggered ()));
			grantAuthReason->setProperty ("Azoth/WithReason", true);

			QAction *revokeAuth = authMenu->addAction (tr ("Revoke"),
					this, SLOT (handleActionRevokeAuthTriggered ()));
			revokeAuth->setProperty ("Azoth/WithReason", false);

			QAction *revokeAuthReason = authMenu->addAction (tr ("Revoke with reason..."),
					this, SLOT (handleActionRevokeAuthTriggered ()));
			revokeAuthReason->setProperty ("Azoth/WithReason", true);

			QAction *unsubscribe = authMenu->addAction (tr ("Unsubscribe"),
					this, SLOT (handleActionUnsubscribeTriggered ()));
			unsubscribe->setProperty ("Azoth/WithReason", false);

			QAction *unsubscribeReason = authMenu->addAction (tr ("Unsubscribe with reason..."),
					this, SLOT (handleActionUnsubscribeTriggered ()));
			unsubscribeReason->setProperty ("Azoth/WithReason", true);

			QAction *rerequest = authMenu->addAction (tr ("Rerequest authentication"),
					this, SLOT (handleActionRerequestTriggered ()));
			rerequest->setProperty ("Azoth/WithReason", false);

			QAction *rerequestReason = authMenu->addAction (tr ("Rerequest authentication with reason..."),
					this, SLOT (handleActionRerequestTriggered ()));
			rerequestReason->setProperty ("Azoth/WithReason", true);
		}

#ifdef ENABLE_CRYPT
		if (qobject_cast<ISupportPGP*> (entry->GetParentAccount ()))
		{
			QAction *manageGPG = new QAction (tr ("Manage PGP keys..."), entry->GetQObject ());
			manageGPG->setProperty ("ActionIcon", "document-encrypt");
			Entry2Actions_ [entry] ["managepgp"] = manageGPG;
			Action2Areas_ [manageGPG] << CLEAAContactListCtxtMenu;
		}
#endif

		if (qobject_cast<ISupportRIEX*> (entry->GetParentAccount ()))
		{
			QAction *shareRIEX = new QAction (tr ("Share contacts..."), entry->GetQObject ());
			Entry2Actions_ [entry] ["shareRIEX"] = shareRIEX;
			Action2Areas_ [shareRIEX] << CLEAAContactListCtxtMenu;
		}

		if (entry->GetEntryType () != ICLEntry::ETMUC)
		{
			QAction *vcard = new QAction (tr ("VCard"), entry->GetQObject ());
			vcard->setProperty ("ActionIcon", "text-x-vcard");
			Entry2Actions_ [entry] ["vcard"] = vcard;
			Action2Areas_ [vcard] << CLEAAContactListCtxtMenu
					<< CLEAATabCtxtMenu
					<< CLEAAToolbar
					<< CLEAAChatCtxtMenu;
		}

		IMUCPerms *perms = qobject_cast<IMUCPerms*> (entry->GetParentCLEntry ());
		if (entry->GetEntryType () == ICLEntry::ETPrivateChat)
		{
			if (perms)
			{
				const QMap<QByteArray, QList<QByteArray>>& possible = perms->GetPossiblePerms ();
				for (const QByteArray& permClass : possible.keys ())
				{
					QMenu *changeClass = new QMenu (perms->GetUserString (permClass));
					Entry2Actions_ [entry] [permClass] = changeClass->menuAction ();
					Action2Areas_ [changeClass->menuAction ()] << CLEAAContactListCtxtMenu
							<< CLEAAChatCtxtMenu
							<< CLEAATabCtxtMenu;

					const auto& possibles = possible [permClass];
					auto addPossible = [&possibles, perms, entry, permClass, this] (QMenu *menu, const char *slot)
					{
						for (const QByteArray& perm : possibles)
						{
							QAction *permAct = menu->addAction (perms->GetUserString (perm),
									this,
									slot);
							permAct->setParent (entry->GetQObject ());
							permAct->setCheckable (true);
							permAct->setProperty ("Azoth/TargetPermClass", permClass);
							permAct->setProperty ("Azoth/TargetPerm", perm);
						}
					};

					addPossible (changeClass, SLOT (handleActionPermTriggered ()));

					changeClass->addSeparator ();
					auto advanced = changeClass->addMenu (tr ("Advanced..."));
					advanced->setToolTip (tr ("Allows to set advanced fields like "
							"reason or global flag"));

					addPossible (advanced, SLOT (handleActionPermAdvancedTriggered ()));
				}

				QAction *sep = Util::CreateSeparator (entry->GetQObject ());
				Entry2Actions_ [entry] ["sep_afterroles"] = sep;
				Action2Areas_ [sep] << CLEAAContactListCtxtMenu;
			}

			QAction *addContact = new QAction (tr ("Add to contact list..."), entry->GetQObject ());
			addContact->setProperty ("ActionIcon", "list-add");
			Entry2Actions_ [entry] ["add_contact"] = addContact;
			Action2Areas_ [addContact] << CLEAAContactListCtxtMenu
					<< CLEAATabCtxtMenu
					<< CLEAAChatCtxtMenu;

			QAction *copyId = new QAction (tr ("Copy ID"), entry->GetQObject ());
			copyId->setProperty ("ActionIcon", "edit-copy");
			Entry2Actions_ [entry] ["copy_muc_id"] = copyId;
			Action2Areas_ [copyId] << CLEAAContactListCtxtMenu
					<< CLEAAChatCtxtMenu;

			QAction *sep = Util::CreateSeparator (entry->GetQObject ());
			Entry2Actions_ [entry] ["sep_afterjid"] = sep;
			Action2Areas_ [sep] << CLEAAContactListCtxtMenu;
		}
		else if (entry->GetEntryType () == ICLEntry::ETMUC)
		{
			QAction *invite = new QAction (tr ("Invite..."), entry->GetQObject ());
			invite->setProperty ("ActionIcon", "azoth_invite");
			Entry2Actions_ [entry] ["invite"] = invite;
			Action2Areas_ [invite] << CLEAAContactListCtxtMenu
					<< CLEAATabCtxtMenu;

			QAction *leave = new QAction (tr ("Leave"), entry->GetQObject ());
			leave->setProperty ("ActionIcon", "irc-close-channel");
			Entry2Actions_ [entry] ["leave"] = leave;
			Action2Areas_ [leave] << CLEAAContactListCtxtMenu
					<< CLEAATabCtxtMenu
					<< CLEAAToolbar;
			sm->RegisterAction ("org.LeechCraft.Azoth.LeaveMUC", leave, true);

			QAction *bookmarks = new QAction (tr ("Add to bookmarks"), entry->GetQObject ());
			bookmarks->setProperty ("ActionIcon", "bookmark-new");
			Entry2Actions_ [entry] ["addtobm"] = bookmarks;
			Action2Areas_ [bookmarks] << CLEAAContactListCtxtMenu
					<< CLEAAToolbar;

			QAction *userList = new QAction (tr ("MUC users..."), entry->GetQObject ());
			userList->setProperty ("ActionIcon", "system-users");
			userList->setShortcut (QString ("Ctrl+M"));
			Entry2Actions_ [entry] ["userslist"] = userList;
			Action2Areas_ [userList] << CLEAAToolbar;
			sm->RegisterAction ("org.LeechCraft.Azoth.MUCUsers", userList, true);

			if (qobject_cast<IConfigurableMUC*> (entry->GetQObject ()))
			{
				QAction *configureMUC = new QAction (tr ("Configure MUC..."), this);
				configureMUC->setProperty ("ActionIcon", "configure");
				Entry2Actions_ [entry] ["configuremuc"] = configureMUC;
				Action2Areas_ [configureMUC] << CLEAAContactListCtxtMenu
						<< CLEAAToolbar;
			}
		}
		else if (entry->GetEntryType () == ICLEntry::ETUnauthEntry)
		{
			QAction *authorize = new QAction (tr ("Authorize"), entry->GetQObject ());
			Entry2Actions_ [entry] ["authorize"] = authorize;
			Action2Areas_ [authorize] << CLEAAContactListCtxtMenu;

			QAction *denyAuth = new QAction (tr ("Deny authorization"), entry->GetQObject ());
			Entry2Actions_ [entry] ["denyauth"] = denyAuth;
			Action2Areas_ [denyAuth] << CLEAAContactListCtxtMenu;
		}
		else if (entry->GetEntryType () == ICLEntry::ETChat)
		{
			QAction *remove = new QAction (tr ("Remove"), entry->GetQObject ());
			remove->setProperty ("ActionIcon", "list-remove");
			connect (remove,
					SIGNAL (triggered ()),
					this,
					SLOT (handleActionRemoveTriggered ()));
			Entry2Actions_ [entry] ["remove"] = remove;
			Action2Areas_ [remove] << CLEAAContactListCtxtMenu;
		}

		QAction *sep = Util::CreateSeparator (entry->GetQObject ());
		Entry2Actions_ [entry] ["sep_afterinitiate"] = sep;
		Action2Areas_ [sep] << CLEAAContactListCtxtMenu;
		sep = Util::CreateSeparator (entry->GetQObject ());
		Entry2Actions_ [entry] ["sep_afterrostermodify"] = sep;
		Action2Areas_ [sep] << CLEAAContactListCtxtMenu;

		struct Entrifier
		{
			QVariant Entry_;

			Entrifier (const QVariant& entry)
			: Entry_ (entry)
			{
			}

			void Do (const QList<QAction*>& actions)
			{
				Q_FOREACH (QAction *act, actions)
				{
					act->setProperty ("Azoth/Entry", Entry_);
					act->setParent (Entry_.value<ICLEntry*> ()->GetQObject ());
					QMenu *menu = act->menu ();
					if (menu)
						Do (menu->actions ());
				}
			}
		} entrifier (QVariant::fromValue<ICLEntry*> (entry));
		entrifier.Do (Entry2Actions_ [entry].values ());
	}

	namespace
	{
		void UpdatePermChangeState (QMenu *menu, IMUCPerms *mucPerms, QObject *entryObj, const QByteArray& permClass)
		{
			for (QAction *action : menu->actions ())
			{
				const QByteArray& perm = action->property ("Azoth/TargetPerm").toByteArray ();
				if (action->menu ())
					UpdatePermChangeState (action->menu (), mucPerms, entryObj, permClass);
				else if (!action->isSeparator ())
				{
					action->setEnabled (mucPerms->MayChangePerm (entryObj, permClass, perm));
					action->setChecked (mucPerms->GetPerms (entryObj) [permClass].contains (perm));
				}
			}
		}
	}

	void ActionsManager::UpdateActionsForEntry (ICLEntry *entry)
	{
		if (!entry)
			return;

		IAdvancedCLEntry *advEntry = qobject_cast<IAdvancedCLEntry*> (entry->GetQObject ());

		IAccount *account = qobject_cast<IAccount*> (entry->GetParentAccount ());
		const bool isOnline = account->GetState ().State_ != SOffline;
		if (entry->GetEntryType () != ICLEntry::ETMUC)
		{
			bool enableVCard =
					account->GetAccountFeatures () & IAccount::FCanViewContactsInfoInOffline ||
					isOnline;
			Entry2Actions_ [entry] ["vcard"]->setEnabled (enableVCard);
		}

		Entry2Actions_ [entry] ["rename"]->setEnabled (entry->GetEntryFeatures () & ICLEntry::FSupportsRenames);

		if (advEntry)
		{
			bool suppAtt = advEntry->GetAdvancedFeatures () & IAdvancedCLEntry::AFSupportsAttention;
			Entry2Actions_ [entry] ["drawattention"]->setEnabled (suppAtt);
		}

		if (entry->GetEntryType () == ICLEntry::ETChat)
		{
			Entry2Actions_ [entry] ["remove"]->setEnabled (isOnline);
			if (Entry2Actions_ [entry] ["authorization"])
				Entry2Actions_ [entry] ["authorization"]->setEnabled (isOnline);
		}

		IMUCEntry *thisMuc = qobject_cast<IMUCEntry*> (entry->GetQObject ());
		if (thisMuc)
			Entry2Actions_ [entry] ["invite"]->
					setEnabled (thisMuc->GetMUCFeatures () & IMUCEntry::MUCFCanInvite);

		IMUCPerms *mucPerms = qobject_cast<IMUCPerms*> (entry->GetParentCLEntry ());
		if (entry->GetEntryType () == ICLEntry::ETPrivateChat)
		{
			if (mucPerms)
			{
				const auto& possible = mucPerms->GetPossiblePerms ();
				QObject *entryObj = entry->GetQObject ();

				for (const auto& permClass : possible.keys ())
					UpdatePermChangeState (Entry2Actions_ [entry] [permClass]->menu (),
							mucPerms, entryObj, permClass);
			}

			const QString& realJid = GetMUCRealID (entry);
			Entry2Actions_ [entry] ["add_contact"]->setEnabled (!realJid.isEmpty ());
			Entry2Actions_ [entry] ["copy_muc_id"]->setEnabled (!realJid.isEmpty ());
		}
	}

	void ActionsManager::handleActoredActionTriggered ()
	{
		QAction *action = qobject_cast<QAction*> (sender ());
		if (!action)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "is not a QAction";
			return;
		}

		auto function = action->property ("Azoth/EntryActor").value<EntryActor_f> ();
		if (!function)
		{
			qWarning () << Q_FUNC_INFO
					<< "no function set on the action"
					<< action->text ();
			return;
		}

		const auto& entriesVar = action->property ("Azoth/Entries");
		if (const auto entry = action->property ("Azoth/Entry").value<ICLEntry*> ())
			function (entry);
		else if (entriesVar.isValid ())
			for (const auto& entry : entriesVar.value<EntriesList_t> ())
				function (entry);
		else
			qWarning () << Q_FUNC_INFO
					<< "neither Entry nor Entries properties are set for"
					<< action->text ();
	}

	void ActionsManager::handleActionGrantAuthTriggered()
	{
		ManipulateAuth ("grantauth",
				tr ("Enter reason for granting authorization to %1:"),
				&IAuthable::ResendAuth);
	}

	void ActionsManager::handleActionRevokeAuthTriggered ()
	{
		ManipulateAuth ("revokeauth",
				tr ("Enter reason for revoking authorization from %1:"),
				&IAuthable::RevokeAuth);
	}

	void ActionsManager::handleActionUnsubscribeTriggered ()
	{
		ManipulateAuth ("unsubscribe",
				tr ("Enter reason for unsubscribing from %1:"),
				&IAuthable::Unsubscribe);
	}

	void ActionsManager::handleActionRerequestTriggered ()
	{
		ManipulateAuth ("rerequestauth",
				tr ("Enter reason for rerequesting authorization from %1:"),
				&IAuthable::RerequestAuth);
	}

	namespace
	{
		void ChangePerm (QAction *action, const QString& text = QString (), bool global = false)
		{
			const auto& permClass = action->property ("Azoth/TargetPermClass").toByteArray ();
			const auto& perm = action->property ("Azoth/TargetPerm").toByteArray ();
			if (permClass.isEmpty () || perm.isEmpty ())
			{
				qWarning () << Q_FUNC_INFO
						<< "invalid perms set"
						<< action->property ("Azoth/TargetPermClass")
						<< action->property ("Azoth/TargetPerm");
				return;
			}

			auto entry = action->property ("Azoth/Entry").value<ICLEntry*> ();
			auto muc = qobject_cast<IMUCEntry*> (entry->GetParentCLEntry ());
			auto mucPerms = qobject_cast<IMUCPerms*> (entry->GetParentCLEntry ());
			if (!muc || !mucPerms)
			{
				qWarning () << Q_FUNC_INFO
						<< entry->GetParentCLEntry ()
						<< "doesn't implement IMUCEntry or IMUCPerms";
				return;
			}

			const auto acc = qobject_cast<IAccount*> (entry->GetParentAccount ());
			const auto& realID = muc->GetRealID (entry->GetQObject ());

			mucPerms->SetPerm (entry->GetQObject (), permClass, perm, text);

			if (!global || realID.isEmpty ())
				return;

			for (auto item : acc->GetCLEntries ())
			{
				auto otherMuc = qobject_cast<IMUCEntry*> (item);
				if (!otherMuc || otherMuc == muc)
					continue;

				auto perms = qobject_cast<IMUCPerms*> (item);
				if (!perms)
					continue;

				bool found = false;
				for (auto part : otherMuc->GetParticipants ())
				{
					if (otherMuc->GetRealID (part) != realID)
						continue;

					found = true;

					if (perms->MayChangePerm (part, permClass, perm))
					{
						perms->SetPerm (part, permClass, perm, text);
						continue;
					}

					const auto& body = ActionsManager::tr ("Failed to change %1 for %2 in %3 "
							"due to insufficient permissions.")
							.arg (perms->GetUserString (permClass))
							.arg ("<em>" + realID + "</em>")
							.arg (qobject_cast<ICLEntry*> (item)->GetEntryName ());
					const auto& e = Util::MakeNotification ("Azoth", body, PWarning_);
					Core::Instance ().GetProxy ()->GetEntityManager ()->HandleEntity (e);
				}

				if (!found)
					perms->TrySetPerm (realID, permClass, perm, text);
			}
		}
	}

	void ActionsManager::handleActionPermTriggered ()
	{
		QAction *action = qobject_cast<QAction*> (sender ());
		if (!action)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "is not a QAction";
			return;
		}

		ChangePerm (action);
	}

	void ActionsManager::handleActionPermAdvancedTriggered ()
	{
		QAction *action = qobject_cast<QAction*> (sender ());
		if (!action)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "is not a QAction";
			return;
		}

		const auto& entry = action->property ("Azoth/Entry").value<ICLEntry*> ();
		if (!qobject_cast<IMUCPerms*> (entry->GetParentCLEntry ()))
			return;

		const auto& permClass = action->property ("Azoth/TargetPermClass").toByteArray ();
		const auto& perm = action->property ("Azoth/TargetPerm").toByteArray ();

		AdvancedPermChangeDialog dia (entry, permClass, perm);
		if (dia.exec () != QDialog::Accepted)
			return;
		const auto& text = dia.GetReason ();
		const auto isGlobal = dia.IsGlobal ();

		ChangePerm (action, text, isGlobal);
	}
}
}
