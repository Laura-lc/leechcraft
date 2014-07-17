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

#include "commands.h"
#include <boost/range/adaptor/reversed.hpp>
#include <QStringList>
#include <QtDebug>
#include <QUrl>
#include <QTimer>
#include <util/util.h>
#include <util/xpc/util.h>
#include <util/sll/slotclosure.h>
#include <util/sll/delayedexecutor.h>
#include <interfaces/azoth/iclentry.h>
#include <interfaces/azoth/imucentry.h>
#include <interfaces/azoth/iproxyobject.h>
#include <interfaces/azoth/iaccount.h>
#include <interfaces/azoth/isupportnonroster.h>
#include <interfaces/azoth/imetainfoentry.h>
#include <interfaces/azoth/ihaveentitytime.h>
#include <interfaces/azoth/iprotocol.h>
#include <interfaces/azoth/imucjoinwidget.h>
#include <interfaces/azoth/ihavepings.h>
#include <interfaces/azoth/imucperms.h>
#include <interfaces/azoth/ihavequeriableversion.h>
#include <interfaces/azoth/isupportlastactivity.h>
#include <interfaces/azoth/ihaveservicediscovery.h>
#include <interfaces/core/ientitymanager.h>

namespace LeechCraft
{
namespace Azoth
{
namespace MuCommands
{
	namespace
	{
		void InjectMessage (IProxyObject *azothProxy, ICLEntry *entry,
				const QString& rich)
		{
			const auto entryObj = entry->GetQObject ();
			const auto msgObj = azothProxy->CreateCoreMessage (rich,
					QDateTime::currentDateTime (),
					IMessage::Type::ServiceMessage,
					IMessage::Direction::In,
					entryObj,
					entryObj);
			const auto msg = qobject_cast<IMessage*> (msgObj);
			msg->Store ();
		}
	}

	bool HandleNames (IProxyObject *azothProxy, ICLEntry *entry, const QString&)
	{
		const auto mucEntry = qobject_cast<IMUCEntry*> (entry->GetQObject ());

		QStringList names;
		for (const auto obj : mucEntry->GetParticipants ())
		{
			ICLEntry *entry = qobject_cast<ICLEntry*> (obj);
			if (!entry)
			{
				qWarning () << Q_FUNC_INFO
						<< obj
						<< "doesn't implement ICLEntry";
				continue;
			}
			const QString& name = entry->GetEntryName ();
			if (!name.isEmpty ())
				names << name;
		}
		names.sort ();

		const auto& contents = QObject::tr ("MUC's participants: ") + "<ul><li>" +
				names.join ("</li><li>") + "</li></ul>";
		InjectMessage (azothProxy, entry, contents);

		return true;
	}

	namespace
	{
		QStringList GetAllUrls (IProxyObject *azothProxy, ICLEntry *entry)
		{
			QStringList urls;
			for (const auto msgObj : entry->GetAllMessages ())
			{
				const auto msg = qobject_cast<IMessage*> (msgObj);
				switch (msg->GetMessageType ())
				{
				case IMessage::Type::ChatMessage:
				case IMessage::Type::MUCMessage:
					break;
				default:
					continue;
				}

				urls += azothProxy->FindLinks (msg->GetBody ());
			}

			urls.removeDuplicates ();

			return urls;
		}
	}

	bool ListUrls (IProxyObject *azothProxy, ICLEntry *entry, const QString&)
	{
		const auto& urls = GetAllUrls (azothProxy, entry);

		const auto& body = urls.isEmpty () ?
				QObject::tr ("Sorry, no links found, chat more!") :
				QObject::tr ("Found links:") + "<ol><li>" + urls.join ("</li><li>") + "</li></ol>";
		InjectMessage (azothProxy, entry, body);

		return true;
	}

	bool OpenUrl (const ICoreProxy_ptr& coreProxy, IProxyObject *azothProxy,
			ICLEntry *entry, const QString& text, TaskParameters params)
	{
		const auto& urls = GetAllUrls (azothProxy, entry);

		const auto& split = text.split (' ', QString::SkipEmptyParts).mid (1);

		QList<int> indexes;
		if (split.isEmpty ())
			indexes << urls.size () - 1;
		else if (split.size () == 1 && split.at (0) == "*")
			for (int i = 0; i < urls.size (); ++i)
				indexes << i;

		for (const auto& item : split)
		{
			bool ok = false;
			const auto idx = item.toInt (&ok);
			if (ok)
				indexes << idx - 1;
		}

		const auto iem = coreProxy->GetEntityManager ();
		for (const auto idx : indexes)
		{
			const auto& url = urls.value (idx);
			if (url.isEmpty ())
				continue;

			const auto& entity = Util::MakeEntity (QUrl::fromEncoded (url.toUtf8 ()),
					{}, params | FromUserInitiated);
			iem->HandleEntity (entity);
		}

		return true;
	}

	namespace
	{
		QHash<QString, ICLEntry*> GetParticipants (IMUCEntry *entry)
		{
			if (!entry)
				return {};

			QHash<QString, ICLEntry*> result;
			for (const auto entryObj : entry->GetParticipants ())
			{
				const auto entry = qobject_cast<ICLEntry*> (entryObj);
				if (entry)
					result [entry->GetEntryName ()] = entry;
			}
			return result;
		}

		QStringList ParseNicks (ICLEntry *entry, const QString& text)
		{
			auto split = text
					.section (' ', 1)
					.split ('\n', QString::SkipEmptyParts);

			if (!split.isEmpty ())
				return split;

			const auto& msgs = entry->GetAllMessages ();
			for (const auto msgObj : boost::adaptors::reverse (msgs))
			{
				const auto msg = qobject_cast<IMessage*> (msgObj);
				if (const auto otherPart = qobject_cast<ICLEntry*> (msg->OtherPart ()))
				{
					split << otherPart->GetEntryName ();
					break;
				}
			}

			return split;
		}

		ICLEntry* ResolveEntry (const QString& name, const QHash<QString, ICLEntry*>& context, QObject *accObj)
		{
			if (context.contains (name))
				return context.value (name);

			const auto acc = qobject_cast<IAccount*> (accObj);
			for (const auto entryObj : acc->GetCLEntries ())
			{
				const auto entry = qobject_cast<ICLEntry*> (entryObj);
				if (!entry)
					continue;

				if (entry->GetEntryName () == name || entry->GetHumanReadableID () == name)
					return entry;
			}

			if (const auto isn = qobject_cast<ISupportNonRoster*> (accObj))
				if (const auto entry = qobject_cast<ICLEntry*> (isn->CreateNonRosterItem (name)))
					return entry;

			return nullptr;
		}

		QString FormatRepresentation (const QList<QPair<QString, QVariant>>& repr)
		{
			QStringList strings;

			for (const auto& pair : repr)
			{
				if (pair.second.isNull ())
					continue;

				auto string = "<strong>" + pair.first + ":</strong> ";

				switch (pair.second.type ())
				{
				case QVariant::String:
				{
					const auto& metaStr = pair.second.toString ();
					if (metaStr.isEmpty ())
						continue;

					string += metaStr;
					break;
				}
				case QVariant::Image:
				{
					const auto& image = pair.second.value<QImage> ();
					if (image.isNull ())
						continue;

					const auto& src = Util::GetAsBase64Src (image);
					string += "<img src='" + src + "' alt=''/>";
					break;
				}
				case QVariant::Date:
				{
					const auto& date = pair.second.toDate ();
					if (date.isNull ())
						continue;

					string += date.toString (Qt::DefaultLocaleLongDate);
					break;
				}
				case QVariant::StringList:
				{
					const auto& list = pair.second.toStringList ();
					if (list.isEmpty ())
						continue;

					string += "<ul><li>" + list.join ("</li><li>") + "</li></ul>";
					break;
				}
				default:
					string += "unhandled data type ";
					string += pair.second.typeName ();
					break;
				}

				strings << string;
			}

			if (strings.isEmpty ())
				return {};

			return "<ul><li>" + strings.join ("</li><li>") + "</li></ul>";
		}

		template<typename T, typename F>
		void PerformAction (T action, F fallback, ICLEntry *entry, const QString& text)
		{
			auto nicks = ParseNicks (entry, text);
			if (nicks.isEmpty ())
			{
				if (entry->GetEntryType () == ICLEntry::EntryType::MUC)
					return;
				else
					nicks << entry->GetHumanReadableID ();
			}

			const auto& participants = GetParticipants (qobject_cast<IMUCEntry*> (entry->GetQObject ()));
			for (const auto& name : nicks)
			{
				const auto target = ResolveEntry (name.trimmed (),
						participants, entry->GetParentAccount ());
				if (!target)
				{
					fallback (name);
					continue;
				}

				action (target, name);
			}
		}

		template<typename T>
		void PerformAction (T action, IProxyObject *azothProxy, ICLEntry *entry, const QString& text)
		{
			PerformAction (action,
					[azothProxy, entry] (const QString& name)
					{
						InjectMessage (azothProxy, entry,
								QObject::tr ("Unable to resolve %1.").arg ("<em>" + name + "</em>"));
					},
					entry, text);
		}
	}

	bool ShowVCard (IProxyObject *azothProxy, ICLEntry *entry, const QString& text)
	{
		PerformAction ([azothProxy, entry, text] (ICLEntry *target, const QString& name) -> void
				{
					const auto targetObj = target->GetQObject ();
					const auto imie = qobject_cast<IMetaInfoEntry*> (targetObj);
					if (!imie)
					{
						InjectMessage (azothProxy, entry,
								QObject::tr ("%1 doesn't support extended metainformation.").arg ("<em>" + name + "</em>"));
						return;
					}

					const auto& repr = FormatRepresentation (imie->GetVCardRepresentation ());
					if (repr.isEmpty ())
					{
						InjectMessage (azothProxy, entry,
								name + ": " + QObject::tr ("no information, would wait for next vcard update..."));

						new Util::SlotClosure<Util::DeleteLaterPolicy>
						{
							[azothProxy, entry, text] { ShowVCard (azothProxy, entry, text); },
							targetObj,
							SIGNAL (vcardUpdated ()),
							targetObj
						};
					}
					else
						InjectMessage (azothProxy, entry, name + ":<br/>" + repr);
				},
				azothProxy, entry, text);

		return true;
	}

	namespace
	{
		void ShowVersionVariant (IProxyObject *azothProxy, ICLEntry *entry,
				const QString& name, ICLEntry *target, const QString& var, bool initial)
		{
			const auto ihqv = qobject_cast<IHaveQueriableVersion*> (target->GetQObject ());
			const auto& info = target->GetClientInfo (var);

			QStringList fields;
			auto add = [&fields] (const QString& name, const QString& value)
			{
				if (!value.isEmpty ())
					fields << "<strong>" + name + ":</strong> " + value;
			};

			add (QObject::tr ("Type"), info ["client_type"].toString ());
			add (QObject::tr ("Name"), info ["client_name"].toString ());
			add (QObject::tr ("Version"), info ["client_version"].toString ());
			add (QObject::tr ("OS"), info ["client_os"].toString ());

			if (initial && !info.contains ("client_version") && ihqv)
			{
				const auto pendingObj = ihqv->QueryVersion (var);

				const auto closure = new Util::SlotClosure<Util::DeleteLaterPolicy>
				{
					[name, azothProxy, entry, target, var]
					{
						ShowVersionVariant (azothProxy, entry, name, target, var, false);
					},
					pendingObj,
					SIGNAL (versionReceived ()),
					pendingObj
				};
				QTimer::singleShot (10 * 1000, closure, SLOT (run ()));
				return;
			}

			auto body = QObject::tr ("Client information for %1:")
					.arg (var.isEmpty () && target->Variants ().size () == 1 ?
							name :
							target->GetHumanReadableID () + '/' + var);
			body += fields.isEmpty () ?
					QObject::tr ("no information available.") :
					"<ul><li>" + fields.join ("</li><li>") + "</li></ul>";

			InjectMessage (azothProxy, entry, body);
		}
	}

	bool ShowVersion (IProxyObject *azothProxy, ICLEntry *entry, const QString& text)
	{
		PerformAction ([azothProxy, entry, text] (ICLEntry *target, const QString& name) -> void
				{
					for (const auto& var : target->Variants ())
						ShowVersionVariant (azothProxy, entry, name, target, var, true);
				},
				azothProxy, entry, text);

		return true;
	}

	namespace
	{
		QString FormatTzo (int tzo)
		{
			const auto& time = QTime { 0, 0 }.addSecs (std::abs (tzo));

			auto str = time.toString ("HH:mm");
			str.prepend (tzo >= 0 ? '+' : '-');
			return str;
		}
	}

	bool ShowTime (IProxyObject *azothProxy, ICLEntry *entry, const QString& text)
	{
		PerformAction ([azothProxy, entry, text] (ICLEntry *target, const QString& name) -> void
				{
					const auto targetObj = target->GetQObject ();
					const auto ihet = qobject_cast<IHaveEntityTime*> (targetObj);
					if (!ihet)
					{
						InjectMessage (azothProxy, entry,
								QObject::tr ("%1 does not support querying time.")
										.arg (name));
						return;
					}

					bool shouldUpdate = false;

					QStringList fields;

					const auto& variants = target->Variants ();
					for (const auto& var : variants)
					{
						const auto time = target->GetClientInfo (var)
								.value ("client_time").toDateTime ();
						const auto& varName = var.isEmpty () ?
								name :
								target->GetHumanReadableID () + '/' + var;
						if (!time.isValid ())
						{
							shouldUpdate = true;
							continue;
						}

						const auto tzo = target->GetClientInfo (var)
								.value ("client_tzo").toInt ();

						QString field = QObject::tr ("Current time for %1:")
								.arg (varName);
						field += "<ul><li>";
						field += QObject::tr ("Local time: %1")
								.arg (QLocale {}.toString (time));
						field += "</li><li>";
						field += QObject::tr ("Timezone: %1")
								.arg (FormatTzo (tzo));
						field += "</li><li>";

						const auto& utcTime = time.addSecs (-tzo);
						field += QObject::tr ("UTC time: %1")
								.arg (QLocale {}.toString (utcTime));

						field += "</li></ul>";
						fields << field;
					}

					if (shouldUpdate)
					{
						ihet->UpdateEntityTime ();

						new Util::SlotClosure<Util::DeleteLaterPolicy>
						{
							[azothProxy, entry, text] { ShowTime (azothProxy, entry, text); },
							targetObj,
							SIGNAL (entityTimeUpdated ()),
							targetObj
						};
					}

					if (fields.isEmpty ())
						return;

					const auto& body = "<ul><li>" + fields.join ("</li><li>") + "</li></ul>";
					InjectMessage (azothProxy, entry,
							QObject::tr ("Entity time for %1:").arg (name) + body);
				},
				azothProxy, entry, text);

		return true;
	}

	bool Disco (IProxyObject *azothProxy, ICLEntry *entry, const QString& text)
	{
		const auto accObj = entry->GetParentAccount ();
		const auto ihsd = qobject_cast<IHaveServiceDiscovery*> (accObj);
		if (!ihsd)
		{
			InjectMessage (azothProxy, entry,
					QObject::tr ("%1's account does not support service discovery.")
							.arg (entry->GetEntryName ()));
			return true;
		}

		const auto requestSD = [ihsd, azothProxy, entry, accObj] (const QString& query)
		{
			const auto sessionObj = ihsd->CreateSDSession ();
			const auto session = qobject_cast<ISDSession*> (sessionObj);
			if (!session)
			{
				InjectMessage (azothProxy, entry,
						QObject::tr ("Unable to create service discovery session for %1.")
								.arg ("<em>" + query + "</em>"));
				return;
			}

			session->SetQuery (query);

			QMetaObject::invokeMethod (accObj,
					"gotSDSession",
					Q_ARG (QObject*, sessionObj));
		};

		PerformAction ([requestSD] (ICLEntry *target, const QString&) { requestSD (target->GetHumanReadableID ()); },
				[requestSD] (const QString& name) { requestSD (name); },
				entry, text);

		return true;
	}

	bool RejoinMuc (IProxyObject*, ICLEntry *entry, const QString& text)
	{
		const auto accObj = entry->GetParentAccount ();
		const auto entryObj = entry->GetQObject ();
		const auto mucEntry = qobject_cast<IMUCEntry*> (entryObj);
		if (!mucEntry)
			return false;

		const auto& mucData = mucEntry->GetIdentifyingData ();

		new Util::SlotClosure<Util::NoDeletePolicy>
		{
			[entryObj, accObj, mucData] () -> void
			{
				const auto acc = qobject_cast<IAccount*> (accObj);
				if (acc->GetCLEntries ().contains (entryObj))
					return;

				new Util::DelayedExecutor
				{
					[acc, mucData] ()
					{
						const auto proto = qobject_cast<IProtocol*> (acc->GetParentProtocol ());
						std::unique_ptr<QWidget> jw { proto->GetMUCJoinWidget () };

						const auto imjw = qobject_cast<IMUCJoinWidget*> (jw.get ());
						imjw->SetIdentifyingData (mucData);
						imjw->Join (acc->GetQObject ());
					},
					1000
				};
			},
			accObj,
			SIGNAL (removedCLItems (QList<QObject*>)),
			entryObj
		};

		mucEntry->Leave (text.section (' ', 1));

		return true;
	}

	bool LeaveMuc (IProxyObject*, ICLEntry *entry, const QString& text)
	{
		const auto mucEntry = qobject_cast<IMUCEntry*> (entry->GetQObject ());
		if (!mucEntry)
			return false;

		mucEntry->Leave (text.section (' ', 1));
		return true;
	}

	bool ChangeSubject (IProxyObject *azothProxy, ICLEntry *entry, const QString& text)
	{
		const auto mucEntry = qobject_cast<IMUCEntry*> (entry->GetQObject ());
		if (!mucEntry)
			return false;

		const auto& newSubject = text.section (' ', 1);
		if (newSubject.trimmed ().isEmpty ())
			InjectMessage (azothProxy, entry, mucEntry->GetMUCSubject ());
		else
			mucEntry->SetMUCSubject (newSubject);
		return true;
	}

	bool ChangeNick (IProxyObject*, ICLEntry *entry, const QString& text)
	{
		const auto mucEntry = qobject_cast<IMUCEntry*> (entry->GetQObject ());
		if (!mucEntry)
			return false;

		const auto& newNick = text.section (' ', 1);
		if (newNick.isEmpty ())
			return false;

		mucEntry->SetNick (newNick);
		return true;
	}

	namespace
	{
		void PerformRoleAction (const QPair<QByteArray, QByteArray>& role,
				QObject *mucEntryObj, QString str)
		{
			if (role.first.isEmpty () && role.second.isEmpty ())
				return;

			str = str.trimmed ();
			const int pos = str.lastIndexOf ('|');
			const auto& nick = pos > 0 ? str.left (pos) : str;
			const auto& reason = pos > 0 ? str.mid (pos + 1) : QString ();

			auto mucEntry = qobject_cast<IMUCEntry*> (mucEntryObj);
			auto mucPerms = qobject_cast<IMUCPerms*> (mucEntryObj);

			const auto& parts = mucEntry->GetParticipants ();
			auto partPos = std::find_if (parts.begin (), parts.end (),
					[&nick] (QObject *entryObj) -> bool
					{
						auto entry = qobject_cast<ICLEntry*> (entryObj);
						return entry && entry->GetEntryName () == nick;
					});
			if (partPos == parts.end ())
				return;

			mucPerms->SetPerm (*partPos, role.first, role.second, reason);
		}
	}

	bool Kick (IProxyObject*, ICLEntry *entry, const QString& text)
	{
		const auto mucPerms = qobject_cast<IMUCPerms*> (entry->GetQObject ());
		if (!mucPerms)
			return false;

		PerformRoleAction (mucPerms->GetKickPerm (), entry->GetQObject (), text.section (' ', 1));
		return true;
	}

	bool Ban (IProxyObject*, ICLEntry *entry, const QString& text)
	{
		const auto mucPerms = qobject_cast<IMUCPerms*> (entry->GetQObject ());
		if (!mucPerms)
			return false;

		PerformRoleAction (mucPerms->GetBanPerm (), entry->GetQObject (), text.section (' ', 1));
		return true;
	}

	namespace
	{
		QString GetLastActivityPattern (IPendingLastActivityRequest::Context context)
		{
			switch (context)
			{
			case IPendingLastActivityRequest::Context::Activity:
				return QObject::tr ("Last activity of %1: %2.");
			case IPendingLastActivityRequest::Context::LastConnection:
				return QObject::tr ("Last connection of %1: %2.");
			case IPendingLastActivityRequest::Context::Uptime:
				return QObject::tr ("%1's uptime: %2.");
			}

			qWarning () << Q_FUNC_INFO
					<< "unknown context"
					<< static_cast<int> (context);
			return {};
		}
	}

	bool Last (IProxyObject *azothProxy, ICLEntry *entry, const QString& text)
	{
		const auto handlePending = [azothProxy, entry] (QObject *pending, const QString& name) -> void
		{
			if (!pending)
			{
				InjectMessage (azothProxy, entry,
						QObject::tr ("%1 does not support last activity.").arg (name));
				return;
			}

			new Util::SlotClosure<Util::DeleteLaterPolicy>
			{
				[pending, azothProxy, entry, name] ()
				{
					const auto iplar = qobject_cast<IPendingLastActivityRequest*> (pending);
					const auto& time = Util::MakeTimeFromLong (iplar->GetTime ());
					const auto& pattern = GetLastActivityPattern (iplar->GetContext ());
					InjectMessage (azothProxy, entry, pattern.arg (name).arg (time));
				},
				pending,
				SIGNAL (gotLastActivity ()),
				pending
			};
		};

		PerformAction ([handlePending] (ICLEntry *target, const QString& name) -> void
				{
					const auto isla = qobject_cast<ISupportLastActivity*> (target->GetParentAccount ());
					const auto pending = isla ?
							isla->RequestLastActivity (target->GetQObject (), {}) :
							nullptr;
					handlePending (pending, name);
				},
				[entry, handlePending] (const QString& name) -> void
				{
					const auto isla = qobject_cast<ISupportLastActivity*> (entry->GetParentAccount ());
					const auto pending = isla ?
							isla->RequestLastActivity (name) :
							nullptr;
					handlePending (pending, name);
				},
				entry, text);

		return true;
	}

	bool Invite (IProxyObject *azothProxy, ICLEntry *entry, const QString& text)
	{
		const auto& id = text.section (' ', 1, 1);
		const auto& reason = text.section (' ', 2);

		if (entry->GetEntryType () == ICLEntry::EntryType::MUC)
		{
			const auto invitee = ResolveEntry (id, {}, entry->GetParentAccount ());
			const auto& inviteeId = invitee ?
					invitee->GetHumanReadableID () :
					id;

			const auto mucEntry = qobject_cast<IMUCEntry*> (entry->GetQObject ());
			mucEntry->InviteToMUC (inviteeId, reason);

			InjectMessage (azothProxy, entry, QObject::tr ("Invited %1 to %2.")
					.arg (inviteeId)
					.arg (entry->GetEntryName ()));
		}
		else
		{
			const auto mucEntry = ResolveEntry (id, {}, entry->GetParentAccount ());
			if (!mucEntry)
			{
				InjectMessage (azothProxy, entry,
						QObject::tr ("Unable to resolve multiuser chat for %1.").arg (id));
				return true;
			}

			const auto mucIface = qobject_cast<IMUCEntry*> (mucEntry->GetQObject ());
			if (!mucIface)
			{
				InjectMessage (azothProxy, entry,
						QObject::tr ("%1 is not a multiuser chat.").arg (id));
				return true;
			}

			mucIface->InviteToMUC (entry->GetHumanReadableID (), reason);
			InjectMessage (azothProxy, entry, QObject::tr ("Invited %1 to %2.")
					.arg (entry->GetEntryName ())
					.arg (mucEntry->GetEntryName ()));
		}

		return true;
	}

	bool Pm (IProxyObject *azothProxy, ICLEntry *entry, const QString& text)
	{
		if (entry->GetEntryType () != ICLEntry::EntryType::MUC)
			return false;

		const auto& firstLine = text.section ('\n', 0, 0);
		const auto& message = text.section ('\n', 1);
		const auto& nick = firstLine.section (' ', 1);

		const auto mucEntry = qobject_cast<IMUCEntry*> (entry->GetQObject ());
		const auto part = GetParticipants (mucEntry).value (nick);
		if (!part)
		{
			InjectMessage (azothProxy, entry,
					QObject::tr ("Unable to find participant %1.")
							.arg ("<em>" + nick + "</em>"));
			return true;
		}

		const auto msgObj = part->CreateMessage (IMessage::Type::ChatMessage,
				part->Variants ().value (0), message);
		const auto msg = qobject_cast<IMessage*> (msgObj);
		msg->Send ();

		return true;
	}

	bool Ping (IProxyObject *azothProxy, ICLEntry *entry, const QString& text)
	{
		PerformAction ([azothProxy, entry] (ICLEntry *target, const QString& name) -> void
				{
					const auto targetObj = target->GetQObject ();
					const auto ihp = qobject_cast<IHavePings*> (targetObj);
					if (!ihp)
					{
						InjectMessage (azothProxy, entry,
								QObject::tr ("%1 does not support pinging.").arg (name));
						return;
					}

					const auto reply = ihp->Ping ({});
					new Util::SlotClosure<Util::DeleteLaterPolicy>
					{
						[reply, azothProxy, entry, name] ()
						{
							const auto ipp = qobject_cast<IPendingPing*> (reply);

							InjectMessage (azothProxy, entry,
									QObject::tr ("Pong from %1: %2 ms.")
											.arg (name)
											.arg (ipp->GetTimeout ()));
						},
						reply,
						SIGNAL (replyReceived (int)),
						reply
					};
				},
				azothProxy, entry, text);

		return true;
	}
}
}
}
