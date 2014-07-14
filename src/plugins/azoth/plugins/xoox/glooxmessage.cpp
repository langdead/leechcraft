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

#include "glooxmessage.h"
#include <QTextDocument>
#include <QtDebug>
#include <QXmppClient.h>
#include "glooxclentry.h"
#include "clientconnection.h"
#include "core.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	const QString NsXhtmlIM = "http://jabber.org/protocol/xhtml-im";
	const QString NsXhtml = "http://www.w3.org/1999/xhtml";

	GlooxMessage::GlooxMessage (IMessage::MessageType type,
			IMessage::Direction dir,
			const QString& jid,
			const QString& variant,
			ClientConnection *conn)
	: Type_ (type)
	, SubType_ (MSTOther)
	, Direction_ (dir)
	, BareJID_ (jid)
	, Variant_ (variant)
	, DateTime_ (QDateTime::currentDateTime ())
	, Connection_ (conn)
	, IsDelivered_ (false)
	{
		const QString& remoteJid = variant.isEmpty () ?
				jid :
				jid + "/" + variant;
		if (type == MTChatMessage && variant.isEmpty ())
		{
			QObject *object = Connection_->GetCLEntry (jid, variant);
			Variant_ = qobject_cast<ICLEntry*> (object)->Variants ().value (0);
		}
		Message_.setTo (dir == Direction::In ? conn->GetOurJID () : remoteJid);
	}

	GlooxMessage::GlooxMessage (const QXmppMessage& message,
			ClientConnection *conn)
	: Type_ (MTChatMessage)
	, SubType_ (MSTOther)
	, Direction_ (Direction::In)
	, Message_ (message)
	, Connection_ (conn)
	, IsDelivered_ (false)
	{
		Connection_->Split (message.from (), &BareJID_, &Variant_);

		if (!Message_.stamp ().isValid ())
			Message_.setStamp (QDateTime::currentDateTime ());
		else
			Message_.setStamp (Message_.stamp ().toLocalTime ());
		DateTime_ = Message_.stamp ();
	}

	QObject* GlooxMessage::GetQObject ()
	{
		return this;
	}

	void GlooxMessage::Send ()
	{
		if (Direction_ == Direction::In)
		{
			qWarning () << Q_FUNC_INFO
					<< "tried to send incoming message";
			return;
		}

		switch (Type_)
		{
		case MTChatMessage:
			Message_.setReceiptRequested (true);
		case MTMUCMessage:
			Connection_->SendMessage (this);
			QMetaObject::invokeMethod (OtherPart (),
					"gotMessage",
					Q_ARG (QObject*, this));
			break;
		default:
			qWarning () << Q_FUNC_INFO
					<< this
					<< "cannot send a message of type"
					<< Type_;
			break;
		}
	}

	void GlooxMessage::Store ()
	{
		QMetaObject::invokeMethod (OtherPart (),
				"gotMessage",
				Q_ARG (QObject*, this));
	}

	IMessage::Direction GlooxMessage::GetDirection () const
	{
		return Direction_;
	}

	IMessage::MessageType GlooxMessage::GetMessageType () const
	{
		return Type_;
	}

	IMessage::MessageSubType GlooxMessage::GetMessageSubType () const
	{
		return SubType_;
	}

	void GlooxMessage::SetMessageSubType (IMessage::MessageSubType subType)
	{
		SubType_ = subType;
	}

	QObject* GlooxMessage::OtherPart () const
	{
		return Connection_->GetCLEntry (BareJID_, Variant_);
	}

	QString GlooxMessage::GetOtherVariant () const
	{
		return Variant_;
	}

	QString GlooxMessage::GetBody () const
	{
		return Message_.body ();
	}

	void GlooxMessage::SetBody (const QString& body)
	{
		Message_.setBody (body);
	}

	QDateTime GlooxMessage::GetDateTime () const
	{
		return DateTime_;
	}

	void GlooxMessage::SetDateTime (const QDateTime& dateTime)
	{
		DateTime_ = dateTime;
		if (Direction_ == Direction::In)
			Message_.setStamp (dateTime);
	}

	bool GlooxMessage::IsDelivered () const
	{
		return IsDelivered_;
	}

	QString GlooxMessage::GetRichBody () const
	{
		return Message_.xhtml ();
	}

	void GlooxMessage::SetRichBody (const QString& html)
	{
		Message_.setXhtml (html);
	}

	void GlooxMessage::SetDelivered (bool delivered)
	{
		IsDelivered_ = delivered;
		if (delivered)
			emit messageDelivered ();
	}

	void GlooxMessage::SetVariant (const QString& variant)
	{
		if (variant == Variant_)
			return;

		Variant_ = variant;

		if (Direction_ == Direction::In)
			Message_.setFrom (Variant_.isEmpty () ?
					BareJID_ :
					(BareJID_ + '/' + Variant_));
	}

	QXmppMessage GlooxMessage::GetNativeMessage () const
	{
		return Message_;
	}
}
}
}
