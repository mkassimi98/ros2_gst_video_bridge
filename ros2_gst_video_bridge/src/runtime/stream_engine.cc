// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/runtime/stream_engine.hpp"

#include <gst/app/gstappsrc.h>

#include <chrono>
#include <cstring>
#include <sstream>

namespace ros2_gst_video_bridge {

StreamEngine::StreamEngine(std::string pipeline) : pipeline_(std::move(pipeline)) {}

StreamEngine::~StreamEngine() {
  stop();
}

bool StreamEngine::start() {
  std::lock_guard<std::mutex> lock(mutex_);
  last_error_.clear();

  if (running_.load()) {
    return true;
  }

  if (!gst_initialized_) {
    gst_init(nullptr, nullptr);
    gst_initialized_ = true;
  }

  GError* parse_error = nullptr;
  pipeline_element_ = gst_parse_launch(pipeline_.c_str(), &parse_error);
  if (parse_error != nullptr) {
    std::ostringstream ss;
    ss << "gst_parse_launch failed: " << parse_error->message;
    last_error_ = ss.str();
    g_error_free(parse_error);
    return false;
  }

  if (pipeline_element_ == nullptr) {
    last_error_ = "gst_parse_launch returned a null pipeline.";
    return false;
  }

  appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_element_), "bridge_appsrc");
  if (appsrc_ == nullptr) {
    last_error_ = "Could not find appsrc element named bridge_appsrc in pipeline.";
    gst_object_unref(pipeline_element_);
    pipeline_element_ = nullptr;
    return false;
  }

  bus_ = gst_element_get_bus(pipeline_element_);
  gst_app_src_set_stream_type(GST_APP_SRC(appsrc_), GST_APP_STREAM_TYPE_STREAM);
  g_object_set(G_OBJECT(appsrc_), "is-live", TRUE, "format", GST_FORMAT_TIME, nullptr);

  const GstStateChangeReturn state_ret =
      gst_element_set_state(pipeline_element_, GST_STATE_PLAYING);
  if (state_ret == GST_STATE_CHANGE_FAILURE) {
    last_error_ = "Failed to transition GStreamer pipeline to PLAYING state.";
    if (bus_ != nullptr) {
      gst_object_unref(bus_);
      bus_ = nullptr;
    }
    gst_object_unref(appsrc_);
    appsrc_ = nullptr;
    gst_object_unref(pipeline_element_);
    pipeline_element_ = nullptr;
    running_.store(false);
    return false;
  }

  caps_width_ = 0;
  caps_height_ = 0;
  caps_format_.clear();
  running_.store(true);
  bus_thread_ = std::jthread([this](std::stop_token stop_token) { busLoop(stop_token); });
  processBusMessages(0);
  return true;
}

void StreamEngine::stop() {
  running_.store(false);

  if (bus_thread_.joinable()) {
    bus_thread_.request_stop();
    bus_thread_.join();
  }

  std::lock_guard<std::mutex> lock(mutex_);

  if (pipeline_element_ != nullptr) {
    gst_element_set_state(pipeline_element_, GST_STATE_NULL);
  }

  if (bus_ != nullptr) {
    gst_object_unref(bus_);
    bus_ = nullptr;
  }

  if (appsrc_ != nullptr) {
    gst_object_unref(appsrc_);
    appsrc_ = nullptr;
  }

  if (pipeline_element_ != nullptr) {
    gst_object_unref(pipeline_element_);
    pipeline_element_ = nullptr;
  }
}

bool StreamEngine::isRunning() const {
  return running_.load();
}

const std::string& StreamEngine::pipeline() const {
  return pipeline_;
}

bool StreamEngine::pushFrame(const uint8_t* data, size_t size, int width, int height,
                             const std::string& gst_format, uint64_t timestamp_ns) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!running_.load() || appsrc_ == nullptr || pipeline_element_ == nullptr || data == nullptr ||
      size == 0) {
    return false;
  }

  if (!setAppSrcCaps(width, height, gst_format)) {
    return false;
  }

  GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
  if (buffer == nullptr) {
    last_error_ = "Failed to allocate GstBuffer.";
    return false;
  }

  GstMapInfo map_info;
  if (!gst_buffer_map(buffer, &map_info, GST_MAP_WRITE)) {
    last_error_ = "Failed to map GstBuffer for writing.";
    gst_buffer_unref(buffer);
    return false;
  }

  std::memcpy(map_info.data, data, size);
  gst_buffer_unmap(buffer, &map_info);

  GST_BUFFER_PTS(buffer) = static_cast<GstClockTime>(timestamp_ns);
  GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;

  const GstFlowReturn flow_ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc_), buffer);
  processBusMessages(0);
  if (flow_ret != GST_FLOW_OK) {
    std::ostringstream ss;
    ss << "appsrc push failed with flow status " << flow_ret;
    last_error_ = ss.str();
    return false;
  }

  return true;
}

std::string StreamEngine::lastError() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_error_;
}

bool StreamEngine::setAppSrcCaps(int width, int height, const std::string& gst_format) {
  if (appsrc_ == nullptr) {
    last_error_ = "Cannot set appsrc caps because appsrc is null.";
    return false;
  }

  if (width == caps_width_ && height == caps_height_ && gst_format == caps_format_) {
    return true;
  }

  GstCaps* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, gst_format.c_str(),
                                      "width", G_TYPE_INT, width, "height", G_TYPE_INT, height,
                                      "framerate", GST_TYPE_FRACTION, 0, 1, nullptr);

  if (caps == nullptr) {
    last_error_ = "Failed to create GstCaps for appsrc.";
    return false;
  }

  gst_app_src_set_caps(GST_APP_SRC(appsrc_), caps);
  gst_caps_unref(caps);

  caps_width_ = width;
  caps_height_ = height;
  caps_format_ = gst_format;
  return true;
}

void StreamEngine::processBusMessages(GstClockTime timeout) {
  if (bus_ == nullptr) {
    return;
  }

  const auto filter =
      static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING | GST_MESSAGE_EOS);
  GstMessage* message = timeout == 0 ? gst_bus_pop_filtered(bus_, filter)
                                     : gst_bus_timed_pop_filtered(bus_, timeout, filter);

  while (true) {
    if (message == nullptr) {
      break;
    }

    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
      GError* error = nullptr;
      gchar* debug_info = nullptr;
      gst_message_parse_error(message, &error, &debug_info);
      if (error != nullptr) {
        last_error_ = error->message;
        g_error_free(error);
      }
      if (debug_info != nullptr) {
        g_free(debug_info);
      }
    } else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
      last_error_ = "GStreamer pipeline reached EOS.";
    }

    gst_message_unref(message);
    message = gst_bus_pop_filtered(bus_, filter);
  }
}

void StreamEngine::busLoop(std::stop_token stop_token) {
  using namespace std::chrono_literals;

  while (!stop_token.stop_requested()) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (running_.load() && bus_ != nullptr) {
        processBusMessages(100 * GST_MSECOND);
      }
    }
    std::this_thread::sleep_for(10ms);
  }
}

} // namespace ros2_gst_video_bridge
