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

#include "core.h"
#include <algorithm>
#include <functional>
#include <QNetworkRequest>
#include <QRegExp>
#include <QFile>
#include <QSettings>
#include <QFileInfo>
#include <QTimer>
#include <QTextCodec>
#include <QMessageBox>
#include <QDir>
#include <qwebframe.h>
#include <qwebpage.h>
#include <qwebelement.h>
#include <QCoreApplication>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <QFutureWatcher>
#include <QMenu>
#include <QMainWindow>
#include <QDir>
#include <qwebview.h>

#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#endif

#include <util/xpc/util.h>
#include <util/network/customnetworkreply.h>
#include <util/sys/paths.h>
#include <util/sll/slotclosure.h>
#include <util/sll/prelude.h>
#include <util/sll/delayedexecutor.h>
#include <interfaces/core/ientitymanager.h>
#include "xmlsettingsmanager.h"
#include "flashonclickplugin.h"
#include "flashonclickwhitelist.h"
#include "userfiltersmodel.h"
#include "lineparser.h"
#include "subscriptionsmodel.h"

Q_DECLARE_METATYPE (QNetworkReply*);
Q_DECLARE_METATYPE (QWebFrame*);

namespace LeechCraft
{
namespace Poshuku
{
namespace CleanWeb
{
	namespace
	{
		enum FilterType
		{
			FTName_,
			FTFilename_,
			FTUrl_
		};

		template<typename T>
		struct FilterFinderBase
		{
			const T& ID_;

			FilterFinderBase (const T& id)
			: ID_ (id)
			{
			}
		};

		template<FilterType>
			struct FilterFinder;

		template<>
			struct FilterFinder<FTName_> : FilterFinderBase<QString>
			{
				FilterFinder (const QString& id)
				: FilterFinderBase<QString> (id)
				{
				}

				bool operator() (const Filter& f) const
				{
					return f.SD_.Name_ == ID_;
				}
			};

		template<>
			struct FilterFinder<FTFilename_> : FilterFinderBase<QString>
			{
				FilterFinder (const QString& id)
				: FilterFinderBase<QString> (id)
				{
				}

				bool operator() (const Filter& f) const
				{
					return f.SD_.Filename_ == ID_;
				}
			};

		template<>
			struct FilterFinder<FTUrl_> : FilterFinderBase<QUrl>
			{
				FilterFinder (const QUrl& id)
				: FilterFinderBase<QUrl> (id)
				{
				}

				bool operator() (const Filter& f) const
				{
					return f.SD_.URL_ == ID_;
				}
			};

		QList<Filter> ParseToFilters (const QStringList& paths)
		{
			QList<Filter> result;
			for (const auto& filePath : paths)
			{
				QFile file (filePath);
				if (!file.open (QIODevice::ReadOnly))
				{
					qWarning () << Q_FUNC_INFO
						<< "could not open file"
						<< filePath
						<< file.errorString ();
					result << Filter ();
					continue;
				}

				const auto& data = QString::fromUtf8 (file.readAll ());
				QStringList rawLines = data.split ('\n', QString::SkipEmptyParts);
				if (rawLines.size ())
					rawLines.removeAt (0);
				const auto& lines = Util::Map (rawLines, std::mem_fn (&QString::trimmed));

				Filter f;
				std::for_each (lines.begin (), lines.end (), LineParser (&f));

				f.SD_.Filename_ = QFileInfo (filePath).fileName ();

				result << f;
			}
			return result;
		}
	}

	Core::Core (SubscriptionsModel *model, UserFiltersModel *ufm, const ICoreProxy_ptr& proxy)
	: FlashOnClickWhitelist_ { new FlashOnClickWhitelist }
	, UserFilters_ { ufm }
	, SubsModel_ { model }
	, Proxy_ { proxy }
	{
		connect (SubsModel_,
				SIGNAL (filtersListChanged ()),
				this,
				SLOT (regenFilterCaches ()));
		connect (UserFilters_,
				SIGNAL (filtersChanged ()),
				this,
				SLOT (regenFilterCaches ()));

		qRegisterMetaType<QWebFrame*> ("QWebFrame*");

		const auto& path = Util::CreateIfNotExists ("cleanweb");
		const auto& infos = path.entryInfoList (QDir::Files | QDir::Readable);
		const auto& paths = Util::Map (infos, std::mem_fn (&QFileInfo::absoluteFilePath));

		auto watcher = new QFutureWatcher<QList<Filter>> ();
		connect (watcher,
				SIGNAL (finished ()),
				this,
				SLOT (handleParsed ()));
		const auto& future = QtConcurrent::run (ParseToFilters, paths);
		watcher->setFuture (future);
	}

	ICoreProxy_ptr Core::GetProxy () const
	{
		return Proxy_;
	}

	bool Core::CouldHandle (const Entity& e) const
	{
		const auto& url = e.Entity_.toUrl ();
		return url.scheme () == "abp" && url.path () == "subscribe";
	}

	void Core::Handle (Entity subscr)
	{
		QUrl subscrUrl = subscr.Entity_.toUrl ();

		Add (subscrUrl);
	}

	namespace
	{
		void RemoveElem (QWebElement elem)
		{
			elem.setStyleProperty ("visibility", "hidden !important");
		}
	}

	void Core::HandleInitialLayout (QWebPage*, QWebFrame *frame)
	{
		QPointer<QWebFrame> safeFrame { frame };
		new Util::DelayedExecutor
		{
			[this, safeFrame] { HandleFrameLayout (safeFrame, false); },
			0
		};
	}

	QNetworkReply* Core::Hook (IHookProxy_ptr hook,
			QNetworkAccessManager*,
			QNetworkAccessManager::Operation*,
			QIODevice**)
	{
		const auto& req = hook->GetValue ("request").value<QNetworkRequest> ();
		if (!req.originatingObject ())
			return nullptr;

		const auto& reqUrl = req.url ();

		if (reqUrl.scheme () == "data")
			return nullptr;

		const auto frame = qobject_cast<QWebFrame*> (req.originatingObject ());
		const QWebPage * const page = frame ? frame->page () : nullptr;
		if (frame &&
				page &&
				frame == page->mainFrame () &&
				frame->requestedUrl () == reqUrl)
			return nullptr;

		if (!ShouldReject (req))
			return nullptr;

		hook->CancelDefault ();

		qDebug () << "rejecting" << frame << reqUrl;
		if (frame)
		{
			QPointer<QWebFrame> safeFrame { frame };
			new Util::DelayedExecutor
			{
				[this, safeFrame, reqUrl] { DelayedRemoveElements (safeFrame, reqUrl); },
				0
			};
		}

		const auto result = new Util::CustomNetworkReply (reqUrl, this);
		result->SetContent (QString ("Blocked by Poshuku CleanWeb"));
		result->SetError (QNetworkReply::ContentAccessDenied,
				tr ("Blocked by Poshuku CleanWeb: %1")
					.arg (reqUrl.toString ()));
		hook->SetReturnValue (QVariant::fromValue<QNetworkReply*> (result));
		return result;
	}

	void Core::HandleExtension (LeechCraft::IHookProxy_ptr proxy,
			QWebPage *page,
			QWebPage::Extension ext,
			const QWebPage::ExtensionOption *opt,
			QWebPage::ExtensionReturn*)
	{
		if (ext != QWebPage::ErrorPageExtension)
			return;

		auto error = static_cast<const QWebPage::ErrorPageExtensionOption*> (opt);
		if (error->error != QNetworkReply::ContentAccessDenied)
			return;

		auto url = error->url;
		proxy->CancelDefault ();
		proxy->SetReturnValue (true);

		const QPointer<QWebFrame> safeFrame { page->mainFrame () };
		new Util::DelayedExecutor
		{
			[this, safeFrame, url] { DelayedRemoveElements (safeFrame, url); },
			0
		};
	}

	void Core::HandleContextMenu (const QWebHitTestResult& r,
			QWebView *view, QMenu *menu,
			WebViewCtxMenuStage stage)
	{
		QUrl iurl = r.imageUrl ();
		if (stage == WVSAfterImage &&
				!iurl.isEmpty ())
		{
			QAction *action = menu->addAction (tr ("Block image..."),
					UserFilters_,
					SLOT (blockImage ()));
			action->setProperty ("CleanWeb/URL", iurl);
			action->setProperty ("CleanWeb/View", QVariant::fromValue<QObject*> (view));
		}
	}

	FlashOnClickPlugin* Core::GetFlashOnClick ()
	{
		if (!FlashOnClickPlugin_)
			FlashOnClickPlugin_ = std::make_shared<FlashOnClickPlugin> (Proxy_, FlashOnClickWhitelist_);

		return FlashOnClickPlugin_.get ();
	}

	FlashOnClickWhitelist* Core::GetFlashOnClickWhitelist ()
	{
		return FlashOnClickWhitelist_;
	}

	namespace
	{
#if defined (Q_OS_WIN32) || defined (Q_OS_MAC)
		// Thanks for this goes to http://www.codeproject.com/KB/string/patmatch.aspx
		bool WildcardMatches (const char *pattern, const char *str)
		{
			enum State {
				Exact,        // exact match
				Any,        // ?
				AnyRepeat    // *
			};

			const char *s = str;
			const char *p = pattern;
			const char *q = 0;
			int state = 0;

			bool match = true;
			while (match && *p) {
				if (*p == '*') {
					state = AnyRepeat;
					q = p+1;
				} else if (*p == '?') state = Any;
				else state = Exact;

				if (*s == 0) break;

				switch (state) {
					case Exact:
						match = *s == *p;
						s++;
						p++;
						break;

					case Any:
						match = true;
						s++;
						p++;
						break;

					case AnyRepeat:
						match = true;
						s++;

						if (*s == *q) p++;
						break;
				}
			}

			if (state == AnyRepeat) return (*s == *q);
			else if (state == Any) return (*s == *p);
			else return match && (*s == *p);
		}
#else
#include <fnmatch.h>

		bool WildcardMatches (const char *pat, const char *str)
		{
			return !fnmatch (pat, str, 0);
		}
#endif

		bool Matches (const FilterItem_ptr& item,
				const QString& urlStr, const QByteArray& urlUtf8, const QString& domain)
		{
			const auto& opt = item->Option_;
			if (opt.MatchObjects_ != FilterOption::MatchObject::All)
			{
				if (!(opt.MatchObjects_ & FilterOption::MatchObject::CSS) &&
						!(opt.MatchObjects_ & FilterOption::MatchObject::Image) &&
						!(opt.MatchObjects_ & FilterOption::MatchObject::Script) &&
						!(opt.MatchObjects_ & FilterOption::MatchObject::Object) &&
						!(opt.MatchObjects_ & FilterOption::MatchObject::ObjSubrequest))
					return false;
			}

			if (std::any_of (opt.NotDomains_.begin (), opt.NotDomains_.end (),
						[&domain, &opt] (const QString& notDomain)
							{ return domain.endsWith (notDomain, opt.Case_); }))
				return false;

			if (!opt.Domains_.isEmpty () &&
					std::none_of (opt.Domains_.begin (), opt.Domains_.end (),
							[&domain, &opt] (const QString& doDomain)
								{ return domain.endsWith (doDomain, opt.Case_); }))
				return false;

			switch (opt.MatchType_)
			{
			case FilterOption::MTRegexp:
				return item->RegExp_.Matches (urlStr);
			case FilterOption::MTWildcard:
				return WildcardMatches (item->PlainMatcher_.constData (), urlUtf8.constData ());
			case FilterOption::MTPlain:
				return urlUtf8.indexOf (item->PlainMatcher_) >= 0;
			case FilterOption::MTBegin:
				return urlStr.startsWith (QString::fromUtf8 (item->PlainMatcher_));
			case FilterOption::MTEnd:
				return urlStr.endsWith (QString::fromUtf8 (item->PlainMatcher_));
			}

			return false;
		}
	}

	namespace
	{
		void DumbReductor (bool& res, bool value)
		{
			if (value)
				res = true;
		}
	}

	/** We test each filter until we know that we should reject it or until
	 * it gets whitelisted.
	 *
	 * So, for each filter we first iterate through the whitelist. For each
	 * entry in the whitelist:
	 * - First, we check if the url's domain ends with a string from a "not
	 *   apply" list if it's not empty. If it does, we skip this whitelist
	 *   entry and go to the next one, if it doesn't, we continue
	 *   processing.
	 * - Then, if we continue processing, we check if the url's domain ends
	 *   with a string from "apply list", if this list isn't empty. If it
	 *   ends, we continue processing, otherwise we skip this whilelist
	 *   entry and go to the next one.
	 * - Then, we check if the URL matches this exception, either by regexp
	 *   or wildcard. If it should be matched only in the beginning or in
	 *   the end, then '*' is appended or prepended and exact match is
	 *   checked. Otherwise only something is required to match. Please not
	 *   that the '*' is prepended by the filter parsing code, not this one.
	 *
	 * The same is applied to the filter strings.
	 */
	bool Core::ShouldReject (const QNetworkRequest& req) const
	{
		if (!XmlSettingsManager::Instance ()->property ("EnableFiltering").toBool ())
			return false;

		if (!req.hasRawHeader ("referer"))
			return false;

		auto acceptList = req.rawHeader ("Accept").split (',');
		for (auto& item : acceptList)
		{
			const int pos = item.indexOf (';');
			if (pos > 0)
				item = item.left (pos);
		}
		acceptList.removeAll ("*/*");
		FilterOption::MatchObjects objs = FilterOption::MatchObject::All;
		if (!acceptList.isEmpty ())
		{
#ifdef USE_CPP14
			auto find = [&acceptList] (const auto& f)
#else
			auto find = [&acceptList] (std::function<bool (QByteArray)> f)
#endif
			{
				return std::any_of (acceptList.begin (), acceptList.end (), f);
			};

			if (find ([] (const QByteArray& arr) { return arr.startsWith ("image/"); }))
				objs |= FilterOption::MatchObject::Image;
			if (find ([] (const QByteArray& arr) { return arr == "text/html" || arr == "application/xhtml+xml" || arr == "application/xml"; }))
				objs |= FilterOption::MatchObject::Subdocument;
			if (find ([] (const QByteArray& arr) { return arr == "text/css"; }))
				objs |= FilterOption::MatchObject::CSS;
		}

		const QUrl referer { req.rawHeader ("Referer") };

		const QUrl& url = req.url ();
		const QString& urlStr = url.toString ();
		const auto& urlUtf8 = urlStr.toUtf8 ();
		const QString& cinUrlStr = urlStr.toLower ();
		const auto& cinUrlUtf8 = cinUrlStr.toUtf8 ();

		const QString& domain = referer.host ();
		const bool isForeign = !url.host ().endsWith (referer.host ());

		auto matches = [=] (const QList<QList<FilterItem_ptr>>& chunks) -> bool
			{
				return QtConcurrent::blockingMappedReduced (chunks.begin (), chunks.end (),
						std::function<bool (const QList<FilterItem_ptr>&)>
						{
							[=] (const QList<FilterItem_ptr>& items) -> bool
							{
								for (const auto& item : items)
								{
									const auto& opt = item->Option_;
									if (opt.AbortForeign_ && isForeign)
										continue;

									if (opt.MatchObjects_ != FilterOption::MatchObject::All &&
											objs != FilterOption::MatchObject::All &&
											!(objs & opt.MatchObjects_))
										continue;

									const auto& url = item->Option_.Case_ == Qt::CaseSensitive ? urlStr : cinUrlStr;
									const auto& utf8 = item->Option_.Case_ == Qt::CaseSensitive ? urlUtf8 : cinUrlUtf8;
									if (Matches (item, url, utf8, domain))
										return true;
								}

								return false;
							}
						}, DumbReductor);
			};
		if (matches (ExceptionsCache_))
			return false;
		if (matches (FilterItemsCache_))
			return true;

		return false;
	}

	void Core::HandleProvider (QObject *provider)
	{
		if (Downloaders_.contains (provider))
			return;

		Downloaders_ << provider;
		connect (provider,
				SIGNAL (jobFinished (int)),
				this,
				SLOT (handleJobFinished (int)));
		connect (provider,
				SIGNAL (jobError (int, IDownload::Error)),
				this,
				SLOT (handleJobError (int, IDownload::Error)));
	}

	void Core::Parse (const QString& filePath)
	{
		SubsModel_->AddFilter (ParseToFilters ({ filePath }).first ());
	}

	bool Core::Add (const QUrl& subscrUrl)
	{
		qDebug () << Q_FUNC_INFO << subscrUrl;
		QUrl url;

#if QT_VERSION < 0x050000
		const auto& location = subscrUrl.queryItemValue ("location");
#else
		const auto& location = QUrlQuery { subscrUrl }.queryItemValue ("location");
#endif

		if (location.contains ("%"))
			url.setUrl (QUrl::fromPercentEncoding (location.toLatin1 ()));
		else
			url.setUrl (location);

#if QT_VERSION < 0x050000
		const auto& subscrName = subscrUrl.queryItemValue ("title");
#else
		const auto& subscrName = QUrlQuery { subscrUrl }.queryItemValue ("title");
#endif

		qDebug () << "adding" << url << "as" << subscrName;
		bool result = Load (url, subscrName);
		if (result)
		{
			QString str = tr ("The subscription %1 was successfully added.")
					.arg (subscrName);
			Proxy_->GetEntityManager ()->HandleEntity (Util::MakeNotification ("Poshuku CleanWeb",
					str, PInfo_));
		}
		return result;
	}

	bool Core::Load (const QUrl& url, const QString& subscrName)
	{
		const auto& name = QFileInfo (url.path ()).fileName ();
		const auto& path = Util::CreateIfNotExists ("cleanweb").absoluteFilePath (name);

		const auto& e = Util::MakeEntity (url,
				path,
				Internal |
					DoNotNotifyUser |
					DoNotSaveInHistory |
					NotPersistent |
					DoNotAnnounceEntity);

		const auto iem = Proxy_->GetEntityManager ();
		const auto& result = iem->DelegateEntity (e);
		if (!result.Handler_)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to delegate"
					<< subscrName
					<< url.toString ();

			const auto& str = tr ("The subscription %1 wasn't delegated.")
					.arg (subscrName);
			iem->HandleEntity (Util::MakeNotification ("Poshuku CleanWeb",
					str, PCritical_));
			return false;
		}

		HandleProvider (result.Handler_);
		PendingJob pj
		{
			path,
			name,
			subscrName,
			url
		};
		PendingJobs_ [result.ID_] = pj;
		return true;
	}

	void Core::handleParsed ()
	{
		auto watcher = dynamic_cast<QFutureWatcher<QList<Filter>>*> (sender ());
		watcher->deleteLater ();

		SubsModel_->SetInitialFilters (watcher->result ());

		QTimer::singleShot (0,
				this,
				SLOT (update ()));
	}

	void Core::update ()
	{
		if (!XmlSettingsManager::Instance ()->
				property ("Autoupdate").toBool ())
			return;

		const auto& current = QDateTime::currentDateTime ();
		int days = XmlSettingsManager::Instance ()->
			property ("UpdateInterval").toInt ();
		for (const auto& f : SubsModel_->GetAllFilters ())
			if (f.SD_.LastDateTime_.daysTo (current) > days)
				Load (f.SD_.URL_, f.SD_.Name_);
	}

	void Core::handleJobFinished (int id)
	{
		if (!PendingJobs_.contains (id))
			return;

		PendingJob pj = PendingJobs_ [id];
		SubscriptionData sd
		{
			pj.URL_,
			pj.Subscr_,
			pj.FileName_,
			QDateTime::currentDateTime ()
		};
		Parse (pj.FullName_);
		PendingJobs_.remove (id);
		SubsModel_->SetSubData (sd);
	}

	void Core::handleJobError (int id, IDownload::Error)
	{
		if (!PendingJobs_.contains (id))
			return;

		PendingJobs_.remove (id);
	}

	void Core::HandleFrameLayout (QPointer<QWebFrame> frame, bool asLoad)
	{
		if (!frame)
			return;

		if (!XmlSettingsManager::Instance ()->property ("EnableElementHiding").toBool ())
			return;

		const QUrl& frameUrl = frame->url ().isEmpty () ?
				frame->baseUrl () :
				frame->url ();
		qDebug () << Q_FUNC_INFO << frame << frameUrl;
		const QString& urlStr = frameUrl.toString ();
		const auto& urlUtf8 = urlStr.toUtf8 ();
		const QString& cinUrlStr = urlStr.toLower ();
		const auto& cinUrlUtf8 = cinUrlStr.toUtf8 ();

		const QString& domain = frameUrl.host ();

		auto allFilters = SubsModel_->GetAllFilters ();
		allFilters << UserFilters_->GetFilter ();

		auto watcher = new QFutureWatcher<HidingWorkerResult> (this);
		connect (watcher,
				SIGNAL (finished ()),
				this,
				SLOT (hidingElementsFound ()));
		watcher->setFuture (QtConcurrent::run ([=] () -> HidingWorkerResult
					{
						QStringList sels;
						for (const Filter& filter : allFilters)
							for (const auto& item : filter.Filters_)
							{
								if (item->Option_.HideSelector_.isEmpty ())
									continue;

								const auto& opt = item->Option_;
								const auto& url = opt.Case_ == Qt::CaseSensitive ? urlStr : cinUrlStr;
								const auto& utf8 = opt.Case_ == Qt::CaseSensitive ? urlUtf8 : cinUrlUtf8;
								if (!Matches (item, url, utf8, domain))
									continue;

								sels << item->Option_.HideSelector_;
							}
						return { frame, 0, sels };
					}));

		auto worker = [this, frame]
		{
			for (auto childFrame : frame->childFrames ())
				HandleFrameLayout (childFrame, true);
		};

		worker ();

		if (!asLoad)
			new Util::SlotClosure<Util::DeleteLaterPolicy>
			{
				worker,
				frame,
				SIGNAL (loadFinished (bool)),
				frame
			};
	}

	void Core::hidingElementsFound ()
	{
		auto watcher = dynamic_cast<QFutureWatcher<HidingWorkerResult>*> (sender ());
		watcher->deleteLater ();

		HideElementsChunk (watcher->result ());
	}

	void Core::HideElementsChunk (HidingWorkerResult result)
	{
		if (!result.Frame_)
			return;

		const int chunkSize = 100;

		auto& i = result.CurrentPos_;
		for (auto end = std::min (i + chunkSize, result.Selectors_.size ()); i < end; ++i)
		{
			const auto& selector = result.Selectors_.value (i);

			const auto& matchingElems = result.Frame_->findAllElements (selector);
			if (matchingElems.count ())
				qDebug () << "removing"
						<< matchingElems.count ()
						<< "elems for"
						<< selector
						<< result.Frame_->url ();

			for (int i = matchingElems.count () - 1; i >= 0; --i)
				RemoveElem (matchingElems.at (i));
		}

		if (result.CurrentPos_ < result.Selectors_.size ())
			new Util::DelayedExecutor
			{
				[this, result] { HideElementsChunk (result); },
				0
			};
	}

	namespace
	{
		bool RemoveElements (QWebFrame *frame, const QList<QUrl>& urls)
		{
			const auto& baseUrl = frame->baseUrl ();

			const auto& elems = frame->findAllElements ("img,script,iframe,applet,object");

			bool removed = false;
			for (int i = elems.count () - 1; i >= 0; --i)
			{
				auto elem = elems.at (i);
				if (urls.contains (baseUrl.resolved (QUrl::fromEncoded (elem.attribute ("src").toUtf8 ()))))
				{
					elem.removeFromDocument ();
					removed = true;
				}
			}
			return removed;
		}
	}

	void Core::DelayedRemoveElements (QPointer<QWebFrame> frame, const QUrl& url)
	{
		if (!frame)
			return;

		if (RemoveElements (frame, { url }))
			return;

		connect (frame,
				SIGNAL (loadFinished (bool)),
				this,
				SLOT (moreDelayedRemoveElements ()),
				Qt::UniqueConnection);
		connect (frame,
				SIGNAL (destroyed (QObject*)),
				this,
				SLOT (handleFrameDestroyed ()),
				Qt::UniqueConnection);
		MoreDelayedURLs_ [frame] << url;
	}

	void Core::moreDelayedRemoveElements ()
	{
		auto frame = qobject_cast<QWebFrame*> (sender ());

		const auto& urls = MoreDelayedURLs_.take (frame);
		if (!RemoveElements (frame, urls))
			qWarning () << Q_FUNC_INFO
					<< urls
					<< "not found for"
					<< frame;
	}

	void Core::handleFrameDestroyed ()
	{
		MoreDelayedURLs_.remove (static_cast<QWebFrame*> (sender ()));
	}

	void Core::regenFilterCaches ()
	{
		ExceptionsCache_.clear ();
		FilterItemsCache_.clear ();

		auto allFilters = SubsModel_->GetAllFilters ();
		allFilters << UserFilters_->GetFilter ();

		int exceptionsCount = 0;
		int filtersCount = 0;
		for (const Filter& filter : allFilters)
		{
			exceptionsCount += filter.Exceptions_.size ();
			filtersCount += filter.Filters_.size ();
		}

		auto idealThreads = std::max (QThread::idealThreadCount (), 2);

		const int exChunkSize = std::max (exceptionsCount / idealThreads / 4, 200);
		const int fChunkSize = std::max (filtersCount / idealThreads / 4, 200);
		qDebug () << Q_FUNC_INFO << exceptionsCount << filtersCount << exChunkSize << fChunkSize;

		QList<FilterItem_ptr> lastItemsChunk, lastExceptionsChunk;
		lastItemsChunk.reserve (fChunkSize);
		lastExceptionsChunk.reserve (exChunkSize);

		for (const Filter& filter : allFilters)
		{
			for (const auto& item : filter.Exceptions_)
			{
				if (!item->Option_.HideSelector_.isEmpty ())
					continue;

				lastExceptionsChunk << item;
				if (lastExceptionsChunk.size () >= exChunkSize)
				{
					ExceptionsCache_ << lastExceptionsChunk;
					lastExceptionsChunk.clear ();
				}
			}

			for (const auto& item : filter.Filters_)
			{
				if (!item->Option_.HideSelector_.isEmpty ())
					continue;

				lastItemsChunk << item;
				if (lastItemsChunk.size () >= fChunkSize)
				{
					FilterItemsCache_ << lastItemsChunk;
					lastItemsChunk.clear ();
				}
			}
		}

		if (!lastItemsChunk.isEmpty ())
			FilterItemsCache_ << lastItemsChunk;
		if (!lastExceptionsChunk.isEmpty ())
			ExceptionsCache_ << lastExceptionsChunk;
	}
}
}
}
