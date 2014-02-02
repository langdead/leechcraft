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

#include "coreproxy.h"
#include <algorithm>
#include <interfaces/ifinder.h>
#include "core.h"
#include "mainwindow.h"
#include "xmlsettingsmanager.h"
#include "iconthemeengine.h"
#include "tagsmanager.h"
#include "entitymanager.h"
#include "config.h"
#include "colorthemeengine.h"
#include "rootwindowsmanager.h"

namespace LeechCraft
{
	CoreProxy::CoreProxy (QObject *parent)
	: QObject (parent)
	, EM_ (new EntityManager (this))
	{
	}

	QNetworkAccessManager* CoreProxy::GetNetworkAccessManager () const
	{
		return Core::Instance ().GetNetworkAccessManager ();
	}

	IShortcutProxy* CoreProxy::GetShortcutProxy () const
	{
		return Core::Instance ().GetShortcutProxy ();
	}

	QModelIndex CoreProxy::MapToSource (const QModelIndex& index) const
	{
		return Core::Instance ().MapToSource (index);
	}

	Util::BaseSettingsManager* CoreProxy::GetSettingsManager () const
	{
		return XmlSettingsManager::Instance ();
	}

	IRootWindowsManager* CoreProxy::GetRootWindowsManager () const
	{
		return Core::Instance ().GetRootWindowsManager ();
	}

	QIcon CoreProxy::GetIcon (const QString& icon, const QString& iconOff) const
	{
		return IconThemeEngine::Instance ().GetIcon (icon, iconOff);
	}

	void CoreProxy::UpdateIconset (const QList<QAction*>& actions) const
	{
		IconThemeEngine::Instance ().UpdateIconset (actions);
	}

	IColorThemeManager* CoreProxy::GetColorThemeManager () const
	{
		return &ColorThemeEngine::Instance ();
	}

	ITagsManager* CoreProxy::GetTagsManager () const
	{
		return &TagsManager::Instance ();
	}

	QStringList CoreProxy::GetSearchCategories () const
	{
		const QList<IFinder*>& finders = Core::Instance ().GetPluginManager ()->
			GetAllCastableTo<IFinder*> ();

		QStringList result;
		for (QList<IFinder*>::const_iterator i = finders.begin (),
				end = finders.end (); i != end; ++i)
			result += (*i)->GetCategories ();
		result.removeDuplicates ();
		std::sort (result.begin (), result.end ());
		return result;
	}

	int CoreProxy::GetID ()
	{
		return Pool_.GetID ();
	}

	void CoreProxy::FreeID (int id)
	{
		Pool_.FreeID (id);
	}

	IPluginsManager* CoreProxy::GetPluginsManager () const
	{
		return Core::Instance ().GetPluginManager ();
	}

	IEntityManager* CoreProxy::GetEntityManager () const
	{
		return EM_;
	}

	QString CoreProxy::GetVersion () const
	{
		return LEECHCRAFT_VERSION;
	}

	QObject* CoreProxy::GetSelf ()
	{
		return this;
	}

	void CoreProxy::RegisterSkinnable (QAction *act)
	{
		IconThemeEngine::Instance ().UpdateIconset (QList<QAction*> () << act);
	}

	bool CoreProxy::IsShuttingDown ()
	{
		return Core::Instance ().IsShuttingDown ();
	}
}
