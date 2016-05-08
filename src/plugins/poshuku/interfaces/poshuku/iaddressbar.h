/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
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

#ifndef PLUGINS_POSHUKU_IADDRESSBAR_H
#define PLUGINS_POSHUKU_IADDRESSBAR_H
#include <QtPlugin>
class QAction;
class QToolButton;

namespace LeechCraft
{
namespace Poshuku
{
	class IAddressBar
	{
	public:
		virtual ~IAddressBar () {}

		virtual QObject* GetQObject () = 0;

		virtual int ButtonsCount () const = 0;

		virtual QToolButton* AddAction (QAction *action, bool hideOnEmptyUrl = false) = 0;
		virtual QToolButton* InsertAction (QAction *action, int pos = -1, bool hideOnEmptyUrl = false) = 0;

		virtual void RemoveAction (QAction *action) = 0;

		virtual QToolButton* GetButtonFromAction (QAction *action) const = 0;

		virtual void SetVisible (QAction *action, bool visible) = 0;
	protected:
		virtual void actionTriggered (QAction *action, const QString& text) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Poshuku::IAddressBar,
		"org.Deviant.LeechCraft.Poshuku.IAddressBar/1.0")

#endif
