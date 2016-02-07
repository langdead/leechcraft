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

#include "templateseditorwidget.h"
#include <QMessageBox>
#include <util/sll/either.h>
#include <util/sll/visitor.h>
#include <interfaces/itexteditor.h>
#include "msgtemplatesmanager.h"
#include "structures.h"

namespace LeechCraft
{
namespace Snails
{
	TemplatesEditorWidget::TemplatesEditorWidget (MsgTemplatesManager *mgr, QWidget *parent)
	: QWidget { parent }
	, TemplatesMgr_ { mgr }
	{
		Ui_.setupUi (this);

		connect (Ui_.MessageType_,
				SIGNAL (currentIndexChanged (int)),
				this,
				SLOT (loadTemplate ()));

		connect (Ui_.ContentType_,
				SIGNAL (currentIndexChanged (int)),
				this,
				SLOT (prepareEditor (int)));

		Ui_.Editor_->SetupEditors ([this] (QAction *action)
				{
					EditorTypeActions_ << action;
					Ui_.ContentType_->addItem (action->icon (), action->text ());
				});

		prepareEditor (Ui_.ContentType_->currentIndex ());
	}

	void TemplatesEditorWidget::accept ()
	{
		const auto currentType = Ui_.Editor_->GetCurrentEditorType ();
		const auto msgType = static_cast<MsgType> (Ui_.MessageType_->currentIndex ());

		const auto& tpl = Ui_.Editor_->GetCurrentEditor ()->GetContents (currentType);

		Util::Visit (TemplatesMgr_->SaveTemplate (currentType, msgType, nullptr, tpl).AsVariant (),
				[=] (Util::Void) {},
				[=] (const auto& err)
				{
					QMessageBox::critical (this,
							"LeechCraft",
							tr ("Unable to save template: %1.")
								.arg (err.what ()));
				});
	}

	void TemplatesEditorWidget::reject ()
	{
		loadTemplate ();
	}

	void TemplatesEditorWidget::prepareEditor (int index)
	{
		EditorTypeActions_.value (index)->trigger ();

		loadTemplate ();
	}

	void TemplatesEditorWidget::loadTemplate ()
	{
		const auto currentType = Ui_.Editor_->GetCurrentEditorType ();
		const auto msgType = static_cast<MsgType> (Ui_.MessageType_->currentIndex ());

		Util::Visit (TemplatesMgr_->GetTemplate (currentType, msgType, nullptr).AsVariant (),
				[=] (const QString& tpl) { Ui_.Editor_->GetCurrentEditor ()->SetContents (tpl, currentType); },
				[=] (const auto& err)
				{
					QMessageBox::critical (this,
							"LeechCraft",
							tr ("Unable to load template: %1.")
								.arg (err.what ()));
				});
	}
}
}
