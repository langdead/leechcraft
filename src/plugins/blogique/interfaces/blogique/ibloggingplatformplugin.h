/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
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

#include <QList>

class QObject;

namespace LeechCraft
{
namespace Blogique
{
	/** This is the base interface for plugins providing blogging platforms.
	 * Since these plugins are plugins for plugins, they
	 * should also implement IPlugin2 and return the
	 * "org.LeechCraft.Plugins.Blogique.Plugins.IBloggingPlatformPlugin"
	 * string, among others, from their IPlugin2::GetPluginClasses()
	 * method.
	 */
	class IBloggingPlatformPlugin
	{
	public:
		virtual ~IBloggingPlatformPlugin () {}

		/** @brief Returns the protocol plugin object as a QObject.
		 *
		 * @return The protocol plugin as a QObject.
		 */
		virtual QObject* GetQObject () = 0;

		/** @brief Returns the blogging platforms list provided by this plugin.
		 *
		 * Each object in this list must implement the IBloggingPlatform
		 * interface.
		 *
		 * @return The list of this plugin's blogging platforms.
		 *
		 * @sa IBloggingPlatform
		 */
		virtual QList<QObject*> GetBloggingPlatforms () const = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Blogique::IBloggingPlatformPlugin,
		"org.Deviant.LeechCraft.Blogique.IBloggingPlatformPlugin/1.0")

