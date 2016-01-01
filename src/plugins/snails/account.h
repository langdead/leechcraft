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
#include <functional>
#include <QObject>
#include <QHash>
#include "message.h"
#include "progresslistener.h"

class QMutex;
class QAbstractItemModel;
class QStandardItemModel;
class QStandardItem;
class QModelIndex;

template<typename T>
class QFuture;

namespace LeechCraft
{
namespace Snails
{
	class AccountLogger;
	class AccountThread;
	class AccountThreadWorker;
	class AccountFolderManager;
	class MailModel;
	class FoldersModel;
	class MailModelsManager;
	struct Folder;

	class Account : public QObject
	{
		Q_OBJECT

		friend class AccountThreadWorker;
		AccountLogger * const Logger_;
		AccountThread * const Thread_;
		AccountThread * const MessageFetchThread_;
		QMutex * const AccMutex_;

		QByteArray ID_;

		QString AccName_;
		QString UserName_;
		QString UserEmail_;

		QString Login_;
		bool UseSASL_ = false;
		bool SASLRequired_ = false;
	public:
		enum class SecurityType
		{
			TLS,
			SSL,
			No
		};
	private:
		bool UseTLS_ = true;
		bool UseSSL_ = false;
		bool InSecurityRequired_ = false;

		SecurityType OutSecurity_ = SecurityType::SSL;
		bool OutSecurityRequired_ = false;

		bool SMTPNeedsAuth_ = true;

		QString InHost_;
		int InPort_;

		QString OutHost_;
		int OutPort_;

		QString OutLogin_;

		int KeepAliveInterval_ = 90 * 1000;

		bool LogToFile_ = true;
	public:
		enum class Direction
		{
			In,
			Out
		};

		enum class OutType
		{
			SMTP,
			Sendmail
		};
	private:
		OutType OutType_;

		AccountFolderManager *FolderManager_;
		FoldersModel *FoldersModel_;

		MailModelsManager * const MailModelsManager_;
	public:
		Account (QObject* = 0);

		QByteArray GetID () const;
		QString GetName () const;
		QString GetServer () const;

		bool ShouldLogToFile () const;
		AccountLogger* GetLogger () const;

		AccountFolderManager* GetFolderManager () const;
		MailModelsManager* GetMailModelsManager () const;
		QAbstractItemModel* GetFoldersModel () const;

		enum class Thread
		{
			LowPriority,
			HighPriority
		};
		AccountThread* GetAccountThread (Thread) const;

		void Synchronize ();
		void Synchronize (const QStringList&, const QByteArray&);

		void FetchWholeMessage (const Message_ptr&);
		QFuture<void> SendMessage (const Message_ptr&);
		void FetchAttachment (const Message_ptr&,
				const QString&, const QString&);

		void SetReadStatus (bool, const QList<QByteArray>&, const QStringList&);

		void CopyMessages (const QList<QByteArray>& ids, const QStringList& from, const QList<QStringList>& to);
		void MoveMessages (const QList<QByteArray>& ids, const QStringList& from, const QList<QStringList>& to);
		void DeleteMessages (const QList<QByteArray>& ids, const QStringList& folder);

		QByteArray Serialize () const;
		void Deserialize (const QByteArray&);

		void OpenConfigDialog (const std::function<void ()>& onAccepted = {});

		bool IsNull () const;

		QString GetInUsername ();
		QString GetOutUsername ();
	private:
		void SynchronizeImpl (const QList<QStringList>&, const QByteArray&);
		QMutex* GetMutex () const;

		void UpdateNoopInterval ();

		QString BuildInURL ();
		QString BuildOutURL ();
		QByteArray GetStoreID (Direction) const;

		void UpdateFolderCount (const QStringList&);

		void RequestMessageCount (const QStringList&);
		void HandleMessageCountFetched (int, int, const QStringList&);

		void HandleUpdatedMessages (const QList<Message_ptr>&, const QStringList&);
		void HandleMessagesRemoved (const QList<QByteArray>&, const QStringList&);
		void HandleGotFolders (const QList<Folder>&);
	private slots:
		void buildInURL (QString*);
		void buildOutURL (QString*);
		void getPassword (QString*, Direction = Direction::In);

		void HandleMsgHeaders (const QList<Message_ptr>&, const QStringList&);
		void HandleGotOtherMessages (const QList<QByteArray>&, const QStringList&);

		void handleFolderSyncFinished (const QStringList&, const QByteArray&);

		void handleFoldersUpdated ();

		void handleMessageBodyFetched (Message_ptr);
	signals:
		void mailChanged ();
		void gotProgressListener (ProgressListener_g_ptr);
		void accountChanged ();
		void messageBodyFetched (const Message_ptr&);
	};

	typedef std::shared_ptr<Account> Account_ptr;
}
}

Q_DECLARE_METATYPE (LeechCraft::Snails::Account_ptr);
