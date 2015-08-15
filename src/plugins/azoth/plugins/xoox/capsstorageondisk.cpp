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

#include "capsstorageondisk.h"
#include <QHash>
#include <QDir>
#include <QDataStream>
#include <QElapsedTimer>
#include <QSqlError>
#include <QXmppDiscoveryIq.h>
#include <util/sll/qtutil.h>
#include <util/sll/util.h>
#include <util/sys/paths.h>
#include <util/db/dblock.h>
#include <util/db/util.h>
#include "util.h"

Q_DECLARE_METATYPE (QXmppDiscoveryIq::Identity);

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	CapsStorageOnDisk::CapsStorageOnDisk (QObject *parent)
	: QObject { parent }
	{
		qRegisterMetaType<QXmppDiscoveryIq::Identity> ("QXmppDiscoveryIq::Identity");
		qRegisterMetaTypeStreamOperators<QXmppDiscoveryIq::Identity> ("QXmppDiscoveryIq::Identity");

		DB_.setDatabaseName (Util::CreateIfNotExists ("azoth/xoox").filePath ("caps2.db"));
		if (!DB_.open ())
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot open the database";
			Util::DBLock::DumpError (DB_.lastError ());
			throw std::runtime_error { "Cannot create database" };
		}

		Util::RunTextQuery (DB_, "PRAGMA synchronous = NORMAL;");
		Util::RunTextQuery (DB_, "PRAGMA journal_mode = WAL;");

		InitTables ();
		InitQueries ();
	}

	namespace
	{
		QByteArray SerializeFeatures (const QStringList& features)
		{
			QByteArray result;

			QDataStream str { &result, QIODevice::WriteOnly };
			str << features;

			return result;
		}

		QStringList DeserializeFeatures (const QByteArray& data)
		{
			QStringList result;

			QDataStream str { data };
			str >> result;

			return result;
		}
	}

	boost::optional<QStringList> CapsStorageOnDisk::GetFeatures (const QByteArray& ver) const
	{
		SelectFeatures_.bindValue (":ver", ver);
		Util::DBLock::Execute (SelectFeatures_);

		const auto finish = Util::MakeScopeGuard ([this] { SelectFeatures_.finish (); });

		if (!SelectFeatures_.next ())
			return {};
		else
			return DeserializeFeatures (SelectFeatures_.value (0).toByteArray ());
	}

	boost::optional<QList<QXmppDiscoveryIq::Identity>> CapsStorageOnDisk::GetIdentities (const QByteArray& ver) const
	{
		SelectIdentities_.bindValue (":ver", ver);
		Util::DBLock::Execute (SelectIdentities_);

		QList<QXmppDiscoveryIq::Identity> result;
		while (SelectIdentities_.next ())
		{
			QXmppDiscoveryIq::Identity id;
			id.setCategory (SelectIdentities_.value (0).toString ());
			id.setLanguage (SelectIdentities_.value (1).toString ());
			id.setName (SelectIdentities_.value (2).toString ());
			id.setType (SelectIdentities_.value (3).toString ());
			result << id;
		}

		SelectIdentities_.finish ();

		return result;
	}

	void CapsStorageOnDisk::AddFeatures (const QByteArray& ver, const QStringList& features)
	{
		Util::DBLock lock { DB_ };
		lock.Init ();

		InsertFeatures_.bindValue (":ver", ver);
		InsertFeatures_.bindValue (":features", SerializeFeatures (features));
		Util::DBLock::Execute (InsertFeatures_);

		lock.Good ();
	}

	void CapsStorageOnDisk::AddIdentities (const QByteArray& ver,
			const QList<QXmppDiscoveryIq::Identity>& identities)
	{
		Util::DBLock lock { DB_ };
		lock.Init ();

		for (const auto& id : identities)
		{
			InsertIdentity_.bindValue (":ver", ver);
			InsertIdentity_.bindValue (":category", id.category ());
			InsertIdentity_.bindValue (":language", id.language ());
			InsertIdentity_.bindValue (":name", id.name ());
			InsertIdentity_.bindValue (":type", id.type ());
			Util::DBLock::Execute (InsertIdentity_);
		}

		lock.Good ();
	}

	void CapsStorageOnDisk::InitTables ()
	{
		if (DB_.tables ().contains ("Features"))
			return;

		Util::DBLock lock { DB_ };
		lock.Init ();

		Util::RunQuery (DB_, "azoth/xoox", "create_features");
		Util::RunQuery (DB_, "azoth/xoox", "create_identities");

		lock.Good ();
	}

	void CapsStorageOnDisk::InitQueries ()
	{
		InsertFeatures_ = QSqlQuery { DB_ };
		InsertFeatures_.prepare (Util::LoadQuery ("azoth/xoox", "insert_feature"));

		InsertIdentity_ = QSqlQuery { DB_ };
		InsertIdentity_.prepare (Util::LoadQuery ("azoth/xoox", "insert_identity"));

		SelectFeatures_ = QSqlQuery { DB_ };
		SelectFeatures_.prepare (Util::LoadQuery ("azoth/xoox", "select_features"));

		SelectIdentities_ = QSqlQuery { DB_ };
		SelectIdentities_.prepare (Util::LoadQuery ("azoth/xoox", "select_identities"));
	}

	void CapsStorageOnDisk::Migrate ()
	{
		QFile file { Util::CreateIfNotExists ("azoth/xoox").filePath ("caps_s.db") };
		if (!file.exists ())
			return;

		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot open file for reading"
					<< file.fileName ()
					<< file.errorString ();
			return;
		}

		QHash<QByteArray, QStringList> features;
		QHash<QByteArray, QList<QXmppDiscoveryIq::Identity>> identities;

		QDataStream stream { &file };
		quint8 ver = 0;
		stream >> ver;
		if (ver < 1 || ver > 2)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown storage version"
					<< ver;
			return;
		}
		if (ver >= 1)
			stream >> features;
		if (ver >= 2)
			stream >> identities;

		QElapsedTimer timer;
		timer.start ();

		Util::DBLock lock { DB_ };
		lock.Init ();

		for (const auto& pair : Util::Stlize (features))
			AddFeatures (pair.first, pair.second);

		for (const auto& pair : Util::Stlize (identities))
			AddIdentities (pair.first, pair.second);

		lock.Good ();

		qDebug () << Q_FUNC_INFO
				<< "migration of"
				<< features.size ()
				<< "features and"
				<< identities.size ()
				<< "identities took"
				<< timer.elapsed ()
				<< "ms";

		file.remove ();
	}
}
}
}
