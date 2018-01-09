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

#include <QDateTime>
#include <QUrl>
#include <QStringList>
#include <QtPlugin>
#include <util/sll/eitherfwd.h>

template<typename>
class QFuture;

namespace Media
{
	/** @brief Enum describing if and how an event is attended by user.
	 *
	 * @sa EventInfo, IEventsProvider
	 */
	enum class EventAttendType
	{
		/** @brief The user won't attend this event.
		 */
		None,

		/** @brief The user is interested and maybe will attend the event.
		 */
		Maybe,

		/** @brief The user surely will attend the event.
		 */
		Surely
	};

	/** @brief A structure describing an event like a gig or a festival.
	 *
	 * @sa EventInfo, IEventsProvider
	 */
	struct EventInfo
	{
		/** @brief The internal ID of the event.
		 */
		qint64 ID_;

		/** @brief The name of the event.
		 */
		QString Name_;

		/** @brief The description of the event.
		 */
		QString Description_;

		/** @brief The date the event will happen.
		 */
		QDateTime Date_;

		/** @brief The URL of a page describing the event in more detail.
		 */
		QUrl URL_;

		/** @brief A thumb image associated with this event.
		 */
		QUrl SmallImage_;

		/** @brief A big, preferably poster-size image of this event.
		 */
		QUrl BigImage_;

		/** @brief The list of all artists present on this event.
		 *
		 * This list should include the headliner as well.
		 */
		QStringList Artists_;

		/** @brief The name of the headliner of this event.
		 *
		 * The name of the headliner of this event.
		 */
		QString Headliner_;

		/** @brief The associated tags like musical genre of bands.
		 */
		QStringList Tags_;

		/** @brief The current number of attendees or -1 if not known.
		 */
		int Attendees_;

		/** @brief The name of the club or other place this event will
		 * be in.
		 */
		QString PlaceName_;

		/** @brief Latitude of the place.
		 *
		 * The geographical latitude of the place this event will be in,
		 * or -1 if not known.
		 */
		double Latitude_;

		/** @brief Longitude of the place.
		 *
		 * The geographical longitude of the place this event will be in,
		 * or -1 if not known.
		 */
		double Longitude_;

		/** @brief The city this event will happen in.
		 */
		QString City_;

		/** @brief The address of the place this event will happen in.
		 */
		QString Address_;

		/** @brief Whether this event can be attended.
		 */
		bool CanBeAttended_;

		/** @brief Current attendance status by the user.
		 */
		EventAttendType AttendType_;
	};

	/** @brief A list of events.
	 */
	typedef QList<EventInfo> EventInfos_t;

	/** @brief Interface for plugins that can provide events.
	 *
	 * Plugins that can provide nearby or recommended events based on
	 * user's location or musical taste should implement this interface.
	 */
	class Q_DECL_EXPORT IEventsProvider
	{
	public:
		virtual ~IEventsProvider () {}

		/** @brief The result of an recommended events query.
		 *
		 * The result of an recommended events query is either a string with a
		 * human-readable error text, or the list of the recommended events.
		 */
		using EventsQueryResult_t = LeechCraft::Util::Either<QString, EventInfos_t>;

		/** @brief Returns the service name.
		 *
		 * This string returns a human-readable string with the service
		 * name, like "Last.FM".
		 *
		 * @return The human-readable service name.
		 */
		virtual QString GetServiceName () const = 0;

		/** @brief Requests re-fetching the list of recommended events.
		 *
		 * @return The future with the result of the recommended events query.
		 */
		virtual QFuture<EventsQueryResult_t> UpdateRecommendedEvents () = 0;

		/** @brief Updates the event attendance status, if possible.
		 *
		 * This function marks the event attendance status as status, if
		 * the service supports it. The event is identified by its ID.
		 *
		 * @param[in] id The ID of the event (EventInfo::ID_).
		 * @param[in] status The new event attendance status.
		 */
		virtual void AttendEvent (qint64 id, EventAttendType status) = 0;
	};
}

Q_DECLARE_INTERFACE (Media::IEventsProvider, "org.LeechCraft.Media.IEventsProvider/1.0")
