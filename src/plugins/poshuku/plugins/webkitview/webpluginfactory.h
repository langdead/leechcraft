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

#include <QHash>
#include <qwebpluginfactory.h>
#include <interfaces/core/ipluginsmanager.h>
#include "interfaces/poshuku/iwebplugin.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace WebKitView
{
	class WebPluginFactory : public QWebPluginFactory
	{
		Q_OBJECT

		IPluginsManager * const PM_;

		QList<IWebPlugin*> Plugins_;
		typedef QHash<QString, IWebPlugin*> MIME2Plugin_t;
		MIME2Plugin_t MIME2Plugin_;
	public:
		WebPluginFactory (IPluginsManager*, QObject* = nullptr);
		virtual ~WebPluginFactory ();

		QObject* create (const QString&, const QUrl&,
				const QStringList&, const QStringList&) const;
		QList<Plugin> plugins () const;
		void refreshPlugins ();
	private:
		void Reload ();
	};
}
}
}
