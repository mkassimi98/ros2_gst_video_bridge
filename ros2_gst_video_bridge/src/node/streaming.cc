// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"

#include <sensor_msgs/image_encodings.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

namespace ros2_gst_video_bridge {

namespace {

consteval int64_t nanosecondsPerSecond() {
  return 1000000000LL;
}

bool resolveGstFormat(const std::string& encoding, std::string& gst_format) {
  if (encoding == sensor_msgs::image_encodings::BGR8 ||
      encoding == sensor_msgs::image_encodings::TYPE_8UC3 ||
      encoding == sensor_msgs::image_encodings::TYPE_8SC3) {
    gst_format = "BGR";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::RGB8) {
    gst_format = "RGB";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::BGRA8 ||
      encoding == sensor_msgs::image_encodings::TYPE_8UC4 ||
      encoding == sensor_msgs::image_encodings::TYPE_8SC4) {
    gst_format = "BGRA";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::RGBA8) {
    gst_format = "RGBA";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::MONO8 ||
      encoding == sensor_msgs::image_encodings::TYPE_8UC1 ||
      encoding == sensor_msgs::image_encodings::TYPE_8SC1) {
    gst_format = "GRAY8";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::MONO16 ||
      encoding == sensor_msgs::image_encodings::TYPE_16UC1 ||
      encoding == sensor_msgs::image_encodings::TYPE_16SC1) {
    gst_format = "GRAY16_LE";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::BAYER_RGGB8 ||
      encoding == sensor_msgs::image_encodings::BAYER_BGGR8 ||
      encoding == sensor_msgs::image_encodings::BAYER_GBRG8 ||
      encoding == sensor_msgs::image_encodings::BAYER_GRBG8) {
    gst_format = "GRAY8";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::BAYER_RGGB16 ||
      encoding == sensor_msgs::image_encodings::BAYER_BGGR16 ||
      encoding == sensor_msgs::image_encodings::BAYER_GBRG16 ||
      encoding == sensor_msgs::image_encodings::BAYER_GRBG16) {
    gst_format = "GRAY16_LE";
    return true;
  }

  return false;
}

bool isBayerEncoding(const std::string& encoding) {
  return encoding == sensor_msgs::image_encodings::BAYER_RGGB8 ||
         encoding == sensor_msgs::image_encodings::BAYER_BGGR8 ||
         encoding == sensor_msgs::image_encodings::BAYER_GBRG8 ||
         encoding == sensor_msgs::image_encodings::BAYER_GRBG8 ||
         encoding == sensor_msgs::image_encodings::BAYER_RGGB16 ||
         encoding == sensor_msgs::image_encodings::BAYER_BGGR16 ||
         encoding == sensor_msgs::image_encodings::BAYER_GBRG16 ||
         encoding == sensor_msgs::image_encodings::BAYER_GRBG16;
}

} // namespace

void GstVideoBridgeNode::initializeModules() {
  topic_introspector_ = std::make_unique<TopicIntrospector>(*this);
  capability_probe_ = std::make_unique<CapabilityProbe>();
  stream_engine_ = std::make_unique<StreamEngine>(effective_pipeline_);
  metrics_publisher_ = std::make_unique<MetricsPublisher>(*this);

  metrics_publisher_->setSessionMetadata(session_id_, stream_id_, config_.source.input_topic,
                                         config_.codec.name, config_.codec.encoder,
                                         config_.transport.kind, config_.transport.sink_uri);
  metrics_publisher_->setCodecSelectionFlags(auto_codec_requested_, hw_encoder_selected_,
                                             sw_fallback_applied_);
  metrics_publisher_->setEffectiveEncoding(static_cast<uint32_t>(config_.codec.bitrate_kbps),
                                           static_cast<uint32_t>(config_.codec.gop),
                                           static_cast<float>(config_.runtime.max_fps));
  metrics_publisher_->setAdaptationState(adaptation_profile_, adaptation_level_);

  set_profile_service_ = this->create_service<ros2_gst_video_bridge_msgs::srv::SetStreamingProfile>(
      "~/set_streaming_profile", std::bind(&GstVideoBridgeNode::handleSetStreamingProfile, this,
                                           std::placeholders::_1, std::placeholders::_2));

  if (!stream_engine_->start()) {
    const auto initial_error = stream_engine_->lastError();
    if (requestSoftwareFallback(initial_error)) {
      stream_engine_ = std::make_unique<StreamEngine>(effective_pipeline_);
      if (stream_engine_->start()) {
        setRuntimeState(RuntimeState::Connecting, "pipeline restarted with software fallback");
        metrics_publisher_->publishHeartbeat();
        return;
      }
    }

    RCLCPP_ERROR(this->get_logger(), "Failed to start stream engine: %s",
                 stream_engine_->lastError().c_str());
    publishRuntimeEvent("error", "PIPELINE_START_FAILED", stream_engine_->lastError());
    publishFirstFailureSnapshot(stream_engine_->lastError());

    if (config_.transport.reconnect_enabled) {
      setRuntimeState(RuntimeState::Degraded, stream_engine_->lastError());
      reconnect_requested_ = true;
    } else {
      setRuntimeState(RuntimeState::Failed, stream_engine_->lastError());
      immediate_exit_code_ = 1;
    }
    return;
  }

  setRuntimeState(RuntimeState::Connecting, "pipeline started");
  publishRuntimeEvent("info", "PIPELINE_STARTED", "Pipeline transitioned to PLAYING");
  metrics_publisher_->publishHeartbeat();
}

void GstVideoBridgeNode::initializeSubscriptions() {
  if (immediate_exit_code_ >= 0) {
    return;
  }

  image_subscription_ = this->create_subscription<sensor_msgs::msg::Image>(
      config_.source.input_topic, rclcpp::SensorDataQoS(),
      std::bind(&GstVideoBridgeNode::onImage, this, std::placeholders::_1));
}

void GstVideoBridgeNode::initializeHealthMonitoring() {
  if (immediate_exit_code_ >= 0) {
    return;
  }

  health_timer_ = this->create_wall_timer(std::chrono::milliseconds(500),
                                          std::bind(&GstVideoBridgeNode::runHealthCheck, this));
}

void GstVideoBridgeNode::onImage(const sensor_msgs::msg::Image::SharedPtr msg) {
  if (!stream_engine_ || !stream_engine_->isRunning()) {
    return;
  }

  if (config_.runtime.max_fps > 0.0) {
    const auto now = std::chrono::steady_clock::now();
    const auto min_period = std::chrono::duration<double>(1.0 / config_.runtime.max_fps);
    if (last_frame_steady_time_.time_since_epoch().count() != 0 &&
        (now - last_frame_steady_time_) < min_period) {
      return;
    }
    last_frame_steady_time_ = now;
  }

  std::string gst_format;
  if (!resolveGstFormat(msg->encoding, gst_format)) {
    RCLCPP_WARN_THROTTLE(
        this->get_logger(), *this->get_clock(), 5000,
        "Unsupported image encoding '%s'. Supported encodings include mono8/mono16, rgb8/bgr8, "
        "rgba8/bgra8, type_8/16 C1/C3/C4, and bayer_*8/*16 (grayscale fallback).",
        msg->encoding.c_str());
    return;
  }

  if (isBayerEncoding(msg->encoding)) {
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 10000,
                         "Input encoding '%s' is being streamed as grayscale Bayer fallback. Use "
                         "image_proc/debayer for color output.",
                         msg->encoding.c_str());
  }

  uint64_t timestamp_ns = 0;
  if (config_.runtime.use_wall_clock_timestamps) {
    timestamp_ns = static_cast<uint64_t>(this->now().nanoseconds());
  } else {
    timestamp_ns =
        static_cast<uint64_t>(static_cast<int64_t>(msg->header.stamp.sec) * nanosecondsPerSecond() +
                              msg->header.stamp.nanosec);
  }

  if (metrics_publisher_) {
    const uint64_t now_ns = static_cast<uint64_t>(this->now().nanoseconds());
    metrics_publisher_->recordFrameIn(now_ns, timestamp_ns);
  }

  const bool pushed =
      stream_engine_->pushFrame(msg->data.data(), msg->data.size(), static_cast<int>(msg->width),
                                static_cast<int>(msg->height), gst_format, timestamp_ns);

  if (!pushed) {
    if (metrics_publisher_) {
      metrics_publisher_->recordFrameDropped();
    }
    scheduleReconnect(stream_engine_->lastError());
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                         "Failed to push frame to GStreamer: %s",
                         stream_engine_->lastError().c_str());
  } else if (metrics_publisher_) {
    metrics_publisher_->recordFrameOut();
    consecutive_stream_failures_ = 0;
  }
}

} // namespace ros2_gst_video_bridge
