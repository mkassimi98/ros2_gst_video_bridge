// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

#include "ros2_gst_video_bridge/core/config_loader.hpp"
#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"

#include <sstream>

namespace ros2_gst_video_bridge
{

GstVideoBridgeNode::GstVideoBridgeNode(const rclcpp::NodeOptions & options)
: Node("gst_video_bridge", options)
{
  config_ = ConfigLoader::loadFromNode(*this);
  effective_pipeline_ = PipelineBuilder::build(config_);

  initializeModules();

  logConfiguration();
  logRuntimeCapabilities();

  RCLCPP_INFO(
    this->get_logger(),
    "Skeleton node ready. Next step: subscribe to raw images from %s and push frames into GStreamer appsrc.",
    config_.input_topic.c_str());
}

void GstVideoBridgeNode::initializeModules()
{
  topic_introspector_ = std::make_unique<TopicIntrospector>(*this);
  capability_probe_ = std::make_unique<CapabilityProbe>();
  stream_engine_ = std::make_unique<StreamEngine>(effective_pipeline_);
  metrics_publisher_ = std::make_unique<MetricsPublisher>(*this);

  stream_engine_->start();
  metrics_publisher_->publishHeartbeat();
}

void GstVideoBridgeNode::logConfiguration() const
{
  RCLCPP_INFO(this->get_logger(), "input_topic: %s", config_.input_topic.c_str());
  RCLCPP_INFO(this->get_logger(), "gst.transport: %s", config_.gst_transport.c_str());
  RCLCPP_INFO(this->get_logger(), "gst.codec: %s", config_.gst_codec.c_str());
  RCLCPP_INFO(this->get_logger(), "gst.profile: %s", config_.gst_profile.c_str());
  RCLCPP_INFO(this->get_logger(), "gst.sink_uri: %s", config_.gst_sink_uri.c_str());
  RCLCPP_INFO(this->get_logger(), "gst.bitrate_kbps: %d", config_.gst_bitrate_kbps);
  RCLCPP_INFO(this->get_logger(), "gst.latency_ms: %d", config_.gst_latency_ms);
  RCLCPP_INFO(
    this->get_logger(), "gst.pipeline_override: %s", config_.gst_pipeline_override.c_str());
  RCLCPP_INFO(this->get_logger(), "max_fps: %.2f", config_.max_fps);
  RCLCPP_INFO(
    this->get_logger(), "use_wall_clock_timestamps: %s",
    config_.use_wall_clock_timestamps ? "true" : "false");
  RCLCPP_INFO(this->get_logger(), "effective_pipeline: %s", effective_pipeline_.c_str());
}

void GstVideoBridgeNode::logRuntimeCapabilities() const
{
  const auto transports = capability_probe_->detectTransports();
  const auto codecs = capability_probe_->detectCodecs();

  std::ostringstream transport_stream;
  for (size_t i = 0; i < transports.size(); ++i) {
    transport_stream << transports[i];
    if (i + 1 < transports.size()) {
      transport_stream << ", ";
    }
  }

  std::ostringstream codec_stream;
  for (size_t i = 0; i < codecs.size(); ++i) {
    codec_stream << codecs[i];
    if (i + 1 < codecs.size()) {
      codec_stream << ", ";
    }
  }

  RCLCPP_INFO(this->get_logger(), "detected_transports: %s", transport_stream.str().c_str());
  RCLCPP_INFO(this->get_logger(), "detected_codecs: %s", codec_stream.str().c_str());

  const auto topics = topic_introspector_->listTopics();
  RCLCPP_INFO(this->get_logger(), "visible_topics_count: %zu", topics.size());
  RCLCPP_INFO(this->get_logger(), "stream_engine_running: %s", stream_engine_->isRunning() ? "true" : "false");
}

}  // namespace ros2_gst_video_bridge
