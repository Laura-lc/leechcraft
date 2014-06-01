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

#include "packetextractor.h"
#include <QtDebug>
#include "exceptions.h"
#include "headers.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Vader
{
namespace Proto
{
	bool PacketExtractor::MayGetPacket () const
	{
#ifdef PROTOCOL_LOGGING
		qDebug () << Q_FUNC_INFO;
#endif
		if (Buffer_.isEmpty ())
			return false;

		try
		{
			QByteArray tmp (Buffer_);
			Header h (tmp);
#ifdef PROTOCOL_LOGGING
			qDebug () << h.DataLength_ << tmp.size ();
#endif
			if (h.DataLength_ > static_cast<quint32> (tmp.size ()))
				return false;
		}
		catch (const TooShortBA&)
		{
			qDebug () << "too short bytearray";
			return false;
		}

#ifdef PROTOCOL_LOGGING
		qDebug () << "may get packet";
#endif

		return true;
	}

	HalfPacket PacketExtractor::GetPacket ()
	{
		Header h (Buffer_);
		const QByteArray& data = Buffer_.left (h.DataLength_);
		if (h.DataLength_)
			Buffer_ = Buffer_.mid (h.DataLength_);
		return { h, data };
	}

	void PacketExtractor::Clear ()
	{
		Buffer_.clear ();
	}

	PacketExtractor& PacketExtractor::operator+= (const QByteArray& ba)
	{
		Buffer_ += ba;
		return *this;
	}
}
}
}
}
