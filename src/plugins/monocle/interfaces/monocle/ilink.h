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

#include <memory>
#include <QRectF>
#include <QMetaType>

namespace LeechCraft
{
namespace Monocle
{
	/** @brief Describes various link types known to Monocle.
	 *
	 * All links implement the ILink interface.
	 *
	 * @sa ILink
	 */
	enum class LinkType
	{
		/** @brief A link to a page.
		 *
		 * The link may refer both the document it belongs to as well as
		 * some other document.
		 *
		 * Links of this type should implement IPageLink as well.
		 *
		 * @sa IPageLink
		 */
		PageLink,

		/** @brief A link to an URL.
		 */
		URL,

		/** @brief Some standard command like printing.
		 */
		Command,

		/** @brief Other link type.
		 */
		OtherLink
	};

	/** @brief Base interface for links.
	 *
	 * Links should implement this interface.
	 *
	 * @sa IPageLink
	 */
	class ILink
	{
	public:
		/** @brief Virtual destructor.
		 */
		virtual ~ILink () {}

		/** @brief Returns the link type.
		 *
		 * @return The type of this link.
		 */
		virtual LinkType GetLinkType () const = 0;

		/** @brief Returns the area on the page of this link.
		 *
		 * The returned rectangle is in page's coordinates, that is, with
		 * width from 0 to page width and height from 0 to page height.
		 *
		 * If the link doesn't belong to a page (i. e. is a TOC link) the
		 * return value isn't used and may be arbitrary.
		 *
		 * @return The area of this link on its page.
		 */
		virtual QRectF GetArea () const = 0;

		/** @brief Executes the link.
		 *
		 * This method is called to indicate that the user has chosen to
		 * execute the action related to the link.
		 */
		virtual void Execute () = 0;
	};
	typedef std::shared_ptr<ILink> ILink_ptr;

	/** @brief Additional interface for page links.
	 *
	 * Links of type LinkType::PageLink should implement this interface
	 * in addition to ILink.
	 *
	 * @sa ILink
	 */
	class IPageLink
	{
	public:
		/** @brief Virtual destructor.
		 */
		virtual ~IPageLink () {}

		/** @brief The name of the document to open.
		 *
		 * If the link is relative to the current document, this method
		 * returns an empty string.
		 *
		 * @return The name of the document to open, or empty string for
		 * current document.
		 */
		virtual QString GetDocumentFilename () const = 0;

		/** @brief Returns the index of the page this link refers to.
		 *
		 * @return The index of the page this link refers to.
		 */
		virtual int GetPageNumber () const = 0;

		/** @brief Returns the new \em x coordinate of the viewport.
		 *
		 * The returned value is in page's coordinates, that is, between
		 * 0 and the width of the page it belongs to.
		 *
		 * @return The new \em x coordinate of the viewport in page.
		 */
		virtual double NewX () const = 0;

		/** @brief Returns the new \em x coordinate of the viewport.
		 *
		 * The returned value is in page's coordinates, that is, between
		 * 0 and the width of the page it belongs to.
		 *
		 * @return The new \em x coordinate of the viewport in page.
		 */
		virtual double NewY () const = 0;

		/** @brief Returns the new zoom value for the page.
		 *
		 * @return The new zoom value.
		 */
		virtual double NewZoom () const = 0;
	};
}
}

Q_DECLARE_INTERFACE (LeechCraft::Monocle::ILink, "org.LeechCraft.Monocle.ILink/1.0")
Q_DECLARE_INTERFACE (LeechCraft::Monocle::IPageLink, "org.LeechCraft.Monocle.IPageLink/1.0")
