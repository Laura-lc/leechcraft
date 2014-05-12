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

#include "visualfilter.h"
#include <QtDebug>
#include <QWidget>
#include <gst/gst.h>

#if GST_CHECK_VERSION (1, 0, 0)
#include <gst/video/videooverlay.h>
#else
#include <gst/interfaces/xoverlay.h>
#endif

#include <util/lmp/gstutil.h>
#include <interfaces/lmp/ilmpguiproxy.h>
#include "viswidget.h"

namespace LeechCraft
{
namespace LMP
{
namespace Potorchu
{
	namespace
	{
		void EnumerateVisualizers ()
		{
#if GST_CHECK_VERSION (1, 0, 0)
			const auto registry = gst_registry_get ();
#else
			const auto registry = gst_registry_get_default ();
#endif
			const auto list = gst_registry_feature_filter (registry,
					[] (GstPluginFeature *feature, gpointer) -> gboolean
					{
						if (!GST_IS_ELEMENT_FACTORY (feature))
							return false;

						const auto factory = GST_ELEMENT_FACTORY (feature);
						const auto klass = gst_element_factory_get_klass (factory);
						return g_strrstr (klass, "Visualization") != nullptr;
					},
					false,
					nullptr);

			for (auto item = list; item; item = g_list_next (item))
			{
				const auto factory = GST_ELEMENT_FACTORY (item->data);
				qDebug () << gst_element_factory_get_longname (factory);
			}

			gst_plugin_feature_list_free (list);
		}
	}

	VisBranch::VisBranch (GstElement *elem, GstElement *tee, GstPadTemplate *teeTemplate)
	: Elem_ { elem }
	, Tee_ { tee }
	, TeeTemplate_ { teeTemplate }
	, VisQueue_ { gst_element_factory_make ("queue", nullptr) }
	, VisConverter_ { gst_element_factory_make ("audioconvert", nullptr) }
	, Visualizer_ { gst_element_factory_make ("synaescope", nullptr) }
	, VisColorspace_ { gst_element_factory_make ("colorspace", nullptr) }
	, XSink_ { gst_element_factory_make ("xvimagesink", nullptr) }
	{
		gst_bin_add_many (GST_BIN (Elem_),
				VisQueue_, VisConverter_, Visualizer_, VisColorspace_, XSink_, nullptr);
		gst_element_link_many (VisQueue_, VisConverter_, Visualizer_, VisColorspace_, XSink_, nullptr);

		auto teeVisPad = gst_element_request_pad (Tee_, TeeTemplate_, nullptr, nullptr);
		auto streamPad = gst_element_get_static_pad (VisQueue_, "sink");
		gst_pad_link (teeVisPad, streamPad);
		gst_object_unref (streamPad);
	}

	GstElement* VisBranch::GetXSink () const
	{
		return XSink_;
	}

	VisualFilter::VisualFilter (const QByteArray& effectId, const ILMPProxy_ptr& proxy)
	: EffectId_ { effectId }
	, LmpProxy_ { proxy }
	, Widget_ { new VisWidget }
	, Elem_ { gst_bin_new ("visualbin") }
	, Tee_ { gst_element_factory_make ("tee", nullptr) }
	, TeeTemplate_ { gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (Tee_), "src%d") }
	, AudioQueue_ { gst_element_factory_make ("queue", nullptr) }
	{
		gst_bin_add_many (GST_BIN (Elem_), Tee_, AudioQueue_, nullptr);

		TeeAudioPad_ = gst_element_request_pad (Tee_, TeeTemplate_, nullptr, nullptr);
		auto audioPad = gst_element_get_static_pad (AudioQueue_, "sink");
		gst_pad_link (TeeAudioPad_, audioPad);
		gst_object_unref (audioPad);

		Widget_->resize (800, 600);
		Widget_->show ();
		//proxy->GetGuiProxy ()->AddCurrentSongTab (tr ("Visualization"), Widget_.get ());

		GstUtil::AddGhostPad (Tee_, Elem_, "sink");
		GstUtil::AddGhostPad (AudioQueue_, Elem_, "src");

		VisBranch_.reset (new VisBranch { Elem_, Tee_, TeeTemplate_ });
	}

	QByteArray VisualFilter::GetEffectId () const
	{
		return EffectId_;
	}

	QByteArray VisualFilter::GetInstanceId () const
	{
		return EffectId_;
	}

	IFilterConfigurator* VisualFilter::GetConfigurator () const
	{
		return nullptr;
	}

	GstElement* VisualFilter::GetElement () const
	{
		return Elem_;
	}

	void VisualFilter::PostAdd (IPath *path)
	{
		path->AddSyncHandler ([this] (GstBus*, GstMessage *message)
				{
#if GST_CHECK_VERSION (1, 0, 0)
					if (!gst_is_video_overlay_prepare_window_handle_message (message))
						return GST_BUS_PASS;
#else
					if (GST_MESSAGE_TYPE (message) != GST_MESSAGE_ELEMENT)
						return GST_BUS_PASS;

					if (!gst_structure_has_name (message->structure, "prepare-xwindow-id"))
						return GST_BUS_PASS;
#endif

					SetOverlay ();

					gst_message_unref (message);

					return GST_BUS_DROP;
				},
				this);
	}

	void VisualFilter::SetOverlay ()
	{
		auto sink = VisBranch_->GetXSink ();

#if GST_CHECK_VERSION (1, 0, 0)
		gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (sink), Widget_->GetVisWinId ());
#else
		gst_x_overlay_set_window_handle (GST_X_OVERLAY (sink), Widget_->GetVisWinId ());
#endif
	}

	void VisualFilter::handleNextVis ()
	{
		/*
		auto streamPad = gst_element_get_static_pad (VisQueue_, "sink");

		gst_pad_unlink (TeeVisPad_, streamPad);
		gst_element_release_request_pad (Tee_, TeeVisPad_);
		gst_element_unlink (VisConverter_, Visualizer_);
		gst_element_unlink (Visualizer_, VisColorspace_);

		gst_element_set_state (Visualizer_, GST_STATE_NULL);

		gst_bin_remove (GST_BIN (Elem_), Visualizer_);

		Visualizer_ = gst_element_factory_make ("synaescope", nullptr);

		gst_bin_add (GST_BIN (Elem_), Visualizer_);

		gst_element_link (VisConverter_, Visualizer_);
		gst_element_link (Visualizer_, VisColorspace_);
		TeeVisPad_ = gst_element_request_pad (Tee_, TeeTemplate_, nullptr, nullptr);
		gst_pad_link (TeeVisPad_, streamPad);

		gst_element_sync_state_with_parent (Visualizer_);

		gst_object_unref (streamPad);

		SetOverlay ();
		*/
	}
}
}
}
