/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "path.h"
#include <QtDebug>
#include <gst/gst.h>
#include "sourceobject.h"
#include "output.h"

namespace LeechCraft
{
namespace LMP
{
	Path::Path (SourceObject *source, Output *output, QObject *parent)
	: QObject (parent)
	, SrcObj_ (source)
	, WholeBin_ (gst_bin_new ("whole_bin"))
	, Identity_ (gst_element_factory_make ("identity", "effect_placeholder"))
	, Pipeline_ (nullptr)
	, OutputBin_ (nullptr)
	{
		NextWholeElems_ << Identity_;

		source->AddToPath (this);
		output->AddToPathExposed (this);

		gst_bin_add_many (GST_BIN (WholeBin_), Identity_, OutputBin_, nullptr);
		gst_element_link (Identity_, OutputBin_);

		auto pad = gst_element_get_static_pad (Identity_, "sink");
		auto ghostPad = gst_ghost_pad_new ("sink", pad);
		gst_pad_set_active (ghostPad, TRUE);
		gst_element_add_pad (WholeBin_, ghostPad);
		gst_object_unref (pad);

		source->SetSink (WholeBin_);
		output->PostAddExposed (this);
	}

	Path::~Path ()
	{
	}

	GstElement* Path::GetPipeline () const
	{
		return Pipeline_;
	}

	void Path::SetPipeline (GstElement *pipeline)
	{
		Pipeline_ = pipeline;
	}

	GstElement* Path::GetOutPlaceholder () const
	{
		return Identity_;
	}

	GstElement* Path::GetWholeOut () const
	{
		return WholeBin_;
	}

	GstElement* Path::GetOutputBin () const
	{
		return OutputBin_;
	}

	void Path::SetOutputBin (GstElement *bin)
	{
		if (OutputBin_)
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot change output bin now";
		}

		OutputBin_ = bin;
		NextWholeElems_ << OutputBin_;
	}

	SourceObject* Path::GetSourceObject () const
	{
		return SrcObj_;
	}

	namespace
	{
		struct CallbackData
		{
			Path * const Path_;
			GstElement * const Elem_;
			QList<GstElement*>& NextElems_;
			guint ID_;

			enum class Action
			{
				Add,
				Remove
			} const Action_;
		};

		gboolean EventProbeHandler (GstPad *pad, GstEvent *event, CallbackData *cbData)
		{
			if (GST_EVENT_TYPE (event) != GST_EVENT_EOS)
				return TRUE;

			qDebug () << Q_FUNC_INFO << "eos";
			gst_pad_remove_event_probe (pad, cbData->ID_);

			const auto path = cbData->Path_;
			const auto elem = cbData->Elem_;

			auto& nextElems = cbData->NextElems_;

			auto deleteGuard = std::shared_ptr<CallbackData> (cbData);

			switch (cbData->Action_)
			{
			case CallbackData::Action::Add:
			{
				gst_bin_add (GST_BIN (path->GetWholeOut ()), elem);

				qDebug () << "unlinking...";
				gst_element_unlink (nextElems.at (0), nextElems.at (1));

				qDebug () << "linking...";
				gst_element_link_many (nextElems.at (0), elem, nextElems.at (1), nullptr);

				GstState wholeCurrent, elemCurrent;
				gst_element_get_state (path->GetWholeOut (), &wholeCurrent, nullptr, GST_SECOND);
				gst_element_set_state (elem, wholeCurrent);

				nextElems.insert (1, elem);

				break;
			}
			case CallbackData::Action::Remove:
			{
				const auto idx = nextElems.indexOf (elem);
				if (idx == -1)
				{
					qWarning () << Q_FUNC_INFO
							<< "element not found";
					return FALSE;
				}
				if (!idx || idx == nextElems.size () - 1)
				{
					qWarning () << Q_FUNC_INFO
							<< "cannot remove side element";
					return FALSE;
				}

				const auto prev = nextElems.at (idx - 1);
				const auto next = nextElems.at (idx + 1);

				gst_element_unlink_many (prev, elem, next, nullptr);
				gst_element_link (prev, next);

				gst_element_set_state (elem, GST_STATE_NULL);

				gst_bin_remove (GST_BIN (path->GetWholeOut ()), elem);

				nextElems.removeAt (idx);

				break;
			}
			}

			return FALSE;
		}

		gboolean ProbeHandler (GstPad *pad, GstMiniObject*, CallbackData *cbData)
		{
			qDebug () << Q_FUNC_INFO;
			gst_pad_remove_data_probe (pad, cbData->ID_);

			cbData->ID_ = gst_pad_add_event_probe (pad, G_CALLBACK (EventProbeHandler), cbData);

			const auto sinkpad = gst_element_get_static_pad (cbData->Path_->GetOutPlaceholder (), "sink");
			gst_pad_send_event (sinkpad, gst_event_new_eos ());
			gst_object_unref (sinkpad);

			return TRUE;
		}
	}

	void Path::InsertElement (GstElement *elem)
	{
		auto srcpad = gst_element_get_static_pad (GetOutPlaceholder (), "src");
		qDebug () << Q_FUNC_INFO << elem << srcpad;
		auto data = new CallbackData { this, elem, NextWholeElems_, 0, CallbackData::Action::Add };
		data->ID_ = gst_pad_add_data_probe (srcpad, G_CALLBACK (ProbeHandler), data);
	}

	void Path::RemoveElement (GstElement *elem)
	{
		auto srcpad = gst_element_get_static_pad (GetOutPlaceholder (), "src");
		qDebug () << Q_FUNC_INFO << elem << srcpad;
		auto data = new CallbackData { this, elem, NextWholeElems_, 0, CallbackData::Action::Remove };
		data->ID_ = gst_pad_add_data_probe (srcpad, G_CALLBACK (ProbeHandler), data);
	}
}
}
