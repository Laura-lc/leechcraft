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

#include <tuple>
#include <QObject>

class QXmppClient;

class QXmppArchiveManager;
class QXmppBookmarkManager;
class QXmppCallManager;
class QXmppDiscoveryManager;
class QXmppEntityTimeManager;
class QXmppMessageReceiptManager;
class QXmppTransferManager;

namespace LeechCraft::Azoth::Xoox
{
	class ClientConnection;

	using SimpleExtensions = std::tuple<
				class AdHocCommandManager*,
				class JabberSearchManager*,
				class LastActivityManager*,
				class LegacyEntityTimeExt*,
				class MsgArchivingManager*,
				class PingManager*,
				class RIEXManager*,
				class XMPPAnnotationsManager*,
				class XMPPBobManager*,
				class XMPPCaptchaManager*,
				QXmppArchiveManager*,
				QXmppBookmarkManager*,
#ifdef ENABLE_MEDIACALLS
				QXmppCallManager*,
#endif
				QXmppMessageReceiptManager*,
				QXmppTransferManager*
			>;

	using DefaultExtensions = std::tuple<
				QXmppDiscoveryManager*,
				QXmppEntityTimeManager*
			>;

	class ClientConnectionExtensionsManager : public QObject
	{
		DefaultExtensions DefaultExtensions_;
		SimpleExtensions SimpleExtensions_;
	public:
		explicit ClientConnectionExtensionsManager (ClientConnection&, QXmppClient&, QObject* = nullptr);

		template<typename T>
		T& Get ()
		{
			if constexpr (HasExt<T> (DefaultExtensions {}))
				return *std::get<T*> (DefaultExtensions_);
			else if constexpr (HasExt<T> (SimpleExtensions {}))
				return *std::get<T*> (SimpleExtensions_);
			else
				static_assert (std::is_same_v<T, struct Dummy>, "Unable to find the given extension type");
		}
	private:
		template<typename T, typename... ExtsTypes>
		constexpr static bool HasExt (const std::tuple<ExtsTypes...>&)
		{
			return (std::is_same_v<T*, ExtsTypes> || ...);
		}
	};
}
