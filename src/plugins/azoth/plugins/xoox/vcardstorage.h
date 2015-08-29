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
#include <boost/optional.hpp>
#include <QObject>
#include <QCache>
#include <QXmppVCardIq.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	class VCardStorageOnDisk;
	class VCardStorageOnDiskWriter;

	class VCardStorage : public QObject
	{
		VCardStorageOnDisk * const DB_;
		const std::shared_ptr<VCardStorageOnDiskWriter> Writer_;

		QMap<QString, QString> Pending_;

		mutable QCache<QString, QXmppVCardIq> VCardCache_;
	public:
		VCardStorage (QObject* = nullptr);

		void SetVCard (const QString& jid, const QString& vcard);
		void SetVCard (const QString& jid, const QXmppVCardIq& vcard);

		boost::optional<QXmppVCardIq> GetVCard (const QString& jid) const;
	private:
		boost::optional<QString> GetVCardString (const QString& jid) const;
	};
}
}
}
