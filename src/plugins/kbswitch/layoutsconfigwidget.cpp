/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "layoutsconfigwidget.h"
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QtDebug>
#include <QSettings>
#include <util/models/modeliterator.h>
#include "kbctl.h"
#include "flagiconprovider.h"
#include "rulesstorage.h"

namespace LeechCraft
{
namespace KBSwitch
{
	namespace
	{
		QList<QStringList> ToSortedList (const QHash<QString, QString>& hash)
		{
			QList<QStringList> result;
			for (auto i = hash.begin (); i != hash.end (); ++i)
				result.append ({ i.key (), i.value () });

			std::sort (result.begin (), result.end (),
					[] (decltype (result.at (0)) l, decltype (result.at (0)) r)
						{ return l.at (0) < r.at (0); });

			return result;
		}

		void SetList (const QList<QStringList>& lists, QStandardItemModel *model)
		{
			FlagIconProvider flagProv;

			for (const auto& list : lists)
			{
				QList<QStandardItem*> row;
				for (const auto& item : list)
					row << new QStandardItem (item);

				const auto& img = flagProv.requestPixmap (list.at (0), nullptr, {});
				row.first ()->setIcon ({ img });

				row.value (0)->setEditable (false);
				row.value (1)->setEditable (false);
				model->appendRow (row);
			}
		}

		class EnabledItemDelegate : public QStyledItemDelegate
		{
		public:
			EnabledItemDelegate (QObject *parent = 0)
			: QStyledItemDelegate (parent)
			{
			}

			QWidget* createEditor (QWidget *parent,
					const QStyleOptionViewItem& option, const QModelIndex& index) const
			{
				if (index.column () != LayoutsConfigWidget::EnabledColumn::EnabledVariant)
					return QStyledItemDelegate::createEditor (parent, option, index);

				return new QComboBox (parent);
			}

			void setEditorData (QWidget *editor, const QModelIndex& index) const
			{
				if (index.column () != LayoutsConfigWidget::EnabledColumn::EnabledVariant)
					return QStyledItemDelegate::setEditorData (editor, index);

				const auto& codeIdx = index.sibling (index.row (),
						LayoutsConfigWidget::EnabledColumn::EnabledCode);
				const auto& code = codeIdx.data ().toString ();

				const auto& variants = KBCtl::Instance ()
						.GetRulesStorage ()->GetLayoutVariants (code);

				auto box = qobject_cast<QComboBox*> (editor);
				box->clear ();
				box->addItem ({});
				box->addItems (variants);
			}

			void setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const
			{
				if (index.column () != LayoutsConfigWidget::EnabledColumn::EnabledVariant)
					return QStyledItemDelegate::setModelData (editor, model, index);

				const auto& item = qobject_cast<QComboBox*> (editor)->currentText ();
				model->setData (index, item, Qt::DisplayRole);
			}
		};
	}

	LayoutsConfigWidget::LayoutsConfigWidget (QWidget *parent)
	: QWidget (parent)
	, AvailableModel_ (new QStandardItemModel (this))
	, EnabledModel_ (new QStandardItemModel (this))
	{
		QStringList availHeaders { tr ("Code"), tr ("Description") };
		AvailableModel_->setHorizontalHeaderLabels (availHeaders);
		EnabledModel_->setHorizontalHeaderLabels (availHeaders << tr ("Variant"));

		FillModels ();

		Ui_.setupUi (this);
		Ui_.AvailableView_->setModel (AvailableModel_);
		Ui_.EnabledView_->setModel (EnabledModel_);

		Ui_.EnabledView_->setItemDelegate (new EnabledItemDelegate (Ui_.EnabledView_));

		connect (Ui_.AvailableView_->selectionModel (),
				SIGNAL (currentRowChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (updateActionsState ()));
		connect (Ui_.EnabledView_->selectionModel (),
				SIGNAL (currentRowChanged (QModelIndex, QModelIndex)),
				this,
				SLOT (updateActionsState ()));
		updateActionsState ();
	}

	void LayoutsConfigWidget::FillModels ()
	{
		if (auto rc = AvailableModel_->rowCount ())
			AvailableModel_->removeRows (0, rc);
		if (auto rc = EnabledModel_->rowCount ())
			EnabledModel_->removeRows (0, rc);

		auto layouts = KBCtl::Instance ().GetRulesStorage ()->GetLayoutsN2D ();

		const auto& enabledGroups = KBCtl::Instance ().GetEnabledGroups ();

		QList<QStringList> enabled;
		for (const auto& name : enabledGroups)
		{
			const QStringList enabledRow
			{
				name,
				layouts.take (name),
				KBCtl::Instance ().GetGroupVariant (name)
			};
			enabled << enabledRow;
		}

		SetList (ToSortedList (layouts), AvailableModel_);
		SetList (enabled, EnabledModel_);
	}

	void LayoutsConfigWidget::accept ()
	{
		QStringList codes;
		QHash<QString, QString> variants;
		for (int i = 0; i < EnabledModel_->rowCount (); ++i)
		{
			const auto& code = EnabledModel_->item (i, EnabledColumn::EnabledCode)->text ();
			codes << code;

			const auto& variant = EnabledModel_->item (i, EnabledColumn::EnabledVariant)->text ();
			if (!variant.isEmpty ())
				variants [code] = variant;
		}
		KBCtl::Instance ().SetEnabledGroups (codes);
		KBCtl::Instance ().SetGroupVariants (variants);
	}

	void LayoutsConfigWidget::reject ()
	{
		FillModels ();
	}

	void LayoutsConfigWidget::on_Enable__released ()
	{
		const auto& toEnableIdx = Ui_.AvailableView_->currentIndex ();
		if (!toEnableIdx.isValid ())
			return;

		auto row = AvailableModel_->takeRow (toEnableIdx.row ());
		row << new QStandardItem;
		EnabledModel_->appendRow (row);
	}

	void LayoutsConfigWidget::on_Disable__released ()
	{
		const auto& toDisableIdx = Ui_.EnabledView_->currentIndex ();
		if (!toDisableIdx.isValid ())
			return;

		auto row = EnabledModel_->takeRow (toDisableIdx.row ());
		delete row.takeLast ();

		auto pos = std::upper_bound (Util::ModelIterator (AvailableModel_, 0),
				Util::ModelIterator (AvailableModel_, AvailableModel_->rowCount ()),
				row.first ()->text (),
				[] (const QString& code, const QModelIndex& mi)
					{ return code < mi.data ().toString (); });
		AvailableModel_->insertRow (pos.GetRow (), row);
	}

	void LayoutsConfigWidget::updateActionsState ()
	{
		const auto& availIdx = Ui_.AvailableView_->currentIndex ();
		const auto maxEnabled = KBCtl::Instance ().GetMaxEnabledGroups ();
		Ui_.Enable_->setEnabled (availIdx.isValid () &&
				EnabledModel_->rowCount () < maxEnabled);

		Ui_.Disable_->setEnabled (Ui_.EnabledView_->currentIndex ().isValid ());
	}
}
}
