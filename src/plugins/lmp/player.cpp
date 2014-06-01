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

#include "player.h"
#include <algorithm>
#include <random>
#include <QStandardItemModel>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QtConcurrentRun>
#include <QApplication>
#include <util/util.h>
#include <util/xpc/util.h>
#include <util/sll/delayedexecutor.h>
#include <interfaces/core/ientitymanager.h>
#include "core.h"
#include "mediainfo.h"
#include "localfileresolver.h"
#include "util.h"
#include "localcollection.h"
#include "playlistmanager.h"
#include "staticplaylistmanager.h"
#include "xmlsettingsmanager.h"
#include "playlistmodel.h"
#include "playlistparsers/playlistfactory.h"
#include "engine/sourceobject.h"
#include "engine/audiosource.h"
#include "engine/output.h"
#include "engine/path.h"
#include "localcollectionmodel.h"
#include "playerrulesmanager.h"

namespace LeechCraft
{
namespace LMP
{
	Player::Sorter::Sorter ()
	{
		Criteria_ << SortingCriteria::Artist
				<< SortingCriteria::Year
				<< SortingCriteria::Album
				<< SortingCriteria::TrackNumber;
	}

	bool Player::Sorter::operator() (const MediaInfo& left, const MediaInfo& right) const
	{
		Q_FOREACH (auto crit, Criteria_)
		{
			switch (crit)
			{
			case SortingCriteria::Artist:
				if (left.Artist_ != right.Artist_)
					return left.Artist_ < right.Artist_;
				break;
			case SortingCriteria::Year:
				if (left.Year_ != right.Year_)
					return left.Year_ < right.Year_;
				break;
			case SortingCriteria::Album:
				if (left.Album_ != right.Album_)
					return left.Album_ < right.Album_;
				break;
			case SortingCriteria::TrackNumber:
				if (left.TrackNumber_ != right.TrackNumber_)
					return left.TrackNumber_ < right.TrackNumber_;
				break;
			case SortingCriteria::TrackTitle:
				if (left.Title_ != right.Title_)
					return left.Title_ < right.Title_;
				break;
			case SortingCriteria::DirectoryPath:
			{
				const auto& leftPath = QFileInfo (left.LocalPath_).dir ().absolutePath ();
				const auto& rightPath = QFileInfo (right.LocalPath_).dir ().absolutePath ();
				if (leftPath != rightPath)
					return QString::localeAwareCompare (leftPath, rightPath) < 0;
				break;
			}
			case SortingCriteria::FileName:
			{
				const auto& leftPath = QFileInfo (left.LocalPath_).fileName ();
				const auto& rightPath = QFileInfo (right.LocalPath_).fileName ();
				if (leftPath != rightPath)
					return QString::localeAwareCompare (leftPath, rightPath) < 0;
				break;
			}
			}
		}

		return left.LocalPath_ < right.LocalPath_;
	}

	Player::Player (QObject *parent)
	: QObject (parent)
	, PlaylistModel_ (new PlaylistModel (this))
	, Source_ (new SourceObject (Category::Music, this))
	, Output_ (new Output (this))
	, Path_ (new Path (Source_, Output_))
	, RadioItem_ (nullptr)
	, RulesManager_ (new PlayerRulesManager (PlaylistModel_, this))
	, FirstPlaylistRestore_ (true)
	, PlayMode_ (PlayMode::Sequential)
	{
		qRegisterMetaType<QList<AudioSource>> ("QList<AudioSource>");
		qRegisterMetaType<StringPair_t> ("StringPair_t");

		connect (Source_,
				SIGNAL (currentSourceChanged (AudioSource)),
				this,
				SLOT (handleCurrentSourceChanged (AudioSource)));
		connect (Source_,
				SIGNAL (aboutToFinish ()),
				this,
				SLOT (handleUpdateSourceQueue ()));

		XmlSettingsManager::Instance ().RegisterObject ("SingleTrackDisplayMask",
				this, "refillPlaylist");

		const auto& criteriaVar = XmlSettingsManager::Instance ().property ("SortingCriteria");
		if (!criteriaVar.isNull ())
			Sorter_.Criteria_ = LoadCriteria (criteriaVar);

		connect (Source_,
				SIGNAL (finished ()),
				this,
				SLOT (handlePlaybackFinished ()));
		connect (Source_,
				SIGNAL (stateChanged (SourceState, SourceState)),
				this,
				SLOT (handleStateChanged (SourceState, SourceState)));

		connect (Source_,
				SIGNAL (metaDataChanged ()),
				this,
				SLOT (handleMetadata ()));
		connect (Source_,
				SIGNAL (bufferStatus (int)),
				this,
				SIGNAL (bufferStatusChanged (int)));

		connect (Source_,
				SIGNAL (error (QString, SourceError)),
				this,
				SLOT (handleSourceError (QString, SourceError)));

		auto collection = Core::Instance ().GetLocalCollection ();
		if (collection->IsReady ())
			restorePlaylist ();
		else
			connect (collection,
					SIGNAL (collectionReady ()),
					this,
					SLOT (restorePlaylist ()));
	}

	void Player::InitWithOtherPlugins ()
	{
		RulesManager_->InitializePlugins ();
	}

	QAbstractItemModel* Player::GetPlaylistModel () const
	{
		return PlaylistModel_;
	}

	SourceObject* Player::GetSourceObject () const
	{
		return Source_;
	}

	Output* Player::GetAudioOutput () const
	{
		return Output_;
	}

	Path* Player::GetPath () const
	{
		return Path_;
	}

	Player::PlayMode Player::GetPlayMode () const
	{
		return PlayMode_;
	}

	void Player::SetPlayMode (Player::PlayMode playMode)
	{
		if (PlayMode_ == playMode)
			return;

		PlayMode_ = playMode;
		emit playModeChanged (PlayMode_);
	}

	SourceState Player::GetState () const
	{
		return Source_->GetState ();
	}

	QList<SortingCriteria> Player::GetSortingCriteria () const
	{
		return Sorter_.Criteria_;
	}

	void Player::SetSortingCriteria (const QList<SortingCriteria>& criteria)
	{
		Sorter_.Criteria_ = criteria;

		AddToPlaylistModel ({}, true);

		XmlSettingsManager::Instance ().setProperty ("SortingCriteria", SaveCriteria (criteria));
	}

	namespace
	{
		Playlist FileToSource (const AudioSource& source)
		{
			if (!source.IsLocalFile ())
				return Playlist { { source } };

			const auto& file = source.GetLocalPath ();

			if (auto parser = MakePlaylistParser (file))
			{
				const auto& sources = parser (file);
				if (!sources.IsEmpty ())
					return sources;
			}

			return Playlist { { file } };
		}
	}

	void Player::PrepareURLInfo (const QUrl& url, const MediaInfo& info)
	{
		if (!info.IsUseless ())
			Url2Info_ [url] = info;
	}

	void Player::Enqueue (const QStringList& paths, EnqueueFlags flags)
	{
		QList<AudioSource> parsedSources;
		for (const auto& path : paths)
			parsedSources << AudioSource (path);
		Enqueue (parsedSources, flags);
	}

	void Player::Enqueue (const QList<AudioSource>& sources, EnqueueFlags flags)
	{
		if (CurrentQueue_.isEmpty ())
			emit shouldClearFiltering ();

		if (flags & EnqueueReplace)
		{
			PlaylistModel_->clear ();
			Items_.clear ();
			AlbumRoots_.clear ();
			CurrentQueue_.clear ();
		}

		Playlist parsedSources;
		std::for_each (sources.begin (), sources.end (),
				[&parsedSources] (decltype (sources.front ()) path)
					{ parsedSources += FileToSource (path); });

		for (auto i = parsedSources.begin (); i != parsedSources.end (); )
		{
			if (Items_.contains (i->Source_))
				i = parsedSources.erase (i);
			else
				++i;
		}

		const auto curSrcPos = std::find_if (parsedSources.begin (), parsedSources.end (),
				[] (const PlaylistItem& item) { return item.Additional_ ["Current"].toBool (); });
		if (curSrcPos != parsedSources.end ())
			switch (Source_->GetState ())
			{
			case SourceState::Error:
			case SourceState::Stopped:
				Source_->SetCurrentSource (curSrcPos->Source_);
				break;
			default:
				AddToOneShotQueue (curSrcPos->Source_);
				break;
			}

		AddToPlaylistModel (parsedSources.ToSources (), flags & EnqueueSort);
	}

	QList<AudioSource> Player::GetQueue () const
	{
		return CurrentQueue_;
	}

	QList<AudioSource> Player::GetIndexSources (const QModelIndex& index) const
	{
		QList<AudioSource> sources;
		if (index.data (Role::IsAlbum).toBool ())
			for (int i = 0; i < PlaylistModel_->rowCount (index); ++i)
				sources << PlaylistModel_->index (i, 0, index).data (Role::Source).value<AudioSource> ();
		else
			sources << index.data (Role::Source).value<AudioSource> ();
		return sources;
	}

	QModelIndex Player::GetSourceIndex (const AudioSource& source) const
	{
		const auto item = Items_ [source];
		return item ? item->index () : QModelIndex ();
	}

	namespace
	{
		void IncAlbumLength (QStandardItem *albumItem, int length)
		{
			const int prevLength = albumItem->data (Player::Role::AlbumLength).toInt ();
			albumItem->setData (length + prevLength, Player::Role::AlbumLength);
		}
	}

	void Player::Dequeue (const QModelIndex& index)
	{
		if (!index.isValid ())
			return;

		Dequeue (GetIndexSources (index));
	}

	void Player::Dequeue (const QList<AudioSource>& sources)
	{
		for (const auto& source : sources)
		{
			Url2Info_.remove (source.ToUrl ());

			if (!CurrentQueue_.removeAll (source))
				continue;

			RemoveFromOneShotQueue (source);

			auto item = Items_.take (source);
			auto parent = item->parent ();
			if (parent)
			{
				if (parent->rowCount () == 1)
				{
					Q_FOREACH (const auto& key, AlbumRoots_.keys ())
					{
						auto& items = AlbumRoots_ [key];
						if (!items.contains (parent))
							continue;

						items.removeAll (parent);
						if (items.isEmpty ())
							AlbumRoots_.remove (key);
					}
					PlaylistModel_->removeRow (parent->row ());
				}
				else
				{
					const auto& info = item->data (Role::Info).value<MediaInfo> ();
					if (!info.LocalPath_.isEmpty ())
						IncAlbumLength (parent, -info.Length_);
					parent->removeRow (item->row ());
				}
			}
			else
				PlaylistModel_->removeRow (item->row ());
		}

		Core::Instance ().GetPlaylistManager ()->
				GetStaticManager ()->SetOnLoadPlaylist (CurrentQueue_);
	}

	void Player::SetStopAfter (const QModelIndex& index)
	{
		if (!index.isValid ())
			return;

		AudioSource stopSource;
		if (index.data (Role::IsAlbum).toBool ())
			stopSource = PlaylistModel_->index (0, 0, index).data (Role::Source).value<AudioSource> ();
		else
			stopSource = index.data (Role::Source).value<AudioSource> ();

		if (!CurrentStopSource_.IsEmpty ())
			Items_ [CurrentStopSource_]->setData (false, Role::IsStop);

		if (CurrentStopSource_ == stopSource)
			CurrentStopSource_ = AudioSource ();
		else
		{
			CurrentStopSource_ = stopSource;
			Items_ [stopSource]->setData (true, Role::IsStop);
		}
	}

	void Player::RestorePlayState ()
	{
		const auto wasPlaying = XmlSettingsManager::Instance ().Property ("WasPlaying", false).toBool ();
		if (wasPlaying)
			Source_->Play ();

		IgnoreNextSaves_ = false;
	}

	void Player::SavePlayState (bool ignoreNext)
	{
		if (IgnoreNextSaves_)
			return;

		const auto state = Source_->GetState ();
		const auto isPlaying = state == SourceState::Playing ||
				state == SourceState::Buffering;
		XmlSettingsManager::Instance ().setProperty ("WasPlaying", isPlaying);

		IgnoreNextSaves_ = ignoreNext;
	}

	void Player::AddToOneShotQueue (const QModelIndex& index)
	{
		if (index.data (Role::IsAlbum).toBool ())
		{
			for (int i = 0, rc = PlaylistModel_->rowCount (index); i < rc; ++i)
				AddToOneShotQueue (PlaylistModel_->index (i, 0, index));
			return;
		}

		AddToOneShotQueue (index.data (Role::Source).value<AudioSource> ());
	}

	void Player::AddToOneShotQueue (const AudioSource& source)
	{
		if (CurrentOneShotQueue_.contains (source))
			return;

		CurrentOneShotQueue_ << source;

		const auto pos = CurrentOneShotQueue_.size () - 1;

		if (auto item = Items_ [source])
			item->setData (pos, Role::OneShotPos);
	}

	void Player::RemoveFromOneShotQueue (const QModelIndex& index)
	{
		if (index.data (Role::IsAlbum).toBool ())
		{
			for (int i = 0, rc = PlaylistModel_->rowCount (index); i < rc; ++i)
				RemoveFromOneShotQueue (PlaylistModel_->index (i, 0, index));
			return;
		}

		const auto& source = index.data (Role::Source).value<AudioSource> ();
		RemoveFromOneShotQueue (source);
	}

	void Player::OneShotMoveUp (const QModelIndex& index)
	{
		if (index.data (Role::IsAlbum).toBool ())
		{
			for (int i = 0, rc = PlaylistModel_->rowCount (index); i < rc; ++i)
				OneShotMoveUp (PlaylistModel_->index (i, 0, index));
			return;
		}

		const auto& source = index.data (Role::Source).value<AudioSource> ();
		const auto pos = CurrentOneShotQueue_.indexOf (source);
		if (pos <= 0)
			return;

		std::swap (CurrentOneShotQueue_ [pos], CurrentOneShotQueue_ [pos - 1]);
		Items_ [CurrentOneShotQueue_.at (pos)]->setData (pos, Role::OneShotPos);
		Items_ [CurrentOneShotQueue_.at (pos - 1)]->setData (pos - 1, Role::OneShotPos);
	}

	void Player::OneShotMoveDown (const QModelIndex& index)
	{
		if (index.data (Role::IsAlbum).toBool ())
		{
			for (int i = PlaylistModel_->rowCount (index) - 1; i >= 0; --i)
				OneShotMoveDown (PlaylistModel_->index (i, 0, index));
			return;
		}

		const auto& source = index.data (Role::Source).value<AudioSource> ();
		const auto pos = CurrentOneShotQueue_.indexOf (source);
		if (pos == CurrentOneShotQueue_.size () - 1)
			return;

		std::swap (CurrentOneShotQueue_ [pos], CurrentOneShotQueue_ [pos + 1]);
		Items_ [CurrentOneShotQueue_.at (pos)]->setData (pos, Role::OneShotPos);
		Items_ [CurrentOneShotQueue_.at (pos + 1)]->setData (pos + 1, Role::OneShotPos);
	}

	int Player::GetOneShotQueueSize () const
	{
		return CurrentOneShotQueue_.size ();
	}

	void Player::SetRadioStation (Media::IRadioStation_ptr station)
	{
		clear ();

		CurrentStation_ = station;

		connect (CurrentStation_->GetQObject (),
				SIGNAL (gotError (const QString&)),
				this,
				SLOT (handleStationError (const QString&)));
		connect (CurrentStation_->GetQObject (),
				SIGNAL (gotNewStream (QUrl, Media::AudioInfo)),
				this,
				SLOT (handleRadioStream (QUrl, Media::AudioInfo)));
		connect (CurrentStation_->GetQObject (),
				SIGNAL (gotPlaylist (QString, QString)),
				this,
				SLOT (handleGotRadioPlaylist (QString, QString)));
		connect (CurrentStation_->GetQObject (),
				SIGNAL (gotAudioInfos (QList<Media::AudioInfo>)),
				this,
				SLOT (handleGotAudioInfos (QList<Media::AudioInfo>)));
		CurrentStation_->RequestNewStream ();

		auto radioName = station->GetRadioName ();
		if (radioName.isEmpty ())
			radioName = tr ("Radio");
		RadioItem_ = new QStandardItem (radioName);
		RadioItem_->setEditable (false);
		PlaylistModel_->appendRow (RadioItem_);
	}

	MediaInfo Player::GetCurrentMediaInfo () const
	{
		const auto& source = Source_->GetActualSource ();
		if (source.IsEmpty ())
			return MediaInfo ();

		auto info = GetMediaInfo (source);
		if (!info.LocalPath_.isEmpty ())
			return info;

		info = GetPhononMediaInfo ();
		return info;
	}

	QString Player::GetCurrentAAPath () const
	{
		const auto& info = GetCurrentMediaInfo ();
		auto coll = Core::Instance ().GetLocalCollection ();
		auto album = coll->GetAlbum (coll->FindAlbum (info.Artist_, info.Album_));
		return album ? album->CoverPath_ : QString ();;
	}

	MediaInfo Player::GetMediaInfo (const AudioSource& source) const
	{
		return Items_.contains (source) ?
				Items_ [source]->data (Role::Info).value<MediaInfo> () :
				MediaInfo ();
	}

	MediaInfo Player::GetPhononMediaInfo () const
	{
		MediaInfo info;
		info.Artist_ = Source_->GetMetadata (SourceObject::Metadata::Artist);
		info.Album_ = Source_->GetMetadata (SourceObject::Metadata::Album);
		info.Title_ = Source_->GetMetadata (SourceObject::Metadata::Title);
		info.Genres_ << Source_->GetMetadata (SourceObject::Metadata::Genre);
		info.TrackNumber_ = Source_->GetMetadata (SourceObject::Metadata::Tracknumber).toInt ();
		info.Length_ = Source_->GetTotalTime () / 1000;
		info.LocalPath_ = Source_->GetActualSource ().ToUrl ().toString ();

		if (info.Artist_.isEmpty () && info.Title_.contains (" - "))
		{
			const auto& strs = info.Title_.split (" - ", QString::SkipEmptyParts);
			switch (strs.size ())
			{
			case 2:
				info.Artist_ = strs.value (0);
				info.Title_ = strs.value (1);
				break;
			case 3:
				info.Artist_ = strs.value (0);
				info.Album_ = strs.value (1);
				info.Title_ = strs.value (2);
				break;
			}
		}

		auto append = [&info, this] (SourceObject::Metadata md, const QString& name) -> void
		{
			const auto& value = Source_->GetMetadata (md);
			if (!value.isEmpty ())
				info.Additional_ [name] = value;
		};

		append (SourceObject::Metadata::NominalBitrate, tr ("Bitrate"));
		append (SourceObject::Metadata::MinBitrate, tr ("Minimum bitrate"));
		append (SourceObject::Metadata::MaxBitrate, tr ("Maximum bitrate"));

		return info;
	}

	namespace
	{
		QStandardItem* MakeAlbumItem (const MediaInfo& info)
		{
			auto albumItem = new QStandardItem (QString ("%1 - %2")
							.arg (info.Artist_, info.Album_));
			albumItem->setEditable (false);
			albumItem->setData (true, Player::Role::IsAlbum);
			albumItem->setData (QVariant::fromValue (info), Player::Role::Info);
			auto art = FindAlbumArt (info.LocalPath_);
			const int dim = 48;
			if (art.isNull ())
				art = QIcon::fromTheme ("media-optical").pixmap (dim, dim);
			else if (std::max (art.width (), art.height ()) > dim)
				art = art.scaled (dim, dim, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			albumItem->setData (art, Player::Role::AlbumArt);
			albumItem->setData (0, Player::Role::AlbumLength);
			return albumItem;
		}

		QPair<AudioSource, MediaInfo> PairResolve (const AudioSource& source)
		{
			MediaInfo info;
			if (!source.IsLocalFile ())
				return { source, info };

			info.LocalPath_ = source.GetLocalPath ();

			auto collection = Core::Instance ().GetLocalCollection ();

			const auto trackId = collection->FindTrack (source.GetLocalPath ());
			if (trackId == -1)
			{
				auto resolver = Core::Instance ().GetLocalFileResolver ();
				try
				{
					info = resolver->ResolveInfo (source.GetLocalPath ());
				}
				catch (const std::exception& e)
				{
					qWarning () << Q_FUNC_INFO
							<< "could not find track"
							<< info.LocalPath_
							<< "in library and cannot resolve its info, probably missing?";
				}
				return { source, info };
			}

			info.Artist_ = collection->GetTrackData (trackId,
					LocalCollectionModel::Role::ArtistName).toString ();
			info.Album_ = collection->GetTrackData (trackId,
					LocalCollectionModel::Role::AlbumName).toString ();
			info.Title_ = collection->GetTrackData (trackId,
					LocalCollectionModel::Role::TrackTitle).toString ();
			info.Genres_ = collection->GetTrackData (trackId,
					LocalCollectionModel::Role::TrackGenres).toStringList ();
			info.Length_ = collection->GetTrackData (trackId,
					LocalCollectionModel::Role::TrackLength).toInt ();
			info.Year_ = collection->GetTrackData (trackId,
					LocalCollectionModel::Role::AlbumYear).toInt ();
			info.TrackNumber_ = collection->GetTrackData (trackId,
					LocalCollectionModel::Role::TrackNumber).toInt ();

			return { source, info };
		}

		QList<QPair<AudioSource, MediaInfo>> PairResolveAll (const QList<AudioSource>& sources)
		{
			QList<QPair<AudioSource, MediaInfo>> result;
			std::transform (sources.begin (), sources.end (), std::back_inserter (result), PairResolve);
			return result;
		}

		template<typename T>
		QList<QPair<AudioSource, MediaInfo>> PairResolveSort (const QList<AudioSource>& sources, T sorter, bool sort)
		{
			auto result = PairResolveAll (sources);

			if (sorter.Criteria_.isEmpty () || !sort)
				return result;

			std::sort (result.begin (), result.end (),
					[sorter] (decltype (result.at (0)) s1, decltype (result.at (0)) s2) -> bool
					{
						if (s1.first.IsLocalFile () && !s2.first.IsLocalFile ())
							return true;
						else if (!s1.first.IsLocalFile () && s2.first.IsLocalFile ())
							return false;
						else if (!s1.first.IsLocalFile () || !s2.first.IsLocalFile ())
							return s1.first.ToUrl () < s2.first.ToUrl ();
						else
							return sorter (s1.second, s2.second);
					});

			return result;
		}
	}

	void Player::AddToPlaylistModel (QList<AudioSource> sources, bool sort)
	{
		if (!CurrentQueue_.isEmpty ())
		{
			EnqueueFlags flags { EnqueueReplace };
			if (sort)
				flags |= EnqueueSort;
			Enqueue (CurrentQueue_ + sources, flags);
			return;
		}

		PlaylistModel_->setHorizontalHeaderLabels (QStringList (tr ("Playlist")));

		std::function<QList<QPair<AudioSource, MediaInfo>> ()> worker =
				[this, sources, sort] ()
				{
					return PairResolveSort (sources, Sorter_, sort);
				};

		emit playerAvailable (false);

		auto watcher = new QFutureWatcher<QList<QPair<AudioSource, MediaInfo>>> ();
		connect (watcher,
				SIGNAL (finished ()),
				this,
				SLOT (handleSorted ()));
		watcher->setFuture (QtConcurrent::run (worker));
	}

	bool Player::HandleCurrentStop (const AudioSource& source)
	{
		if (source != CurrentStopSource_)
			return false;

		CurrentStopSource_ = AudioSource ();
		Items_ [source]->setData (false, Role::IsStop);

		return true;
	}

	void Player::RemoveFromOneShotQueue (const AudioSource& source)
	{
		const auto pos = CurrentOneShotQueue_.indexOf (source);
		if (pos < 0)
			return;

		CurrentOneShotQueue_.removeAt (pos);
		for (int i = pos; i < CurrentOneShotQueue_.size (); ++i)
			Items_ [CurrentOneShotQueue_.at (i)]->setData (i, Role::OneShotPos);

		Items_ [source]->setData ({}, Role::OneShotPos);
	}

	void Player::UnsetRadio ()
	{
		if (!CurrentStation_)
			return;

		if (RadioItem_)
			PlaylistModel_->removeRow (RadioItem_->row ());
		RadioItem_ = 0;

		CurrentStation_.reset ();
	}

	void Player::EmitStateChange (SourceState state)
	{
		QString stateStr;
		QString hrStateStr;
		switch (state)
		{
		case SourceState::Paused:
			stateStr = "Paused";
			hrStateStr = tr ("paused");
			break;
		case SourceState::Buffering:
		case SourceState::Playing:
			stateStr = "Playing";
			hrStateStr = tr ("playing");
			break;
		default:
			stateStr = "Stopped";
			hrStateStr = tr ("stopped");
			break;
		}

		const auto& mediaInfo = GetCurrentMediaInfo ();
		const auto& str = tr ("%1 by %2 is now %3")
				.arg (mediaInfo.Title_)
				.arg (mediaInfo.Artist_)
				.arg (hrStateStr);

		auto e = Util::MakeAN ("LMP", str, PInfo_,
				"org.LeechCraft.LMP", AN::CatMediaPlayer, AN::TypeMediaPlaybackStatus,
				"org.LeechCraft.LMP.PlaybackStatus", {}, 0, 1, str);
		e.Mime_ += "+advanced";
		e.Additional_ [AN::Field::MediaPlaybackStatus] = stateStr;
		e.Additional_ [AN::Field::MediaPlayerURL] = Source_->GetActualSource ().ToUrl ();

		e.Additional_ [AN::Field::MediaArtist] = mediaInfo.Artist_;
		e.Additional_ [AN::Field::MediaAlbum] = mediaInfo.Album_;
		e.Additional_ [AN::Field::MediaTitle] = mediaInfo.Title_;
		e.Additional_ [AN::Field::MediaLength] = mediaInfo.Length_;

		Core::Instance ().GetProxy ()->GetEntityManager ()->HandleEntity (e);
	}

	template<typename T>
	AudioSource Player::GetRandomBy (QList<AudioSource>::const_iterator pos,
			std::function<T (AudioSource)> feature) const
	{
		auto randPos = [&feature] (const QList<AudioSource>& sources) -> int
		{
			QHash<T, QList<int>> fVals;
			for (int i = 0; i < sources.size (); ++i)
				fVals [feature (sources.at (i))] << i;

			static std::random_device generator;
			std::uniform_int_distribution<int> dist (0, sources.size () - 1);
			auto fIdx = std::uniform_int_distribution<int> (0, fVals.size () - 1) (generator);

			auto fPos = fVals.begin ();
			std::advance (fPos, fIdx);
			const auto& positions = *fPos;
			if (positions.size () < 2)
				return positions [0];

			auto posIdx = std::uniform_int_distribution<int> (0, positions.size () - 1) (generator);
			return positions [posIdx];
		};
		auto rand = [&randPos] (const QList<AudioSource>& sources)
			{ return sources.at (randPos (sources)); };

		if (pos == CurrentQueue_.end ())
			return rand (CurrentQueue_);

		const auto& current = feature (*pos);
		++pos;
		if (pos != CurrentQueue_.end () && feature (*pos) == current)
			return *pos;

		auto modifiedQueue = CurrentQueue_;
		auto endPos = std::remove_if (modifiedQueue.begin (), modifiedQueue.end (),
				[&current, &feature, this] (decltype (modifiedQueue.at (0)) source)
					{ return feature (source) == current; });
		modifiedQueue.erase (endPos, modifiedQueue.end ());
		if (modifiedQueue.isEmpty ())
			return rand (CurrentQueue_);

		pos = modifiedQueue.begin () + randPos (modifiedQueue);
		const auto& origFeature = feature (*pos);
		while (pos != modifiedQueue.begin ())
		{
			if (feature (*(pos - 1)) != origFeature)
				break;
			--pos;
		}
		return *pos;
	}

	AudioSource Player::GetNextSource (const AudioSource& current)
	{
		if (CurrentQueue_.isEmpty ())
			return {};

		if (!CurrentOneShotQueue_.isEmpty ())
		{
			const auto first = CurrentOneShotQueue_.front ();
			RemoveFromOneShotQueue (first);
			return first;
		}

		auto pos = std::find (CurrentQueue_.begin (), CurrentQueue_.end (), current);

		switch (PlayMode_)
		{
		case PlayMode::Sequential:
			if (pos == CurrentQueue_.end ())
				return CurrentQueue_.value (0);
			else if (++pos != CurrentQueue_.end ())
				return *pos;
			else
				return {};
		case PlayMode::Shuffle:
			return GetRandomBy<int> (pos,
					[this] (const AudioSource& source)
						{ return CurrentQueue_.indexOf (source); });
		case PlayMode::ShuffleAlbums:
			return GetRandomBy<QString> (pos,
					[this] (const AudioSource& source)
						{ return GetMediaInfo (source).Album_; });
		case PlayMode::ShuffleArtists:
			return GetRandomBy<QString> (pos,
					[this] (const AudioSource& source)
						{ return GetMediaInfo (source).Artist_; });
		case PlayMode::RepeatTrack:
			return current;
		case PlayMode::RepeatAlbum:
		{
			if (pos == CurrentQueue_.end ())
				return CurrentQueue_.value (0);

			const auto& curAlbum = GetMediaInfo (*pos).Album_;
			++pos;
			if (pos == CurrentQueue_.end () ||
					GetMediaInfo (*pos).Album_ != curAlbum)
			{
				do
					--pos;
				while (pos >= CurrentQueue_.begin () &&
						GetMediaInfo (*pos).Album_ == curAlbum);
				++pos;
			}
			return *pos;
		}
		case PlayMode::RepeatWhole:
			if (pos == CurrentQueue_.end () || ++pos == CurrentQueue_.end ())
				pos = CurrentQueue_.begin ();
			return *pos;
		}

		return {};
	}

	void Player::MarkAsCurrent (QStandardItem *curItem)
	{
		if (curItem)
			curItem->setData (true, Role::IsCurrent);
		for (auto item : Items_)
		{
			if (item == curItem)
				continue;
			if (item->data (Role::IsCurrent).toBool ())
			{
				item->setData (false, Role::IsCurrent);
				break;
			}
		}
	}

	void Player::play (const QModelIndex& index)
	{
		if (CurrentStation_)
		{
			auto item = PlaylistModel_->itemFromIndex (index);
			if (item == RadioItem_)
				return;
			else
				UnsetRadio ();
		}

		if (index.data (Role::IsAlbum).toBool ())
		{
			play (index.child (0, 0));
			return;
		}

		if (!index.isValid ())
		{
			qWarning () << Q_FUNC_INFO
					<< "invalid index"
					<< index;
			return;
		}

		Source_->Stop ();
		Source_->ClearQueue ();
		const auto& source = index.data (Role::Source).value<AudioSource> ();
		Source_->SetCurrentSource (source);
		Source_->Play ();
	}

	void Player::previousTrack ()
	{
		const auto& current = Source_->GetCurrentSource ();

		AudioSource next;
		if (PlayMode_ == PlayMode::Shuffle)
		{
			next = GetNextSource (current);
			if (next.IsEmpty ())
				return;
		}
		else
		{
			const auto pos = std::find (CurrentQueue_.begin (), CurrentQueue_.end (), current);
			if (pos == CurrentQueue_.begin ())
				return;

			next = pos == CurrentQueue_.end () ? CurrentQueue_.value (0) : *(pos - 1);
		}

		Source_->Stop ();
		Source_->SetCurrentSource (next);
		Source_->Play ();
	}

	void Player::nextTrack ()
	{
		if (CurrentStation_)
		{
			Source_->Clear ();
			CurrentStation_->RequestNewStream ();
			return;
		}

		const auto& current = Source_->GetCurrentSource ();
		const auto& next = GetNextSource (current);
		if (next.IsEmpty ())
			return;

		Source_->Stop ();
		Source_->SetCurrentSource (next);
		Source_->Play ();
	}

	void Player::togglePause ()
	{
		if (Source_->GetState () == SourceState::Playing)
			Source_->Pause ();
		else
		{
			const auto& current = Source_->GetCurrentSource ();
			if (current.IsEmpty ())
				Source_->SetCurrentSource (CurrentQueue_.value (0));
			Source_->Play ();
		}
	}

	void Player::setPause ()
	{
		Source_->Pause ();
	}

	void Player::stop ()
	{
		Source_->Stop ();

		if (CurrentStation_)
			UnsetRadio ();
	}

	void Player::clear ()
	{
		UnsetRadio ();

		PlaylistModel_->clear ();
		Items_.clear ();
		AlbumRoots_.clear ();
		CurrentQueue_.clear ();
		Url2Info_.clear ();
		CurrentOneShotQueue_.clear ();
		Source_->ClearQueue ();

		XmlSettingsManager::Instance ().setProperty ("LastSong", QString ());

		Core::Instance ().GetPlaylistManager ()->
				GetStaticManager ()->SetOnLoadPlaylist (CurrentQueue_);

		if (Source_->GetState () != SourceState::Playing)
			Source_->SetCurrentSource ({});
	}

	void Player::shufflePlaylist ()
	{
		SetPlayMode (PlayMode::Sequential);

		auto queue = GetQueue ();
		std::random_shuffle (queue.begin (), queue.end ());
		Enqueue (queue, EnqueueReplace);
	}

	void Player::handleSorted ()
	{
		auto watcher = dynamic_cast<QFutureWatcher<QList<QPair<AudioSource, MediaInfo>>>*> (sender ());
		continueAfterSorted (watcher->result ());
		emit playerAvailable (true);
	}

	namespace
	{
		void FillItem (QStandardItem *item, const MediaInfo& info)
		{
			QString text;
			if (!info.IsUseless ())
			{
				text = XmlSettingsManager::Instance ()
						.property ("SingleTrackDisplayMask").toString ();

				text = PerformSubstitutions (text, info).simplified ();
				text.replace ("- -", "-");
				if (text.startsWith ("- "))
					text = text.mid (2);
				if (text.endsWith (" -"))
					text.chop (2);
			}
			else
				text = QFileInfo (info.LocalPath_).fileName ();

			item->setText (text);

			item->setData (QVariant::fromValue (info), Player::Role::Info);
		}
	}

	void Player::continueAfterSorted (const QList<QPair<AudioSource, MediaInfo>>& sources)
	{
		CurrentQueue_.clear ();

		QMetaObject::invokeMethod (PlaylistModel_, "modelAboutToBeReset");
		PlaylistModel_->blockSignals (true);

		QString prevAlbumRoot;

		QList<QStandardItem*> managedRulesItems;

		for (const auto& sourcePair : sources)
		{
			const auto& source = sourcePair.first;
			CurrentQueue_ << source;

			auto item = new QStandardItem ();
			item->setEditable (false);
			item->setData (QVariant::fromValue (source), Role::Source);
			item->setData (source == CurrentStopSource_, Role::IsStop);

			const auto oneShotPos = CurrentOneShotQueue_.indexOf (source);
			if (oneShotPos >= 0)
				item->setData (oneShotPos, Role::OneShotPos);

			switch (source.GetType ())
			{
			case AudioSource::Type::Stream:
				item->setText (tr ("Stream"));
				PlaylistModel_->appendRow (item);
				break;
			case AudioSource::Type::Url:
			{
				const auto& url = source.ToUrl ();

				auto info = Core::Instance ().TryURLResolve (url);
				if (!info && Url2Info_.contains (url))
					info = Url2Info_ [url];

				if (info)
					FillItem (item, *info);
				else
					item->setText (url.toString ());

				PlaylistModel_->appendRow (item);
				break;
			}
			case AudioSource::Type::File:
			{
				const auto& info = sourcePair.second;

				managedRulesItems << item;

				const auto& albumID = info.Album_;
				FillItem (item, info);
				if (albumID != prevAlbumRoot ||
						AlbumRoots_ [albumID].isEmpty ())
				{
					PlaylistModel_->appendRow (item);

					if (!info.Album_.simplified ().isEmpty ())
						AlbumRoots_ [albumID] << item;
				}
				else if (AlbumRoots_ [albumID].last ()->data (Role::IsAlbum).toBool ())
				{
					IncAlbumLength (AlbumRoots_ [albumID].last (), info.Length_);
					AlbumRoots_ [albumID].last ()->appendRow (item);
				}
				else
				{
					auto albumItem = MakeAlbumItem (info);

					const int row = AlbumRoots_ [albumID].last ()->row ();
					const auto& existing = PlaylistModel_->takeRow (row);
					albumItem->appendRow (existing);
					albumItem->appendRow (item);
					PlaylistModel_->insertRow (row, albumItem);

					const auto& existingInfo = existing.at (0)->data (Role::Info).value<MediaInfo> ();
					albumItem->setData (existingInfo.Length_, Role::AlbumLength);
					IncAlbumLength (albumItem, info.Length_);

					emit insertedAlbum (albumItem->index ());

					AlbumRoots_ [albumID].last () = albumItem;
				}
				prevAlbumRoot = albumID;
				break;
			}
			default:
				item->setText ("unknown");
				PlaylistModel_->appendRow (item);
				break;
			}

			Items_ [source] = item;
		}

		PlaylistModel_->blockSignals (false);

		QMetaObject::invokeMethod (PlaylistModel_, "modelReset");

		Core::Instance ().GetPlaylistManager ()->
				GetStaticManager ()->SetOnLoadPlaylist (CurrentQueue_);

		if (Source_->GetState () == SourceState::Stopped)
		{
			const auto& songUrl = XmlSettingsManager::Instance ().property ("LastSong").toByteArray ();
			const auto& song = QUrl::fromEncoded (songUrl);
			if (!song.isEmpty ())
			{
				const auto pos = std::find_if (CurrentQueue_.begin (), CurrentQueue_.end (),
						[&song] (decltype (CurrentQueue_.front ()) item)
							{ return song == item.ToUrl (); });
				if (pos != CurrentQueue_.end ())
					Source_->SetCurrentSource (*pos);
			}

			if (FirstPlaylistRestore_ &&
					XmlSettingsManager::Instance ().property ("AutoContinuePlayback").toBool ())
				RestorePlayState ();
			FirstPlaylistRestore_ = false;
		}

		const auto& currentSource = Source_->GetCurrentSource ();
		if (Items_.contains (currentSource))
			Items_ [currentSource]->setData (true, Role::IsCurrent);
	}

	void Player::restorePlaylist ()
	{
		auto staticMgr = Core::Instance ().GetPlaylistManager ()->GetStaticManager ();
		Enqueue (staticMgr->GetOnLoadPlaylist ());
	}

	void Player::handleStationError (const QString& error)
	{
		const auto& e = Util::MakeNotification ("LMP",
				tr ("Radio station error: %1.")
					.arg (error),
				PCritical_);
		Core::Instance ().SendEntity (e);
	}

	void Player::handleRadioStream (const QUrl& url, const Media::AudioInfo& info)
	{
		Url2Info_ [url] = info;
		Source_->SetCurrentSource (url);

		qDebug () << Q_FUNC_INFO << static_cast<int> (Source_->GetState ());
		if (Source_->GetState () == SourceState::Stopped)
			Source_->Play ();
	}

	void Player::handleGotRadioPlaylist (const QString& name, const QString& format)
	{
		QMetaObject::invokeMethod (this,
				"postPlaylistCleanup",
				Qt::QueuedConnection,
				Q_ARG (QString, name));

		auto parser = MakePlaylistParser (format);
		if (!parser)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to find parser for format"
					<< format;
			return;
		}

		const auto& list = parser (name).ToSources ();
		Enqueue (list, EnqueueNone);
	}

	void Player::handleGotAudioInfos (const QList<Media::AudioInfo>& infos)
	{
		QList<AudioSource> sources;
		for (const auto& info : infos)
		{
			const auto& url = info.Other_ ["URL"].toUrl ();
			if (!url.isValid ())
			{
				qWarning () << Q_FUNC_INFO
						<< "skipping invalid URL";
				continue;
			}

			Url2Info_ [url] = info;
			sources << url;
		}

		if (!sources.isEmpty ())
			Enqueue (sources, EnqueueNone);
	}

	void Player::postPlaylistCleanup (const QString& filename)
	{
		UnsetRadio ();
		QFile::remove (filename);
	}

	void Player::handleUpdateSourceQueue ()
	{
		const auto& current = Source_->GetCurrentSource ();

		if (CurrentStation_)
		{
			Url2Info_.remove (current.ToUrl ());
			CurrentStation_->RequestNewStream ();
			return;
		}

		const auto& path = current.GetLocalPath ();
		if (!path.isEmpty ())
			QMetaObject::invokeMethod (Core::Instance ().GetLocalCollection (),
					"recordPlayedTrack",
					Qt::QueuedConnection,
					Q_ARG (QString, path));

		const auto& next = GetNextSource (current);

		if (HandleCurrentStop (current))
		{
			if (!next.IsEmpty ())
				new Util::DelayedExecutor
				{
					[this, next] { Source_->SetCurrentSource (next); },
					1000
				};

			MarkAsCurrent (Items_.value (next));
			return;
		}

		if (!next.IsEmpty ())
		{
			EmitStateChange (SourceState::Stopped);
			Source_->PrepareNextSource (next);
		}
	}

	void Player::handlePlaybackFinished ()
	{
		emit songChanged (MediaInfo ());
		Source_->SetCurrentSource ({});
	}

	void Player::handleStateChanged (SourceState state, SourceState oldState)
	{
		qDebug () << Q_FUNC_INFO << static_cast<int> (state) << static_cast<int> (oldState);
		switch (state)
		{
		case SourceState::Stopped:
			emit songChanged ({});
			if (!CurrentQueue_.contains (Source_->GetCurrentSource ()))
				Source_->SetCurrentSource ({});
			break;
		default:
			break;
		}

		SavePlayState (false);

		EmitStateChange (state);
	}

	void Player::handleCurrentSourceChanged (const AudioSource& source)
	{
		XmlSettingsManager::Instance ().setProperty ("LastSong", source.ToUrl ().toEncoded ());

		QStandardItem *curItem = 0;
		if (CurrentStation_)
			curItem = RadioItem_;
		else if (Items_.contains (source))
			curItem = Items_ [source];

		if (Url2Info_.contains (source.ToUrl ()))
		{
			const auto& info = Url2Info_ [source.ToUrl ()];
			emit songChanged (info);
		}
		else if (curItem)
			emit songChanged (curItem->data (Role::Info).value<MediaInfo> ());
		else
			emit songChanged (MediaInfo ());

		if (curItem)
			emit indexChanged (PlaylistModel_->indexFromItem (curItem));

		MarkAsCurrent (curItem);

		handleMetadata ();

		EmitStateChange (Source_->GetState ());
	}

	void Player::handleMetadata ()
	{
		const auto& source = Source_->GetCurrentSource ();
		if (!source.IsRemote () ||
				CurrentStation_ ||
				!Items_.contains (source))
			return;

		auto curItem = Items_ [source];

		const auto& info = GetPhononMediaInfo ();

		if (info.Album_ == LastPhononMediaInfo_.Album_ &&
				info.Artist_ == LastPhononMediaInfo_.Artist_ &&
				info.Title_ == LastPhononMediaInfo_.Title_)
			emit songInfoUpdated (info);
		else
		{
			FillItem (curItem, info);
			emit songChanged (info);
		}

		LastPhononMediaInfo_ = info;

		EmitStateChange (Source_->GetState ());
	}

	void Player::handleSourceError (const QString& sourceText, SourceError error)
	{
		auto text = tr ("GStreamer says: %1.").arg (sourceText);
		switch (error)
		{
		case SourceError::MissingPlugin:
			text.prepend ("<br/>");
			text.prepend (tr ("Cannot find a proper audio decoder. "
					"You probably don't have all the codec plugins installed."));
			break;
		case SourceError::SourceNotFound:
			text = tr ("Audio source %1 not found, playing next track...")
					.arg (QFileInfo (Source_->GetCurrentSource ().ToUrl ().path ()).fileName ());
			nextTrack ();
			break;
		case SourceError::Other:
			break;
		}

		const auto& e = Util::MakeNotification ("LMP", text, PCritical_);
		Core::Instance ().GetProxy ()->GetEntityManager ()->HandleEntity (e);
	}

	void Player::refillPlaylist ()
	{
		Enqueue (GetQueue (), EnqueueReplace);
	}
}
}
