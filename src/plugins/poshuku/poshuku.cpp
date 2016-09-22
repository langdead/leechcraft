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

#include "poshuku.h"
#include <stdexcept>
#include <QMessageBox>
#include <qwebsettings.h>
#include <QHeaderView>
#include <QToolBar>
#include <QDir>
#include <QUrl>
#include <QTextCodec>
#include <QInputDialog>
#include <QMenu>
#include <QMainWindow>
#include <QtDebug>
#include <interfaces/entitytesthandleresult.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/irootwindowsmanager.h>
#include <interfaces/core/iiconthememanager.h>
#include <util/util.h>
#include <util/xpc/util.h>
#include <util/sys/paths.h>
#include <util/tags/tagscompletionmodel.h>
#include <util/db/backendselector.h>
#include <util/xsd/wkfontswidget.h>
#include <util/shortcuts/shortcutmanager.h>
#include "core.h"
#include "xmlsettingsmanager.h"
#include "browserwidget.h"
#include "cookieseditdialog.h"

namespace LeechCraft
{
namespace Poshuku
{
	using LeechCraft::Util::TagsCompletionModel;

	Poshuku::~Poshuku ()
	{
		Core::Instance ().setParent (0);
	}

	void Poshuku::Init (ICoreProxy_ptr coreProxy)
	{
		Util::InstallTranslator ("poshuku");

		BrowserWidget::SetParentMultiTabs (this);
		Core::Instance ().setParent (this);
		Core::Instance ().SetProxy (coreProxy);

		ShortcutMgr_ = new Util::ShortcutManager { coreProxy };
		ShortcutMgr_->SetObject (this);
		Core::Instance ().SetShortcutManager (ShortcutMgr_);

		XmlSettingsDialog_ = std::make_shared<Util::XmlSettingsDialog> ();
		XmlSettingsDialog_->RegisterObject (XmlSettingsManager::Instance (),
				"poshukusettings.xml");
		XmlSettingsDialog_->SetCustomWidget ("BackendSelector",
				new Util::BackendSelector (XmlSettingsManager::Instance ()));

		FontsWidget_ = new Util::WkFontsWidget { XmlSettingsManager::Instance () };
		XmlSettingsDialog_->SetCustomWidget ("FontsSelector", FontsWidget_);
		connect (&Core::Instance (),
				SIGNAL (browserWidgetCreated (BrowserWidget*)),
				this,
				SLOT (handleBrowserWidgetCreated (BrowserWidget*)));

		InitConnections ();

		ImportXbel_ = new QAction (tr ("Import XBEL..."), this);
		ImportXbel_->setProperty ("ActionIcon", "document-import");
		ShortcutMgr_->RegisterAction ("EAImportXbel_", ImportXbel_);

		ExportXbel_ = new QAction (tr ("Export XBEL..."), this);
		ExportXbel_->setProperty ("ActionIcon", "document-export");
		ShortcutMgr_->RegisterAction ("EAExportXbel_", ExportXbel_);

		CheckFavorites_ = new QAction (tr ("Check favorites..."), this);
		CheckFavorites_->setProperty ("ActionIcon", "checkbox");
		ShortcutMgr_->RegisterAction ("EACheckFavorites_", CheckFavorites_);

		ReloadAll_ = new QAction (tr ("Reload all pages"), this);
		ReloadAll_->setProperty ("ActionIcon", "system-software-update");
		ShortcutMgr_->RegisterAction ("EAReloadAll_", ReloadAll_);

		try
		{
			Core::Instance ().Init ();
		}
		catch (const std::exception& e)
		{
			qWarning () << Q_FUNC_INFO
					<< e.what ();
			QMessageBox::critical (nullptr,
					"LeechCraft",
					tr ("Poshuku failed to initialize properly. "
						"Check logs and talk with the developers. "
						"Or, at least, check the storage backend "
						"settings and restart LeechCraft."));
			throw;
		}

		PrepopulateShortcuts ();

		connect (Core::Instance ().GetFavoritesModel (),
				SIGNAL (error (const QString&)),
				this,
				SLOT (handleError (const QString&)));

		connect (ImportXbel_,
				SIGNAL (triggered ()),
				&Core::Instance (),
				SLOT (importXbel ()));
		connect (ExportXbel_,
				SIGNAL (triggered ()),
				&Core::Instance (),
				SLOT (exportXbel ()));
		connect (CheckFavorites_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleCheckFavorites ()));
		connect (ReloadAll_,
				SIGNAL (triggered ()),
				this,
				SLOT (handleReloadAll ()));

		ToolMenu_ = new QMenu ("Poshuku");
		ToolMenu_->setIcon (GetIcon ());
		ToolMenu_->addAction (ImportXbel_);
		ToolMenu_->addAction (ExportXbel_);
	}

	void Poshuku::SecondInit ()
	{
		QTimer::singleShot (1000,
				this,
				SLOT (createTabFirstTime ()));
	}

	void Poshuku::Release ()
	{
		Core::Instance ().setParent (0);
		Core::Instance ().Release ();
		XmlSettingsDialog_.reset ();
	}

	QByteArray Poshuku::GetUniqueID () const
	{
		return "org.LeechCraft.Poshuku";
	}

	QString Poshuku::GetName () const
	{
		return tr ("Poshuku Browser");
	}

	QString Poshuku::GetInfo () const
	{
		return tr ("Simple yet functional web browser");
	}

	QStringList Poshuku::Provides () const
	{
		return { "webbrowser" };
	}

	QIcon Poshuku::GetIcon () const
	{
		static QIcon icon ("lcicons:/resources/images/poshuku.svg");
		return icon;
	}

	TabClasses_t Poshuku::GetTabClasses () const
	{
		TabClasses_t result;
		result << Core::Instance ().GetTabClass ();
		return result;
	}

	void Poshuku::TabOpenRequested (const QByteArray& tabClass)
	{
		if (tabClass == "Poshuku")
			Core::Instance ().NewURL ("", true);
		else
			qWarning () << Q_FUNC_INFO
					<< "unknown tab class"
					<< tabClass;
	}

	QSet<QByteArray> Poshuku::GetExpectedPluginClasses () const
	{
		return Core::Instance ().GetExpectedPluginClasses ();
	}

	void Poshuku::AddPlugin (QObject *plugin)
	{
		Core::Instance ().AddPlugin (plugin);
	}

	std::shared_ptr<LeechCraft::Util::XmlSettingsDialog> Poshuku::GetSettingsDialog () const
	{
		return XmlSettingsDialog_;
	}

	EntityTestHandleResult Poshuku::CouldHandle (const LeechCraft::Entity& e) const
	{
		return Core::Instance ().CouldHandle (e) ?
				EntityTestHandleResult (EntityTestHandleResult::PIdeal) :
				EntityTestHandleResult ();
	}

	void Poshuku::Handle (LeechCraft::Entity e)
	{
		Core::Instance ().Handle (e);
	}

	void Poshuku::Open (const QString& link)
	{
		Core::Instance ().NewURL (link);
	}

	IWebWidget* Poshuku::GetWidget () const
	{
		return Core::Instance ().GetWidget ();
	}

	void Poshuku::SetShortcut (const QString& name, const QKeySequences_t& sequences)
	{
		ShortcutMgr_->SetShortcut (name, sequences);
	}

	QMap<QString, ActionInfo> Poshuku::GetActionInfo () const
	{
		return ShortcutMgr_->GetActionInfo ();
	}

	QList<QAction*> Poshuku::GetActions (ActionsEmbedPlace place) const
	{
		QList<QAction*> result;

		switch (place)
		{
		case ActionsEmbedPlace::ToolsMenu:
			result << CheckFavorites_;
			result << ToolMenu_->menuAction ();
			break;
		case ActionsEmbedPlace::CommonContextMenu:
			result << ReloadAll_;
			break;
		default:
			break;
		}

		return result;
	}

	void Poshuku::RecoverTabs (const QList<TabRecoverInfo>& infos)
	{
		for (const auto& info : infos)
		{
			auto bw = Core::Instance ().NewURL (QUrl (), false, info.DynProperties_);
			bw->SetTabRecoverData (info.Data_);
			emit tabRecovered (info.Data_, bw);
		}
	}

	bool Poshuku::HasSimilarTab (const QByteArray& data, const QList<QByteArray>& existing) const
	{
		return StandardSimilarImpl (data, existing,
				[] (const QByteArray& data)
				{
					QUrl url;
					QDataStream str { data };
					str >> url;
					return url;
				});
	}

	void Poshuku::InitConnections ()
	{
		connect (XmlSettingsDialog_.get (),
				SIGNAL (pushButtonClicked (const QString&)),
				this,
				SLOT (handleSettingsClicked (const QString&)));

		connect (&Core::Instance (),
				SIGNAL (addNewTab (const QString&, QWidget*)),
				this,
				SIGNAL (addNewTab (const QString&, QWidget*)));
		connect (&Core::Instance (),
				SIGNAL (removeTab (QWidget*)),
				this,
				SIGNAL (removeTab (QWidget*)));
		connect (&Core::Instance (),
				SIGNAL (changeTabName (QWidget*, const QString&)),
				this,
				SIGNAL (changeTabName (QWidget*, const QString&)));
		connect (&Core::Instance (),
				SIGNAL (changeTabIcon (QWidget*, const QIcon&)),
				this,
				SIGNAL (changeTabIcon (QWidget*, const QIcon&)));
		connect (&Core::Instance (),
				SIGNAL (changeTooltip (QWidget*, QWidget*)),
				this,
				SIGNAL (changeTooltip (QWidget*, QWidget*)));
		connect (&Core::Instance (),
				SIGNAL (statusBarChanged (QWidget*, const QString&)),
				this,
				SIGNAL (statusBarChanged (QWidget*, const QString&)));
		connect (&Core::Instance (),
				SIGNAL (raiseTab (QWidget*)),
				this,
				SIGNAL (raiseTab (QWidget*)));
		connect (&Core::Instance (),
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SIGNAL (gotEntity (const LeechCraft::Entity&)));
		connect (&Core::Instance (),
				SIGNAL (delegateEntity (const LeechCraft::Entity&, int*, QObject**)),
				this,
				SIGNAL (delegateEntity (const LeechCraft::Entity&, int*, QObject**)));
		connect (&Core::Instance (),
				SIGNAL (couldHandle (const LeechCraft::Entity&, bool*)),
				this,
				SIGNAL (couldHandle (const LeechCraft::Entity&, bool*)));
		connect (&Core::Instance (),
				SIGNAL (error (const QString&)),
				this,
				SLOT (handleError (const QString&)));
	}

	void Poshuku::PrepopulateShortcuts ()
	{
		const auto itm = Core::Instance ().GetProxy ()->GetIconThemeManager ();
#define REG(id,name,icon,shortcut) \
		ShortcutMgr_->RegisterActionInfo ("Browser" id, \
				{ name, QKeySequence { shortcut }, itm->GetIcon (icon) })

		REG ("Cut_", tr ("Cut"), "edit-cut", tr ("Ctrl+X"));
		REG ("Copy_", tr ("Copy"), "edit-copy", tr ("Ctrl+C"));
		REG ("Paste_", tr ("Paste"), "edit-paste", tr ("Ctrl+V"));
		REG ("Back_", tr ("Back"), "go-previous", Qt::ALT + Qt::Key_Left);
		REG ("Forward_", tr ("Forward"), "go-next", Qt::ALT + Qt::Key_Right);
		REG ("Reload_", tr ("Reload"), "view-refresh", Qt::Key_F5);
		REG ("Stop_", tr ("Stop"), "process-stop", Qt::Key_Escape);
		REG ("ZoomIn_", tr ("Zoom in"), "zoom-in", Qt::CTRL + Qt::Key_Plus);
		REG ("ZoomOut_", tr ("Zoom out"), "zoom-out", Qt::CTRL + Qt::Key_Minus);
		REG ("ZoomReset_", tr ("Reset zoom"), "zoom-original", tr ("Ctrl+0"));
		REG ("TextZoomIn_", tr ("Text zoom in"), {}, Qt::CTRL + Qt::SHIFT + Qt::Key_Plus);
		REG ("TextZoomOut_", tr ("Text zoom out"), {}, Qt::CTRL + Qt::SHIFT + Qt::Key_Minus);
		REG ("TextZoomReset_", tr ("Reset text zoom"), {}, tr ("Ctrl+Shift+0"));
		REG ("Add2Favorites_", tr ("Bookmark..."), "bookmark-new", tr ("Ctrl+D"));
		REG ("Print_", tr ("Print..."), "document-print", tr ("Ctrl+P"));
		REG ("PrintPreview_", tr ("Print with preview..."), "document-print-preview", tr ("Ctrl+Shift+P"));
		REG ("ScreenSave_", tr ("Take page's screenshot..."), "camera-photo", Qt::Key_F12);
		REG ("ViewSources_", tr ("View sources..."), "applications-development-web", tr ("Ctrl+Shift+V"));
#undef REG
	}

	void Poshuku::createTabFirstTime ()
	{
		bool firstTime = XmlSettingsManager::Instance ()->
				Property ("FirstTimeRun", true).toBool ();
		bool startWithHome = XmlSettingsManager::Instance ()->
				property ("StartWithHomeTab").toBool ();
		if (firstTime || startWithHome)
			Core::Instance ().NewURL ("about:home", true);
		XmlSettingsManager::Instance ()->
				setProperty ("FirstTimeRun", false);
	}

	void Poshuku::handleError (const QString& msg)
	{
		emit gotEntity (Util::MakeNotification ("Poshuku", msg, PWarning_));
	}

	void Poshuku::handleSettingsClicked (const QString& name)
	{
		if (name == "CookiesEdit")
		{
			auto rootWM = Core::Instance ().GetProxy ()->GetRootWindowsManager ();
			auto dia = new CookiesEditDialog (rootWM->GetPreferredWindow ());
			dia->show ();
		}
		else
			qWarning () << Q_FUNC_INFO
				<< "unknown name"
				<< name;
	}

	void Poshuku::handleCheckFavorites ()
	{
		Core::Instance ().CheckFavorites ();
	}

	void Poshuku::handleReloadAll ()
	{
		Core::Instance ().ReloadAll ();
	}

	void Poshuku::handleBrowserWidgetCreated (BrowserWidget *widget)
	{
		FontsWidget_->RegisterSettable (widget);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_poshuku, LeechCraft::Poshuku::Poshuku);
