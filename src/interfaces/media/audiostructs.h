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

#include <QStringList>
#include <QVariantMap>
#include <QUrl>
#include <util/sll/eitherfwd.h>

namespace Media
{
	/** @brief Describes a single audio track.
	 */
	struct AudioInfo
	{
		/** @brief The artist performing this track.
		 */
		QString Artist_;

		/** @brief The album this track is on.
		 */
		QString Album_;

		/** @brief The title of this track.
		 */
		QString Title_;

		/** @brief The genres of this track.
		 */
		QStringList Genres_;

		/** @brief The length of this track in seconds.
		 */
		qint32 Length_ = 0;

		/** @brief The year of the Album_ this track is on.
		 */
		qint32 Year_ = 0;

		/** @brief The number of this track on the Album_.
		 */
		qint32 TrackNumber_ = 0;

		/** @brief Other fields of this audio info.
		 *
		 * Other fields known to be used:
		 * - URL with a QUrl pointing to either local file (if the scheme
		 *   is "file:") or a remote file or radio stream otherwise.
		 */
		QVariantMap Other_;

		/** @brief Returns whether this audio info is equal to another one.
		 */
		inline bool operator== (const AudioInfo& other) const
		{
			return Artist_ == other.Artist_ &&
				Album_ == other.Album_ &&
				Title_ == other.Title_ &&
				Genres_ == other.Genres_ &&
				Length_ == other.Length_ &&
				Year_ == other.Year_ &&
				TrackNumber_ == other.TrackNumber_ &&
				Other_ == other.Other_;
		}

		/** @brief Returns whether this audio info isn't equal to another one.
		 */
		inline bool operator!= (const AudioInfo& other) const
		{
			return !(*this == other);
		}
	};

	/** @brief Information about a tag like a genre.
	 */
	struct TagInfo
	{
		/** @brief Name of the tag.
		 */
		QString Name_;
	};

	/** @brief A list of tags.
	 */
	typedef QList<TagInfo> TagInfos_t;

	/** @brief A structure describing an artist.
	 */
	struct ArtistInfo
	{
		/** @brief The artist name.
		 */
		QString Name_;

		/** @brief Short artist description.
		 */
		QString ShortDesc_;

		/** @brief Full artist description, not including the short description.
		 */
		QString FullDesc_;

		/** @brief An URL of a thumbnail artist image.
		 */
		QUrl Image_;

		/** @brief A bigger artist image.
		 */
		QUrl LargeImage_;

		/** @brief An URL to a page describing this artist.
		 *
		 * Generally this should be set to the artist page on some
		 * service this ArtistInfo is fetched from, not to the artist's
		 * own web site. For example, a Last.FM client would set
		 * this to "http://www.last.fm/music/Fellsilent" for Fellsilent
		 * band.
		 */
		QUrl Page_;

		/** @brief Genres this artist plays in.
		 */
		TagInfos_t Tags_;
	};

	/** @brief Describes similarty information of an artist.
	 *
	 * Similarity information may contain similarity percentage (the
	 * Similarity_ field) with respect to some other artist or artist
	 * names similar to this one. For example, both IRecommendedArtists
	 * and ISimilarArtists return IPendingSimilarArtists which carries
	 * a list of SimilarityInfo structures, but IRecommendedArtists will
	 * typically contain the list of names of artists from the user
	 * library that an artist is similar to, while ISimilarArtists will
	 * carry around similarity percentage.
	 *
	 * @sa ISimilarArtists, IRecommendedArtists, IPendingSimilarArtists
	 */
	struct SimilarityInfo
	{
		/** @brief Information about artist this similary info is about.
		 */
		ArtistInfo Artist_;

		/** @brief Similarity in percents.
		 *
		 * May be 0 if only SimilarTo_ field is filled.
		 */
		int Similarity_;

		/** @brief Names of the artists similar to this one.
		 */
		QStringList SimilarTo_;
	};

	/** @brief A list of SimilarityInfo structures.
	 */
	using SimilarityInfos_t = QList<SimilarityInfo>;

	using SimilarityQueryResult_t = LC::Util::Either<QString, SimilarityInfos_t>;
}

Q_DECLARE_METATYPE (Media::AudioInfo)
Q_DECLARE_METATYPE (QList<Media::AudioInfo>)
