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
  topic_introspector_ = std::make_unique<TopicIntrospector>(*this);
  capability_probe_ = std::make_unique<CapabilityProbe>();
  effective_pipeline_ = PipelineBuilder::build(config_);

  if (config_.runtime.print_effective_config) {
    RCLCPP_INFO(
      this->get_logger(), "Effective runtime configuration:\n%s",
      ConfigLoader::toDebugString(config_).c_str());
  }

  if (!validateConfiguration()) {
    RCLCPP_ERROR(this->get_logger(), "Configuration validation failed. Node will shutdown.");
    immediate_exit_code_ = 1;
    return;
  }

  if (handleRuntimeMode()) {
    immediate_exit_code_ = 0;
    return;
  }

  initializeModules();

  logConfiguration();
  logRuntimeCapabilities();

  RCLCPP_INFO(
    this->get_logger(),
    "Skeleton node ready. Next step: subscribe to raw images from %s and push frames into GStreamer appsrc.",
    config_.source.input_topic.c_str());
}

bool GstVideoBridgeNode::hasImmediateExit() const
{
  return immediate_exit_code_ >= 0;
}

int GstVideoBridgeNode::immediateExitCode() const
{
  return immediate_exit_code_;
}

bool GstVideoBridgeNode::handleRuntimeMode()
{
  if (config_.runtime.mode == "stream") {
    return false;
  }

  if (config_.runtime.mode == "list_topics") {
    printImageTopics();
    return true;
  }

  if (config_.runtime.mode == "list_capabilities") {
    printGstCapabilities();
    return true;
  }

  if (config_.runtime.mode == "validate_config") {
    RCLCPP_INFO(this->get_logger(), "Configuration is valid.");
    return true;
  }

  if (config_.runtime.mode == "discover") {
    printImageTopics();
    printGstCapabilities();
    RCLCPP_INFO(this->get_logger(), "Discovery mode completed successfully.");
    return true;
  }

  RCLCPP_ERROR(
    this->get_logger(), "Unsupported runtime.mode value: %s", config_.runtime.mode.c_str());
  return true;
}

void GstVideoBridgeNode::printImageTopics() const
{
  const auto image_topics = topic_introspector_->listImageTopics();
  RCLCPP_INFO(this->get_logger(), "Detected image topics: %zu", image_topics.size());

  for (const auto & topic : image_topics) {
    std::ostringstream types;
    for (size_t i = 0; i < topic.second.size(); ++i) {
      types << topic.second[i];
      if (i + 1 < topic.second.size()) {
        types << ", ";
      }
    }
    RCLCPP_INFO(this->get_logger(), " - %s [%s]", topic.first.c_str(), types.str().c_str());
  }
}

void GstVideoBridgeNode::printGstCapabilities() const
{
  if (!capability_probe_->hasGstInspect()) {
    RCLCPP_WARN(
      this->get_logger(),
      "gst-inspect-1.0 was not found on PATH. Reporting static fallback capabilities.");
  }

  const auto plugins = capability_probe_->detectPlugins();
  const auto encoders = capability_probe_->detectEncoders();
  const auto sinks = capability_probe_->detectSinks();

  RCLCPP_INFO(this->get_logger(), "Detected GStreamer plugins: %zu", plugins.size());
  for (const auto & plugin : plugins) {
    RCLCPP_INFO(this->get_logger(), " - %s", plugin.c_str());
  }

  RCLCPP_INFO(this->get_logger(), "Detected encoders: %zu", encoders.size());
  for (const auto & encoder : encoders) {
    RCLCPP_INFO(this->get_logger(), " - %s", encoder.c_str());
  }

  RCLCPP_INFO(this->get_logger(), "Detected sinks/transports: %zu", sinks.size());
  for (const auto & sink : sinks) {
    RCLCPP_INFO(this->get_logger(), " - %s", sink.c_str());
  }
}

bool GstVideoBridgeNode::validateConfiguration() const
{
  const auto errors = ConfigLoader::validate(config_);
  if (errors.empty()) {
    return true;
  }

  for (const auto & error : errors) {
    RCLCPP_ERROR(this->get_logger(), "Config error: %s", error.c_str());
  }

  return false;
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
  RCLCPP_INFO(this->get_logger(), "profile.machine: %s", config_.profile.machine.c_str());
  RCLCPP_INFO(this->get_logger(), "profile.stream: %s", config_.profile.stream.c_str());
  RCLCPP_INFO(this->get_logger(), "input_topic: %s", config_.source.input_topic.c_str());

  RCLCPP_INFO(this->get_logger(), "transport.kind: %s", config_.transport.kind.c_str());
  RCLCPP_INFO(this->get_logger(), "transport.sink_uri: %s", config_.transport.sink_uri.c_str());
  RCLCPP_INFO(this->get_logger(), "transport.latency_ms: %d", config_.transport.latency_ms);
  RCLCPP_INFO(
    this->get_logger(), "transport.reconnect.enabled: %s",
    config_.transport.reconnect_enabled ? "true" : "false");
  RCLCPP_INFO(
    this->get_logger(), "transport.reconnect.interval_ms: %d",
    config_.transport.reconnect_interval_ms);
  RCLCPP_INFO(
    this->get_logger(), "transport.reconnect.max_attempts: %d",
    config_.transport.reconnect_max_attempts);

  RCLCPP_INFO(this->get_logger(), "codec.name: %s", config_.codec.name.c_str());
  RCLCPP_INFO(this->get_logger(), "codec.profile: %s", config_.codec.profile.c_str());
  RCLCPP_INFO(this->get_logger(), "codec.tune: %s", config_.codec.tune.c_str());
  RCLCPP_INFO(this->get_logger(), "codec.rate_control: %s", config_.codec.rate_control.c_str());
  RCLCPP_INFO(this->get_logger(), "codec.bitrate_kbps: %d", config_.codec.bitrate_kbps);
  RCLCPP_INFO(this->get_logger(), "codec.gop: %d", config_.codec.gop);

  RCLCPP_INFO(
    this->get_logger(), "gst.pipeline_override: %s", config_.gst.pipeline_override.c_str());

  RCLCPP_INFO(this->get_logger(), "max_fps: %.2f", config_.runtime.max_fps);
  RCLCPP_INFO(
    this->get_logger(), "use_wall_clock_timestamps: %s",
    config_.runtime.use_wall_clock_timestamps ? "true" : "false");
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
