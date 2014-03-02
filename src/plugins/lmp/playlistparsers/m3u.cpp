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

#include "m3u.h"
#include <memory>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QtDebug>

namespace LeechCraft
{
namespace LMP
{
namespace M3U
{
	namespace
	{
		QPair<QString, QVariant> ParseMetadata (QString str)
		{
			const auto eqIdx = str.indexOf ('=');
			if (eqIdx == -1)
				return {};

			return { str.mid (1, eqIdx - 1), str.mid (eqIdx + 1) };
		}
	}

	Playlist Read2Sources (const QString& path)
	{
		QFile file (path);
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open"
					<< path
					<< file.errorString ();
			return {};
		}

		const auto& m3uDir = QFileInfo (path).absoluteDir ();

		QVariantMap lastMetadata;

		Playlist result;
		while (!file.atEnd ())
		{
			const auto& line = file.readLine ().trimmed ();
			if (line.startsWith ('#'))
			{
				const auto& pair = ParseMetadata (line);
				if (!pair.first.isEmpty ())
					lastMetadata [pair.first] = pair.second;
				continue;
			}

			const auto& url = QUrl::fromEncoded (line);
			auto src = QString::fromUtf8 (line);

			const auto mdGuard = std::shared_ptr<void> (nullptr,
					[&lastMetadata] (void*) { lastMetadata.clear (); });

#ifdef Q_OS_WIN32
			if (url.scheme ().size () > 1)
#else
			if (!url.scheme ().isEmpty ())
#endif
			{
				result.Append ({ url, lastMetadata });
				continue;
			}

			src.replace ('\\', '/');

			const QFileInfo fi (src);
			if (fi.isRelative ())
				src = m3uDir.absoluteFilePath (src);

			if (fi.suffix () == "m3u" || fi.suffix () == "m3u8")
				result += Read2Sources (src);
			else
				result.Append ({ src, lastMetadata });
		}
		return result;
	}

	void Write (const QString& path, const Playlist& sources)
	{
		QFile file (path);
		if (!file.open (QIODevice::WriteOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open"
					<< path
					<< file.errorString ();
			return;
		}

		for (const auto& item : sources)
		{
			for (auto i = item.Additional_.begin (); i != item.Additional_.end (); ++i)
				file.write (("#" + i.key () + "=" + i.value ().toString () + "\n").toUtf8 ());
			file.write (item.Source_.ToUrl ().toEncoded ());
			file.write ("\n");
		}
	}
}
}
}
