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

#include <QtPlugin>

class QString;
class QRect;

namespace LeechCraft
{
namespace Monocle
{
	/** @brief Interface for documents supporting querying text contents.
	 *
	 * This interface should be implemented by the documents of formats
	 * supporting obtaining the text contained in some page rectangle.
	 */
	class IHaveTextContent
	{
	public:
		/** @brief Virtual destructor.
		 */
		virtual ~IHaveTextContent () {}

		/** @brief Returns the text in the given rectangle.
		 *
		 * This function should return the text contained in the given
		 * \em rect at the given \em page, or an empty string if there is
		 * no text in this \em rect or the document doesn't contain any
		 * text information.
		 *
		 * The \em rect is expected to be in absolute page coordinates,
		 * that is, from 0 to page width and page height correspondingly
		 * as returned by IDocument::GetPageSize().
		 *
		 * If \em rect is empty or null, the text from the whole page
		 * should be returned.
		 *
		 * @param[in] page The index of the page to query.
		 * @param[in] rect The rectangle on the \em page to query.
		 * @return The text in \em rect at \em page.
		 */
		virtual QString GetTextContent (int page, const QRect& rect) = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Monocle::IHaveTextContent,
		"org.LeechCraft.Monocle.IHaveTextContent/1.0")
