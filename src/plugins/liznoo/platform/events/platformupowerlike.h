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

#include <memory>
#include "platformlayer.h"
#include "../common/dbusthread.h"

namespace LeechCraft
{
namespace Liznoo
{
namespace Events
{
	template<typename ConnT>
	class PlatformUPowerLike : public PlatformLayer
	{
		using DBusThread_ptr = std::shared_ptr<DBusThread<ConnT>>;

		const DBusThread_ptr Thread_;
	public:
		PlatformUPowerLike (const DBusThread_ptr& thread,
				const ICoreProxy_ptr& proxy, QObject *parent = nullptr)
		: PlatformLayer { proxy, parent }
		, Thread_ { thread }
		{
			Thread_->ScheduleImpl ([this] (ConnT *conn)
					{
						connect (conn,
								SIGNAL (gonnaSleep (int)),
								this,
								SLOT (emitGonnaSleep (int)));
						connect (conn,
								SIGNAL (wokeUp ()),
								this,
								SLOT (emitWokeUp ()));

						QMetaObject::invokeMethod (this,
								"setAvailable",
								 Qt::QueuedConnection,
								 Q_ARG (bool, conn->ArePowerEventsAvailable ()));
					});
		}
	};

	template<typename ConnT>
	using PlatformUPowerLike_ptr = std::shared_ptr<PlatformUPowerLike<ConnT>>;

	template<typename ConnT>
	PlatformUPowerLike_ptr<ConnT> MakeUPowerLike (const DBusThread_ptr<ConnT>& thread, const ICoreProxy_ptr& proxy)
	{
		return std::make_shared<PlatformUPowerLike<ConnT>> (thread, proxy);
	}
}
}
}
