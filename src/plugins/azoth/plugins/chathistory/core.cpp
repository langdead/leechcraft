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

#include "core.h"
#include <cmath>
#include <QMetaObject>
#include <QVariant>
#include <QSettings>
#include <QCoreApplication>
#include <QtDebug>
#include <interfaces/azoth/imessage.h>
#include <interfaces/azoth/iproxyobject.h>
#include <interfaces/azoth/iclentry.h>
#include <interfaces/azoth/iaccount.h>
#include "storage.h"
#include "storagethread.h"

namespace LeechCraft
{
namespace Azoth
{
namespace ChatHistory
{
	std::shared_ptr<Core> Core::InstPtr_;

	Core::Core ()
	: StorageThread_ (new StorageThread ())
	, PluginProxy_ (0)
	{
		StorageThread_->start (QThread::LowestPriority);

		TabClass_.TabClass_ = "Chathistory";
		TabClass_.VisibleName_ = tr ("Chat history");
		TabClass_.Description_ = tr ("Chat history viewer for the Azoth IM");
		TabClass_.Priority_ = 40;
		TabClass_.Features_ = TFOpenableByRequest;
		TabClass_.Icon_ = QIcon ("lcicons:/azoth/chathistory/resources/images/chathistory.svg");

		LoadDisabled ();
	}

	std::shared_ptr<Core> Core::Instance ()
	{
		if (!InstPtr_)
			InstPtr_.reset (new Core);
		return InstPtr_;
	}

	Core::~Core ()
	{
		StorageThread_->quit ();
		StorageThread_->wait (2000);

		if (StorageThread_->isRunning ())
		{
			qWarning () << Q_FUNC_INFO
					<< "storage thread still running, forcefully terminating...";
			StorageThread_->terminate ();
			StorageThread_->wait (5000);
		}
		else
			delete StorageThread_;
	}

	TabClassInfo Core::GetTabClass () const
	{
		return TabClass_;
	}

	void Core::SetCoreProxy (ICoreProxy_ptr proxy)
	{
		CoreProxy_ = proxy;
	}

	ICoreProxy_ptr Core::GetCoreProxy () const
	{
		return CoreProxy_;
	}

	void Core::SetPluginProxy (QObject *proxy)
	{
		PluginProxy_ = qobject_cast<IProxyObject*> (proxy);
	}

	IProxyObject* Core::GetPluginProxy () const
	{
		return PluginProxy_;
	}

	bool Core::IsLoggingEnabled (QObject *entryObj) const
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< entryObj
					<< "could not be casted to ICLEntry";
			return true;
		}

		return !DisabledIDs_.contains (entry->GetEntryID ());
	}

	void Core::SetLoggingEnabled (QObject *entryObj, bool enable)
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< entryObj
					<< "could not be casted to ICLEntry";
			return;
		}

		const QString& id = entry->GetEntryID ();
		if (enable)
			DisabledIDs_.remove (id);
		else
			DisabledIDs_ << id;

		SaveDisabled ();
	}

	void Core::Process (QObject *msgObj)
	{
		IMessage *msg = qobject_cast<IMessage*> (msgObj);
		if (msg->GetMessageType () != IMessage::MTChatMessage &&
			msg->GetMessageType () != IMessage::MTMUCMessage)
			return;

		if (msg->GetBody ().isEmpty ())
			return;

		if (msg->GetDirection () == IMessage::Direction::Out &&
				msg->GetMessageType () == IMessage::MTMUCMessage)
			return;

		const double secsDiff = msg->GetDateTime ().secsTo (QDateTime::currentDateTime ());
		if (msg->GetMessageType () == IMessage::MTMUCMessage &&
				std::abs (secsDiff) >= 2)
			return;

		ICLEntry *entry = qobject_cast<ICLEntry*> (msg->ParentCLEntry ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "message's other part doesn't implement ICLEntry"
					<< msg->GetQObject ()
					<< msg->OtherPart ();
			return;
		}
		if (DisabledIDs_.contains (entry->GetEntryID ()))
			return;

		IAccount *acc = qobject_cast<IAccount*> (entry->GetParentAccount ());
		if (!acc)
		{
			qWarning () << Q_FUNC_INFO
					<< "message's account doesn't implement IAccount"
					<< entry->GetParentAccount ();
			return;
		}

		QVariantMap data;
		data ["EntryID"] = entry->GetEntryID ();
		data ["AccountID"] = acc->GetAccountID ();
		data ["DateTime"] = msg->GetDateTime ();
		data ["Direction"] = msg->GetDirection () == IMessage::Direction::In ? "IN" : "OUT";
		data ["Body"] = msg->GetBody ();
		data ["OtherVariant"] = msg->GetOtherVariant ();
		data ["MessageType"] = static_cast<int> (msg->GetMessageType ());

		if (entry->GetEntryType () == ICLEntry::ETPrivateChat)
		{
			ICLEntry *parent = qobject_cast<ICLEntry*> (entry->GetParentCLEntry ());
			data ["VisibleName"] = parent->GetEntryName () + "/" + entry->GetEntryName ();
		}
		else
			data ["VisibleName"] = entry->GetEntryName ();

		QMetaObject::invokeMethod (StorageThread_->GetStorage (),
				"addMessage",
				Qt::QueuedConnection,
				Q_ARG (QVariantMap, data));
	}

	void Core::Process (QVariantMap data)
	{
		data ["Direction"] = data ["Direction"].toString ().toUpper ();

		QMetaObject::invokeMethod (StorageThread_->GetStorage (),
				"addMessage",
				Qt::QueuedConnection,
				Q_ARG (QVariantMap, data));
	}

	void Core::GetOurAccounts ()
	{
		QMetaObject::invokeMethod (StorageThread_->GetStorage (),
				"getOurAccounts",
				Qt::QueuedConnection);
	}

	void Core::GetUsersForAccount (const QString& accountID)
	{
		QMetaObject::invokeMethod (StorageThread_->GetStorage (),
				"getUsersForAccount",
				Qt::QueuedConnection,
				Q_ARG (QString, accountID));
	}

	void Core::GetChatLogs (const QString& accountId,
			const QString& entryId, int backpages, int amount)
	{
		QMetaObject::invokeMethod (StorageThread_->GetStorage (),
				"getChatLogs",
				Qt::QueuedConnection,
				Q_ARG (QString, accountId),
				Q_ARG (QString, entryId),
				Q_ARG (int, backpages),
				Q_ARG (int, amount));
	}

	void Core::Search (const QString& accountId, const QString& entryId,
			const QString& text, int shift, bool cs)
	{
		QMetaObject::invokeMethod (StorageThread_->GetStorage (),
				"search",
				Qt::QueuedConnection,
				Q_ARG (QString, accountId),
				Q_ARG (QString, entryId),
				Q_ARG (QString, text),
				Q_ARG (int, shift),
				Q_ARG (bool, cs));
	}

	void Core::Search (const QString& accountId, const QString& entryId, const QDateTime& dt)
	{
		QMetaObject::invokeMethod (StorageThread_->GetStorage (),
				"searchDate",
				Qt::QueuedConnection,
				Q_ARG (QString, accountId),
				Q_ARG (QString, entryId),
				Q_ARG (QDateTime, dt));
	}

	void Core::GetDaysForSheet (const QString& accountId, const QString& entryId, int year, int month)
	{
		QMetaObject::invokeMethod (StorageThread_->GetStorage (),
				"getDaysForSheet",
				Qt::QueuedConnection,
				Q_ARG (QString, accountId),
				Q_ARG (QString, entryId),
				Q_ARG (int, year),
				Q_ARG (int, month));
	}

	void Core::ClearHistory (const QString& accountId, const QString& entryId)
	{
		QMetaObject::invokeMethod (StorageThread_->GetStorage (),
				"clearHistory",
				Qt::QueuedConnection,
				Q_ARG (QString, accountId),
				Q_ARG (QString, entryId));
	}

	void Core::RegenUsersCache ()
	{
		QMetaObject::invokeMethod (StorageThread_->GetStorage (),
				"regenUsersCache",
				Qt::QueuedConnection);
	}

	void Core::LoadDisabled ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Azoth_ChatHistory");
		DisabledIDs_ = settings.value ("DisabledIDs").toStringList ().toSet ();
	}

	void Core::SaveDisabled ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Azoth_ChatHistory");
		settings.setValue ("DisabledIDs", QStringList (DisabledIDs_.toList ()));
	}
}
}
}
