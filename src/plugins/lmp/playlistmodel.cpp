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

#include "playlistmodel.h"
#include <QMimeData>
#include <QFileInfo>
#include <util/sll/prelude.h>
#include "playlistparsers/playlistfactory.h"
#include "player.h"
#include "util.h"
#include "core.h"
#include "radiomanager.h"

namespace LeechCraft
{
namespace LMP
{
	PlaylistModel::PlaylistModel (Player *parent)
	: DndActionsMixin<QStandardItemModel> (parent)
	, Player_ (parent)
	{
		setSupportedDragActions (Qt::CopyAction | Qt::MoveAction);
	}

	QStringList PlaylistModel::mimeTypes () const
	{
		return { "text/uri-list" };
	}

	QMimeData* PlaylistModel::mimeData (const QModelIndexList& indexes) const
	{
		QList<QUrl> urls;
		for (const auto& index : indexes)
			urls += Util::Map (Player_->GetIndexSources (index), &AudioSource::ToUrl);
		urls.removeAll ({});

		const auto result = new QMimeData;
		result->setUrls (urls);
		return result;
	}

	namespace
	{
		QList<AudioSource> GetSources (const QMimeData *data)
		{
			QList<AudioSource> sources;
			for (const auto& url : data->urls ())
			{
				if (url.scheme () != "file")
				{
					sources << AudioSource (url);
					continue;
				}

				const auto& localPath = url.toLocalFile ();
				if (QFileInfo (localPath).isFile ())
				{
					sources << AudioSource (localPath);
					continue;
				}

				for (const auto& path : RecIterate (localPath, true))
					sources << AudioSource (path);
			}

			return sources;
		}

		QList<MediaInfo> GetInfos (const QMimeData *data)
		{
			const auto& serialized = data->data ("x-leechcraft-lmp/media-info-list");
			if (serialized.isEmpty ())
				return {};

			QDataStream stream { serialized };
			QList<MediaInfo> result;
			stream >> result;
			return result;
		}
	}

	bool PlaylistModel::dropMimeData (const QMimeData *data,
			Qt::DropAction action, int row, int, const QModelIndex& parent)
	{
		if (action == Qt::IgnoreAction)
			return true;

		if (data->hasUrls ())
			HandleDroppedUrls (data, row, parent);

		HandleRadios (data);

		return true;
	}

	Qt::DropActions PlaylistModel::supportedDropActions () const
	{
		return Qt::CopyAction | Qt::MoveAction;
	}

	void PlaylistModel::HandleRadios (const QMimeData *data)
	{
		QStringList radioIds;

		QDataStream stream { data->data ("x-leechcraft-lmp/radio-ids") };
		stream >> radioIds;

		for (const auto& radioId : radioIds)
			if (const auto station = Core::Instance ().GetRadioManager ()->GetRadioStation (radioId))
			{
				Player_->SetRadioStation (station);
				break;
			}
	}

	void PlaylistModel::HandleDroppedUrls (const QMimeData *data, int row, const QModelIndex& parent)
	{
		const auto& sources = GetSources (data);
		const auto& infos = GetInfos (data);

		if (infos.size () == sources.size ())
			for (int i = 0; i < sources.size (); ++i)
				Player_->PrepareURLInfo (sources.at (i).ToUrl (), infos.at (i));

		auto afterIdx = row >= 0 ?
				parent.child (row, 0) :
				parent;
		const auto& firstSrc = afterIdx.isValid () ?
				Player_->GetIndexSources (afterIdx).value (0) :
				AudioSource ();

		auto existingQueue = Player_->GetQueue ();
		for (const auto& src : sources)
		{
			auto remPos = std::remove (existingQueue.begin (), existingQueue.end (), src);
			existingQueue.erase (remPos, existingQueue.end ());
		}

		auto pos = std::find (existingQueue.begin (), existingQueue.end (), firstSrc);
		if (pos == existingQueue.end ())
			existingQueue << sources;
		else
		{
			for (const auto& src : sources)
				pos = existingQueue.insert (pos, src) + 1;
		}

		Player_->Enqueue (existingQueue, Player::EnqueueReplace | Player::EnqueueSort);
	}
}
}
