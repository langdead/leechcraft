/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
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

#include "channelsmanager.h"
#include <util/util.h>
#include <util/sll/prelude.h>
#include "xmlsettingsmanager.h"
#include "ircserverhandler.h"
#include "channelhandler.h"
#include "channelclentry.h"
#include "ircaccount.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Acetamide
{
	ChannelsManager::ChannelsManager (IrcServerHandler *ish)
	: QObject (ish)
	, ISH_ (ish)
	{
	}

	IrcAccount* ChannelsManager::GetAccount () const
	{
		return ISH_->GetAccount ();
	}

	QString ChannelsManager::GetOurNick () const
	{
		return ISH_->GetNickName ();
	}

	QString ChannelsManager::GetServerID () const
	{
		return ISH_->GetServerID ();
	}

	ServerOptions ChannelsManager::GetServerOptions () const
	{
		return ISH_->GetServerOptions ();
	}

	QObjectList ChannelsManager::GetCLEntries () const
	{
		QObjectList result;
		for (const auto& ich : ChannelHandlers_)
		{
			result << ich->GetParticipants ();
			result << ich->GetCLEntry ();
		}

		return result;
	}

	ChannelHandler* ChannelsManager::GetChannelHandler (const QString& channel)
	{
		return ChannelHandlers_.value (channel.toLower ()).get ();
	}

	QList<ChannelHandler*> ChannelsManager::GetChannels () const
	{
		return Util::Map (ChannelHandlers_, &ChannelHandler_ptr::get);
	}

	bool ChannelsManager::IsChannelExists (const QString& channel) const
	{
		return ChannelHandlers_.contains (channel.toLower ());
	}

	int ChannelsManager::Count () const
	{
		return ChannelHandlers_.count ();
	}

	QSet<ChannelOptions> ChannelsManager::GetChannelsQueue () const
	{
		return ChannelsQueue_;
	}

	void ChannelsManager::CleanQueue ()
	{
		ChannelsQueue_.clear ();
	}

	void ChannelsManager::AddChannel2Queue (const ChannelOptions& options)
	{
		ChannelsQueue_ << options;
	}

	bool ChannelsManager::AddChannel (const ChannelOptions& options)
	{
		const auto ch = std::make_shared<ChannelHandler> (options, this);
		ChannelHandlers_ [options.ChannelName_.toLower ()] = ch;

		ChannelCLEntry *ichEntry = ch->GetCLEntry ();
		if (!ichEntry)
			return false;

		return true;
	}

	void ChannelsManager::LeaveChannel (const QString& channel, const QString& msg)
	{
		ISH_->LeaveChannel (channel, msg);
	}

	void ChannelsManager::CloseChannel (const QString& channel)
	{
		const QString& chnnl = channel.toLower ();
		if (ChannelHandlers_.contains (chnnl))
			ChannelHandlers_ [chnnl]->CloseChannel ();
	}

	void ChannelsManager::CloseAllChannels () const
	{
		Util::Map (ChannelHandlers_,
				[] (const ChannelHandler_ptr& ich) { ich->CloseChannel (); });
	}

	void ChannelsManager::UnregisterChannel (ChannelHandler *ich)
	{
		ChannelHandlers_.remove (ich->GetChannelOptions ().ChannelName_);

		if (!ChannelHandlers_.count () &&
				XmlSettingsManager::Instance ()
						.property ("AutoDisconnectFromServer").toBool ())
			ISH_->DisconnectFromServer ();
	}

	QHash<QString, QObject*> ChannelsManager::GetParticipantsByNick (const QString& nick) const
	{
		QHash<QString, QObject*> result;
		for (const auto& ich : ChannelHandlers_)
			if (ich->IsUserExists (nick))
				result [ich->GetChannelOptions ().ChannelName_] = ich->GetParticipantEntry (nick).get ();
		return result;
	}

	void ChannelsManager::AddParticipant (const QString& channel, const QString& nick,
			const QString& user, const QString& host)
	{
		const QString& chnnl = channel.toLower ();
		if (ChannelHandlers_.contains (chnnl))
			ChannelHandlers_ [chnnl]->SetChannelUser (nick, user, host);
	}

	void ChannelsManager::LeaveParticipant (const QString& channel,
			const QString& nick, const QString& msg)
	{
		const QString& chnnl = channel.toLower ();
		if (ChannelHandlers_.contains (chnnl))
			ChannelHandlers_ [chnnl]->LeaveParticipant (nick, msg);
	}

	void ChannelsManager::QuitParticipant (const QString& nick, const QString& msg)
	{
		Q_FOREACH (auto ch, ChannelHandlers_)
			if (ch->IsUserExists (nick))
				ch->LeaveParticipant (nick, msg);
	}

	void ChannelsManager::KickParticipant (const QString& channel,
			const QString& target, const QString& reason, const QString& who)
	{
		const QString& chnnl = channel.toLower ();
		if (ChannelHandlers_.contains (chnnl))
			ChannelHandlers_ [chnnl]->KickParticipant (target, reason, who);
	}

	void ChannelsManager::KickCommand (const QString& channel,
			const QString& nick, const QString& reason)
	{
		ISH_->KickParticipant (channel, nick, reason);
	}

	void ChannelsManager::ChangeNickname (const QString& oldNick, const QString& newNick)
	{
		for (const auto& ich : ChannelHandlers_)
			if (ich->IsUserExists (oldNick))
				ich->ChangeNickname (oldNick, newNick);
	}

	void ChannelsManager::GotNames (const QString& channel, const QStringList& participants)
	{
		if (IsChannelExists (channel) &&
				!ChannelHandlers_ [channel]->IsRosterReceived ())
		{
			for (const auto& nick : participants)
				if (!nick.isEmpty ())
					ChannelHandlers_ [channel]->SetChannelUser (nick);
		}
		else
			ReceiveCmdAnswerMessage ("names", participants.join (" "), false);
	}

	void ChannelsManager::GotEndOfNamesCmd (const QString& channel)
	{
		if (ChannelHandlers_.contains (channel) &&
				!ChannelHandlers_ [channel]->IsRosterReceived ())
		{
			ChannelHandlers_ [channel]->SetRosterReceived (true);
			ISH_->GetAccount ()->handleGotRosterItems (QObjectList () << ChannelHandlers_ [channel]->GetCLEntry ());
		}
		else
			ReceiveCmdAnswerMessage ("names", "End of /NAMES", true);
	}

	void ChannelsManager::SendPublicMessage (const QString& channel, const QString& msg)
	{
		LastActiveChannel_ = channel.toLower ();
		const QString& chnnl = channel.toLower ();
		if (msg.startsWith ('/'))
		{
			if (ChannelHandlers_.contains (chnnl))
			{
				const QString& cmd = ISH_->ParseMessageForCommand (msg, chnnl);
				if (cmd == "say")
					ChannelHandlers_ [chnnl]->HandleIncomingMessage (ISH_->GetNickName (),
							msg.mid (4));
				else if (cmd == "me")
					ChannelHandlers_ [chnnl]->HandleIncomingMessage (ISH_->GetNickName (),
							msg);
				else
					ChannelHandlers_ [chnnl]->HandleServiceMessage (msg,
							IMessage::Type::EventMessage,
							IMessage::SubType::Other);
			}
		}
		else
		{
			ISH_->SendPublicMessage (msg, chnnl);
			if (ChannelHandlers_.contains (chnnl))
				ChannelHandlers_ [chnnl]->HandleIncomingMessage (ISH_->GetNickName (),
						msg);
		}
	}

	void ChannelsManager::ReceivePublicMessage (const QString& channel,
			const QString& nick, const QString& msg)
	{
		const QString& chnnl = channel.toLower ();
		if (ChannelHandlers_.contains (chnnl))
			ChannelHandlers_ [chnnl]->HandleIncomingMessage (nick, msg);
	}

	bool ChannelsManager::ReceiveCmdAnswerMessage (const QString&,
			const QString& answer, bool)
	{
		if (LastActiveChannel_.isEmpty () ||
				!ChannelHandlers_.contains (LastActiveChannel_))
			return false;

		ChannelHandlers_ [LastActiveChannel_]->HandleServiceMessage (answer,
				IMessage::Type::EventMessage,
				IMessage::SubType::Other);

		return true;
	}

	void ChannelsManager::SetMUCSubject (const QString& channel, const QString& topic)
	{
		const QString& chnnl = channel.toLower ();
		if (ChannelHandlers_.contains (chnnl))
			ChannelHandlers_ [chnnl]->SetMUCSubject (topic);

		ReceiveCmdAnswerMessage ("topic", topic, ISH_->IsCmdHasLongAnswer ("topic"));
	}

	void ChannelsManager::SetTopic (const QString& channel, const QString& topic)
	{
		ISH_->SetTopic (channel, topic);
	}

	void ChannelsManager::CTCPReply (const QString& msg)
	{
		for (const auto& ich : ChannelHandlers_)
			ich->HandleServiceMessage (msg,
					IMessage::Type::ServiceMessage,
					IMessage::SubType::Other);
	}

	void ChannelsManager::CTCPRequestResult (const QString& msg)
	{
		for (const auto& ich : ChannelHandlers_)
			ich->HandleServiceMessage (msg,
					IMessage::Type::ServiceMessage,
					IMessage::SubType::Other);
	}

	void ChannelsManager::SetBanListItem (const QString& channel,
			const QString& mask, const QString& nick, const QDateTime& time)
	{
		if (ChannelHandlers_.contains (channel))
			ChannelHandlers_ [channel]->SetBanListItem (mask, nick, time);
	}

	void ChannelsManager::RequestBanList (const QString& channel)
	{
		ISH_->GetBanList (channel);
	}

	void ChannelsManager::AddBanListItem (const QString& channel, const QString& mask)
	{
		ISH_->AddBanListItem (channel, mask);
	}

	void ChannelsManager::RemoveBanListItem (const QString& channel, const QString& mask)
	{
		ISH_->RemoveBanListItem (channel, mask);
	}

	void ChannelsManager::SetExceptListItem (const QString& channel,
			const QString& mask, const QString& nick, const QDateTime& time)
	{
		if (ChannelHandlers_.contains (channel))
			ChannelHandlers_ [channel]->SetExceptListItem (mask, nick, time);
	}

	void ChannelsManager::RequestExceptList (const QString& channel)
	{
		ISH_->GetExceptList (channel);
	}

	void ChannelsManager::AddExceptListItem (const QString& channel, const QString& mask)
	{
		ISH_->AddExceptListItem (channel, mask);
	}

	void ChannelsManager::RemoveExceptListItem (const QString& channel, const QString& mask)
	{
		ISH_->RemoveExceptListItem (channel, mask);
	}

	void ChannelsManager::SetInviteListItem (const QString& channel,
			const QString& mask, const QString& nick, const QDateTime& time)
	{
		if (ChannelHandlers_.contains (channel))
			ChannelHandlers_ [channel]->SetInviteListItem (mask, nick, time);
	}

	void ChannelsManager::RequestInviteList (const QString& channel)
	{
		ISH_->GetInviteList (channel);
	}

	void ChannelsManager::AddInviteListItem (const QString& channel, const QString& mask)
	{
		ISH_->AddInviteListItem (channel, mask);
	}

	void ChannelsManager::RemoveInviteListItem (const QString& channel, const QString& mask)
	{
		ISH_->RemoveInviteListItem (channel, mask);
	}

	void ChannelsManager::ParseChanMode (const QString& channel,
			const QString& mode, const QString& value)
	{
		bool action = mode [0] == '+';

		for (int i = 1; i < mode.length (); ++i)
		{
			switch (mode [i].toLatin1 ())
			{
				case 'o':
					if (!value.isEmpty () &&
							ChannelHandlers_ [channel]->IsUserExists (value))
					{
						ChannelParticipantEntry_ptr entry =
								ChannelHandlers_ [channel]->GetParticipantEntry (value);
						if (action)
							entry->SetRole (ChannelRole::Operator);
						else
							entry->RemoveRole (ChannelRole::Operator);

						ChannelHandlers_ [channel]->MakePermsChangedMessage (value,
								ChannelRole::Operator, action);
					}
					break;
				case 'v':
					if (!value.isEmpty () &&
							ChannelHandlers_ [channel]->IsUserExists (value))
					{
						ChannelParticipantEntry_ptr entry =
								ChannelHandlers_ [channel]->GetParticipantEntry (value);
						if (action)
							entry->SetRole (ChannelRole::Voiced);
						else
							entry->RemoveRole (ChannelRole::Voiced);

						ChannelHandlers_ [channel]->MakePermsChangedMessage (value,
								ChannelRole::Voiced, action);
					}
					break;
				case 'a':
					// may be it is nessesary
					break;
				case 'i':
					ChannelHandlers_ [channel]->SetInviteMode (action);
					break;
				case 'm':
					ChannelHandlers_ [channel]->SetModerateMode (action);
					break;
				case 'n':
					ChannelHandlers_ [channel]->SetBlockOutsideMessagesMode (action);
					break;
				case 'q':
					if (ISH_->GetISupport ().contains ("PREFIX") &&
							ISH_->GetISupport () ["PREFIX"].contains ('q'))
						if (!value.isEmpty () &&
								ChannelHandlers_ [channel]->IsUserExists (value))
						{
							const auto& entry = ChannelHandlers_ [channel]->GetParticipantEntry (value);
							if (action)
								entry->SetRole (ChannelRole::Owner);
							else
								entry->RemoveRole (ChannelRole::Owner);

							ChannelHandlers_ [channel]->MakePermsChangedMessage (value,
									ChannelRole::Owner, action);
						}
					break;
				case 'p':
					ChannelHandlers_ [channel]->SetPrivateMode (action);
					break;
				case 'r':
					ChannelHandlers_ [channel]->SetServerReOpMode (action);
					break;
				case 's':
					ChannelHandlers_ [channel]->SetSecretMode (action);
					break;
				case 't':
					ChannelHandlers_ [channel]->SetOnlyOpTopicChangeMode (action);
					break;
				case 'l':
					ChannelHandlers_ [channel]->
							SetUserLimit (action, value.toInt ());
					break;
				case 'k':
					ChannelHandlers_ [channel]->SetChannelKey (action, value);
					break;
				case 'b':
					ISH_->ShowAnswer ("mode", tr ("%1 added to your ban list.")
							.arg (value));
					break;
				case 'e':
					ISH_->ShowAnswer ("mode", tr ("%1 added to your except list.")
							.arg (value));
					break;
				case 'I':
					ISH_->ShowAnswer ("mode", tr ("%1 added to your invite list.")
							.arg (value));
					break;
			}
		}
	}

	void ChannelsManager::SetNewChannelMode (const QString& channel,
			const QString& mode, const QString& name)
	{
		ISH_->SetNewChannelMode (channel, mode, name);
	}

	void ChannelsManager::SetNewChannelModes (const QString& channel,
			const ChannelModes& modes)
	{
		ISH_->SetNewChannelModes (channel, modes);
	}

	void ChannelsManager::RequestWhoIs (const QString& channel, const QString& nick)
	{
		LastActiveChannel_ = channel;
		ISH_->RequestWhoIs (nick);
	}

	void ChannelsManager::RequestWhoWas (const QString& channel, const QString& nick)
	{
		LastActiveChannel_ = channel;
		ISH_->RequestWhoWas (nick);
	}

	void ChannelsManager::RequestWho (const QString& channel, const QString& nick)
	{
		LastActiveChannel_ = channel;
		ISH_->RequestWho (nick);
	}

	void ChannelsManager::CTCPRequest (const QStringList& cmd, const QString& channel)
	{
		LastActiveChannel_ = channel;
		ISH_->CTCPRequst (cmd);
	}

	QMap<QString, QString> ChannelsManager::GetISupport () const
	{
		return ISH_->GetISupport ();
	}

	void ChannelsManager::SetPrivateChat (const QString& nick)
	{
		for (const auto entryObj : GetParticipantsByNick (nick))
			if (const auto entry = qobject_cast<IrcParticipantEntry*> (entryObj))
				entry->SetPrivateChat (true);
	}

	void ChannelsManager::CreateServerParticipantEntry (QString nick)
	{
		ISH_->CreateServerParticipantEntry (nick);
	}

	void ChannelsManager::UpdateEntry (const WhoMessage& message)
	{
		if (!ChannelHandlers_.contains (message.Channel_.toLower ()))
			return;

		ChannelHandlers_ [message.Channel_.toLower ()]->UpdateEntry (message);
	}

	int ChannelsManager::GetChannelUsersCount (const QString& channel)
	{
		if (!ChannelHandlers_.contains (channel.toLower ()))
			return 0;

		return ChannelHandlers_ [channel.toLower ()]->GetParticipants ().count ();
	}

	void ChannelsManager::ClosePrivateChat (const QString& nick)
	{
		ISH_->ClosePrivateChat (nick);
	}

	void ChannelsManager::SetChannelUrl (const QString& channel, const QString& url)
	{
		if (const auto& handler = ChannelHandlers_ [channel.toLower ()])
			handler->SetUrl (url);
	}

	void ChannelsManager::SetTopicWhoTime (const QString& channel, const QString& who, quint64 time)
	{
		const auto& handler = ChannelHandlers_ [channel.toLower ()];
		if (!handler)
			return;

		QString msg (tr ("Topic was set by %1 at %2")
				.arg (who)
				.arg (QDateTime::fromTime_t (time).toString (Qt::TextDate)));
		handler->HandleServiceMessage (msg,
				IMessage::Type::ServiceMessage,
				IMessage::SubType::Other);
	}

	uint qHash (const ChannelOptions& opts)
	{
		return qHash (opts.ChannelName_ + opts.ChannelPassword_ + opts.ServerName_);
	}
}
}
}
