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

#include "liznoo.h"
#include <cmath>
#include <limits>
#include <QIcon>
#include <QAction>
#include <QMessageBox>
#include <QTimer>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ientitymanager.h>
#include <interfaces/entitytesthandleresult.h>
#include <util/util.h>
#include <util/xpc/util.h>
#include <util/sll/either.h>
#include <util/sll/visitor.h>
#include <util/sll/qtutil.h>
#include <util/sll/unreachable.h>
#include <util/threads/futures.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include "xmlsettingsmanager.h"
#include "batteryhistorydialog.h"
#include "quarkmanager.h"
#include "platformobjects.h"

namespace LC
{
namespace Liznoo
{
	const int HistSize = 300;
	const auto UpdateMsecs = 3000;

	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
		qRegisterMetaType<BatteryInfo> ("Liznoo::BatteryInfo");

		Util::InstallTranslator ("liznoo");

		XSD_ = std::make_shared<Util::XmlSettingsDialog> ();
		XSD_->RegisterObject (XmlSettingsManager::Instance (), "liznoosettings.xml");

		Platform_ = std::make_shared<PlatformObjects> (proxy);
		connect (Platform_.get (),
				SIGNAL (batteryInfoUpdated (Liznoo::BatteryInfo)),
				this,
				SLOT (handleBatteryInfo (Liznoo::BatteryInfo)));

		const auto battTimer = new QTimer { this };
		connect (battTimer,
				SIGNAL (timeout ()),
				this,
				SLOT (handleUpdateHistory ()));
		battTimer->start (UpdateMsecs);

		Suspend_ = new QAction (tr ("Suspend"), this);
		connect (Suspend_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleSuspendRequested ()));
		Suspend_->setProperty ("ActionIcon", "system-suspend");

		Hibernate_ = new QAction (tr ("Hibernate"), this);
		connect (Hibernate_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleHibernateRequested ()));
		Hibernate_->setProperty ("ActionIcon", "system-suspend-hibernate");

		connect (XSD_.get (),
				SIGNAL (pushButtonClicked (QString)),
				this,
				SLOT (handlePushButton (QString)));

		const auto qm = new QuarkManager;
		LiznooQuark_ = std::make_shared<QuarkComponent> ("liznoo", "LiznooQuark.qml");
		LiznooQuark_->DynamicProps_.append ({ "Liznoo_proxy", qm });

		connect (Platform_.get (),
				SIGNAL (batteryInfoUpdated (Liznoo::BatteryInfo)),
				qm,
				SLOT (handleBatteryInfo (Liznoo::BatteryInfo)));

		connect (qm,
				SIGNAL (batteryHistoryDialogRequested (QString)),
				this,
				SLOT (handleHistoryTriggered (QString)));
	}

	void Plugin::SecondInit ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Liznoo";
	}

	void Plugin::Release ()
	{
		Platform_.reset ();
	}

	QString Plugin::GetName () const
	{
		return "Liznoo";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("UPower/WinAPI-based power manager.");
	}

	QIcon Plugin::GetIcon () const
	{
		static QIcon icon ("lcicons:/liznoo/resources/images/liznoo.svg");
		return icon;
	}

	Util::XmlSettingsDialog_ptr Plugin::GetSettingsDialog () const
	{
		return XSD_;
	}

	EntityTestHandleResult Plugin::CouldHandle (const Entity& entity) const
	{
		return entity.Mime_ == "x-leechcraft/power-management" ?
				EntityTestHandleResult (EntityTestHandleResult::PIdeal) :
				EntityTestHandleResult ();
	}

	void Plugin::Handle (Entity entity)
	{
		const auto& context = entity.Entity_.toString ();
		if (context == "ScreensaverProhibition")
			Platform_->ProhibitScreensaver (entity.Additional_ ["Enable"].toBool (),
					entity.Additional_ ["ContextID"].toString ());
	}

	QList<QAction*> Plugin::GetActions (ActionsEmbedPlace) const
	{
		return {};
	}

	QMap<QString, QList<QAction*>> Plugin::GetMenuActions () const
	{
		return { { "System", { Suspend_, Hibernate_ } } };
	}

	QuarkComponents_t Plugin::GetComponents () const
	{
		return { LiznooQuark_ };
	}

	void Plugin::CheckNotifications (const BatteryInfo& info)
	{
		auto check = [&info, this] (const std::function<bool (BatteryInfo)>& f)
		{
			if (!Battery2LastInfo_.contains (info.ID_))
				return f (info);

			return f (info) && !f (Battery2LastInfo_ [info.ID_]);
		};

		auto checkPerc = [] (const BatteryInfo& b, const QByteArray& prop)
		{
			if (!XmlSettingsManager::Instance ()->property ("NotifyOn" + prop).toBool ())
				return false;

			return b.Percentage_ <= XmlSettingsManager::Instance ()->
					property (prop + "Level").toInt ();
		};

		const bool isExtremeLow = check ([checkPerc] (const BatteryInfo& b)
				{ return checkPerc (b, "ExtremeLowPower"); });
		const bool isLow = check ([checkPerc] (const BatteryInfo& b)
				{ return checkPerc (b, "LowPower"); });

		const auto iem = Proxy_->GetEntityManager ();
		if (isExtremeLow || isLow)
			iem->HandleEntity (Util::MakeNotification ("Liznoo",
						tr ("Battery charge level is %1%.")
							.arg (static_cast<int> (info.Percentage_)),
						isLow ? Priority::Info : Priority::Warning));

		if (XmlSettingsManager::Instance ()->property ("NotifyOnPowerTransitions").toBool ())
		{
			const bool startedCharging = check ([] (const BatteryInfo& b)
					{ return b.TimeToFull_ && !b.TimeToEmpty_; });
			const bool startedDischarging = check ([] (const BatteryInfo& b)
					{ return !b.TimeToFull_ && b.TimeToEmpty_; });

			if (startedCharging)
				iem->HandleEntity (Util::MakeNotification ("Liznoo",
							tr ("The device started charging."),
							Priority::Info));
			else if (startedDischarging)
				iem->HandleEntity (Util::MakeNotification ("Liznoo",
							tr ("The device started discharging."),
							Priority::Warning));
		}
	}

	void Plugin::handleBatteryInfo (BatteryInfo info)
	{
		CheckNotifications (info);

		Battery2LastInfo_ [info.ID_] = info;
	}

	void Plugin::handleUpdateHistory ()
	{
		for (const auto& pair : Util::Stlize (Battery2LastInfo_))
		{
			const auto& id = pair.first;
			auto pos = Battery2History_.find (id);
			if (pos == Battery2History_.end ())
				pos = Battery2History_.insert (id, BatteryHistoryList { HistSize });
			pos->push_back (BatteryHistory { pair.second });
		}

		for (const auto& pair : Util::Stlize (Battery2Dialog_))
		{
			const auto& id = pair.first;
			pair.second->UpdateHistory (Battery2History_ [id], Battery2LastInfo_ [id]);
		}
	}

	void Plugin::handleHistoryTriggered ()
	{
		const auto& id = sender ()->property ("Liznoo/BatteryID").toString ();
		handleHistoryTriggered (id);
	}

	void Plugin::handleHistoryTriggered (const QString& id)
	{
		if (!Battery2History_.contains (id) ||
				Battery2Dialog_.contains (id))
		{
			if (auto dia = Battery2Dialog_.value (id))
				dia->close ();
			return;
		}

		auto dialog = new BatteryHistoryDialog (HistSize, UpdateMsecs / 1000.);
		dialog->UpdateHistory (Battery2History_ [id], Battery2LastInfo_ [id]);
		dialog->setAttribute (Qt::WA_DeleteOnClose);
		Battery2Dialog_ [id] = dialog;
		connect (dialog,
				&QObject::destroyed,
				this,
				[this, id] { Battery2Dialog_.remove (id); });
		dialog->show ();
		dialog->activateWindow ();
		dialog->raise ();
	}

	namespace
	{
		QString GetReasonString (PlatformObjects::ChangeStateFailed::Reason reason)
		{
			switch (reason)
			{
			case PlatformObjects::ChangeStateFailed::Reason::Unavailable:
				return Plugin::tr ("No platform backend is available.");
			case PlatformObjects::ChangeStateFailed::Reason::PlatformFailure:
				return Plugin::tr ("Platform backend failed.");
			case PlatformObjects::ChangeStateFailed::Reason::Other:
				return Plugin::tr ("Unknown reason.");
			}

			Util::Unreachable ();
		}

		void HandleChangeStateResult (IEntityManager *iem,
				const QFuture<PlatformObjects::ChangeStateResult_t>& future)
		{
			Util::Sequence (nullptr, future) >>
					Util::Visitor
					{
						[] (PlatformObjects::ChangeStateSucceeded) {},
						[iem] (PlatformObjects::ChangeStateFailed f)
						{
							auto msg = GetReasonString (f.Reason_);
							if (!f.ReasonString_.isEmpty ())
								msg += " " + f.ReasonString_;
							const auto& entity = Util::MakeNotification ("Liznoo", msg, Priority::Critical);
							iem->HandleEntity (entity);
						}
					};
		}
	}

	void Plugin::handleSuspendRequested ()
	{
		HandleChangeStateResult (Proxy_->GetEntityManager (),
				Platform_->ChangeState (PowerActions::Platform::State::Suspend));
	}

	void Plugin::handleHibernateRequested ()
	{
		HandleChangeStateResult (Proxy_->GetEntityManager (),
				Platform_->ChangeState (PowerActions::Platform::State::Hibernate));
	}

	void Plugin::handlePushButton (const QString& button)
	{
		const auto res = [&button, this]
		{
			if (button == "TestSleep")
				return Platform_->EmitTestSleep ();
			else if (button == "TestWake")
				return Platform_->EmitTestWakeup ();
			else
				return true;
		} ();

		if (!res)
			QMessageBox::critical (nullptr,
					"LeechCraft",
					tr ("Unable to send test power events."));
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_liznoo, LC::Liznoo::Plugin);
