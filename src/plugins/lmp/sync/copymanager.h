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

#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <util/util.h>
#include "../core.h"

namespace LeechCraft
{
namespace LMP
{
	class ISyncPlugin;

	class CopyManagerBase : public QObject
	{
		Q_OBJECT
	protected:
		using QObject::QObject;
	protected slots:
		virtual void handleUploadFinished (const QString& localPath,
				QFile::FileError error, const QString& errorStr) = 0;
	signals:
		void startedCopying (const QString&);
		void copyProgress (qint64, qint64);
		void finishedCopying ();
		void errorCopying (const QString&, const QString&);
	};

	template<typename CopyJobT>
	class CopyManager : public CopyManagerBase
	{
		QList<CopyJobT> Queue_;
		CopyJobT CurrentJob_;
	public:
		CopyManager (QObject *parent = 0)
		: CopyManagerBase (parent)
		{
		}

		void Copy (const CopyJobT& job)
		{
			if (IsRunning ())
				Queue_ << job;
			else
				StartJob (job);
		}
	private:
		void StartJob (const CopyJobT& job)
		{
			CurrentJob_ = job;

			connect (job.GetQObject (),
					SIGNAL (uploadFinished (QString, QFile::FileError, QString)),
					this,
					SLOT (handleUploadFinished (QString, QFile::FileError, QString)),
					Qt::UniqueConnection);

			const auto& norm = QMetaObject::normalizedSignature ("uploadProgress (qint64, qint64)");
			if (job.GetQObject ()->metaObject ()->indexOfSignal (norm) >= 0)
				connect (job.GetQObject (),
						SIGNAL (uploadProgress (qint64, qint64)),
						this,
						SIGNAL (copyProgress (qint64, qint64)));

			job.Upload ();

			emit startedCopying (job.Filename_);
		}

		bool IsRunning () const
		{
			return !CurrentJob_.Filename_.isEmpty ();
		}
	protected:
		void handleUploadFinished (const QString& localPath, QFile::FileError error, const QString& errorStr) override
		{
			const bool remove = CurrentJob_.RemoveOnFinish_;
			CurrentJob_ = CopyJobT ();

			if (!Queue_.isEmpty ())
				StartJob (Queue_.takeFirst ());

			if (remove)
				QFile::remove (localPath);

			if (!errorStr.isEmpty () && error != QFile::NoError)
				emit errorCopying (localPath, errorStr);
			else
				emit finishedCopying ();
		}
	};
}
}
