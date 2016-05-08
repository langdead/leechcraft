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

#include <QString>
#include <QFlags>

namespace LeechCraft
{
namespace AN
{
	/** @brief Event cancel pseudo-category.
	 *
	 * This category is used to cancel an event by a given event ID.
	 */
	const QString CatEventCancel = "org.LC.AdvNotifications.Cancel";

	/** @brief Category of Instant Messaging-related events.
	 */
	const QString CatIM = "org.LC.AdvNotifications.IM";
	/** @brief Another user has requested our user's attention.
	 */
	const QString TypeIMAttention = CatIM + ".AttentionDrawn";
	/** @brief Another user has sent our user a file.
	 */
	const QString TypeIMIncFile = CatIM + ".IncomingFile";
	/** @brief User has received a message in a standard one-to-one chat.
	 */
	const QString TypeIMIncMsg = CatIM + ".IncomingMessage";
	/** @brief User has been highlighted in a multiuser chat.
	 *
	 * The primary difference from TypeIMMUCMsg is that our user must be
	 * explicitly mentioned in another user's message for this event.
	 *
	 * @sa TypeIMMUCMsg
	 */
	const QString TypeIMMUCHighlight = CatIM + ".MUCHighlightMessage";
	/** @brief User has been invited to a multiuser chat.
	 */
	const QString TypeIMMUCInvite = CatIM + ".MUCInvitation";
	/** @brief A message has been sent to a multiuser chat.
	 *
	 * This event should be emitted for each MUC message, even for those
	 * our user isn't mentioned in.
	 *
	 * @sa TypeIMMUCHighlight
	 */
	const QString TypeIMMUCMsg = CatIM + ".MUCMessage";
	/** @brief Another user in our user's contact list has changed its
	 * status.
	 */
	const QString TypeIMStatusChange = CatIM + ".StatusChange";
	/** @brief Another user has granted subscription to our user.
	 */
	const QString TypeIMSubscrGrant = CatIM + ".Subscr.Granted";
	/** @brief Another user has revoked subscription from our user.
	 */
	const QString TypeIMSubscrRevoke = CatIM + ".Subscr.Revoked";
	/** @brief Another user has requested subscription from our user.
	 */
	const QString TypeIMSubscrRequest = CatIM + ".Subscr.Requested";
	/** @brief Another user has subscribed to our user.
	 */
	const QString TypeIMSubscrSub = CatIM + ".Subscr.Subscribed";
	/** @brief Another user has unsubscribed from our user.
	 */
	const QString TypeIMSubscrUnsub = CatIM + ".Subscr.Unsubscribed";
	/** @brief User's tune has changed.
	 */
	const QString TypeIMEventTuneChange = CatIM + ".Event.Tune";
	/** @brief User's mood has changed.
	 */
	const QString TypeIMEventMoodChange = CatIM + ".Event.Mood";
	/** @brief User's activity has changed.
	 */
	const QString TypeIMEventActivityChange = CatIM + ".Event.Activity";
	/** @brief User's location has changed.
	 */
	const QString TypeIMEventLocationChange = CatIM + ".Event.Location";

	/** @brief Category of Organizer-related events.
	 */
	const QString CatOrganizer = "org.LC.AdvNotifications.Organizer";
	/** @brief An event due date is coming.
	 */
	const QString TypeOrganizerEventDue = CatOrganizer + ".EventDue";

	/** @brief Category of Downloads-related events.
	 */
	const QString CatDownloads = "org.LC.AdvNotifications.Downloads";
	/** @brief A download has been finished successfully without errors.
	 */
	const QString TypeDownloadFinished = CatDownloads + ".DownloadFinished";
	/** @brief A download has been failed.
	 */
	const QString TypeDownloadError = CatDownloads + ".DownloadError";

	/** @brief Category of package manager-related events.
	 */
	const QString CatPackageManager = "org.LC.AdvNotifications.PackageManager";
	/** @brief A package has been updated.
	 */
	const QString TypePackageUpdated = CatPackageManager + ".PackageUpdated";

	/** @brief Category of media player-related events.
	 */
	const QString CatMediaPlayer = "org.LC.AdvNotifications.MediaPlayer";

	/** @brief A media file playback status has been changed.
	 */
	const QString TypeMediaPlaybackStatus = CatMediaPlayer + ".PlaybackStatus";

	/** @brief Category for terminal emulation events.
	 */
	const QString CatTerminal = "org.LC.AdvNotifications.Terminal";

	/** @brief A bell has ringed in a terminal window.
	 */
	const QString TypeTerminalBell = CatTerminal + ".Bell";

	/** @brief Activity in terminal window.
	 */
	const QString TypeTerminalActivity = CatTerminal + ".Activity";

	/** @brief Inactivity in terminal window.
	 */
	const QString TypeTerminalInactivity = CatTerminal + ".Inactivity";

	/** @brief Generic notifications that don't fit into any other category.
	 */
	const QString CatGeneric = "org.LC.AdvNotifications.Generic";

	/** @brief Generic type for generic notifications.
	 */
	const QString TypeGeneric = CatGeneric + ".Generic";

	/** @brief Describes the notification parameters.
	 */
	enum NotifyFlag
	{
		/** @brief No notifications.
		 */
		NotifyNone			= 0,

		/** @brief Rule should be triggered only once.
		 *
		 * This corresponds to the single shot events. That is, after
		 * first triggering of the rule it should be disabled and user
		 * shouldn't get further notifications.
		 */
		NotifySingleShot	= 1 << 0,

		/** @brief User should be notified visually.
		 *
		 * The user should be notified via transient notifications like
		 * a non-intrusive tooltip that will hide soon.
		 *
		 * This is ortogonal to NotifyPersistent.
		 *
		 * @sa NotifyPersistent
		 */
		NotifyTransient		= 1 << 1,

		/** @brief User should be notified visually via persistent
		 * notifications.
		 *
		 * A persistent notification is something like a tray icon
		 * that will be displayed until the user reacts to the event.
		 *
		 * This is ortogonal to NotifyTransient.
		 *
		 * @sa NotifyTransient
		 */
		NotifyPersistent	= 1 << 2,

		/** @brief Notify by playing back an audio file.
		 */
		NotifyAudio			= 1 << 3
	};
	Q_DECLARE_FLAGS (NotifyFlags, NotifyFlag);

	namespace Field
	{
		/** @brief The URL to the file being played.
		*/
		const QString MediaPlayerURL = CatMediaPlayer + ".Fields.URL";

		/** @brief Playback status of the URL (QString).
		 *
		 * A string, one of:
		 * - Playing
		 * - Paused
		 * - Stopped
		 */
		const QString MediaPlaybackStatus = CatMediaPlayer + ".Fields.PlaybackStatus";

		/** @brief The title of the currently playing media (QString).
		 */
		const QString MediaTitle = CatMediaPlayer + ".Fields.Title";

		/** @brief The artist of the currently playing media (QString).
		 */
		const QString MediaArtist = CatMediaPlayer + ".Fields.Artist";

		/** @brief The album of the currently playing media (QString).
		 */
		const QString MediaAlbum = CatMediaPlayer + ".Fields.Album";

		/** @brief The length of the currently playing media (int).
		 */
		const QString MediaLength = CatMediaPlayer + ".Fields.Length";

		/** @brief Whether the terminal window is active (bool).
		 */
		const QString TerminalActive = CatTerminal + ".Fields.Active";

		/** @brief General activity name of a contact (QString).
		 */
		const QString IMActivityGeneral = CatIM + ".Fields.Activity.General";
		/** @brief Specific activity name of a contact (QString).
		 */
		const QString IMActivitySpecific = CatIM + ".Fields.Activity.Specific";
		/** @brief Accompanying activity text entered by a contact (QString).
		 */
		const QString IMActivityText = CatIM + ".Fields.Activity.Text";

		/** @brief General mood name of a contact (QString).
		 */
		const QString IMMoodGeneral = CatIM + ".Fields.Mood.General";
		/** @brief Accompanying mood text entered by a contact (QString).
		 */
		const QString IMMoodText = CatIM + ".Fields.Mood.Text";

		/** @brief Longitude of a contact's position (double).
		 */
		const QString IMLocationLongitude = CatIM + ".Fields.Location.Longitude";
		/** @brief Latitude of a contact's position (double).
		 */
		const QString IMLocationLatitude = CatIM + ".Fields.Location.Latitude";
		/** @brief Country a contact is currently in (QString).
		 */
		const QString IMLocationCountry = CatIM + ".Fields.Location.Country";
		/** @brief Exact locality, like a town or a city, a contact is
		 * currently in (QString).
		 */
		const QString IMLocationLocality = CatIM + ".Fields.Location.Locality";
	}
}
}

Q_DECLARE_OPERATORS_FOR_FLAGS (LeechCraft::AN::NotifyFlags)
