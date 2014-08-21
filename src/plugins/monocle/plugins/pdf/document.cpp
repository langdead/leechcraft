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

#include "document.h"
#include <thread>
#include <QThread>
#include <QtDebug>
#include <QBuffer>
#include <QFile>

#if QT_VERSION < 0x050000
#include <poppler-qt4.h>
#else
#include <poppler-qt5.h>
#endif

#include <poppler-form.h>
#include <poppler-version.h>
#include "links.h"
#include "fields.h"
#include "annotations.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Monocle
{
namespace PDF
{
	Document::Document (const QString& path, QObject *plugin)
	: PDocument_ (Poppler::Document::load (path))
	, DocURL_ (QUrl::fromLocalFile (path))
	, Plugin_ (plugin)
	{
		if (!PDocument_)
			return;

		auto setRenderHint = [this] (const QByteArray& optName, Poppler::Document::RenderHint hint)
		{
			PDocument_->setRenderHint (hint,
					XmlSettingsManager::Instance ().property (optName).toBool ());
		};
		setRenderHint ("EnableAntialiasing", Poppler::Document::Antialiasing);
		setRenderHint ("EnableTextAntialiasing", Poppler::Document::TextAntialiasing);
		setRenderHint ("EnableTextHinting", Poppler::Document::TextHinting);
		setRenderHint ("EnableTextSlightHinting", Poppler::Document::TextSlightHinting);

		const auto& enhanceMode = XmlSettingsManager::Instance ()
				.property ("ThinLineEnhancement").toString ();
		if (enhanceMode == "Solid")
			PDocument_->setRenderHint (Poppler::Document::ThinLineSolid);
		else if (enhanceMode == "Shape")
			PDocument_->setRenderHint (Poppler::Document::ThinLineShape);

		BuildTOC ();
	}

	QObject* Document::GetBackendPlugin () const
	{
		return Plugin_;
	}

	QObject* Document::GetQObject ()
	{
		return this;
	}

	bool Document::IsValid () const
	{
		return PDocument_ && !PDocument_->isLocked ();
	}

	DocumentInfo Document::GetDocumentInfo () const
	{
		DocumentInfo info;
		if (!PDocument_)
			return info;

		info.Title_ = PDocument_->info ("Title");
		info.Subject_ = PDocument_->info ("Subject");
		info.Author_ = PDocument_->info ("Author");
		return info;
	}

	int Document::GetNumPages () const
	{
		return PDocument_->numPages ();
	}

	QSize Document::GetPageSize (int num) const
	{
		std::unique_ptr<Poppler::Page> page (PDocument_->page (num));
		if (!page)
			return QSize ();

		return page->pageSize ();
	}

	QImage Document::RenderPage (int num, double xScale, double yScale)
	{
		std::unique_ptr<Poppler::Page> page (PDocument_->page (num));
		if (!page)
			return QImage ();

		return page->renderToImage (72 * xScale, 72 * yScale);
	}

	QList<ILink_ptr> Document::GetPageLinks (int num)
	{
		QList<ILink_ptr> result;
		std::unique_ptr<Poppler::Page> page (PDocument_->page (num));
		if (!page)
			return result;

		Q_FOREACH (auto link, page->links ())
			result << ILink_ptr (new Link (this, link));

		return result;
	}

	QUrl Document::GetDocURL () const
	{
		return DocURL_;
	}

	TOCEntryLevel_t Document::GetTOC ()
	{
		return TOC_;
	}

	QString Document::GetTextContent (int pageNum, const QRect& rect)
	{
		std::unique_ptr<Poppler::Page> page (PDocument_->page (pageNum));
		if (!page)
			return QString ();

		return page->text (rect);
	}

	QList<IAnnotation_ptr> Document::GetAnnotations (int pageNum)
	{
		std::unique_ptr<Poppler::Page> page (PDocument_->page (pageNum));
		if (!page)
			return {};

		QList<IAnnotation_ptr> annotations;
		for (const auto ann : page->annotations ())
			if (const auto wrapper = MakeAnnotation (this, ann))
				annotations << wrapper;
		return annotations;
	}

	IFormFields_t Document::GetFormFields (int pageNum)
	{
		std::unique_ptr<Poppler::Page> page (PDocument_->page (pageNum));
		if (!page)
			return IFormFields_t ();

		QList<std::shared_ptr<Poppler::FormField>> popplerFields;
		for (auto field : page->formFields ())
			popplerFields << std::shared_ptr<Poppler::FormField> (field);

		IFormFields_t fields;
		for (auto field : popplerFields)
		{
			if (!field->isVisible ())
				continue;

			switch (field->type ())
			{
			case Poppler::FormField::FormText:
				fields << IFormField_ptr (new FormFieldText (field));
				break;
			case Poppler::FormField::FormChoice:
				fields << IFormField_ptr (new FormFieldChoice (field));
				break;
			case Poppler::FormField::FormButton:
				fields << IFormField_ptr (new FormFieldButton (field, this));
				break;
			default:
				break;
			}
		}
		return fields;
	}

	QMap<int, QList<QRectF>> Document::GetTextPositions (const QString& text, Qt::CaseSensitivity cs)
	{
		typedef QMap<int, QList<QRectF>> Result_t;
		Result_t result;
#if POPPLER_VERSION_MAJOR > 0 || POPPLER_VERSION_MINOR >= 22
		const auto popplerCS = cs == Qt::CaseSensitive ?
						Poppler::Page::CaseSensitive :
						Poppler::Page::CaseInsensitive;

		const auto numPages = PDocument_->numPages ();

		QVector<QList<QRectF>> resVec;
		resVec.resize (numPages);

		std::vector<std::thread> threads;

		auto worker = [&resVec, &popplerCS, &text, this] (int start, int count)
		{
			std::unique_ptr<Poppler::Document> doc (Poppler::Document::load (DocURL_.toLocalFile ()));
			for (auto i = start, end = start + count; i < end; ++i)
			{
				std::unique_ptr<Poppler::Page> p (doc->page (i));
				resVec [i] = p->search (text, popplerCS);
			}
		};
		const auto threadCount = QThread::idealThreadCount ();
		const auto packSize = numPages / threadCount;
		for (int i = 0; i < threadCount; ++i)
			threads.emplace_back (worker, i * packSize, (i == threadCount - 1) ? (numPages % threadCount) : packSize);

		for (auto& thread : threads)
			thread.join ();

		for (int i = 0; i < numPages; ++i)
			if (!resVec.at (i).isEmpty ())
				result [i] = resVec.at (i);
#endif
		return result;
	}

	auto Document::CanSave () const -> SaveQueryResult
	{
		if (PDocument_->isEncrypted ())
			return { false, tr ("saving encrypted documents is not supported") };

		return { true, QString () };
	}

	bool Document::Save (const QString& path)
	{
		std::unique_ptr<Poppler::PDFConverter> conv (PDocument_->pdfConverter ());
		conv->setPDFOptions (Poppler::PDFConverter::WithChanges);

		if (path == DocURL_.toLocalFile ())
		{
			QBuffer buffer;
			buffer.open (QIODevice::WriteOnly);
			conv->setOutputDevice (&buffer);
			if (!conv->convert ())
				return false;

			QFile file (path);
			if (!file.open (QIODevice::WriteOnly))
				return false;

			file.write (buffer.buffer ());
			return true;
		}
		else
		{
			conv->setOutputFileName (path);
			return conv->convert ();
		}
	}

	void Document::RequestNavigation (const QString& filename,
			int page, double x, double y)
	{
		emit navigateRequested (filename, page, x, y);
	}

	void Document::RequestPrinting ()
	{
		emit printRequested (QList<int> ());
	}

	namespace
	{
		template<typename T>
		TOCEntryLevel_t BuildTOCLevel (Document *doc, PDocument_ptr pDoc, const T& levelRoot)
		{
			TOCEntryLevel_t result;

			QDomElement elem = levelRoot.firstChildElement ();
			auto nextGuard = [&elem] (void*) { elem = elem.nextSiblingElement (); };
			while (!elem.isNull ())
			{
				std::shared_ptr<void> guard (static_cast<void*> (0), nextGuard);

				const auto& name = elem.tagName ();

				ILink_ptr link;
				const auto& destStr = elem.attribute ("Destination");
				if (!destStr.isEmpty ())
					link.reset (new TOCLink (doc, new Poppler::LinkDestination (destStr)));
				else
				{
					const auto& destName = elem.attribute ("DestinationName");
					if (destName.isEmpty ())
					{
						qWarning () << Q_FUNC_INFO
								<< "empty destination name, dunno how to handle that";
						continue;
					}

					const auto dest = pDoc->linkDestination (destName);
					if (!dest)
					{
						qWarning () << Q_FUNC_INFO
								<< "empty destination for"
								<< destName;
						continue;
					}
					link.reset (new TOCLink (doc, dest));
				}

				TOCEntry entry =
				{
					link,
					name,
					BuildTOCLevel (doc, pDoc, elem)
				};
				result << entry;
			}

			return result;
		}
	}

	void Document::BuildTOC ()
	{
		std::unique_ptr<QDomDocument> doc (PDocument_->toc ());
		if (!doc)
			return;
		TOC_ = BuildTOCLevel (this, PDocument_, *doc);
	}
}
}
}
