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

#include <QList>
#include <QVariantMap>
#include <util/sll/eitherfwd.h>
#include "imessage.h"

template<typename>
class QFuture;

class QDateTime;

class QObject;

namespace LeechCraft
{
namespace Azoth
{
	class IAccount;

	/** @brief Describes a single chat log item.
	 */
	struct HistoryItem
	{
		/** @brief The timestamp of the message.
		 */
		QDateTime Date_;

		/** @brief The direction of the message.
		 */
		IMessage::Direction Dir_;

		/** @brief The message itself.
		 */
		QString Message_;

		/** @brief The variant of the other entry.
		 */
		QString Variant_;

		/** @brief The message type.
		 */
		IMessage::Type Type_;

		/** @brief The rich message contents, if any.
		 */
		QString RichMessage_;

		/** @brief Whether the message should be HTML-escaped when
		 * displayed to the user.
		 */
		IMessage::EscapePolicy EscPolicy_;
	};

	/** @brief Interface for plugins storing chat history.
	 *
	 * This interface should be implemented by plugins that store chat
	 * history for Azoth to provide additional features using these
	 * plugins.
	 */
	class IHistoryPlugin
	{
	public:
		virtual ~IHistoryPlugin () {}

		/** @brief Whether history is enabled for the given entry.
		 *
		 * This method checks if history logging is enabled for the
		 * given entry.
		 *
		 * @param[in] entry The entry to check (implements ICLEntry).
		 * @return Whether history logging is enabled for this entry.
		 */
		virtual bool IsHistoryEnabledFor (QObject *entry) const = 0;

		/** @brief Requests last messages for the given entry.
		 *
		 * This method, when called, requests last num messages from
		 * the chat log with the entry.
		 *
		 * This method is asynchronous: it is expected to return soon
		 * after being called, and the result is expected to be emitted
		 * via the gotLastMessages() signal.
		 *
		 * @param[in] entry The entry for which to query the history
		 * (implements ICLEntry).
		 * @param[in] num The maximum number of messages to retrieve.
		 *
		 * @sa gotLastMessages()
		 */
		virtual void RequestLastMessages (QObject *entry, int num) = 0;

		using MaxTimestampResult_t = Util::Either<QString, QDateTime>;

		virtual QFuture<MaxTimestampResult_t> RequestMaxTimestamp (IAccount *acc) = 0;

		/** @brief Adds a message to the history.
		 *
		 * @param[in] accountId The unique ID of the corresponding account.
		 * @param[in] entryId The unique ID of the corresponding entry.
		 * @param[in] visibleName The human-readable name of the entry.
		 * @param[in] item The HistoryItem struct describing the message.
		 */
		virtual void AddRawMessages (const QString& accountId,
				const QString& entryId,
				const QString& visibleName,
				const QList<HistoryItem>& items) = 0;
	protected:
		/** @brief Notifies about last messages for the given entry.
		 *
		 * This signal should be emitted when last chat messages with
		 * the given entry have been retrieved from the history as the
		 * result of the call to RequestLastMessages().
		 *
		 * If there are no messages for the entry, the implementation
		 * may either emit this signal with empty messages list or
		 * choose to not emit any signals at all.
		 *
		 * @note This function is expected to be a signal.
		 *
		 * @sa RequestLastMessages()
		 */
		virtual void gotLastMessages (QObject *entry, const QList<QObject*>& messages) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Azoth::IHistoryPlugin,
		"org.Deviant.LeechCraft.Azoth.IHistoryPlugin/1.0")
