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

#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <QMainWindow>
#include <QMessageBox>
#include <QtDebug>
#include <QTimer>
#include <QNetworkProxy>
#include <QApplication>
#include <QAction>
#include <QToolBar>
#include <QDir>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QAbstractNetworkCache>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QInputDialog>
#include <QMimeData>
#include <QNetworkCookie>
#include <util/xpc/util.h>
#include <util/network/customcookiejar.h>
#include <util/xpc/defaulthookproxy.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>
#include <interfaces/iinfo.h>
#include <interfaces/idownload.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/ijobholder.h>
#include <interfaces/ihavetabs.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/ihaveshortcuts.h>
#include <interfaces/iactionsexporter.h>
#include <interfaces/isummaryrepresentation.h>
#include <interfaces/structures.h>
#include <interfaces/entitytesthandleresult.h>
#include "application.h"
#include "mainwindow.h"
#include "pluginmanager.h"
#include "core.h"
#include "xmlsettingsmanager.h"
#include "sqlstoragebackend.h"
#include "handlerchoicedialog.h"
#include "tagsmanager.h"
#include "application.h"
#include "newtabmenumanager.h"
#include "networkaccessmanager.h"
#include "tabmanager.h"
#include "localsockethandler.h"
#include "storagebackend.h"
#include "coreinstanceobject.h"
#include "coreplugin2manager.h"
#include "dockmanager.h"
#include "entitymanager.h"
#include "rootwindowsmanager.h"

using namespace LeechCraft::Util;

namespace LeechCraft
{
	Core::Core ()
	: NetworkAccessManager_ (new NetworkAccessManager)
	, StorageBackend_ (new SQLStorageBackend)
	, LocalSocketHandler_ (new LocalSocketHandler)
	, NewTabMenuManager_ (new NewTabMenuManager)
	, CoreInstanceObject_ (new CoreInstanceObject)
	, RootWindowsManager_ (new RootWindowsManager)
	, DM_ (new DockManager (RootWindowsManager_.get (), this))
	{
		CoreInstanceObject_->GetCorePluginManager ()->RegisterHookable (NetworkAccessManager_.get ());
		CoreInstanceObject_->GetCorePluginManager ()->RegisterHookable (DM_);
		CoreInstanceObject_->GetCorePluginManager ()->RegisterHookable (RootWindowsManager_.get ());

		connect (RootWindowsManager_.get (),
				SIGNAL (tabIsMoving (int, int, int)),
				DM_,
				SLOT (handleTabMove (int, int, int)));

		connect (CoreInstanceObject_->GetSettingsDialog ().get (),
				SIGNAL (pushButtonClicked (const QString&)),
				this,
				SLOT (handleSettingClicked (const QString&)));

		connect (LocalSocketHandler_.get (),
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SLOT (queueEntity (const LeechCraft::Entity&)));
		connect (NetworkAccessManager_.get (),
				SIGNAL (error (const QString&)),
				this,
				SIGNAL (error (const QString&)));

		StorageBackend_->Prepare ();

		QStringList paths;
		const auto& map = qobject_cast<Application*> (qApp)->GetVarMap ();
		if (map.count ("plugin"))
		{
			const auto& plugins = map ["plugin"].as<std::vector<std::string>> ();
			for (const auto& plugin : plugins)
				paths << QString::fromUtf8 (plugin.c_str ());
		}
		PluginManager_ = new PluginManager (paths, this);
	}

	Core::~Core ()
	{
	}

	Core& Core::Instance ()
	{
		static Core core;
		return core;
	}

	void Core::Release ()
	{
		IsShuttingDown_ = true;

		RootWindowsManager_->Release ();

		LocalSocketHandler_.reset ();
		XmlSettingsManager::Instance ()->setProperty ("FirstStart", "false");

		PluginManager_->Release ();
		delete PluginManager_;

		CoreInstanceObject_.reset ();

		NetworkAccessManager_.reset ();

		StorageBackend_.reset ();

		XmlSettingsManager::Instance ()->Release ();
	}

	bool Core::IsShuttingDown () const
	{
		return IsShuttingDown_;
	}

	DockManager* Core::GetDockManager () const
	{
		return DM_;
	}

	IShortcutProxy* Core::GetShortcutProxy () const
	{
		return CoreInstanceObject_->GetShortcutProxy ();
	}

	QObjectList Core::GetSettables () const
	{
		return PluginManager_->GetAllCastableRoots<IHaveSettings*> ();
	}

	QList<QList<QAction*>> Core::GetActions2Embed () const
	{
		const QList<IActionsExporter*>& plugins =
				PluginManager_->GetAllCastableTo<IActionsExporter*> ();
		QList<QList<QAction*>> actions;
		Q_FOREACH (const IActionsExporter *plugin, plugins)
		{
			const QList<QAction*>& list = plugin->GetActions (ActionsEmbedPlace::CommonContextMenu);
			if (!list.size ())
				continue;
			actions << list;
		}
		return actions;
	}

	QAbstractItemModel* Core::GetPluginsModel () const
	{
		return PluginManager_;
	}

	PluginManager* Core::GetPluginManager () const
	{
		return PluginManager_;
	}

	StorageBackend* Core::GetStorageBackend () const
	{
		return StorageBackend_.get ();
	}

	CoreInstanceObject* Core::GetCoreInstanceObject () const
	{
		return CoreInstanceObject_.get ();
	}

	void Core::Setup (QObject *plugin)
	{
		InitDynamicSignals (plugin);
		if (qobject_cast<IHaveTabs*> (plugin))
			InitMultiTab (plugin);
	}

	void Core::PostSecondInit (QObject *plugin)
	{
		if (qobject_cast<IHaveTabs*> (plugin))
			GetNewTabMenuManager ()->AddObject (plugin);
	}

	void Core::DelayedInit ()
	{
		const auto& map = qobject_cast<Application*> (qApp)->GetVarMap ();
		if (map.count ("list-plugins"))
		{
			for (auto loader : PluginManager_->GetAllAvailable ())
				std::cout << "Found plugin: " << loader->GetFileName ().toUtf8 ().constData () << std::endl;
			std::exit (0);
		}

		PluginManager_->Init (map.count ("safe-mode"));

		NewTabMenuManager_->SetToolbarActions (GetActions2Embed ());

		disconnect (LocalSocketHandler_.get (),
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SLOT (queueEntity (const LeechCraft::Entity&)));

		connect (LocalSocketHandler_.get (),
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SLOT (handleGotEntity (const LeechCraft::Entity&)));

		QTimer::singleShot (1000,
				LocalSocketHandler_.get (),
				SLOT (pullCommandLine ()));

		QTimer::singleShot (2000,
				this,
				SLOT (pullEntityQueue ()));

		QTimer::singleShot (10000,
				this,
				SLOT (handlePluginLoadErrors ()));
	}

	void Core::TryToAddJob (QString name)
	{
		Entity e;
		if (QFile::exists (name))
			e.Entity_ = QUrl::fromLocalFile (name);
		else
		{
			const QUrl url (name);
			e.Entity_ = url.isValid () ? url : name;
		}
		e.Parameters_ = FromUserInitiated;

		if (!handleGotEntity (e))
			emit error (tr ("No plugins are able to download \"%1\"").arg (name));
	}

	QPair<qint64, qint64> Core::GetSpeeds () const
	{
		qint64 download = 0;
		qint64 upload = 0;

		Q_FOREACH (QObject *plugin, PluginManager_->GetAllPlugins ())
		{
			IDownload *di = qobject_cast<IDownload*> (plugin);
			if (di)
			{
				try
				{
					download += di->GetDownloadSpeed ();
					upload += di->GetUploadSpeed ();
				}
				catch (const std::exception& e)
				{
					qWarning () << Q_FUNC_INFO
						<< "unable to get speeds"
						<< e.what ()
						<< plugin;
				}
				catch (...)
				{
					qWarning () << Q_FUNC_INFO
						<< "unable to get speeds"
						<< plugin;
				}
			}
		}

		return QPair<qint64, qint64> (download, upload);
	}

	QNetworkAccessManager* Core::GetNetworkAccessManager () const
	{
		return NetworkAccessManager_.get ();
	}

	RootWindowsManager* Core::GetRootWindowsManager () const
	{
		return RootWindowsManager_.get ();
	}

	QModelIndex Core::MapToSource (const QModelIndex& index) const
	{
		const QList<ISummaryRepresentation*>& summaries =
			PluginManager_->GetAllCastableTo<ISummaryRepresentation*> ();
		Q_FOREACH (const ISummaryRepresentation *summary, summaries)
		{
			const QModelIndex& mapped = summary->MapToSource (index);
			if (mapped.isValid ())
				return mapped;
		}
		return QModelIndex ();
	}

	NewTabMenuManager* Core::GetNewTabMenuManager () const
	{
		return NewTabMenuManager_.get ();
	}

	void Core::handleSettingClicked (const QString& name)
	{
		auto win = RootWindowsManager_->GetPreferredWindow ();
		if (name == "ClearCache")
		{
			if (QMessageBox::question (win,
						"LeechCraft",
						tr ("Do you really want to clear the network cache?"),
						QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
				return;

			QAbstractNetworkCache *cache = NetworkAccessManager_->cache ();
			if (cache)
				cache->clear ();
		}
		else if (name == "ClearCookies")
		{
			if (QMessageBox::question (win,
						"LeechCraft",
						tr ("Do you really want to clear cookies?"),
						QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
				return;

			CustomCookieJar *jar = static_cast<CustomCookieJar*> (NetworkAccessManager_->cookieJar ());
			jar->setAllCookies (QList<QNetworkCookie> ());
			jar->Save ();
		}
		else if (name == "SetStartupPassword")
		{
			if (QMessageBox::question (win,
						"LeechCraft",
						tr ("This security measure is easily circumvented by modifying "
							"LeechCraft's settings files (or registry on Windows) in a text "
							"editor. For proper and robust protection consider using some "
							"third-party tools like <em>encfs</em> (http://www.arg0.net/encfs/)."
							"<br/><br/>Accept this dialog if you understand the above and "
							"this kind of security through obscurity is OK for you."),
						QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
				return;

			bool ok = false;
			const auto& newPass = QInputDialog::getText (win,
					"LeechCraft",
					tr ("Enter new startup password:"),
					QLineEdit::Password,
					QString (),
					&ok);
			if (!ok)
				return;

			QString contents;
			if (!newPass.isEmpty ())
				contents = QCryptographicHash::hash (newPass.toUtf8 (), QCryptographicHash::Sha1).toHex ();
			XmlSettingsManager::Instance ()->setProperty ("StartupPassword", contents);
		}
	}

	bool Core::handleGotEntity (Entity p, int *id, QObject **pr)
	{
		if (id && pr)
		{
			const auto& result = EntityManager ().DelegateEntity (p);
			*id = result.ID_;
			*pr = result.Handler_;
			return result.Handler_;
		}
		else
			return EntityManager ().HandleEntity (p);
	}

	void Core::handleCouldHandle (const Entity& e, bool *could)
	{
		*could = EntityManager ().CouldHandle (e);
	}

	void Core::queueEntity (const Entity& e)
	{
		QueuedEntities_ << e;
	}

	void Core::pullEntityQueue ()
	{
		for (const auto& e : QueuedEntities_)
			handleGotEntity (e);
		QueuedEntities_.clear ();
	}

	void Core::handlePluginLoadErrors ()
	{
		Q_FOREACH (const QString& error, PluginManager_->GetPluginLoadErrors ())
			handleGotEntity (Util::MakeNotification (tr ("Plugin load error"),
					error, PCritical_));
	}

	void Core::InitDynamicSignals (const QObject *plugin)
	{
		const QMetaObject *qmo = plugin->metaObject ();

		if (qmo->indexOfSignal (QMetaObject::normalizedSignature (
						"couldHandle (const LeechCraft::Entity&, bool*)"
						).constData ()) != -1)
			connect (plugin,
					SIGNAL (couldHandle (const LeechCraft::Entity&, bool*)),
					this,
					SLOT (handleCouldHandle (const LeechCraft::Entity&, bool*)));

		if (qmo->indexOfSignal (QMetaObject::normalizedSignature (
						"gotEntity (const LeechCraft::Entity&)"
						).constData ()) != -1)
			connect (plugin,
					SIGNAL (gotEntity (const LeechCraft::Entity&)),
					this,
					SLOT (handleGotEntity (LeechCraft::Entity)),
					Qt::QueuedConnection);

		if (qmo->indexOfSignal (QMetaObject::normalizedSignature (
						"delegateEntity (const LeechCraft::Entity&, int*, QObject**)"
						).constData ()) != -1)
			connect (plugin,
					SIGNAL (delegateEntity (const LeechCraft::Entity&,
							int*, QObject**)),
					this,
					SLOT (handleGotEntity (LeechCraft::Entity,
							int*, QObject**)));
	}

	void Core::InitMultiTab (const QObject *plugin)
	{
		connect (plugin,
				SIGNAL (addNewTab (const QString&, QWidget*)),
				RootWindowsManager_.get (),
				SLOT (add (const QString&, QWidget*)));
		connect (plugin,
				SIGNAL (removeTab (QWidget*)),
				RootWindowsManager_.get (),
				SLOT (remove (QWidget*)));
		connect (plugin,
				SIGNAL (changeTabName (QWidget*, const QString&)),
				RootWindowsManager_.get (),
				SLOT (changeTabName (QWidget*, const QString&)),
				Qt::UniqueConnection);
		connect (plugin,
				SIGNAL (changeTabIcon (QWidget*, const QIcon&)),
				RootWindowsManager_.get (),
				SLOT (changeTabIcon (QWidget*, const QIcon&)),
				Qt::UniqueConnection);
		connect (plugin,
				SIGNAL (raiseTab (QWidget*)),
				RootWindowsManager_.get (),
				SLOT (bringToFront (QWidget*)),
				Qt::UniqueConnection);
	}
}
