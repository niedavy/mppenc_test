

#include <iostream>
#include <ostream>
#include <string>
#include <thread>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

#include <gst/gst.h>

using namespace std::chrono_literals;

static bool     exiting = false;
static gboolean bus_msg(GstBus* bus, GstMessage* message, gpointer data)
{
	(void)bus;
	(void)data;
	auto* main_loop = static_cast<GMainLoop*>(data);

	switch (GST_MESSAGE_TYPE(message)) {
		case GST_MESSAGE_ERROR: {
			GError* err;
			gchar*  debug;

			gst_message_parse_error(message, &err, &debug);
			std::cerr << "Error: " << err->message << std::endl;
			exiting = true;
			g_error_free(err);
			g_free(debug);
			g_main_loop_quit(main_loop);

			break;
		}
		case GST_MESSAGE_EOS:
			/* end-of-stream */
			std::cerr << "eos" << std::endl;
			exiting = true;
			g_main_loop_quit(main_loop);

			break;
		default:
			/* unhandled message */
			break;
	}

	/* we want to be notified again the next time there is a message
     * on the bus, so returning TRUE (FALSE means we want to stop watching
     * for messages on the bus and our callback should not be called again)
     */
	return TRUE;
}

static void hand_off_player(GstElement* fakesink, GstBuffer* buf, GstPad* new_pad, gpointer user_data)
{
	(void)fakesink;
	(void)new_pad;
	(void)user_data;

	if (buf == nullptr) {
		return;
	}

	std::cerr << "recv a buffer" << std::endl;
}

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;


	GMainLoop* main_loop = nullptr;

	main_loop = g_main_loop_new(nullptr, FALSE);

	/* Initialize GStreamer */
	gst_init(NULL, NULL);

	std::vector<std::string> elements{"mppvideodec", "mpph264enc"};
	for (auto const& element : elements) {
		GstElementFactory* factory = gst_element_factory_find(element.c_str());
		gst_object_unref(GST_OBJECT(factory));
		if (factory == nullptr) {
			std::cerr << element << " not fount" << std::endl;
			return 0;
		}
	}


	std::string uri = "http://vfx.mtime.cn/Video/2021/07/10/mp4/210710122716702150.mp4";
	std::thread thread([&]() {
		GstBus*     bus = nullptr;
		GstElement* pipeline = nullptr;
		GstElement* sink_player = nullptr;
		while (not exiting) {
			std::string pipeline_description = fmt::format(
			    "urisourcebin  uri={} ! parsebin name=parsebin ! mppvideodec  !"
			    "mpph264enc bps-max=500000 max-reenc=1 gop=50  ! h264parse ! video/x-h264, "
			    "alignment=(string)au ! fakesink name=fakesink "
			    "signal-handoffs=true sync=true",
			    uri);

			pipeline = gst_parse_launch(pipeline_description.c_str(), nullptr);
			bus = gst_element_get_bus(pipeline);
			sink_player = gst_bin_get_by_name(GST_BIN(pipeline), "fakesink");

			g_signal_connect(G_OBJECT(sink_player), "handoff", G_CALLBACK(hand_off_player), NULL);
			gst_bus_add_watch(bus, bus_msg, main_loop);

			/* Start playing */
			gst_element_set_state(pipeline, GST_STATE_PLAYING);
			gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);


			if (FALSE == gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME,
			                              GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), GST_SEEK_TYPE_SET,
			                              10 * GST_NSECOND, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
				std::cerr << "failed to seek" << std::endl;
			}

			std::this_thread::sleep_for(2s);
			
			if (bus != nullptr) {
				gst_bus_remove_watch(bus);
				gst_object_unref(bus);
				bus = nullptr;
			}

			if (pipeline != nullptr) {
				gst_element_set_state(pipeline, GST_STATE_PAUSED);
				gst_element_get_state(pipeline, nullptr, nullptr, GST_SECOND);
				gst_element_set_state(pipeline, GST_STATE_READY);
				gst_element_get_state(pipeline, nullptr, nullptr, GST_SECOND);
				gst_element_set_state(pipeline, GST_STATE_NULL);
				gst_element_get_state(pipeline, nullptr, nullptr, GST_SECOND);
				gst_object_unref(pipeline);
				pipeline = nullptr;
			}
			if (sink_player != nullptr) {
				gst_object_unref(sink_player);
				sink_player = nullptr;
			}
		}
	});

	g_main_loop_run(main_loop);
	g_main_loop_unref(main_loop);
	gst_deinit();
	thread.join();
	return 0;
}