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

#include "checktab.h"
#include <QStandardItemModel>
#include <QDeclarativeContext>
#include <QSortFilterProxyModel>
#include <util/sys/paths.h>
#include <util/qml/colorthemeproxy.h>
#include <interfaces/lmp/ilmpproxy.h>
#include <interfaces/lmp/ilocalcollection.h>
#include "checkmodel.h"

namespace LeechCraft
{
namespace LMP
{
namespace BrainSlugz
{
	namespace
	{
		class CheckFilterModel : public QSortFilterProxyModel
		{
			const bool ShouldBeChecked_;
		public:
			CheckFilterModel (QAbstractItemModel *source, bool shouldBeChecked, QObject *parent)
			: QSortFilterProxyModel { parent }
			, ShouldBeChecked_ { shouldBeChecked }
			{
				setSourceModel (source);
				setDynamicSortFilter (true);
			}
		protected:
			bool filterAcceptsRow (int row, const QModelIndex&) const
			{
				const auto idx = sourceModel ()->index (row, 0);
				return idx.data (CheckModel::IsChecked).toBool () == ShouldBeChecked_;
			}
		};
	}

	CheckTab::CheckTab (const ILMPProxy_ptr& lmpProxy,
			const ICoreProxy_ptr& coreProxy,
			const TabClassInfo& tc,
			QObject* plugin)
	: LmpProxy_ { lmpProxy }
	, TC_ (tc)
	, Plugin_ { plugin }
	, Model_ { new CheckModel { lmpProxy->GetLocalCollection ()->GetAllArtists (), this } }
	, CheckedModel_ { new CheckFilterModel { Model_, true, this } }
	, UncheckedModel_ { new CheckFilterModel { Model_, false, this } }
	{
		Ui_.setupUi (this);

		const auto root = Ui_.CheckView_->rootContext ();
		root->setContextProperty ("colorProxy",
				new Util::ColorThemeProxy { coreProxy->GetColorThemeManager (), this });
		root->setContextProperty ("artistsModel", Model_);
		root->setContextProperty ("checkedModel", CheckedModel_);
		root->setContextProperty ("uncheckedModel", UncheckedModel_);

		const auto& filename = Util::GetSysPath (Util::SysPath::QML, "lmp/brainslugz", "CheckView.qml");
		Ui_.CheckView_->setSource (QUrl::fromLocalFile (filename));
	}

	TabClassInfo CheckTab::GetTabClassInfo () const
	{
		return TC_;
	}

	QObject* CheckTab::ParentMultiTabs ()
	{
		return Plugin_;
	}

	void CheckTab::Remove ()
	{
		emit removeTab (this);
	}

	QToolBar* CheckTab::GetToolBar () const
	{
		return nullptr;
	}
}
}
}
