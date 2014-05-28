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

namespace LeechCraft
{
namespace AN
{
/** @brief Namespace for various AN entity fields.
 *
 * This namespace contains various constants for widely-used fields
 * in the AdvancedNotifications-related Entity structure.
 */
namespace EF
{
	/** @brief The plugin ID of the sender (QByteArray or QString).
	 */
	const QString SenderID { "org.LC.AdvNotifications.SenderID" };

	/** @brief The category of the event (QString).
	 *
	 * To notify about an event, this field should contain one of the
	 * predefined event categories (like CatIM, CatDownloads and so on).
	 * To cancel an event (for example, when all unread messages have
	 * been read), this field should contain the CatEventCancel category.
	 */
	const QString EventCategory { "org.LC.AdvNotifications.EventCategory" };

	/** @brief The ID of the event (QString).
	 *
	 * Events relating to the same object (like IM messages from the same
	 * contact) should have the same event ID.
	 */
	const QString EventID { "org.LC.AdvNotifications.EventID" };

	/** @brief Visual path to this event (QStringList).
	 *
	 * This field should contain the list of human-readable strings that
	 * allow grouping of various events into tree-like structures.
	 */
	const QString VisualPath { "org.LC.AdvNotifications.VisualPath" };

	/** @brief The type of the event (QString).
	 *
	 * This field should contain one of the event types related to the
	 * given EventCategory, like TypeIMAttention or TypeIMIncMsg for
	 * the CatIM category.
	 *
	 * @note This field is also used when creating rules. In this case,
	 * it should contain a QStringList with all the event types the rule
	 * being created relates to.
	 */
	const QString EventType { "org.LC.AdvNotifications.EventType" };

	/** @brief The detailed text of the event (QString).
	 *
	 * @note This field is optional.
	 */
	const QString FullText { "org.LC.AdvNotifications.FullText" };

	/** @brief The even more detailed text than FullText (QString).
	 *
	 * @note This field is optional.
	 */
	const QString ExtendedText { "org.LC.AdvNotifications.ExtendedText" };

	/** @brief The change in event count (int).
	 *
	 * This field represents the change in the count of the events with
	 * the given EventID.
	 *
	 * For example, if two messages arrive simultaneously from the same
	 * contact in an IM client, this field should be equal to 2.
	 *
	 * @note Either this field or the Count field should be present.
	 */
	const QString DeltaCount { "org.LC.AdvNotifications.DeltaCount" };

	/** @brief The new total event count (int).
	 *
	 * This field represents how many events with the given EventID are
	 * there pending now.
	 *
	 * @note Either this field or the DeltaCount field should be present.
	 */
	const QString Count { "org.LC.AdvNotifications.Count" };
}
}
}
