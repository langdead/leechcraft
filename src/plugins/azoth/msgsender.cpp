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

#include "msgsender.h"
#include <memory>
#include <QMessageBox>
#include <util/xpc/defaulthookproxy.h>
#include "interfaces/azoth/iclentry.h"
#include "interfaces/azoth/irichtextmessage.h"
#include "core.h"

namespace LeechCraft
{
namespace Azoth
{
	MsgSender::MsgSender (ICLEntry *e, IMessage::Type type, QString text, QString variant, QString richText)
	{
		deleteLater ();

		Core::Instance ().RegisterHookable (this);

		auto proxy = std::make_shared<Util::DefaultHookProxy> ();

		// TODO pass type without casts
		emit hookMessageWillCreated (proxy, this, e->GetQObject (), static_cast<int> (type), variant);
		if (proxy->IsCancelled ())
			return;

		int intType = static_cast<int> (type);
		proxy->FillValue ("type", intType);
		type = static_cast<IMessage::Type> (intType);
		proxy->FillValue ("variant", variant);
		proxy->FillValue ("text", text);

		const auto msg = e->CreateMessage (type, variant, text);
		if (!msg)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to create message to"
					<< e->GetEntryID ();
			return;
		}
		const auto richMsg = qobject_cast<IRichTextMessage*> (msg->GetQObject ());
		if (richMsg &&
				!richText.isEmpty ())
			richMsg->SetRichBody (richText);

		proxy = std::make_shared<Util::DefaultHookProxy> ();
		emit hookMessageCreated (proxy, this, msg->GetQObject ());
		if (proxy->IsCancelled ())
			return;

		try
		{
			msg->Send ();
		}
		catch (const std::exception& ex)
		{
			qWarning () << Q_FUNC_INFO
					<< "error sending message to"
					<< e->GetEntryID ()
					<< e->GetEntryName ()
					<< variant
					<< ex.what ();

			throw;
		}
	}
}
}
