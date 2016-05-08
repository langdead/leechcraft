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

#ifndef PLUGINS_AGGREGATOR_INTERFACES_AGGREGATOR_CHANNEL_H
#define PLUGINS_AGGREGATOR_INTERFACES_AGGREGATOR_CHANNEL_H
#include <memory>
#include <vector>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QStringList>
#include <QImage>
#include "item.h"
#include "common.h"

namespace LeechCraft
{
namespace Aggregator
{
	struct ChannelShort
	{
		IDType_t ChannelID_;
		IDType_t FeedID_;
		QString Author_;
		QString Title_;
		QString DisplayTitle_;
		QString Link_;
		QStringList Tags_;
		QDateTime LastBuild_;
		QImage Favicon_;
		int Unread_;
	};

	struct Channel
	{
		IDType_t ChannelID_;
		IDType_t FeedID_;
		QString Title_;
		QString DisplayTitle_;
		QString Link_;
		QString Description_;
		QDateTime LastBuild_;
		QStringList Tags_;
		QString Language_;
		QString Author_;
		QString PixmapURL_;
		QImage Pixmap_;
		QImage Favicon_;
		items_container_t Items_;

		Channel (const IDType_t& feedId);
		Channel (const IDType_t& feedId, const IDType_t& channelId);
		Channel (const Channel&);
		Channel& operator= (const Channel&);

		int CountUnreadItems () const;
		void Equalify (const Channel&);
		ChannelShort ToShort () const;
	};

	typedef std::shared_ptr<Channel> Channel_ptr;
	typedef std::vector<Channel_ptr> channels_container_t;
	typedef std::vector<ChannelShort> channels_shorts_t;

	bool operator< (const ChannelShort&, const ChannelShort&);
	bool operator== (const ChannelShort&, const ChannelShort&);
	bool operator== (const Channel_ptr&, const ChannelShort&);
	bool operator== (const ChannelShort&, const Channel_ptr&);
	bool operator== (const Channel&, const Channel&);
	QDataStream& operator<< (QDataStream&, const Channel&);
	QDataStream& operator>> (QDataStream&, Channel&);
}
}

Q_DECLARE_METATYPE (LeechCraft::Aggregator::ChannelShort)
Q_DECLARE_METATYPE (LeechCraft::Aggregator::Channel_ptr)
Q_DECLARE_METATYPE (LeechCraft::Aggregator::channels_container_t)

#endif
