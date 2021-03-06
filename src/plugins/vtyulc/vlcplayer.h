/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Vladislav Tyulbashev
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

#include <memory>
#include <QObject>
#include <QUrl>
#include <QStringList>
#include <vlc/vlc.h>

class QWidget;
class QTime;

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_t;
struct libvlc_track_description_t;

namespace LC
{
namespace vlc
{
	class VlcPlayer : public QObject
	{
		Q_OBJECT

		std::shared_ptr<libvlc_instance_t> VlcInstance_;
		std::shared_ptr<libvlc_media_player_t> Mp_;
		std::shared_ptr<libvlc_media_t> M_;
		
		QWidget *Parent_;
		bool DVD_;
		
		QStringList Subtitles_;
		
		libvlc_time_t FreezeCur_;
		int FreezeAudio_;
		int FreezeSubtitle_;
		bool FreezePlayingMedia_;
		bool FreezeIsPlaying_;
		bool FreezeDVD_;
		
	public:
		explicit VlcPlayer (QWidget *parent = 0);
		~VlcPlayer();
		
		void AddSubtitles (const QString&);
		bool NowPlaying () const;
		double GetPosition () const;
		QWidget* GetParent () const;
		
		void Init (QWidget *parent);
		
		int GetAudioTracksNumber () const;
		int GetCurrentAudioTrack () const;
		QString GetAudioTrackDescription (int) const;
		int GetAudioTrackId (int) const;
		
		int GetSubtitlesNumber () const;
		int GetCurrentSubtitle () const;
		QString GetSubtitleDescription (int) const;
		int GetSubtitleId (int) const;
		
		QString GetAspectRatio () const;
		
		void DVDNavigate (unsigned);
		
		std::shared_ptr<libvlc_media_player_t> GetPlayer () const;
		libvlc_instance_t* GetInstance () const;
		
		QTime GetCurrentTime () const;
		QTime GetFullTime () const;
		void SetCurrentTime (libvlc_time_t);
		
	private:
		libvlc_track_description_t* GetTrack (libvlc_track_description_t *t, int track) const;
		void WaitForPlaying () const;
		void WaitForDVDPlaying () const;
		void ReloadSubtitles ();
		
		void Freeze ();
		void UnFreeze ();		
		
	public slots:
		void stop ();
		void togglePlay ();
		void pause ();
		void addUrl (const QUrl&);
		void setUrl (const QUrl&);
		void changePosition (double);
		void switchWidget (QWidget*);
		
		void setAudioTrack (int);
		void setSubtitle (int);
		void setAspectRatio (const QByteArray&);
		void setRealZoom (const QByteArray&);
		
		void dvdNavigateLeft ();
		void dvdNavigateRight ();
		void dvdNavigateUp ();
		void dvdNavigateDown ();
		void dvdNavigateEnter ();
		
		void minus10seconds ();
		void plus10seconds ();
		void plus3percent ();
		void minus3percent ();
		
	signals:
		void unstable ();
		void stable ();
	};
}
}
