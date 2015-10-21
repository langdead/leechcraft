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

#include "radiotracksgrabdialog.h"
#include <QStandardItemModel>
#include <util/sll/prelude.h>
#include <util/sll/slotclosure.h>
#include <util/lmp/util.h>
#include "mediainfo.h"

namespace LeechCraft
{
namespace LMP
{
	RadioTracksGrabDialog::RadioTracksGrabDialog (const QList<Media::AudioInfo>& infos, QWidget *parent)
	: RadioTracksGrabDialog { Util::Map (infos, &MediaInfo::FromAudioInfo), parent }
	{
	}

	RadioTracksGrabDialog::RadioTracksGrabDialog (const QList<MediaInfo>& infos, QWidget *parent)
	: QDialog { parent }
	, NamesPreviewModel_ { new QStandardItemModel { this } }
	{
		NamesPreviewModel_->setHorizontalHeaderLabels ({ tr ("Artist"), tr ("Title"), tr ("File name") });
		for (const auto& info : infos)
		{
			const QList<QStandardItem*> items
			{
				new QStandardItem { info.Artist_ },
				new QStandardItem { info.Title_ },
				new QStandardItem {}
			};

			for (auto item : items)
				item->setEditable (false);

			NamesPreviewModel_->appendRow (items);
		}

		Ui_.setupUi (this);
		Ui_.NamesPreview_->setModel (NamesPreviewModel_);

		new Util::SlotClosure<Util::NoDeletePolicy>
		{
			[this, infos]
			{
				Names_ = PerformSubstitutions (Ui_.NameMask_->text (), infos,
						[this] (int row, const QString& name)
							{ NamesPreviewModel_->item (row, 2)->setText (name); });
			},
			Ui_.NameMask_,
			SIGNAL (textChanged ()),
			this
		};
	}
}
}
