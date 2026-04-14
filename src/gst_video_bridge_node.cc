// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

#include "ros2_gst_video_bridge/core/config_loader.hpp"
#include "ros2_gst_video_bridge/core/pipeline_builder.hpp"

#include <sensor_msgs/image_encodings.hpp>

#include <chrono>
#include <functional>
#include <sstream>

namespace ros2_gst_video_bridge
{

namespace
{

consteval int64_t nanosecondsPerSecond()
{
  return 1000000000LL;
}

bool resolveGstFormat(const std::string & encoding, std::string & gst_format)
{
  if (encoding == sensor_msgs::image_encodings::BGR8) {
    gst_format = "BGR";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::RGB8) {
    gst_format = "RGB";
    return true;
  }

  if (encoding == sensor_msgs::image_encodings::MONO8) {
    gst_format = "GRAY8";
    return true;
  }

  // Fallback for Bayer 8-bit streams (common on raw industrial cameras).
  // We forward as monochrome to avoid dropping frames when no debayer stage is present.
  if (
    encoding == sensor_msgs::image_encodings::BAYER_RGGB8 ||
    encoding == sensor_msgs::image_encodings::BAYER_BGGR8 ||
    encoding == sensor_msgs::image_encodings::BAYER_GBRG8 ||
    encoding == sensor_msgs::image_encodings::BAYER_GRBG8)
  {
    gst_format = "GRAY8";
    return true;
  }

  return false;
}

}  // namespace

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
  initializeSubscriptions();
  initializeHealthMonitoring();

  if (immediate_exit_code_ >= 0) {
    return;
  }

  logConfiguration();
  logRuntimeCapabilities();

  RCLCPP_INFO(
    this->get_logger(),
    "Streaming node ready. Subscribed to %s and forwarding frames to GStreamer appsrc.",
    config_.source.input_topic.c_str());

  if (stream_engine_ && stream_engine_->isRunning()) {
    setRuntimeState(RuntimeState::Streaming, "pipeline running and subscriptions active");
  }
}

GstVideoBridgeNode::~GstVideoBridgeNode()
{
  if (stream_engine_) {
    stream_engine_->stop();
  }
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

  if (!stream_engine_->start()) {
    RCLCPP_ERROR(
      this->get_logger(), "Failed to start stream engine: %s",
      stream_engine_->lastError().c_str());
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

  metrics_publisher_->publishHeartbeat();
}

void GstVideoBridgeNode::initializeSubscriptions()
{
  if (immediate_exit_code_ >= 0) {
    return;
  }

  image_subscription_ = this->create_subscription<sensor_msgs::msg::Image>(
    config_.source.input_topic,
    rclcpp::SensorDataQoS(),
    std::bind(&GstVideoBridgeNode::onImage, this, std::placeholders::_1));
}

void GstVideoBridgeNode::initializeHealthMonitoring()
{
  if (immediate_exit_code_ >= 0) {
    return;
  }

  health_timer_ = this->create_wall_timer(
    std::chrono::milliseconds(500),
    std::bind(&GstVideoBridgeNode::runHealthCheck, this));
}

void GstVideoBridgeNode::onImage(const sensor_msgs::msg::Image::SharedPtr msg)
{
  if (!stream_engine_ || !stream_engine_->isRunning()) {
    return;
  }

  if (config_.runtime.max_fps > 0.0) {
    const auto now = std::chrono::steady_clock::now();
    const auto min_period = std::chrono::duration<double>(1.0 / config_.runtime.max_fps);
    if (last_frame_steady_time_.time_since_epoch().count() != 0 &&
      (now - last_frame_steady_time_) < min_period)
    {
      return;
    }
    last_frame_steady_time_ = now;
  }

  std::string gst_format;
  if (!resolveGstFormat(msg->encoding, gst_format)) {
    RCLCPP_WARN_THROTTLE(
      this->get_logger(), *this->get_clock(), 5000,
      "Unsupported image encoding '%s'. Supported encodings: bgr8, rgb8, mono8, bayer_*8 (grayscale fallback).",
      msg->encoding.c_str());
    return;
  }

  if (
    msg->encoding == sensor_msgs::image_encodings::BAYER_RGGB8 ||
    msg->encoding == sensor_msgs::image_encodings::BAYER_BGGR8 ||
    msg->encoding == sensor_msgs::image_encodings::BAYER_GBRG8 ||
    msg->encoding == sensor_msgs::image_encodings::BAYER_GRBG8)
  {
    RCLCPP_WARN_THROTTLE(
      this->get_logger(), *this->get_clock(), 10000,
      "Input encoding '%s' is being streamed as GRAY8 fallback. Use image_proc/debayer for color output.",
      msg->encoding.c_str());
  }

  uint64_t timestamp_ns = 0;
  if (config_.runtime.use_wall_clock_timestamps) {
    timestamp_ns = static_cast<uint64_t>(this->now().nanoseconds());
  } else {
    timestamp_ns = static_cast<uint64_t>(
      static_cast<int64_t>(msg->header.stamp.sec) * nanosecondsPerSecond() +
      msg->header.stamp.nanosec);
  }

  if (metrics_publisher_) {
    const uint64_t now_ns = static_cast<uint64_t>(this->now().nanoseconds());
    metrics_publisher_->recordFrameIn(now_ns, timestamp_ns);
  }

  const bool pushed = stream_engine_->pushFrame(
    msg->data.data(), msg->data.size(), static_cast<int>(msg->width),
    static_cast<int>(msg->height), gst_format, timestamp_ns);

  if (!pushed) {
    if (metrics_publisher_) {
      metrics_publisher_->recordFrameDropped();
    }
    scheduleReconnect(stream_engine_->lastError());
    RCLCPP_WARN_THROTTLE(
      this->get_logger(), *this->get_clock(), 2000,
      "Failed to push frame to GStreamer: %s", stream_engine_->lastError().c_str());
  } else if (metrics_publisher_) {
    metrics_publisher_->recordFrameOut();
  }
}

void GstVideoBridgeNode::runHealthCheck()
{
  if (!stream_engine_) {
    return;
  }

  if (runtime_state_ == RuntimeState::Failed) {
    return;
  }

  if (reconnect_requested_) {
    (void)tryReconnect();
    return;
  }

  const auto engine_error = stream_engine_->lastError();
  if (!engine_error.empty()) {
    scheduleReconnect(engine_error);
  }
}

void GstVideoBridgeNode::scheduleReconnect(const std::string & reason)
{
  if (runtime_state_ == RuntimeState::Failed) {
    return;
  }

  if (!config_.transport.reconnect_enabled) {
    setRuntimeState(RuntimeState::Failed, "reconnect disabled: " + reason);
    return;
  }

  if (!reconnect_requested_) {
    reconnect_requested_ = true;
    setRuntimeState(RuntimeState::Degraded, reason);
  }
}

bool GstVideoBridgeNode::tryReconnect()
{
  if (!stream_engine_) {
    return false;
  }

  const auto now = std::chrono::steady_clock::now();
  const auto reconnect_interval = std::chrono::milliseconds(config_.transport.reconnect_interval_ms);
  if (last_reconnect_attempt_.time_since_epoch().count() != 0 &&
    (now - last_reconnect_attempt_) < reconnect_interval)
  {
    return false;
  }

  if (config_.transport.reconnect_max_attempts > 0 &&
    reconnect_attempts_ >= config_.transport.reconnect_max_attempts)
  {
    setRuntimeState(RuntimeState::Failed, "reconnect attempts exhausted");
    reconnect_requested_ = false;
    return false;
  }

  last_reconnect_attempt_ = now;
  ++reconnect_attempts_;
  setRuntimeState(
    RuntimeState::Reconnecting,
    "attempt " + std::to_string(reconnect_attempts_));

  stream_engine_->stop();
  const bool started = stream_engine_->start();
  if (!started) {
    setRuntimeState(RuntimeState::Degraded, stream_engine_->lastError());
    return false;
  }

  if (metrics_publisher_) {
    metrics_publisher_->recordReconnect();
  }

  reconnect_requested_ = false;
  reconnect_attempts_ = 0;
  last_frame_steady_time_ = std::chrono::steady_clock::time_point{};
  setRuntimeState(RuntimeState::Streaming, "reconnected successfully");
  return true;
}

void GstVideoBridgeNode::setRuntimeState(RuntimeState new_state, const std::string & reason)
{
  if (runtime_state_ == new_state) {
    return;
  }

  runtime_state_ = new_state;
  if (metrics_publisher_) {
    metrics_publisher_->updateRuntimeState(runtimeStateToString(runtime_state_));
  }
  if (reason.empty()) {
    RCLCPP_INFO(this->get_logger(), "runtime.state=%s", runtimeStateToString(runtime_state_));
    return;
  }

  if (new_state == RuntimeState::Degraded || new_state == RuntimeState::Failed) {
    RCLCPP_WARN(
      this->get_logger(), "runtime.state=%s reason=%s", runtimeStateToString(runtime_state_),
      reason.c_str());
  } else {
    RCLCPP_INFO(
      this->get_logger(), "runtime.state=%s reason=%s", runtimeStateToString(runtime_state_),
      reason.c_str());
  }
}

const char * GstVideoBridgeNode::runtimeStateToString(RuntimeState state)
{
  switch (state) {
    case RuntimeState::Connecting:
      return "connecting";
    case RuntimeState::Streaming:
      return "streaming";
    case RuntimeState::Degraded:
      return "degraded";
    case RuntimeState::Reconnecting:
      return "reconnecting";
    case RuntimeState::Failed:
      return "failed";
    default:
      return "unknown";
  }
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
