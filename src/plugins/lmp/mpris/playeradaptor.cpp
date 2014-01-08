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

#include "playeradaptor.h"
#include <QMetaObject>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include "../player.h"
#include "../engine/sourceobject.h"
#include "../engine/output.h"
#include "fdopropsadaptor.h"

namespace LeechCraft
{
namespace LMP
{
namespace MPRIS
{
	PlayerAdaptor::PlayerAdaptor (FDOPropsAdaptor *fdo, Player *player)
	: QDBusAbstractAdaptor (player)
	, Props_ (fdo)
	, Player_ (player)
	{
		setAutoRelaySignals (true);

		connect (Player_,
				SIGNAL (songChanged (MediaInfo)),
				this,
				SLOT (handleSongChanged ()));
		connect (Player_,
				SIGNAL (playModeChanged (Player::PlayMode)),
				this,
				SLOT (handlePlayModeChanged ()));
		connect (Player_->GetSourceObject (),
				SIGNAL (stateChanged (SourceState, SourceState)),
				this,
				SLOT (handleStateChanged ()));
		connect (Player_->GetAudioOutput (),
				SIGNAL (volumeChanged (qreal)),
				this,
				SLOT (handleVolumeChanged ()));
	}

	PlayerAdaptor::~PlayerAdaptor ()
	{
	}

	bool PlayerAdaptor::GetCanControl () const
	{
		return true;
	}

	bool PlayerAdaptor::GetCanGoNext () const
	{
		return true;
	}

	bool PlayerAdaptor::GetCanGoPrevious () const
	{
		return true;
	}

	bool PlayerAdaptor::GetCanPause () const
	{
		return true;
	}

	bool PlayerAdaptor::GetCanPlay () const
	{
		return true;
	}

	bool PlayerAdaptor::GetCanSeek () const
	{
		return Player_->GetSourceObject ()->IsSeekable ();
	}

	QString PlayerAdaptor::GetLoopStatus () const
	{
		switch (Player_->GetPlayMode ())
		{
		case Player::PlayMode::RepeatTrack:
			return "Track";
		case Player::PlayMode::RepeatAlbum:
		case Player::PlayMode::RepeatWhole:
			return "Playlist";
		case Player::PlayMode::Sequential:
		case Player::PlayMode::Shuffle:
		case Player::PlayMode::ShuffleAlbums:
		case Player::PlayMode::ShuffleArtists:
			return "None";
		}
		return "None";
	}

	void PlayerAdaptor::SetLoopStatus (const QString& value)
	{
		if (value == "Track")
			Player_->SetPlayMode (Player::PlayMode::RepeatTrack);
		else if (value == "Playlist")
			Player_->SetPlayMode (Player::PlayMode::RepeatWhole);
		else
			Player_->SetPlayMode (Player::PlayMode::Sequential);
	}

	double PlayerAdaptor::GetMaximumRate () const
	{
		return 1;
	}

	QVariantMap PlayerAdaptor::GetMetadata () const
	{
		auto info = Player_->GetCurrentMediaInfo ();
		if (info.LocalPath_.isEmpty () && info.Title_.isEmpty ())
			return QVariantMap ();

		QVariantMap result;
		result ["mpris:trackid"] = info.LocalPath_.isEmpty () ?
					QString ("/nonlocal/%1_/%2_/%3_/%4_")
						.arg (info.Artist_)
						.arg (info.Album_)
						.arg (info.TrackNumber_)
						.arg (info.Title_) :
					QString ("/local/%1")
						.arg (info.LocalPath_);
		result ["mpris:length"] = info.Length_ * 1000;
		result ["mpris:artUrl"] = QUrl (Player_->GetCurrentAAPath ()).toLocalFile ();
		result ["xesam:album"] = info.Album_;
		result ["xesam:artist"] = QStringList { info.Artist_ };
		result ["xesam:genre"] = info.Genres_.join (" / ");
		result ["xesam:title"] = info.Title_;
		result ["xesam:trackNumber"] = info.TrackNumber_;
		return result;
	}

	double PlayerAdaptor::GetMinimumRate () const
	{
		return 1;
	}

	QString PlayerAdaptor::GetPlaybackStatus () const
	{
		switch (Player_->GetSourceObject ()->GetState ())
		{
		case SourceState::Paused:
			return "Paused";
		case SourceState::Stopped:
		case SourceState::Error:
			return "Stopped";
		default:
			return "Playing";
		}
	}

	qlonglong PlayerAdaptor::GetPosition () const
	{
		return Player_->GetSourceObject ()->GetCurrentTime () * 1000;
	}

	double PlayerAdaptor::GetRate () const
	{
		return 1.0;
	}

	void PlayerAdaptor::SetRate (double)
	{
	}

	bool PlayerAdaptor::GetShuffle () const
	{
		return Player_->GetPlayMode () == Player::PlayMode::Shuffle;
	}

	void PlayerAdaptor::SetShuffle (bool value)
	{
		Player_->SetPlayMode (value ?
				Player::PlayMode::Shuffle :
				Player::PlayMode::Sequential);
	}

	double PlayerAdaptor::GetVolume () const
	{
		return Player_->GetAudioOutput ()->GetVolume ();
	}

	void PlayerAdaptor::SetVolume (double value)
	{
		Player_->GetAudioOutput ()->setVolume (value);
	}

	void PlayerAdaptor::Notify (const QString& propName)
	{
		Props_->Notify ("org.mpris.MediaPlayer2.Player",
				propName, property (propName.toUtf8 ()));
	}

	void PlayerAdaptor::Next ()
	{
		Player_->nextTrack ();
	}

	void PlayerAdaptor::OpenUri (const QString& uri)
	{
		const auto& url = QUrl (uri);
		if (url.scheme () == "file")
			Player_->Enqueue (QStringList (uri));
		else
			Player_->Enqueue ({ url });
	}

	void PlayerAdaptor::Pause ()
	{
		Player_->setPause ();
	}

	void PlayerAdaptor::Play ()
	{
		if (GetPlaybackStatus () == "Playing")
			return;

		Player_->togglePause ();
	}

	void PlayerAdaptor::PlayPause ()
	{
		Player_->togglePause ();
	}

	void PlayerAdaptor::Previous ()
	{
		Player_->previousTrack ();
	}

	void PlayerAdaptor::Seek (qlonglong offset)
	{
		Player_->GetSourceObject ()->Seek (offset / 1000);
	}

	void PlayerAdaptor::SetPosition (const QDBusObjectPath&, qlonglong)
	{
	}

	void PlayerAdaptor::Stop ()
	{
		Player_->stop ();
	}

	void PlayerAdaptor::handleSongChanged ()
	{
		Notify ("Metadata");
	}

	void PlayerAdaptor::handlePlayModeChanged ()
	{
		Notify ("LoopStatus");
		Notify ("Shuffle");
	}

	void PlayerAdaptor::handleStateChanged ()
	{
		Notify ("PlaybackStatus");
	}

	void PlayerAdaptor::handleVolumeChanged ()
	{
		Notify ("Volume");
	}
}
}
}
