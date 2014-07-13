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

#include <interfaces/core/icoreproxy.h>
#include <interfaces/structures.h>

class QString;

namespace LeechCraft
{
namespace Azoth
{
class IProxyObject;
class ICLEntry;

namespace MuCommands
{
	bool HandleNames (IProxyObject*, ICLEntry*, const QString&);

	bool ListUrls (IProxyObject*, ICLEntry*, const QString&);

	bool OpenUrl (const ICoreProxy_ptr&, IProxyObject*, ICLEntry*, const QString&, TaskParameters);

	bool ShowVCard (IProxyObject*, ICLEntry*, const QString&);

	bool ShowVersion (IProxyObject*, ICLEntry*, const QString&);

	bool ShowTime (IProxyObject*, ICLEntry*, const QString&);

	bool Disco (IProxyObject*, ICLEntry*, const QString&);

	bool RejoinMuc (IProxyObject*, ICLEntry*, const QString&);

	bool LeaveMuc (IProxyObject*, ICLEntry*, const QString&);

	bool ChangeSubject (IProxyObject*, ICLEntry*, const QString&);

	bool ChangeNick (IProxyObject*, ICLEntry*, const QString&);

	bool Kick (IProxyObject*, ICLEntry*, const QString&);

	bool Ban (IProxyObject*, ICLEntry*, const QString&);

	bool Last (IProxyObject*, ICLEntry*, const QString&);

	bool Ping (IProxyObject*, ICLEntry*, const QString&);
}
}
}
