/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2011  Minh Ngo
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

#include "lastfmsubmitter.h"
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QTimer>
#include <interfaces/media/audiostructs.h>
#include <util/util.h>
#include "util.h"

namespace LeechCraft
{
namespace Lastfmscrobble
{
	LastFMSubmitter::LastFMSubmitter (QNetworkAccessManager *nam, QObject *parent)
	: QObject (parent)
	, NAM_ (nam)
	, SubmitTimer_ (new QTimer (this))
	{
		lastfm::ws::ApiKey = "a5ca8821e39cdb5efd2e5667070084b2";
		lastfm::ws::SharedSecret = "50fb8b6f35fc55b7ddf6bb033dfc6fbe";

		SubmitTimer_->setSingleShot (true);
		connect (SubmitTimer_,
				SIGNAL (timeout ()),
				this,
				SLOT (cacheAndSubmit ()),
				Qt::UniqueConnection);
	}

	bool LastFMSubmitter::IsConnected () const
	{
		return Scrobbler_ ? true : false;
	}

	namespace
	{
		lastfm::MutableTrack ToLastFMTrack (const Media::AudioInfo& info)
		{
			lastfm::MutableTrack mutableTrack;
			mutableTrack.setTitle (info.Title_);
			mutableTrack.setAlbum (info.Album_);
			mutableTrack.setArtist (info.Artist_);
			mutableTrack.stamp ();
			mutableTrack.setSource (lastfm::Track::Player);
			mutableTrack.setDuration (info.Length_);
			mutableTrack.setTrackNumber (info.TrackNumber_);
			return mutableTrack;
		}
	}

	void LastFMSubmitter::NowPlaying (const Media::AudioInfo& info)
	{
		SubmitTimer_->stop ();

		if (!NextSubmit_.isNull ())
		{
			const int secsTo = NextSubmit_.timestamp ().secsTo (QDateTime::currentDateTime ());
			if (!NextSubmit_.duration () && secsTo > 30)
			{
				NextSubmit_.setDuration (secsTo);
				if (Scrobbler_)
					cacheAndSubmit ();
			}
			else
				NextSubmit_ = lastfm::Track ();
		}

		if (info.Length_ && info.Length_ < 30)
			return;

		const auto& lfmTrack = ToLastFMTrack (info);
		if (!Scrobbler_)
			return;
		Scrobbler_->nowPlaying (lfmTrack);

		NextSubmit_ = lfmTrack;
		if (info.Length_)
			SubmitTimer_->start (std::min (info.Length_ / 2, 240) * 1000);
	}

	void LastFMSubmitter::Love ()
	{
		if (NextSubmit_.isNull ())
		{
			qWarning () << Q_FUNC_INFO
					<< "no track in submit queue, can't make love";
			return;
		}

		QList<QPair<QString, QString>> params;
		params << QPair<QString, QString> ("track", NextSubmit_.title ());
		params << QPair<QString, QString> ("artist", NextSubmit_.artist ());
		QNetworkReply *reply = Request ("track.love", NAM_, params);
		connect (reply,
				SIGNAL (finished ()),
				reply,
				SLOT (deleteLater ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				reply,
				SLOT (deleteLater ()));
	}

	void LastFMSubmitter::Ban ()
	{
		if (NextSubmit_.isNull ())
		{
			qWarning () << Q_FUNC_INFO
					<< "no track in submit queue, can't ban";
			return;
		}

		QList<QPair<QString, QString>> params;
		params << QPair<QString, QString> ("track", NextSubmit_.title ());
		params << QPair<QString, QString> ("artist", NextSubmit_.artist ());
		QNetworkReply *reply = Request ("track.ban", NAM_, params);
		connect (reply,
				 SIGNAL (finished ()),
				 reply,
				 SLOT (deleteLater ()));
		connect (reply,
				 SIGNAL (error (QNetworkReply::NetworkError)),
				 reply,
				 SLOT (deleteLater ()));
	}

	void LastFMSubmitter::Clear ()
	{
		NextSubmit_ = lastfm::MutableTrack ();
		SubmitTimer_->stop ();
	}

	void LastFMSubmitter::handleAuthenticated ()
	{
		Scrobbler_.reset (new lastfm::Audioscrobbler ("tst"));

		connect (Scrobbler_.get (),
				SIGNAL (nowPlayingError (int, QString)),
				this,
				SLOT (handleNPError (int, QString)));
	}

	void LastFMSubmitter::handleNPError (int code, const QString& msg)
	{
		qWarning () << Q_FUNC_INFO
				<< code
				<< msg;
	}

	void LastFMSubmitter::cacheAndSubmit ()
	{
		Scrobbler_->cache (NextSubmit_);
		submit ();
		NextSubmit_ = lastfm::Track ();
	}

	void LastFMSubmitter::submit ()
	{
		if (!IsConnected ())
			return;

		Scrobbler_->submit ();
	}
}
}
