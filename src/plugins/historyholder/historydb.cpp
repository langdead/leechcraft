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

#include "historydb.h"
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QDataStream>
#include <QDir>
#include <QSettings>
#include <QTextCodec>
#include <QCoreApplication>
#include <QUrl>
#include <QSqlQueryModel>
#include <QElapsedTimer>
#include <util/structuresops.h>
#include <util/sys/paths.h>
#include <util/db/dblock.h>
#include <interfaces/core/itagsmanager.h>
#include "historyentry.h"

namespace LeechCraft
{
namespace HistoryHolder
{
	namespace
	{
		void RunTextQuery (QSqlDatabase& db, const QString& text)
		{
			QSqlQuery query { db };
			query.prepare (text);
			Util::DBLock::Execute (query);
		}
	}

	HistoryDB::HistoryDB (ITagsManager *tm, const ILoadProgressReporter_ptr& reporter, QObject *parent)
	: QObject { parent }
	, TM_ { tm }
	{
		DB_.setDatabaseName (Util::CreateIfNotExists ("historyholder").filePath ("history.db"));
		if (!DB_.open ())
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot open the database";
			Util::DBLock::DumpError (DB_.lastError ());
			throw std::runtime_error { "Cannot create database" };
		}

		RunTextQuery (DB_, "PRAGMA foreign_keys = ON;");
		RunTextQuery (DB_, "PRAGMA synchronous = NORMAL;");
		RunTextQuery (DB_, "PRAGMA journal_mode = WAL;");

		InitTables ();
		InitQueries ();

		LoadTags ();

		Migrate (reporter);
	}

	std::shared_ptr<QAbstractItemModel> HistoryDB::CreateModel () const
	{
		auto model = std::make_shared<QSqlQueryModel> ();

		model->setQuery (SelectHistory_);

		/* The following roles should also be handled by the model:
		 *
		 * RoleTags
		 * RoleControls
		 * RoleHash
		 * RoleMime
		 */

		return model;
	}

	void HistoryDB::Add (const Entity& entity)
	{
		if (entity.Parameters_ & LeechCraft::DoNotSaveInHistory ||
				entity.Parameters_ & LeechCraft::Internal ||
				!(entity.Parameters_ & LeechCraft::IsDownloaded))
			return;

		Add (entity, QDateTime::currentDateTime ());
	}

	namespace
	{
		QString LoadQuery (const QString& filename)
		{
			QFile file { ":/historyholder/resources/sql/" + filename + ".sql" };
			if (!file.open (QIODevice::ReadOnly))
			{
				qWarning () << Q_FUNC_INFO
						<< file.fileName ()
						<< file.errorString ();
				throw std::runtime_error { "Cannot open query file" };
			}

			return QString::fromUtf8 (file.readAll ());
		}

		void RunQuery (QSqlDatabase& db, const QString& filename)
		{
			QSqlQuery query { db };
			query.prepare (LoadQuery (filename));
			Util::DBLock::Execute (query);
		}
	}

	void HistoryDB::InitTables ()
	{
		if (DB_.tables ().contains ("History"))
			return;

		try
		{
			for (const auto table : QStringList { "history", "entities", "tags", "tags_mapping" })
				RunQuery (DB_, "create_" + table);
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to initialize queries:"
					<< e.what ();
			throw;
		}
	}

	void HistoryDB::InitQueries ()
	{
		InsertHistory_ = QSqlQuery { DB_ };
		InsertHistory_.prepare (LoadQuery ("insert_history"));

		InsertTags_ = QSqlQuery { DB_ };
		InsertTags_.prepare (LoadQuery ("insert_tags"));

		InsertTagsMapping_ = QSqlQuery { DB_ };
		InsertTagsMapping_.prepare (LoadQuery ("insert_tags_mapping"));

		InsertEntity_ = QSqlQuery { DB_ };
		InsertEntity_.prepare (LoadQuery ("insert_entity"));

		SelectHistory_ = QSqlQuery { DB_ };
		SelectHistory_.prepare (LoadQuery ("select_history"));
	}

	void HistoryDB::LoadTags ()
	{
		QSqlQuery query { DB_ };
		query.prepare (LoadQuery ("select_tags"));
		Util::DBLock::Execute (query);

		while (query.next ())
		{
			const auto id = query.value (0).toInt ();
			const auto& lcId = query.value (1).toString ();
			Tags_ [lcId] = id;
		}
	}

	namespace
	{
		QString GetTitle (const Entity& e)
		{
			QString stren;
			if (e.Additional_.contains ("UserVisibleName") &&
					e.Additional_ ["UserVisibleName"].canConvert<QString> ())
				stren = e.Additional_ ["UserVisibleName"].toString ();
			else if (e.Entity_.canConvert<QUrl> ())
				stren = e.Entity_.toUrl ().toString ();
			else if (e.Entity_.canConvert<QByteArray> ())
			{
				const auto& entity = e.Entity_.toByteArray ();
				if (entity.size () < 250)
					stren = QTextCodec::codecForName ("UTF-8")->toUnicode (entity);
			}
			else
				stren = HistoryDB::tr ("Binary data");

			if (!e.Location_.isEmpty ())
			{
				stren += " (";
				stren += e.Location_;
				stren += ")";
			}

			return stren;
		}

		template<typename T = int>
		T GetLastId (const QSqlQuery& query)
		{
			const auto& lastVar = query.lastInsertId ();
			if (lastVar.isNull ())
				throw std::runtime_error { "No last ID has been reported." };

			if (!lastVar.canConvert<T> ())
			{
				qWarning () << Q_FUNC_INFO
						<< "cannot convert"
						<< lastVar;
				throw std::runtime_error { "Cannot convert last ID." };
			}

			return lastVar.value<T> ();
		}

		QByteArray SerializeEntity (const Entity& e)
		{
			QByteArray result;

			QDataStream ostr { &result, QIODevice::ReadWrite };
			ostr << e;

			return result;
		}
	}

	void HistoryDB::Add (const Entity& entity, const QDateTime& ts)
	{
		Util::DBLock lock { DB_ };
		lock.Init ();

		InsertHistory_.bindValue (":title", GetTitle (entity));
		InsertHistory_.bindValue (":ts", ts);
		Util::DBLock::Execute (InsertHistory_);

		const auto& historyId = GetLastId (InsertHistory_);

		auto tags = entity.Additional_ [" Tags"].toStringList ();
		if (tags.isEmpty ())
			tags.push_back ({});
		AssociateTags (historyId, AddTags (tags));

		InsertEntity_.bindValue (":entryId", historyId);
		InsertEntity_.bindValue (":entity", SerializeEntity (entity));

		lock.Good ();
	}

	QList<int> HistoryDB::AddTags (const QStringList& tags)
	{
		QList<int> result;

		for (const auto& tag : tags)
		{
			if (Tags_.contains (tag))
			{
				result << Tags_.value (tag);
				continue;
			}

			InsertTags_.bindValue (":lcid", tag);
			InsertTags_.bindValue (":text", TM_->GetTag (tag));
			Util::DBLock::Execute (InsertTags_);

			const auto id = GetLastId (InsertTags_);
			Tags_ [tag] = id;
			result << id;
		}

		return result;
	}

	void HistoryDB::AssociateTags (int historyId, const QList<int>& tags)
	{
		for (const auto tag : tags)
		{
			InsertTagsMapping_.bindValue (":tagId", tag);
			InsertTagsMapping_.bindValue (":entryId", historyId);
			Util::DBLock::Execute (InsertTagsMapping_);
		}
	}

	void HistoryDB::Migrate (const ILoadProgressReporter_ptr& reporter)
	{
		QSettings settings
		{
			QCoreApplication::organizationName (),
			QCoreApplication::applicationName () + "_HistoryHolder"
		};
		int size = settings.beginReadArray ("History");
		if (!size)
			return;

		const auto& process = reporter->InitiateProcess (tr ("Migrating downloads history..."), 0, size);

		QElapsedTimer timer;
		timer.start ();

		{
			Util::DBLock lock { DB_ };
			lock.Init ();

			for (int i = 0; i < size; ++i)
			{
				settings.setArrayIndex (i);

				const auto& var = settings.value ("Item");
				if (var.isValid ())
				{
					const auto& entity = var.value<HistoryEntry> ();
					Add (entity.Entity_, entity.DateTime_);
				}

				process->ReportValue (i);
			}

			lock.Good ();
		}
		settings.endArray ();

		qDebug () << Q_FUNC_INFO
				<< "done in"
				<< timer.elapsed ()
				<< "ms for"
				<< size
				<< "entries";
	}
}
}
