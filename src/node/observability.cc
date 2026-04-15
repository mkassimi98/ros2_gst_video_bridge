// Author: Mouhsine Kassimi Farhaoui
// Contact: mouhsine98@gmail.com

#include "ros2_gst_video_bridge/gst_video_bridge_node.hpp"

#include <iomanip>
#include <sstream>

namespace ros2_gst_video_bridge {

void GstVideoBridgeNode::setRuntimeState(RuntimeState new_state, const std::string& reason) {
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
    RCLCPP_WARN(this->get_logger(), "runtime.state=%s reason=%s",
                runtimeStateToString(runtime_state_), reason.c_str());
  } else {
    RCLCPP_INFO(this->get_logger(), "runtime.state=%s reason=%s",
                runtimeStateToString(runtime_state_), reason.c_str());
  }
}

void GstVideoBridgeNode::publishRuntimeEvent(const std::string& severity, const std::string& code,
                                             const std::string& message) const {
  if (!metrics_publisher_) {
    return;
  }
  metrics_publisher_->publishEvent(severity, code, message);
}

void GstVideoBridgeNode::publishFirstFailureSnapshot(const std::string& reason) {
  if (first_failure_snapshot_published_) {
    return;
  }

  std::ostringstream snapshot;
  snapshot << "session_id=" << session_id_ << " stream_id=" << stream_id_
           << " state=" << runtimeStateToString(runtime_state_) << " reason=" << reason
           << " codec=" << config_.codec.name << " encoder=" << config_.codec.encoder
           << " transport=" << config_.transport.kind << " sink_uri=" << config_.transport.sink_uri
           << " sw_fallback_applied=" << (sw_fallback_applied_ ? "true" : "false") << " pipeline='"
           << effective_pipeline_ << "'";

  publishRuntimeEvent("error", "FIRST_FAILURE_SNAPSHOT", snapshot.str());
  first_failure_snapshot_published_ = true;
}

void GstVideoBridgeNode::handleSetStreamingProfile(
    const std::shared_ptr<ros2_gst_video_bridge_msgs::srv::SetStreamingProfile::Request> request,
    std::shared_ptr<ros2_gst_video_bridge_msgs::srv::SetStreamingProfile::Response> response) {
  if (!request || !response) {
    return;
  }

  if (!request->adaptation_profile.empty()) {
    if (!isValidAdaptationProfile(request->adaptation_profile)) {
      response->success = false;
      response->message = "invalid adaptation profile (allowed: conservative|balanced|aggressive)";
      return;
    }
    adaptation_profile_ = request->adaptation_profile;
  }

  if (request->reset_counters) {
    adaptation_level_ = 0;
    reconnect_count_ = 0;
    reconnect_attempts_ = 0;
    consecutive_stream_failures_ = 0;
    first_failure_snapshot_published_ = false;
    config_.codec.bitrate_kbps = base_bitrate_kbps_;
    config_.codec.gop = base_gop_;
    config_.runtime.max_fps = base_max_fps_;
  }

  if (metrics_publisher_) {
    metrics_publisher_->setAdaptationState(adaptation_profile_, adaptation_level_);
    metrics_publisher_->setEffectiveEncoding(static_cast<uint32_t>(config_.codec.bitrate_kbps),
                                             static_cast<uint32_t>(config_.codec.gop),
                                             static_cast<float>(config_.runtime.max_fps));
    metrics_publisher_->publishHeartbeat();
  }

  publishRuntimeEvent("info", "OPERATOR_PROFILE_UPDATE",
                      "runtime profile updated via service request");

  response->success = true;
  response->message = "streaming profile updated";
}

bool GstVideoBridgeNode::isValidAdaptationProfile(const std::string& profile) {
  return profile == "conservative" || profile == "balanced" || profile == "aggressive";
}

std::string GstVideoBridgeNode::buildSessionId() const {
  const auto now = this->now();
  std::ostringstream oss;
  oss << "bridge-" << std::hex << now.nanoseconds();
  return oss.str();
}

const char* GstVideoBridgeNode::runtimeStateToString(RuntimeState state) {
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

void GstVideoBridgeNode::logConfiguration() const {
  RCLCPP_INFO(this->get_logger(), "profile.machine: %s", config_.profile.machine.c_str());
  RCLCPP_INFO(this->get_logger(), "profile.stream: %s", config_.profile.stream.c_str());
  RCLCPP_INFO(this->get_logger(), "input_topic: %s", config_.source.input_topic.c_str());

  RCLCPP_INFO(this->get_logger(), "transport.kind: %s", config_.transport.kind.c_str());
  RCLCPP_INFO(this->get_logger(), "transport.sink_uri: %s", config_.transport.sink_uri.c_str());
  RCLCPP_INFO(this->get_logger(), "transport.latency_ms: %d", config_.transport.latency_ms);
  RCLCPP_INFO(this->get_logger(), "transport.reconnect.enabled: %s",
              config_.transport.reconnect_enabled ? "true" : "false");
  RCLCPP_INFO(this->get_logger(), "transport.reconnect.interval_ms: %d",
              config_.transport.reconnect_interval_ms);
  RCLCPP_INFO(this->get_logger(), "transport.reconnect.max_attempts: %d",
              config_.transport.reconnect_max_attempts);

  RCLCPP_INFO(this->get_logger(), "codec.name: %s", config_.codec.name.c_str());
  RCLCPP_INFO(this->get_logger(), "codec.encoder: %s", config_.codec.encoder.c_str());
  RCLCPP_INFO(this->get_logger(), "codec.profile: %s", config_.codec.profile.c_str());
  RCLCPP_INFO(this->get_logger(), "codec.tune: %s", config_.codec.tune.c_str());
  RCLCPP_INFO(this->get_logger(), "codec.rate_control: %s", config_.codec.rate_control.c_str());
  RCLCPP_INFO(this->get_logger(), "codec.bitrate_kbps: %d", config_.codec.bitrate_kbps);
  RCLCPP_INFO(this->get_logger(), "codec.gop: %d", config_.codec.gop);

  RCLCPP_INFO(this->get_logger(), "gst.pipeline_override: %s",
              config_.gst.pipeline_override.c_str());

  RCLCPP_INFO(this->get_logger(), "max_fps: %.2f", config_.runtime.max_fps);
  RCLCPP_INFO(this->get_logger(), "use_wall_clock_timestamps: %s",
              config_.runtime.use_wall_clock_timestamps ? "true" : "false");
  RCLCPP_INFO(this->get_logger(), "effective_pipeline: %s", effective_pipeline_.c_str());
}

void GstVideoBridgeNode::logRuntimeCapabilities() const {
  const auto host_profile = capability_probe_->detectHostProfile();
  const auto transports = capability_probe_->detectTransports();
  const auto codecs = capability_probe_->detectCodecs();
  const auto encoder_impls = capability_probe_->detectEncoderImplementations();

  std::ostringstream host_stream;
  for (size_t i = 0; i < host_profile.size(); ++i) {
    host_stream << host_profile[i];
    if (i + 1 < host_profile.size()) {
      host_stream << ", ";
    }
  }

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

  std::ostringstream encoder_impl_stream;
  for (size_t i = 0; i < encoder_impls.size(); ++i) {
    encoder_impl_stream << encoder_impls[i];
    if (i + 1 < encoder_impls.size()) {
      encoder_impl_stream << ", ";
    }
  }

  RCLCPP_INFO(this->get_logger(), "detected_host: %s", host_stream.str().c_str());
  RCLCPP_INFO(this->get_logger(), "detected_transports: %s", transport_stream.str().c_str());
  RCLCPP_INFO(this->get_logger(), "detected_codecs: %s", codec_stream.str().c_str());
  RCLCPP_INFO(this->get_logger(), "detected_encoder_implementations: %s",
              encoder_impl_stream.str().c_str());

  const auto topics = topic_introspector_->listTopics();
  RCLCPP_INFO(this->get_logger(), "visible_topics_count: %zu", topics.size());
  RCLCPP_INFO(this->get_logger(), "stream_engine_running: %s",
              stream_engine_->isRunning() ? "true" : "false");
}

} // namespace ros2_gst_video_bridge
