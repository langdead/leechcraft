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
#include "audiostructs.h"

class QObject;

namespace Media
{
	/** @brief Describes a single artist photo.
	 */
	struct ArtistImage
	{
		/** @brief The title of the image.
		 */
		QString Title_;

		/** @brief The author if the image.
		 */
		QString Author_;

		/** @brief The date and time the image was taken.
		 */
		QDateTime Date_;

		/** @brief URL of the thumbnail version of the image.
		 */
		QUrl Thumb_;

		/** @brief URL of the full version of the image.
		 */
		QUrl Full_;
	};

	/** @brief Information about artist biography.
	 *
	 * For now this structure only contains basic information about the
	 * artist, duplicating ArtistInfo. This may be changed/extended some
	 * time in the future, though.
	 *
	 * @sa ArtistInfo
	 */
	struct ArtistBio
	{
		/** @brief Basic information about this artist.
		 */
		ArtistInfo BasicInfo_;

		/** @brief Other images for this artist.
		 *
		 * This list will typically include posters, concerts photos and
		 * similar stuff.
		 */
		QList<ArtistImage> OtherImages_;
	};

	/** @brief Pending biography request handle.
	 *
	 * Interface to a pending biography search in an IArtistBioFetcher.
	 * An object implementing this interface is returned from the
	 * IArtistBioFetcher::RequestArtistBio() method and is used to track
	 * the status of biography requests.
	 *
	 * This class has some signals (ready() and error()), and one can use
	 * the GetQObject() method to get an object of this class as a
	 * QObject and connect to those signals.
	 *
	 * @note The object of this class should schedule its deletion (via
	 * <code>QObject::deleteLater()</code>, for example) after ready() or
	 * error() signal is emitted. Thus the calling code should never
	 * delete it explicitly, neither it should use this object after
	 * ready() or error() signals or connect to its signals via
	 * <code>Qt::QueuedConnection</code>.
	 *
	 * @sa IArtistBioFetcher
	 */
	class Q_DECL_EXPORT IPendingArtistBio
	{
	public:
		virtual ~IPendingArtistBio () {}

		/** @brief Returns this object as a QObject.
		 *
		 * This function can be used to connect to the signals of this
		 * class.
		 *
		 * @return This object as a QObject.
		 */
		virtual QObject* GetQObject () = 0;

		/** @brief Returns the artist biography.
		 *
		 * This function returns the fetched artist biography, or an
		 * empty biography if it is not found or search isn't completed
		 * yet.
		 *
		 * @return The fetched artist biography.
		 */
		virtual ArtistBio GetArtistBio () const = 0;
	protected:
		/** @brief Emitted when the biography is ready and fetched.
		 *
		 * The object will be invalid after this signal is emitted and
		 * the event loop is run.
		 */
		virtual void ready () = 0;

		/** @brief Emitted when there is an error fetching the biography.
		 *
		 * The object will be invalid after this signal is emitted and
		 * the event loop is run.
		 */
		virtual void error () = 0;
	};

	/** @brief Interface for plugins supporting fetching artist biography.
	 *
	 * Plugins that support fetching artist biography from the sources
	 * Last.FM should implement this interface.
	 */
	class Q_DECL_EXPORT IArtistBioFetcher
	{
	public:
		virtual ~IArtistBioFetcher () {}

		/** @brief Returns the service name.
		 *
		 * This string returns a human-readable string with the service
		 * name, like "Last.FM".
		 *
		 * @return The human-readable service name.
		 */
		virtual QString GetServiceName () const = 0;

		/** @brief Requests the biography of the given artist.
		 *
		 * This function initiates a search for artist biography and
		 * returns a handle through which the results of the search could
		 * be obtained. The handle owns itself and deletes itself after
		 * results are available — see its documentation for details.
		 *
		 * @param[in] artist The artist name.
		 * @param[in] additionalImages Whether additional images for the
		 * ArtistBio::OtherImages_ field should be requested.
		 * @return The pending biography search handle.
		 */
		virtual IPendingArtistBio* RequestArtistBio (const QString& artist,
				bool additionalImages = true) = 0;
	};
}

Q_DECLARE_INTERFACE (Media::IPendingArtistBio, "org.LeechCraft.Media.IPendingArtistBio/1.0")
Q_DECLARE_INTERFACE (Media::IArtistBioFetcher, "org.LeechCraft.Media.IArtistBioFetcher/1.0")
