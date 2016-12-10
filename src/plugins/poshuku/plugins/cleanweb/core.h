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

#include <QAbstractItemModel>
#include <QHash>
#include <QStringList>
#include <QNetworkReply>
#include <QDateTime>
#include <QWebPage>
#include <interfaces/iinfo.h>
#include <interfaces/idownload.h>
#include <interfaces/poshuku/poshukutypes.h>
#include <interfaces/core/ihookproxy.h>
#include "filter.h"

class QNetworkRequest;
class QWebPage;

namespace LeechCraft
{
namespace Poshuku
{
class IWebView;
class IBrowserWidget;

namespace CleanWeb
{
	class UserFiltersModel;
	class SubscriptionsModel;

	struct HidingWorkerResult
	{
		IWebView *View_;
		QStringList Selectors_;
	};

	class Core : public QObject
	{
		Q_OBJECT

		UserFiltersModel * const UserFilters_;
		SubscriptionsModel * const SubsModel_;

		QList<QList<FilterItem_ptr>> ExceptionsCache_;
		QList<QList<FilterItem_ptr>> FilterItemsCache_;

		QObjectList Downloaders_;

		struct PendingJob
		{
			QString FullName_;
			QString FileName_;
			QString Subscr_;
			QUrl URL_;
		};
		QMap<int, PendingJob> PendingJobs_;

		QHash<QObject*, QSet<QUrl>> MoreDelayedURLs_;

		QSet<IWebView*> ScheduledHidings_;

		const ICoreProxy_ptr Proxy_;
	public:
		Core (SubscriptionsModel*, UserFiltersModel*, const ICoreProxy_ptr&);

		ICoreProxy_ptr GetProxy () const;

		bool CouldHandle (const Entity&) const;
		void Handle (Entity);

		void HandleBrowserWidget (IBrowserWidget*);

		void InstallInterceptor ();

		void HandleContextMenu (const ContextMenuInfo&,
				IWebView*, QMenu*,
				WebViewCtxMenuStage);

		/** Parses the abp:-schemed url, gets subscription
		 * name and real url from there and adds it via Load().
		 *
		 * Returns true if the url is added successfully or
		 * false otherwise (if url is malformed or such
		 * subscription already exists, for example).
		 *
		 * @param[in] url The abp:-schemed URL.
		 *
		 * @return Whether addition was successful.
		 */
		bool Add (const QUrl& url);

		/** Loads the subscription from the url with the name
		 * subscrName. Returns true if the load delegation was
		 * successful, otherwise returns false.
		 *
		 * url is expected to be a "real" URL of the filters
		 * file — with, say, http:// scheme.
		 *
		 * Returns true if the url is added successfully or
		 * false otherwise (if url is malformed or such
		 * subscription already exists, for example).
		 *
		 * @param[in] url Real URL of the file with the filters.
		 * @param[in] subscrName The name if this subscription.
		 *
		 * @return Whether addition was successful.
		 */
		bool Load (const QUrl& url, const QString& subscrName);
	private:
		void HandleProvider (QObject*);

		void Parse (const QString&);

		void HideElementsChunk (HidingWorkerResult);
		void DelayedRemoveElements (IWebView*, const QUrl&);
		void HandleViewLayout (IWebView*);
	private slots:
		void update ();
		void handleJobFinished (int);
		void handleJobError (int, IDownload::Error);

		void moreDelayedRemoveElements ();

		void handleViewDestroyed (QObject*);

		void regenFilterCaches ();
	};
}
}
}
